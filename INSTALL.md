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

## 🪟 Windows (Bản ARM64 - Tối ưu Snapdragon/Surface Pro X)
Phiên bản dịch sang mã máy native dành riêng cho chip ARM, giúp đạt hiệu suất tối đa và tiết kiệm pin nhất có thể.

* **Cài đặt:**
  ```powershell
  irm https://raw.githubusercontent.com/tctvn/cay/main/scripts/install-windows-arm.ps1 | iex
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

## 🪟 Windows (Cay Classic - Nightly)
Phiên bản cổ điển được giữ lại giao diện quen thuộc của UniKey, tối ưu cho người dùng thích hoài cổ.

* **Cài đặt:**
  ```powershell
  irm https://raw.githubusercontent.com/tctvn/cay/main/scripts/install-windows-classic.ps1 | iex
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

---

## 🤖 Android
Phiên bản bàn phím độc lập trên Android, hỗ trợ gõ tiếng Việt siêu mượt với Cay Engine và giao diện chuẩn iOS.
*(Yêu cầu: Android 7.0 trở lên)*

* **Cài đặt:** Tải file APK mới nhất từ mục Releases:
  [cay-android.apk](https://github.com/tctvn/cay/releases/latest/download/cay-android.apk)
  *(Tải về, mở file và chọn Cài đặt. Sau khi cài đặt, mở ứng dụng Cay trên màn hình chính để cấp quyền kích hoạt bàn phím).*
* **Gỡ cài đặt:** Gỡ cài đặt ứng dụng Cay giống như các ứng dụng thông thường khác trên điện thoại của bạn.
