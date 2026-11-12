# 3D Interactive Vending Machine Simulation

Mô phỏng máy bán hàng tự động 3D viết bằng C++17 và OpenGL, với phong cách kim loại đen/xám bạc, cửa kính trong, 5 tầng kệ, đèn LED, LCD, keypad, khe tiền và khay nhận hàng. Các sản phẩm procedural gồm lon Coca-Cola/Pepsi, chai nước, snack và bánh.

Giao diện được chia hai vùng: máy 3D có thể xoay/zoom ở bên trái và bảng chọn sản phẩm/thanh toán 2D ở bên phải. Chọn hàng, tồn kho, giá, credit, tiền thừa và trạng thái LCD được đồng bộ cùng một luồng giao dịch.

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

- Click sản phẩm trong khung 3D hoặc chọn trong danh sách ImGui.
- Nhấn **Nạp $1**, **Nạp $2**, **Nạp $5** để nạp tiền mô phỏng.
- Nhấn **MUA HÀNG** để mua; tiền thừa được trả tự động và hiển thị trên bảng điều khiển.
- **TRẢ LẠI TIỀN** trả số dư còn lại qua khe trả tiền.
- Giữ chuột phải để xoay camera, cuộn để thu phóng; dùng `A`/`D` hoặc phím mũi tên để chuyển lựa chọn.
- Nhấn `Enter` để mua sản phẩm đang chọn.

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
