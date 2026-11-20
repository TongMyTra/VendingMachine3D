#include "StaffMode.h"

#include "Camera.h"
#include "Product.h"
#include "VendingMachine.h"

#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>

namespace {
constexpr glm::vec3 crateAimPoint{3.35f, 0.90f, 1.32f};
constexpr float crateRadius = 0.90f;
constexpr float crateReach = 4.8f;
constexpr float transferDuration = 0.82f;
constexpr float cratePlacementDuration = 0.72f;
float smooth(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}
}

StaffMode::StaffMode() : reserve_(12, 8) {}

bool StaffMode::switchRole(bool hasDrink, bool busy, VendingMachine& machine) {
    if (busy || hasDrink || doorBusy() || cratePlacing_ || fullStockActive_ || transferActive_ || heldProduct_ >= 0 || crateCarried_) return false;
    if (!active_) {
        active_ = true;
        fullStockAdded_ = 0;
        fullStockTotal_ = 0;
        bool foundSupply = false;
        for (int index = 0; index < static_cast<int>(machine.products().size()); ++index) {
            const int room = std::max(0, productSlotCapacity - machine.products()[static_cast<std::size_t>(index)].stock);
            fullStockTotal_ += std::min(room, reserve(index));
            if (!foundSupply && room > 0 && reserve(index) > 0) {
                selectedSupply_ = index;
                foundSupply = true;
            }
        }
        fullStockActive_ = true;
        crateCarried_ = true;
        crateOpen_ = false;
        status_ = "Đã chuyển sang chế độ nhân viên. Đang cầm kiện hàng; nhấn E để đặt xuống.";
        return true;
    }
    if (!doorLocked() || doorOpen()) {
        status_ = "Hãy đóng và khóa cửa máy trước khi trở về chế độ khách hàng.";
        return false;
    }
    active_ = false;
    crateOpen_ = false;
    hoveredSlot_ = -1;
    machine.hover(-1);
    status_ = "Đã chuyển sang chế độ khách hàng";
    return true;
}

bool StaffMode::aimedAtCrate(const Camera& camera) const {
    return camera.aimingAt(crateAimPoint, crateRadius, crateReach);
}

void StaffMode::updateAim(const Camera& camera, VendingMachine& machine) {
    hoveredSlot_ = -1;
    if (active_ && fullStockActive_ && doorOpen() && crateOpen_ && !crateCarried_ &&
        !doorBusy() && !transferActive_ && heldProduct_ >= 0) {
        float bestDistance = 1000.0f;
        for (int index = 0; index < static_cast<int>(machine.products().size()); ++index) {
            const auto p = machine.productPosition(index);
            if (!camera.aimingAt(p + glm::vec3(0, 0, 0.30f), 0.31f, 7.0f)) continue;
            const float distance = glm::length(camera.position() - p);
            if (distance < bestDistance) {
                bestDistance = distance;
                hoveredSlot_ = index;
            }
        }
    }
    machine.hover(hoveredSlot_);
}

bool StaffMode::canPlaceInHovered(const VendingMachine& machine) const {
    return heldProduct_ >= 0 && hoveredSlot_ >= 0 && hoveredSlot_ < static_cast<int>(machine.products().size()) &&
        heldProduct_ == hoveredSlot_ && machine.products()[static_cast<std::size_t>(hoveredSlot_)].stock < productSlotCapacity;
}

int StaffMode::reserve(int index) const {
    return index >= 0 && index < static_cast<int>(reserve_.size()) ? reserve_[static_cast<std::size_t>(index)] : 0;
}

bool StaffMode::handleInteract(const Camera& camera) {
    if (!active_ || doorBusy() || cratePlacing_ || transferActive_) return false;
    if (!crateCarried_) return false;
    crateCarried_ = false;
    fullStockActive_ = true;
    crateOpen_ = false;
    cratePlacing_ = true;
    cratePlacementTime_ = 0.0f;
    const auto cameraFrame = glm::inverse(camera.viewMatrix());
    cratePlacementStart_ = glm::vec3(cameraFrame * glm::vec4(0.52f, -0.31f, -1.08f, 1.0f));
    cratePlacementPosition_ = cratePlacementStart_;
    status_ = "Đang đặt thùng xuống bên phải máy...";
    soundEvent_ = 3;
    return true;
}

bool StaffMode::openSupplyCrate(const Camera& camera) {
    if (!active_ || doorBusy() || crateCarried_ || cratePlacing_ || transferActive_) return false;
    if (!aimedAtCrate(camera)) {
        status_ = "Hãy hướng camera vào thùng bên phải để mở hoặc đóng.";
        return false;
    }
    if (heldProduct_ >= 0) {
        status_ = "Đặt chai lên kệ hoặc khóa cửa trước khi đóng thùng.";
        return false;
    }
    crateOpen_ = !crateOpen_;
    if (crateOpen_) {
        fullStockActive_ = true;
        status_ = "Thùng đã mở. Dùng mũi tên chọn chai, Enter để cầm.";
    } else status_ = "Đã đóng thùng hàng.";
    return true;
}

