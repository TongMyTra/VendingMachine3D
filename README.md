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
- **MUA HÀNG** mua sản phẩm; **TRẢ LẠI TIỀN** chạy hoạt ảnh đưa tiền ra khe hoàn; nhấn **E** để tờ tiền bay về phía người chơi, tay giữ gần camera. **ESC** hủy giao dịch, đợi xử lý tiền đang chạy hoàn tất rồi hoàn số dư trước khi thoát.
- Trong lúc đưa/nhận tiền và nhả hàng, các thao tác giao dịch xung đột bị khóa. Mọi giá và số dư dùng VNĐ.
- Âm thanh mô-tơ nhận/trả tiền được tổng hợp qua AudioToolbox trên macOS, không cần tải file âm thanh. Các nền tảng khác hiện không phát âm thanh này.

Khi nạp, chỉ tờ tiền bay từ vị trí cầm tới khe rồi bị máy kéo vào; tay giữ ở gần camera. Tay được dựng phân cấp từ cổ tay, lòng bàn tay, bốn ngón và ngón cái. Tiền là tấm 3D mỏng có màu, viền, họa tiết và mệnh giá; đây là hình minh họa procedural. Luồng hoàn tiền tách tổng số dư thành các tờ phù hợp. Tay hỗ trợ nhận đồ uống, mở nắp và uống được dựng cùng hệ thống cầm đồ uống.

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

## Chọn đồ uống và nhả hàng

- Mỗi món có mã **A1–A4, B1–B4, C1–C4** trên kệ, keypad và giao diện. Ngắm bằng tâm nhìn trong POV hoặc rê chuột trong góc mua hàng để xem tên, giá, tồn kho và hiệu ứng sáng trên sản phẩm.
- Click nút trên keypad 3D, chọn trong danh sách hoặc dùng mũi tên: bàn tay tiến tới nút, ngón trỏ nhấn, nút lún rồi nhả lại; nút đã chọn sáng và có tiếng bấm. Màn hình máy hiển thị thông tin món, số dư và yêu cầu xác nhận/thiếu tiền/hết hàng.
- **MUA HÀNG / Enter** xác nhận thanh toán. Khi hợp lệ, máy trừ tiền, giữ giao dịch đang chạy, quay lò xo và đẩy sản phẩm về trước. Lon/chai rơi theo gia tốc, nghiêng và trượt/bật nhẹ trên máng tới khay. Có tiếng mô-tơ và va chạm; đèn khay sáng khi hàng đến nơi.
- Tiền thừa tự nhô ra tại khe hoàn, chờ **E** để nhận. Sản phẩm nằm lại trong khay, nhấn **NHẬN NƯỚC TRONG KHAY** để nhận và mở lượt mua tiếp theo. Nút nhận đưa đồ uống từ khay tới bàn tay và trở lại POV; chưa có ray casting nhặt trực tiếp tại khay.
- Thiếu tiền, hết hàng, đang nhấn nút/nhả hàng hoặc khay chưa được nhận sẽ không tạo thêm giao dịch. Lò xo, đường rơi và va chạm là mô phỏng procedural, không dùng bộ máy vật lý.

`VendingWorkflow` kiểm tra mã sản phẩm, ray casting keypad, hover, nút lún, các giai đoạn chuyển động, tồn kho, khóa giao dịch, sản phẩm trong khay và hoàn tiền thừa đúng một lần. Hình ảnh/âm thanh vẫn cần đánh giá khi chạy cửa sổ OpenGL.

## Khui lon, mở chai và uống trong POV

