# 3D Interactive Vending Machine Simulation

Mô phỏng máy bán nước MINT FRESH viết bằng C++17 và OpenGL, với vỏ mint–cream, 3 kệ và 12 đồ uống procedural.

Người chơi di chuyển ở góc nhìn thứ nhất. Khi đến gần mặt trước máy và nhấn E, camera chuyển mượt tới góc chính diện, hiển thị trọn máy trong 0,75 giây rồi mở giao diện mua hàng. ESC trở lại di chuyển tại vị trí hiện tại. Tồn kho, giá, số dư và tiền thừa được đồng bộ với giao dịch.

## Yêu cầu

- macOS với Xcode Command Line Tools (`xcode-select --install`)
- CMake 3.24 trở lên (`brew install cmake`)
- Python 3 và GLAD generator (`python3 -m pip install glad2`)
- Kết nối mạng trong lần cấu hình đầu tiên để CMake tải GLFW, GLM, GLAD và Dear ImGui
- VS Code (khuyến nghị extension C/C++ và CMake Tools)

OpenGL trên macOS được cung cấp bởi hệ điều hành; không cần cài riêng. Dự án yêu cầu OpenGL 3.3 Core Profile.

## Build và chạy trên macOS

Từ thư mục gốc của dự án:

```bash
python3 -m venv .venv
.venv/bin/python -m pip install glad2
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \
  -DPython_EXECUTABLE="$PWD/.venv/bin/python"
cmake --build build --parallel
./build/VendingMachine3D
```

Trong VS Code, mở thư mục dự án, chọn kit compiler bằng CMake Tools, sau đó chọn **Build** và **Run**. Nếu CMake không tìm thấy OpenGL hoặc compiler, kiểm tra Xcode Command Line Tools trước.

## Điều khiển

- **W / S:** Đi tới / lùi theo hướng nhìn; **A / D:** Đi ngang trái / phải.
- **Chuột:** Xoay đầu; **Shift:** Đi nhanh. Camera có rung bước chân nhẹ.
- **E:** Tương tác khi đứng gần phía trước và nhìn về máy; camera chuyển mượt sang góc chính diện, tự căn khoảng cách để thấy trọn máy.
- **ESC:** Thoát tương tác để tiếp tục đi. Trong chế độ đi bộ, ESC thả chuột; click để bắt chuột lại. Mất focus cũng thả chuột và ngừng đi.
- Trong chế độ tương tác: click sản phẩm hoặc chọn trong danh sách, dùng **mũi tên trái/phải** để đổi món, **Enter** để mua.
- Chọn **10.000đ / 20.000đ / 50.000đ**: tay và tờ tiền 3D xuất hiện, camera chuyển mượt tới khu vực thanh toán. Dùng chuột hướng tâm vào khe nhận tiền, nhấn **E** để đưa tiền vào. Máy kéo tờ tiền vào trong rồi mới cộng số dư; có thể lặp lại để nạp thêm.
- **MUA HÀNG** mua sản phẩm; **TRẢ LẠI TIỀN** chạy hoạt ảnh đưa tiền ra khe hoàn và tay nhận lại. **ESC** hủy giao dịch, đợi xử lý tiền đang chạy hoàn tất rồi hoàn số dư trước khi thoát.
- Trong lúc đưa/nhận tiền và nhả hàng, các thao tác giao dịch xung đột bị khóa. Mọi giá và số dư dùng VNĐ.
- Âm thanh mô-tơ nhận/trả tiền được tổng hợp qua AudioToolbox trên macOS, không cần tải file âm thanh. Các nền tảng khác hiện không phát âm thanh này.

Tay được dựng phân cấp từ cổ tay, lòng bàn tay, bốn ngón và ngón cái. Tiền là tấm 3D mỏng có màu, viền, họa tiết và mệnh giá; đây là hình minh họa procedural. Luồng hoàn tiền tách tổng số dư thành các tờ phù hợp. Tay cho thao tác lấy và uống nước sẽ cần bổ sung cùng hệ thống cầm đồ uống.

Tỉ lệ mô hình hiện tại: 1 đơn vị thế giới = 0,4 m, máy cao khoảng 2,18 m. Mắt người chơi ở 1,65 m trên mặt sàn, tốc độ đi 1,4 m/s và đi nhanh 2,8 m/s. Có va chạm với thân máy; không đi xuyên qua máy.

Giao diện dùng phông Arial có sẵn trên macOS để hiển thị đầy đủ dấu tiếng Việt.

## Cấu trúc source

```text
VendingMachine3D/
├── CMakeLists.txt
├── README.md
├── assets/
│   ├── fonts/
│   ├── models/
│   └── textures/
├── include/
│   ├── Animation.h
│   ├── Camera.h
│   ├── Interaction.h
│   ├── Payment.h
│   ├── Product.h
│   ├── Renderer.h
│   └── VendingMachine.h
├── shaders/
│   ├── fragment.glsl
│   └── vertex.glsl
└── src/
    ├── Animation.cpp
    ├── Camera.cpp
    ├── Interaction.cpp
    ├── main.cpp
    ├── Payment.cpp
    ├── Product.cpp
    ├── Renderer.cpp
    └── VendingMachine.cpp
```

`Renderer` dựng cabinet, kệ, sản phẩm, kính, keypad, khe tiền và ánh sáng bằng mesh procedural nên hiện chưa phụ thuộc asset bên ngoài. Sản phẩm 3D có hình dạng lon/chai/bao bì riêng; danh sách bên phải cung cấp tên và giá đầy đủ. Có thể thay mesh procedural bằng model/textures thực tế trong `assets/` ở giai đoạn tiếp theo. CMake tự lấy dependency vào thư mục build; mã nguồn dependency không được chép vào `src/`.

## Phân chia nhóm 3 người

1. **Đồ họa 3D:** `Renderer`, shaders và nội dung `assets/models`, `assets/textures`.
2. **Logic máy bán hàng:** `VendingMachine`, `Product`, `Payment`, cùng kiểm thử các luồng tồn kho/thanh toán.
3. **Tương tác và tích hợp:** `Camera`, `Interaction`, `Animation`, UI ImGui, tích hợp CMake và kiểm thử trên macOS.

Thống nhất interface giữa các nhánh qua header trong `include/`; tích hợp thường xuyên để tránh xung đột ở `main.cpp` và `CMakeLists.txt`.

## Hướng mở rộng realism

1. Bổ sung model PBR (máy, chai/lon, bao bì) và texture bản quyền/nhóm tự tạo.
2. Thêm vật liệu metallic/roughness, phản xạ kính, ánh sáng môi trường và bóng đổ.
3. Cải thiện ray picking, animation cơ cấu đẩy hàng, âm thanh và trạng thái hoàn tiền.
4. Bổ sung unit test cho thanh toán, tồn kho và các trường hợp cạnh biên.

## Kiểm thử

```bash
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

`CameraControls` kiểm tra di chuyển, va chạm, căn trọn máy, chuyển góc thanh toán và ngắm khe tiền. `CashWorkflow` kiểm tra nạp nhiều tờ, thời điểm cộng số dư, chặn thao tác trùng và hoàn tiền sau hoạt ảnh. Các kiểm thử này không mở cửa sổ OpenGL; cần chạy ứng dụng để đánh giá hình ảnh và âm thanh.
