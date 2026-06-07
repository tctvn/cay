# Hướng Dẫn Cài Đặt & Gỡ Cài Đặt 🚀

Dưới đây là danh sách toàn bộ các kịch bản dòng lệnh tự động tải, cài đặt và cấu hình khởi động cho các phiên bản của Cay. 
Bạn chỉ cần mở Terminal hoặc PowerShell (tương ứng với hệ điều hành) và dán các lệnh sau.

## 🪟 Windows (Bản Minimal)
Phiên bản tối giản, siêu nhẹ (20KB), chỉ có tính năng gõ tiếng Việt.

* **Cài đặt:**
  ```powershell
  irm https://raw.githubusercontent.com/tctvn/cay/main/scripts/install-windows.ps1 | iex
  ```
* **Gỡ cài đặt:**
  ```powershell
  irm https://raw.githubusercontent.com/tctvn/cay/main/scripts/uninstall-windows.ps1 | iex
  ```

---

## 🪟 Windows (Bản Full - Có GUI & Macro)
Phiên bản mở rộng, tích hợp thêm giao diện cấu hình trực quan và tính năng Bảng Gõ Tắt (Macro).

* **Cài đặt:**
  ```powershell
  irm https://raw.githubusercontent.com/tctvn/cay/main/scripts/install-windows-full.ps1 | iex
  ```
* **Gỡ cài đặt:**
  ```powershell
  irm https://raw.githubusercontent.com/tctvn/cay/main/scripts/uninstall-windows.ps1 | iex
  ```

---

## 🍏 macOS
Phiên bản dành cho Apple Silicon & Intel. Hỗ trợ đầy đủ tính năng gõ tiếng Việt mượt mà.

* **Cài đặt:**
  ```bash
  curl -sL https://raw.githubusercontent.com/tctvn/cay/main/scripts/install-mac.sh | bash
  ```
* **Gỡ cài đặt:**
  ```bash
  curl -sL https://raw.githubusercontent.com/tctvn/cay/main/scripts/uninstall-mac.sh | bash
  ```

---

## 🐧 Linux (Fcitx5 - Nightly)
Phiên bản dạng Plugin dành riêng cho engine Fcitx5 trên Linux. *(Lưu ý: Đang thử nghiệm)*

* **Cài đặt:**
  ```bash
  wget -qO- https://raw.githubusercontent.com/tctvn/cay/main/scripts/install-fcitx5.sh | bash
  ```
* **Gỡ cài đặt:**
  ```bash
  wget -qO- https://raw.githubusercontent.com/tctvn/cay/main/scripts/uninstall-fcitx5.sh | bash
  ```