- Nhận đồ uống bằng **NHẬN NƯỚC TRONG KHAY**. Camera trở lại chế độ đi bộ; đồ uống và bàn tay chuyển mượt từ khay lên vị trí cầm.
- **F:** đưa lon/chai lại gần để mở. Lon có khoen bật xoay quanh bản lề, nắp nhỏ bị đẩy xuống, lỗ uống và bọt khí; chai có tay thứ hai xoay nắp nhiều vòng, nâng theo trục cổ chai rồi tháo nắp. Tiếng khui, mở nắp và uống được tổng hợp trên macOS.
- Trạng thái mở được giữ lại; F không thực hiện lại khi đã mở. Đồ uống chưa mở không thể uống.
- **Giữ chuột trái hoặc E** trong POV để đưa đồ uống lên, nghiêng khoảng 75° và uống. Thả nút để hạ xuống; dung tích chỉ giảm trong lúc uống và không đặt lại khi uống tiếp.
- HUD hiển thị dung tích: lon 330 ml, chai 500 ml, ly 400 ml. Khi hết sẽ ngừng uống; Đi đến sọt rác bên trái, nhìn vào sọt và nhấn **E/Q** để bỏ vỏ rỗng rồi cầm món tiếp theo.
- Tiền hoàn và tiền thừa nhô ra ở khe rồi chờ **E**. Sau khi nhấn, chỉ tiền bay về, tay không tiến tới khe. Tiền chỉ được đánh dấu đã nhận khi hoạt ảnh bay hoàn tất; không thể nhận lặp lại. Khi hủy, ESC đợi nhận tiền xong mới thoát tương tác.
- Nút sản phẩm hết hàng bị vô hiệu hóa trong danh sách và không chạy hoạt ảnh bấm trên keypad; vẫn có cảnh báo nếu cố xác nhận mua món hết hàng.

`HeldDrinkWorkflow` kiểm tra nhặt đồ, mở lon/chai một lần, cấm uống khi chưa mở, dừng/tiếp tục uống, dung tích không âm và xử lý vỏ rỗng. `CashWorkflow` và `VendingWorkflow` kiểm tra bước chờ E nhận tiền và ngăn nhận trùng. Các bàn tay, nắp, khoen và bọt khí hiện là mô hình procedural; kiểm thử tự động không đánh giá chất lượng hình ảnh/âm thanh.

## Hiển thị và demo mở rộng

| Điều khiển | Chức năng |
| --- | --- |
| **Z** | Solid có vật liệu/nhãn texture ↔ lưới tam giác Wireframe |
| **X** | X-Ray: ẩn vỏ, kính, bảng hiệu để xem kệ, lò xo, thanh đẩy, máng và đường rơi |
| **C** | Camera quan sát bên ngoài ↔ POV, chuyển mượt và giữ vị trí POV |
| **V** | Bật/tắt camera điện ảnh khi nhả hàng; cận cảnh cơ cấu rồi theo sản phẩm, trở về POV sau cảnh |
| **R** | Quan sát đồ uống đang cầm: kéo chuột trái xoay 360°, nghiêng; cuộn để zoom |
| **R / ESC** | Thoát quan sát đồ uống |
| **E**, nhìn xuống khay | Nhận đồ uống vào tay nếu ở đủ gần và tay chưa có đồ |
| **E**, nhìn vào bàn bên phải | Đặt đồ uống đang cầm lên bàn |
| **E**, ngắm đồ uống trên bàn | Nhặt lại, giữ nguyên trạng thái mở và dung tích |
| **E/Q**, ngắm sọt bên trái | Ném vỏ rỗng vào sọt; không bỏ đồ uống còn nước |

Các nút Solid/Wireframe, X-Ray và Cinema cũng xuất hiện trên HUD; ESC trong POV thả chuột để bấm. Khi quan sát đồ uống hoặc xem camera điện ảnh, di chuyển nhân vật tạm khóa. Camera điện ảnh tự dùng góc cắt vỏ để thấy cơ cấu bên trong và khôi phục góc nhìn đã lưu khi kết thúc.

Nhãn lon/chai là texture RGB procedural có tên, mã và giá, ánh xạ quanh thân trụ bằng UV hình trụ. Không dùng giọt nước trên bề mặt; hơi lạnh xuất hiện ngắn sau khi nhận hàng. Mặt bàn và sọt là vật thể trong cảnh có vùng va chạm; thao tác đặt, nhặt, bỏ dùng hướng nhìn và giới hạn khoảng cách. Hiện quản lý **một đồ uống** (đang cầm hoặc trên bàn) mỗi lần.