bool StaffMode::selectSupply(int direction, const VendingMachine& machine) {
    if (!active_ || !fullStockActive_ || crateCarried_ || cratePlacing_ || !crateOpen_ || heldProduct_ >= 0 || transferActive_) return false;
    for (int attempt = 0; attempt < static_cast<int>(reserve_.size()); ++attempt) {
        selectedSupply_ = (selectedSupply_ + (direction >= 0 ? 1 : static_cast<int>(reserve_.size()) - 1)) %
            static_cast<int>(reserve_.size());
        if (reserve_[static_cast<std::size_t>(selectedSupply_)] > 0 &&
            machine.products()[static_cast<std::size_t>(selectedSupply_)].stock < productSlotCapacity) return true;
    }
    return false;
}

bool StaffMode::takeSupply(const VendingMachine& machine) {
    if (!active_ || !fullStockActive_ || crateCarried_ || cratePlacing_ || !crateOpen_ || heldProduct_ >= 0 || transferActive_ ||
        reserve(selectedSupply_) <= 0) {
        status_ = "Loại nước này đã hết trong thùng.";
        return false;
    }
    if (machine.products()[static_cast<std::size_t>(selectedSupply_)].stock >= productSlotCapacity) {
        status_ = "Ô " + productCode(selectedSupply_) + " đã đầy. Dùng mũi tên chọn loại khác.";
        return false;
    }
    heldProduct_ = selectedSupply_;
    --reserve_[static_cast<std::size_t>(heldProduct_)];
    soundEvent_ = 1;
    return true;
}

bool StaffMode::takeSelectedSupply(const VendingMachine& machine) {
    if (!crateOpen_ || !fullStockActive_) {
        status_ = "Mở thùng bằng B trước khi lấy chai.";
        return false;
    }
    if (!takeSupply(machine)) return false;
    status_ = "Đang cầm " + machine.products()[static_cast<std::size_t>(heldProduct_)].name +
        ". Nhìn ô cùng mã rồi nhấn chuột trái để đặt.";
    return true;
}

bool StaffMode::openDoor() {
    if (!active_ || doorBusy() || crateCarried_ || cratePlacing_ || transferActive_) return false;
    if (doorPhase_ != DoorPhase::Locked) {
        status_ = "Cửa máy chưa khóa. Nhấn L để đóng và khóa cửa.";
        return false;
    }
    doorPhase_ = DoorPhase::Unlocking;
    doorActionProgress_ = 0.0f;
    status_ = "Đang mở khóa cửa máy...";
    soundEvent_ = 1;
    return true;
}

bool StaffMode::lockDoor() {
    if (!active_ || doorBusy() || cratePlacing_ || transferActive_) return false;
    if (doorPhase_ != DoorPhase::Open) {
        status_ = doorLocked() ? "Cửa máy đã khóa." : "Đợi cửa mở xong rồi nhấn L để khóa.";
        return false;
    }
    if (heldProduct_ >= 0) {
        ++reserve_[static_cast<std::size_t>(heldProduct_)];
        heldProduct_ = -1;
        status_ = "Đã trả chai về thùng. Đang đóng và khóa cửa...";
    } else status_ = "Đang đóng và khóa cửa...";
    doorPhase_ = DoorPhase::Closing;
    crateOpen_ = false;
    fullStockActive_ = false;
    doorActionProgress_ = 0.0f;
    soundEvent_ = 8;
    return true;
}

bool StaffMode::placeHeldItem(const Camera& camera, const VendingMachine& machine) {
    if (!active_ || !fullStockActive_ || heldProduct_ < 0 || !doorOpen() || doorBusy() || transferActive_) return false;
    if (hoveredSlot_ < 0) return false;
    if (heldProduct_ != hoveredSlot_) {
        status_ = "Loại nước không khớp với ô " + productCode(hoveredSlot_) + ".";
        return false;
    }
    if (machine.products()[static_cast<std::size_t>(hoveredSlot_)].stock >= productSlotCapacity) {
        status_ = "Ô hàng đã đầy. Hãy chọn ô khác.";
        return false;
    }
    transferProduct_ = heldProduct_;
    transferSlot_ = hoveredSlot_;
    transferActive_ = true;
    transferTime_ = 0.0f;
    transferDuration_ = transferDuration;
    const auto cameraFrame = glm::inverse(camera.viewMatrix());
    transferStart_ = glm::vec3(cameraFrame * glm::vec4(0.30f, -0.40f, -0.92f, 1.0f));
    transferPosition_ = transferStart_;
    status_ = "Đang đặt sản phẩm vào ô " + productCode(transferSlot_) + "...";
    return true;
}

