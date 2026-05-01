# Cay - Bộ gõ tiếng Việt

Bộ gõ tiếng Việt đơn giản cho Windows, tham khảo từ [OpenKey](https://github.com/tuyenvm/openkey).

## Tính năng

- **Kiểu gõ:** Telex chuẩn
- **Bảng mã:** Unicode
- **Gõ tự do:** Luôn bật, đặt dấu ở bất kỳ vị trí nào
- **Hotkey:** `Ctrl + Shift` để bật/tắt
- **Tray Icon:**
  - `V` đỏ = Đang bật
  - `E` xanh = Đang tắt
  - Click trái để bật/tắt

## Gõ Telex

| Gõ | Ra |
|---|---|
| `aa` | â |
| `aw` | ă |
| `dd` | đ |
| `uw` | ư |
| `ow` | ơ |
| `as` | á |
| `af` | à |
| `ar` | ả |
| `ax` | ã |
| `aj` | ạ |

## Menu chuột phải

- **Bật/Tắt bộ gõ** - Toggle trạng thái
- **Tự khởi động** - Khởi động cùng Windows (ghi vào Registry)
- **Thông tin** - Xem thông tin và phím tắt
- **Thoát** - Tắt phần mềm

## Cài đặt

Không cần cài đặt. Chạy `Cay.exe` là xong.

Cài đặt được lưu trong Registry, không cần file cấu hình.

## Build

```bash
dotnet build -c Release
```

File chạy: `bin\Release\net48\Cay.exe`