Demo: nạp 50.000đ, chọn **A1 giá 15.000đ**, xác nhận để xem lò xo và đường rơi; nhấn E nhận 35.000đ tiền thừa. Nhận lon, R xem nhãn/khoen, R thoát, F khui rồi giữ chuột trái/E để uống. Có thể đặt lên bàn và nhặt lại giữa chừng; khi hết thì đến sọt để bỏ vỏ.

`CameraControls` kiểm tra chuyển camera ngoài/điện ảnh và phục hồi POV. `HeldDrinkWorkflow` kiểm tra xoay/zoom, không uống trong chế độ quan sát, đặt/nhặt giữ trạng thái, và chỉ ném được vỏ rỗng. Trên macOS, `RendererModes` tạo context OpenGL ngoài màn hình để biên dịch shader, vẽ texture và ba chế độ, kiểm tra chúng khác nhau và khôi phục trạng thái polygon trước UI. Bài này được bỏ qua nếu môi trường không cung cấp context GPU; có thể chạy trực tiếp `./build/RendererSmokeTests` ngoài sandbox.

## Bảng hướng dẫn theo tình huống

Góc trên trái chỉ hiển thị **danh sách phím có thể dùng trong trạng thái hiện tại**. Đây là bảng chỉ đọc, không bắt chuột/bàn phím, không có nút bấm, kéo hoặc thu gọn; không cần Tab để dùng menu.

Khi tương tác với máy: **1 / 2 / 3** chọn tờ 10.000đ / 20.000đ / 50.000đ; **E** đưa tiền vào khe khi ngắm đúng, nhận tiền hoàn hoặc nhận nước đã đến khay; **mũi tên trái/phải** chọn món, **chuột trái** nhấn keypad 3D, **Enter** xác nhận mua, **Esc** hủy/hoàn tiền và thoát. Khi cầm nước, quan sát, uống hoặc đến gần bàn/sọt, bảng tự thay các dòng hướng dẫn tương ứng.

Camera điện ảnh mặc định tắt (V bật lại nếu cần). Góc tương tác chính diện đã tiến gần hơn nhưng vẫn căn để thấy trọn máy. Dung tích hiển thị bằng thanh ngang có số ml, giảm khi uống. Để bỏ vỏ, đứng gần và ngắm vào thân hoặc miệng sọt rồi nhấn E/Q; không cần ngắm đúng một điểm nhỏ. Máng trượt nằm phía trong mặt ốp dưới máy, nhìn rõ khi bật X-Ray.

## Môi trường sảnh trường

Cảnh nền được dựng trực tiếp bằng mesh OpenGL, lấy cảm hứng từ ảnh tham chiếu VKU: tường sáng, mảng xanh–vàng, lam ngang, kính lớn với khung kim loại và sân vườn phía ngoài. Ghế nan gỗ, chậu cây dạng cọ và đèn trần đặt xa khu vực thao tác. Người chơi vẫn quan sát ở độ cao mắt 1,65 m; tường, ghế và chậu cây có vùng va chạm.

Sàn dùng họa tiết gạch procedural với đường ron, biến thiên màu nhỏ và điểm sáng bóng. Phản chiếu phẳng được render vào framebuffer ở nửa độ phân giải mỗi chiều, làm mờ nhẹ rồi trộn với sàn theo góc nhìn; đây là phản chiếu hình học thực trong cảnh, không phải ảnh nền. Các lớp bóng tiếp xúc gần máy giúp vật thể bám sàn. Ánh sáng hiện là mô phỏng dịu bằng ambient, diffuse và specular, chưa dùng GI hoặc shadow map toàn cảnh.

Bài `RendererModes` cũng kiểm tra lượt phản chiếu khôi phục đúng framebuffer của ứng dụng và cấp lại kích thước khi đổi viewport. Chữ nhỏ không được vẽ trong lượt phản chiếu để giảm chi phí. Nhãn lon/chai và các thao tác mua nước giữ nguyên.
