# 🌶️ Cay — Bộ gõ Telex Siêu Nhỏ Gọn Cho Coder

[![Release](https://img.shields.io/github/v/tag/tctvn/cay?style=for-the-badge&color=FF4500&label=Version)](https://github.com/tctvn/cay/releases)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20macOS%20%7C%20Linux-0078d7?style=for-the-badge)](https://github.com/tctvn/cay/releases)
[![License](https://img.shields.io/badge/license-GPL--3.0-blue?style=for-the-badge)](LICENSE)

> **Cay** là bộ gõ Telex "nhỏ mà có võ", được viết bằng C++ thuần túy dành riêng cho các lập trình viên hệ tối giản. Không rườm rà, không giật lag, sinh ra là để tối ưu hóa hiệu suất gõ và coding mượt mà nhất có thể.

---

## 🚀 Cài Đặt Tự Động (Khuyên Dùng)

Mở Terminal/PowerShell và dán dòng lệnh tương ứng để tự động tải, cài đặt và cấu hình khởi động cùng hệ thống trong nháy mắt:

**🪟 Windows (PowerShell):**
```powershell
irm https://raw.githubusercontent.com/tctvn/cay/main/scripts/install-windows.ps1 | iex
```

**🍏 macOS (Terminal):**
```bash
curl -sL https://raw.githubusercontent.com/tctvn/cay/main/scripts/install-mac.sh | bash
```

**🐧 Linux / Fcitx5 (Bash):**
> [!WARNING]
> **Experimental / Nightly Build:** Phiên bản hỗ trợ Linux qua Fcitx5 hiện tại đang trong giai đoạn thử nghiệm. Có thể hoạt động chưa ổn định và không được đính kèm trong các bản phát hành chính thức.

```bash
wget -qO- https://raw.githubusercontent.com/tctvn/cay/main/scripts/install-fcitx5.sh | bash
```

> **Ghi chú gỡ cài đặt:** Nếu muốn gỡ cài đặt, bạn có thể tải về repo và chạy các file tương ứng trong thư mục `scripts/` (`uninstall-windows.ps1`, `uninstall-mac.sh`, `uninstall-fcitx5.sh`).

---

## 📥 Tải Xuống Thủ Công

Nếu bạn không thích dùng script tự động, bạn có thể tải trực tiếp file chạy độc lập dưới đây:

<p align="center">
  <a href="https://github.com/tctvn/cay/releases/latest/download/cay.exe">
    <img src="https://img.shields.io/badge/T%E1%BA%A3i%20xu%E1%BB%91ng%20Windows-~20%20KB-brightgreen?style=for-the-badge&logo=windows&logoColor=white&color=0078D4" alt="Download Windows" />
  </a>
  &nbsp;&nbsp;
  <a href="https://github.com/tctvn/cay/releases/latest/download/cay-mac.zip">
    <img src="https://img.shields.io/badge/T%E1%BA%A3i%20xu%E1%BB%91ng%20macOS-~40%20KB-brightgreen?style=for-the-badge&logo=apple&logoColor=white&color=000000" alt="Download macOS" />
  </a>
  &nbsp;&nbsp;
  <a href="https://github.com/tctvn/cay/releases/download/nightly/cayime-fcitx5-linux.tar.gz">
    <img src="https://img.shields.io/badge/Nightly%20Linux-~60%20KB-orange?style=for-the-badge&logo=linux&logoColor=white&color=D93F0B" alt="Download Linux Nightly" />
  </a>
</p>

- **Windows:** Chạy file `cay.exe`. Phím tắt mặc định chuyển đổi Anh/Việt là `Ctrl + Shift`.
- **macOS:** Giải nén `cay-mac.zip` và kéo `cay.app` vào Applications. Cấp quyền *Accessibility*. Phím tắt mặc định là `Cmd + Shift`.

---

## 📝 Cú pháp Telex chuẩn

* **Nguyên âm có dấu mũ/móc:** `aa` = â, `oo` = ô, `ee` = ê, `dd` = đ, `w` = ă/ư/ơ.
* **Quy tắc bỏ dấu thanh:** `s` = sắc, `f` = huyền, `r` = hỏi, `x` = ngã, `j` = nặng, `z` = xoá dấu.
* **Gõ nhanh móc:** Gõ `w` sau tổ hợp phím (ví dụ: `luuw` -> `lưu`, `huouw` -> `hươu`) vô cùng tự nhiên.

---

## ✨ Tính năng nổi bật

- 🚀 **Siêu Nhẹ (Zero-CRT):** Viết bằng C++ thuần túy, dung lượng siêu nhỏ (~20KB - 40KB), gần như không tốn RAM (< 1.5MB). Không chạy nền dịch vụ rác.
- 🛠️ **Tạm biệt lỗi nuốt chữ:** Giải quyết triệt để lỗi mất ký tự hay nhảy con trỏ trên Chrome, VS Code, IntelliJ, v.v.
- 🧠 **Smart Bypass (Nhận diện Code):** Tự động nhận diện cụm từ tiếng Anh, camelCase, snake_case để tự ngắt bỏ dấu tiếng Việt mà không cần chuyển E/V thủ công.
- 🚫 **Direct Input (Không Preedit):** Văn bản xuất hiện trực tiếp (không có nét gạch chân hay vùng nháp), không gây chớp giật màn hình khi gõ.
- ⚙️ **Zero-Config:** Tải về bật lên là sử dụng ngay.
- 🌐 **Đa Nền Tảng:** Hỗ trợ mượt mà Windows (10/11), macOS (Intel/Apple Silicon) và Linux (Fcitx5).

---

## 🛠️ Hướng Dẫn Build Từ Source Code (Dành cho Devs)

Yêu cầu máy đã cài đặt **CMake** và compiler C++ (Visual Studio Build Tools / Xcode Command Line Tools).

```bash
# 1. Clone repo về máy
git clone https://github.com/tctvn/cay.git
cd cay

# 2. Tạo thư mục build và cấu hình dự án
cmake -B build

# 3. Tiến hành compile
cmake --build build --config Release
```

* **Kết quả đầu ra:**
  * **Windows:** File thực thi tối ưu nằm tại `build/Release/cay.exe`.
  * **macOS:** App bundle cực gọn nhẹ nằm tại `build/cay.app`.

---

## 🔒 Cam kết bảo mật & Cảnh báo an toàn

> ⚠️ **Lưu ý quan trọng về cảnh báo chống Virus giả (False Positive):**  
> Do **Cay** hoạt động bằng cách chèn Keyboard Hook (bắt phím cấp hệ thống) để xử lý ký tự Telex theo thời gian thực, một số trình diệt virus có thể hiểu lầm đây là phần mềm keylogger độc hại.  
>  
> **Chúng tôi cam kết:** Mã nguồn của Cay hoàn toàn **mở 100%**, cam đoan cực kỳ sạch sẽ, không lưu log bàn phím, không kết nối internet, không telemetry. Bạn hoàn toàn có thể tự kiểm tra source code và tự build nếu cần sự an tâm tuyệt đối.

---

## 📄 Bản Quyền & Phát Triển

Phát triển bởi [tctvn](https://github.com/tctvn).  
Phân phối dưới giấy phép [GPL-3.0 License](LICENSE). Mọi đóng góp PR hoặc Issue từ cộng đồng đều được chào đón nồng nhiệt! 🚀
