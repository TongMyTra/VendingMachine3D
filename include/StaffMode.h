#pragma once

#include <glm/vec3.hpp>
#include <string>
#include <vector>

class Camera;
class VendingMachine;

class StaffMode {
public:
    enum class DoorPhase { Locked, Unlocking, Opening, Open, Closing, Locking };

    StaffMode();
    void update(float dt, VendingMachine& machine);
    bool switchRole(bool hasDrink, bool busy, VendingMachine& machine);
    bool handleInteract(const Camera& camera);
    bool openSupplyCrate(const Camera& camera);
    bool takeSelectedSupply(const VendingMachine& machine);
    bool openDoor();
    bool lockDoor();
    void updateAim(const Camera& camera, VendingMachine& machine);
    bool selectSupply(int direction, const VendingMachine& machine);
    bool placeHeldItem(const Camera& camera, const VendingMachine& machine);
    void toggleInventory() { inventoryVisible_ = !inventoryVisible_; }

    bool active() const { return active_; }
    bool doorOpen() const { return doorPhase_ == DoorPhase::Open || doorPhase_ == DoorPhase::Closing; }
    bool doorBusy() const { return doorPhase_ == DoorPhase::Unlocking || doorPhase_ == DoorPhase::Opening ||
                                   doorPhase_ == DoorPhase::Closing || doorPhase_ == DoorPhase::Locking; }
    bool doorLocked() const { return doorPhase_ == DoorPhase::Locked; }
    DoorPhase doorPhase() const { return doorPhase_; }
    float doorAngle() const { return doorAngle_; }
    float doorActionProgress() const { return doorActionProgress_; }
    bool crateOpen() const { return crateOpen_; }
    bool crateCarried() const { return crateCarried_; }
    bool cratePlacing() const { return cratePlacing_; }
    glm::vec3 cratePlacementPosition() const { return cratePlacementPosition_; }
    float cratePlacementProgress() const { return cratePlacementTime_ / 0.72f; }
    int selectedSupply() const { return selectedSupply_; }
    int heldProduct() const { return heldProduct_; }
    int reserve(int index) const;
    int hoveredSlot() const { return hoveredSlot_; }
    bool canPlaceInHovered(const VendingMachine& machine) const;
    bool inventoryVisible() const { return inventoryVisible_; }
    bool fullStockActive() const { return fullStockActive_; }
    int fullStockAdded() const { return fullStockAdded_; }
    int fullStockTotal() const { return fullStockTotal_; }
    int fullStockTarget() const { return transferActive_ ? transferProduct_ : hoveredSlot_; }
    int transferProduct() const { return transferProduct_; }
    glm::vec3 transferPosition() const { return transferPosition_; }
    bool transferActive() const { return transferActive_; }
    const std::string& status() const { return status_; }
    int takeSound();

    static glm::vec3 stockCratePosition() { return {3.35f, 0.03f, 1.12f}; }

private:
    bool aimedAtCrate(const Camera& camera) const;
    bool takeSupply(const VendingMachine& machine);
    void finishTransfer(VendingMachine& machine);
    glm::vec3 bezier(glm::vec3 from, glm::vec3 to, float t) const;

    std::vector<int> reserve_;
    bool active_{};
    bool crateOpen_{};
    bool crateCarried_{};
    bool cratePlacing_{};
    float cratePlacementTime_{};
    glm::vec3 cratePlacementStart_{};
    glm::vec3 cratePlacementPosition_{};
    bool inventoryVisible_{};
    int selectedSupply_{};
    int heldProduct_{-1};
    int hoveredSlot_{-1};
    DoorPhase doorPhase_{DoorPhase::Locked};
    float doorAngle_{};
    float doorActionProgress_{};
    bool fullStockActive_{};
    int fullStockAdded_{};
    int fullStockTotal_{};
    int transferProduct_{-1};
    int transferSlot_{-1};
    float transferTime_{};
    float transferDuration_{0.82f};
    bool transferActive_{};
    glm::vec3 transferPosition_{};
    glm::vec3 transferStart_{};
    std::string status_{"Chế độ khách hàng"};
    int soundEvent_{};
};
