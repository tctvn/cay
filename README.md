# 🌶️ Cay — Bộ gõ Telex Siêu Nhỏ Gọn

[![Release](https://img.shields.io/github/v/release/tctvn/cay?style=flat-square&color=FF4500)](https://github.com/tctvn/cay/releases)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20macOS-0078d7?style=flat-square)](https://github.com/tctvn/cay/releases)
[![License](https://img.shields.io/badge/license-GPL--3.0-blue?style=flat-square)](LICENSE)

Bộ gõ Telex "nhỏ mà có võ", thiết kế đặc biệt cho anh em coder hệ tối giản.

➡️ [**Tải ngay cay.exe cho Windows (22 KB)**](https://github.com/tctvn/cay/releases/latest/download/cay.exe)  
➡️ [**Tải ngay cay.zip cho macOS 11+ (Intel & Apple Silicon)**](https://github.com/tctvn/cay/releases/latest/download/cay.zip)

> ⚠️ **Lưu ý quan trọng về Bảo mật (Cảnh báo Virus giả):** Do đặc thù bộ gõ cần can thiệp sâu vào hệ thống bàn phím (sử dụng kĩ thuật Keyboard Hook), trình duyệt (Chrome, Edge) hoặc Windows SmartScreen/Defender có thể báo cáo file tải về là tệp nguy hiểm hoặc virus. Đây là **Positive False (Báo cáo nhầm)**. Dự án hoàn toàn mã nguồn mở, bạn có thể tự kiểm chứng hoặc build từ Source để an tâm tuyệt đối!

---

## ✨ Điểm Ăn Tiền

- 🚀 **Siêu Nhẹ (Zero-CRT):** C++ thuần túy, tốn cực ít dung lượng, ngốn RAM gần bằng 0.
- 🛠️ **Sạch Lỗi Vặt:** Nói không với nuốt chữ, nhảy con trỏ nháy trên Chrome Omnibox, Excel hay các IDE (VSCode, CodeMirror).
- 🧠 **Smart Bypass (Gõ Code Bao Mượt):** Nhận diện từ tiếng Anh/code để nhường lại quyền gõ gốc.
- 🌐 **Đa nền tảng:** Hỗ trợ hoàn hảo Windows 10/11 và macOS 11 Big Sur trở lên (Universal Binary cho cả Intel và Apple Silicon).
- 🗑️ **Zero-Bloat & Zero-Config:** Cắt sạch các tính năng thừa. Tập trung duy nhất vào **Unicode + Telex**. Nhấp đúp là chạy!

---

## ⌨️ Cách Xài

### 🪟 Trên Windows
- Tải file `cay.exe` về và chạy (icon V màu đỏ dưới System Tray).
- Bật/Tắt bộ gõ: **Ctrl + Shift**.

### 🍏 Trên macOS
1. Tải và giải nén `cay.zip`, copy `cay.app` vào thư mục **Applications**.
2. Mở ứng dụng lần đầu, hệ thống sẽ yêu cầu cấp quyền Accessibility (Trợ năng). Hãy vào **System Settings > Privacy & Security > Accessibility** và cấp quyền cho `cay.app`.
3. Bật/Tắt bộ gõ: **Cmd + Shift** (Khuyên dùng: tắt bộ gõ mặc định của macOS để tránh đụng độ phím tắt).

### 📝 Cú pháp Telex chuẩn
- `aa`=â, `oo`=ô, `ee`=ê, `dd`=đ, `w`=ă/ư/ơ.
- `s`=sắc, `f`=huyền, `r`=hỏi, `x`=ngã, `j`=nặng, `z`=xoá dấu.

---

## 🛠️ Build Từ Source (CMake)

```bash
git clone https://github.com/tctvn/cay.git
cd cay
cmake -B build
cmake --build build --config Release
```
* Trên Windows: File thực thi nằm tại `build/Release/cay.exe`.
* Trên macOS: App bundle nằm tại `build/cay.app`.

---
[GPL-3.0 License](LICENSE) © [tctvn](https://github.com/tctvn/cay).
