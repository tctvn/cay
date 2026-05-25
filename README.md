# 🌶️ Cay — Bộ gõ tiếng Việt siêu nhỏ gọn

> Bộ gõ Telex tối giản dành cho Windows. Không cài đặt, không giao diện thừa, tập trung 100% vào tốc độ và trải nghiệm gõ.

[![Release](https://img.shields.io/github/v/release/tctvn/cay?style=flat-square&color=FF4500)](https://github.com/tctvn/cay/releases)
[![Size](https://img.shields.io/badge/size-28_KB-brightgreen?style=flat-square)](https://github.com/tctvn/cay/releases/download/cay/Cay.exe)
[![Platform](https://img.shields.io/badge/platform-Windows-0078d7?style=flat-square)](https://github.com/tctvn/cay/releases)
[![License](https://img.shields.io/badge/license-GPL--3.0-blue?style=flat-square)](LICENSE)

---

Cay là bộ gõ tiếng Việt tối giản được thiết kế nhằm thay thế các bộ gõ cồng kềnh. Tập trung hoàn toàn vào sự tinh gọn, Cay chỉ hỗ trợ kiểu gõ Telex và hoạt động độc lập không cần cấu hình.

➡️ [**Tải về Cay.exe (28 KB)**](https://github.com/tctvn/cay/releases/download/cay/Cay.exe)

> [!IMPORTANT]
> **Yêu cầu hệ thống:** Windows 10 / 11 (64-bit). Đã tích hợp sẵn .NET Framework 4.8. (Đối với Windows 7 / 8, cần cài đặt thủ công .NET Framework 4.8).

---

## ⚡ Triết lý thiết kế

Cay hướng tới sự tối giản tối đa trong vận hành:

- **Kích thước siêu nhỏ (28 KB):** Tải và chạy tức thì, sử dụng rất ít tài nguyên hệ thống.
- **Không cài đặt:** 1 file thực thi duy nhất, không tạo file rác, không ghi đè Registry. Gỡ bỏ hoàn toàn bằng cách xóa file.
- **Không tiến trình ngầm:** Không có dịch vụ chạy ẩn hoặc cập nhật ngầm làm chậm máy.

---

## 🚀 Tính năng chính

### 🧠 Tự động chuẩn chính tả (Smart Orthography)
Tự động phân tích cấu trúc nguyên âm tiếng Việt để đặt dấu thanh chuẩn xác theo ngữ pháp. Hỗ trợ tự động dịch chuyển vị trí dấu tương ứng với cấu trúc từ (Ví dụ: `hoas` ➔ `hóa`, nhưng `hoans` ➔ `hoán`).

### ⚡ Tương thích cao (Chrome & Excel)
Được tối ưu để hoạt động ổn định trên các ứng dụng thường gặp lỗi gõ tiếng Việt, ví dụ như Google Chrome (khắc phục lỗi autocomplete trên thanh địa chỉ gây ra từ dạng "goôgle") và Microsoft Excel.

### 🔌 Không cần cấu hình (Zero-Config)
Hoạt động ngay sau khi mở mà không cần thiết lập bảng mã hay phím kích hoạt phức tạp.

### 🌐 Tự động nhận diện ngữ cảnh (Context-Aware)
Tự động nhận diện và tạm ngưng bộ gõ khi người dùng viết code, viết tiếng Anh hoặc sử dụng phím tắt, giúp hạn chế việc phải chuyển chế độ thủ công.

### 🤫 Hoạt động ẩn
Không có giao diện người dùng (UI) hay thông báo pop-up gây phân tâm. Lần đầu khởi chạy sẽ hỏi quyền tự động bật cùng Windows, sau đó hoạt động hoàn toàn ở chế độ nền.

---

## ⌨️ Quy tắc gõ Telex

Hỗ trợ kiểu gõ Telex chuẩn:

### Nguyên âm & Phụ âm kép

| Tổ hợp phím | Kết quả | | Tổ hợp phím | Kết quả |
| :--- | :--- | - | :--- | :--- |
| `aa` | **â** | | `ow` | **ơ** |
| `aw` | **ă** | | `uw` | **ư** |
| `ee` | **ê** | | `dd` | **đ** |
| `oo` | **ô** | | | |

### Quy tắc dấu thanh

| Phím gõ | Dấu thanh | Ví dụ | Kết quả |
| :---: | :--- | :--- | :--- |
| **`s`** | Sắc | `hoas` | **hóa** |
| **`f`** | Huyền | `chaof` | **chào** |
| **`r`** | Hỏi | `hoir` | **hỏi** |
| **`x`** | Ngã | `ngax` | **ngã** |
| **`j`** | Nặng | `nawjng` | **nặng** |
| **`z`** | Xóa dấu | `hoasz` | **hoa** |

### Ví dụ thực tế

| Phím gõ | Kết quả hiển thị | Ghi chú |
| :--- | :--- | :--- |
| `xin chaof` | **xin chào** | Xử lý dấu huyền |
| `truwowngf` | **trường** | Xử lý nguyên âm đôi & dấu |
| `nguwowif` | **người** | Xử lý nguyên âm ba & dấu |
| `ddaats` | **đất** | Xử lý phụ âm ghép và dấu sắc |
| `word` | **word** | Tự động nhận diện từ tiếng Anh |

*💡 Mẹo: Nhấn tổ hợp phím `Ctrl + Shift` bất kỳ lúc nào để bật hoặc tắt nhanh bộ gõ.*

---

## 🛠️ Kiến trúc hệ thống

Cấu trúc thư mục mã nguồn dành cho nhà phát triển:

```text
src/
├── Program.cs             # Điểm khởi chạy, System Tray, Đăng ký phím tắt bật/tắt nhanh (Ctrl+Shift)
├── CayEngine.cs           # Lõi xử lý chính: Quản lý bộ đệm (Buffer), theo dõi trạng thái gõ phím
├── CayProcessor.cs        # Bộ phân tích Telex: Ghép vần, xử lý bỏ dấu, kiểm tra ngữ pháp tiếng Việt
├── CayData.cs             # Cơ sở dữ liệu tĩnh: Định nghĩa bảng mã Telex & quy tắc chính tả
├── KeyboardHookManager.cs # Low-level Keyboard Hook: Đánh chặn sự kiện bàn phím cấp thấp của Windows
└── InputInjector.cs       # Giả lập nhập liệu: Gửi phím Backspace và chèn ký tự tiếng Việt mới
```

---

## 📝 Giấy phép

Dự án được phân phối dưới giấy phép **GNU General Public License v3.0 (GPL-3.0)**.