glm::vec3 StaffMode::bezier(glm::vec3 from, glm::vec3 to, float t) const {
    const float u = 1.0f - t;
    const glm::vec3 controlA = from + glm::vec3(-0.45f, 1.65f, 0.10f);
    const glm::vec3 controlB = to + glm::vec3(0.0f, 1.25f, 0.30f);
    return u * u * u * from + 3.0f * u * u * t * controlA + 3.0f * u * t * t * controlB + t * t * t * to;
}

void StaffMode::finishTransfer(VendingMachine& machine) {
    if (transferSlot_ >= 0 && machine.addStock(transferSlot_)) {
        heldProduct_ = -1;
        ++fullStockAdded_;
        soundEvent_ = 3;
        const bool allFull = std::all_of(machine.products().begin(), machine.products().end(),
            [](const Product& product) { return product.stock >= productSlotCapacity; });
        status_ = allFull ? "Hoàn tất bổ sung hàng hóa." : "Đã bổ sung ô " + productCode(transferSlot_) + ".";
    } else {
        if (heldProduct_ >= 0 && heldProduct_ < static_cast<int>(reserve_.size()))
            ++reserve_[static_cast<std::size_t>(heldProduct_)];
        heldProduct_ = -1;
        status_ = "Ô hàng đã đầy. Hãy chọn ô khác.";
    }
    transferActive_ = false;
    transferProduct_ = -1;
}

void StaffMode::update(float dt, VendingMachine& machine) {
    const float step = std::clamp(dt, 0.0f, 0.10f);
    if (cratePlacing_) {
        cratePlacementTime_ += step;
        const float t = smooth(cratePlacementTime_ / cratePlacementDuration);
        const glm::vec3 target = stockCratePosition() + glm::vec3(0, 0.39f, 0);
        cratePlacementPosition_ = glm::mix(cratePlacementStart_, target, t) +
            glm::vec3(0, 0.26f * std::sin(glm::pi<float>() * t), 0);
        if (cratePlacementTime_ >= cratePlacementDuration) {
            cratePlacementPosition_ = target;
            cratePlacing_ = false;
            crateOpen_ = true;
            status_ = "Đã đặt thùng bên phải máy. Dùng mũi tên chọn chai rồi nhấn E để cầm.";
        }
    }
    if (doorPhase_ == DoorPhase::Unlocking || doorPhase_ == DoorPhase::Locking) {
        doorActionProgress_ += step / 0.55f;
        if (doorActionProgress_ >= 1.0f) {
            doorActionProgress_ = 0.0f;
            doorPhase_ = doorPhase_ == DoorPhase::Unlocking ? DoorPhase::Opening : DoorPhase::Locked;
            status_ = doorPhase_ == DoorPhase::Opening ? "Đã mở khóa. Đang mở cửa máy..." : "Máy đã sẵn sàng. Cửa đã khóa.";
            if (doorPhase_ == DoorPhase::Opening) soundEvent_ = 8;
        }
    } else if (doorPhase_ == DoorPhase::Opening || doorPhase_ == DoorPhase::Closing) {
        doorActionProgress_ += step / 1.35f;
        const float t = smooth(doorActionProgress_);
        doorAngle_ = doorPhase_ == DoorPhase::Opening ? 100.0f * t : 100.0f * (1.0f - t);
        if (doorActionProgress_ >= 1.0f) {
            doorAngle_ = doorPhase_ == DoorPhase::Opening ? 100.0f : 0.0f;
            doorActionProgress_ = 0.0f;
            doorPhase_ = doorPhase_ == DoorPhase::Opening ? DoorPhase::Open : DoorPhase::Locking;
            if (doorPhase_ == DoorPhase::Open) status_ = "Cửa đã mở. Có thể chọn và bổ sung từng chai.";
        }
    }
    machine.setServiceDoorOpen(doorPhase_ != DoorPhase::Locked);
    if (!transferActive_) return;
    transferTime_ += step;
    const float t = smooth(transferTime_ / transferDuration_);
    const glm::vec3 from = transferStart_;
    glm::vec3 to = machine.productPosition(std::max(transferSlot_, 0));
    if (transferSlot_ >= 0) {
        const int existing = machine.products()[static_cast<std::size_t>(transferSlot_)].stock;
        to.z = 0.48f - existing * 0.31f;
    }
    transferPosition_ = bezier(from, to, t);
    if (transferTime_ >= transferDuration_) finishTransfer(machine);
}

int StaffMode::takeSound() {
    const int event = soundEvent_;
    soundEvent_ = 0;
    return event;
}
