# Hướng dẫn cài đặt sau khi pull code

Tài liệu này hướng dẫn cách lấy code từ GitHub về, mở bằng STM32CubeIDE, build và nạp vào board **STM32F103C8T6 (Blue Pill)**.

Repo: <https://github.com/NguyenDuyNam29032002/vixuly>

---

## 1. Yêu cầu

| Thành phần | Phiên bản khuyến nghị | Ghi chú |
|---|---|---|
| [Git](https://git-scm.com/downloads) | Bất kỳ | Để clone / pull code |
| [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html) | ≥ 1.18 (CubeMX 6.15) | Đã kèm compiler `arm-none-eabi-gcc` |
| STM32Cube FW_F1 | V1.8.7 | Chỉ cần nếu muốn sinh lại code từ `BTL.ioc` |
| Driver ST-Link | Kèm theo CubeIDE | Để nạp / debug |
| Phần cứng | Blue Pill + ST-Link V2 | Xem sơ đồ nối dây trong [README.md](README.md) |

---

## 2. Lấy code về

### Lần đầu (clone)

```bash
git clone https://github.com/NguyenDuyNam29032002/vixuly.git
```

Lệnh trên tạo thư mục `vixuly/` chứa toàn bộ project (`BTL.ioc`, `.project`, `.cproject`, `Core/`, `Drivers/`…).

> [!TIP]
> Nên clone vào đường dẫn **không có dấu tiếng Việt và khoảng trắng**, ví dụ `D:\stm32\vixuly`, để tránh lỗi build của Eclipse/GCC.

### Các lần sau (cập nhật code mới)

```bash
cd vixuly
git pull
```

Sau khi pull, trong CubeIDE chọn project và nhấn **F5 (Refresh)** để IDE nhận file mới.

---

## 3. Mở project trong STM32CubeIDE

1. Mở STM32CubeIDE, chọn một **workspace** bất kỳ (nên để workspace **khác** thư mục repo).
2. `File → Open Projects from File System…`
3. Bấm **Directory…** → chọn thư mục `vixuly` vừa clone → **Finish**.
4. Project sẽ hiện trong **Project Explorer** với tên `BTL`.

> [!NOTE]
> Không dùng `File → Import → Existing Projects` với tùy chọn *Copy projects into workspace* nếu bạn muốn tiếp tục `git pull`/`git push` trực tiếp trên thư mục repo.

---

## 4. Build

1. Chuột phải project `BTL` → **Build Project** (hoặc `Ctrl + B`).
2. Thư mục `Debug/` sẽ được tạo tự động, chứa `BTL.elf`, `BTL.map`…
3. Kết quả đúng: cửa sổ **Console** báo `Build Finished. 0 errors`.

> [!IMPORTANT]
> Thư mục `Debug/` **không** được đưa lên Git (đã khai báo trong `.gitignore`). Mỗi máy tự build lại.

---

## 5. Nạp và debug

1. Nối ST-Link với Blue Pill: `SWDIO`, `SWCLK`, `GND`, `3.3V`.
2. `Run → Debug As → STM32 C/C++ Application` → **OK** ở hộp thoại cấu hình (giữ mặc định ST-LINK GDB server).
3. Bấm **Resume (F8)** để chạy chương trình.
4. Muốn xem thời gian nhấn nút theo thời gian thực: thêm `g_last_press_ms`, `g_last_event`, `g_press_count` vào **Live Expressions**.

---

## 6. (Tùy chọn) Sinh lại code từ `BTL.ioc`

- Mở `BTL.ioc` → `Project → Generate Code` (`Alt + K`).
- Nếu được hỏi tải **STM32Cube FW_F1 V1.8.7**, chọn đồng ý (cần Internet).
- Code ứng dụng nằm trong các vùng `/* USER CODE BEGIN … */` và các file riêng (`button_ic.c`, `led_effects.c`) nên sẽ **không bị mất**.

> [!WARNING]
> **Không bật TIM2 trong CubeMX.** TIM2 được cấu hình trực tiếp bằng thanh ghi trong `button_ic.c`; bật TIM2 trong `.ioc` sẽ gây trùng hàm `TIM2_IRQHandler()`.

---

## 7. Lỗi thường gặp

| Hiện tượng | Cách xử lý |
|---|---|
| Project không hiện sau khi import | Kiểm tra đã chọn đúng thư mục chứa file `.project` |
| `make: *** No rule to make target` | `Project → Clean…` rồi build lại |
| Không thấy file mới sau `git pull` | Chọn project → `F5` (Refresh) |
| `No ST-LINK detected` | Kiểm tra dây, cài lại driver ST-Link, thử cổng USB khác |
| `multiple definition of TIM2_IRQHandler` | Tắt TIM2 trong `BTL.ioc` rồi Generate Code lại |
| Đổi tên thư mục repo làm hỏng build | `Project → Clean…` để xóa cache đường dẫn cũ, rồi build lại |

---

## 8. Đẩy thay đổi lên repo

```bash
git add .
git commit -m "Mô tả thay đổi"
git push
```
