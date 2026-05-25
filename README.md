# 🌶️ Cay — Bộ gõ Telex Siêu Nhỏ Gọn

[![Release](https://img.shields.io/github/v/release/tctvn/cay?style=flat-square&color=FF4500)](https://github.com/tctvn/cay/releases)
[![Size](https://img.shields.io/badge/size-22_KB-brightgreen?style=flat-square)](https://github.com/tctvn/cay/releases/latest/download/cay.exe)
[![Platform](https://img.shields.io/badge/platform-Windows-0078d7?style=flat-square)](https://github.com/tctvn/cay/releases)
[![License](https://img.shields.io/badge/license-GPL--3.0-blue?style=flat-square)](LICENSE)

Bộ gõ Telex "nhỏ mà có võ", thiết kế đặc biệt cho anh em coder hệ tối giản. 

➡️ [**Tải ngay cay.exe (22 KB)**](https://github.com/tctvn/cay/releases/latest/download/cay.exe)

---

## ✨ Điểm Ăn Tiền

- 🚀 **Siêu Nhẹ (Zero-CRT):** C++ thuần túy, đúng **22KB**, ngốn RAM gần bằng 0.
- 🛠️ **Sạch Lỗi Vặt:** Nói không với nuốt chữ, nhảy con trỏ nháy trên Chrome Omnibox, Excel hay các IDE (VSCode, CodeMirror).
- 🧠 **Smart Bypass (Gõ Code Bao Mượt):** Nhận diện từ tiếng Anh/code để nhường lại quyền gõ gốc. Nâng cấp thêm **Hard Filter (Luật phụ âm kép)** ngắt ngay từ tiếng Anh (như `style`, `class`) từ ký tự thứ 2!
- 🌐 **Cross-platform Ready:** Lõi Telex tách biệt hoàn toàn khỏi Windows API, không STL, không cấp phát động. Sẵn sàng lên macOS/Linux.
- 🗑️ **Zero-Bloat & Zero-Config:** Cắt sạch các tính năng thừa (bảng mã cũ, macro). Tập trung duy nhất vào **Unicode + Telex**. Nhấp đúp là chạy!

---

## ⌨️ Cách Xài
- Chạy `cay.exe` (icon V đỏ dưới System Tray).
- Cú pháp: `aa`=â, `oo`=ô, `ee`=ê, `dd`=đ, `w`=ă/ư/ơ.
- Dấu: `s`=sắc, `f`=huyền, `r`=hỏi, `x`=ngã, `j`=nặng, `z`=xoá.
- Bật/Tắt: `Ctrl + Shift`.

---

## 🛠️ Build Từ Source (CMake)
```bash
git clone https://github.com/tctvn/cay.git
cd cay
cmake -B build
cmake --build build --config Release
```
*File thực thi: `build/Release/cay.exe`.*

---
[GPL-3.0 License](LICENSE) © [tctvn](https://github.com/tctvn/cay).
