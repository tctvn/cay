# Cay — Bộ gõ Telex. Vậy thôi.

> Tôi gõ Telex. Tại sao phải cài cả một bộ gõ hỗ trợ VNI, VIQR, 12 kiểu gõ, bảng điều khiển, cập nhật tự động, và đủ thứ tôi không bao giờ dùng?
>
> Nên tôi tự làm cái này. Một file. Chạy được. Dùng hằng ngày.

➡️ [**Cay.exe**](https://github.com/tctvn/cay/releases/download/cay/Cay.exe) — tải về, double-click, xong.

> ⚠️ **Windows 64-bit** · **.NET Framework 4.8** (có sẵn trên Win 10/11)

---

## Nó làm được gì

- **25 KB** — nhỏ hơn một cái ảnh JPEG bình thường
- **Không cài đặt** — không registry rác, không DLL lung tung, xóa file là hết
- **Tự bật cùng Windows** — lần đầu chạy hỏi một lần, sau đó im
- **Tự nhận ra khi không phải tiếng Việt** — gõ `loading`, `ctrl+c`, tên file, địa chỉ email... bộ gõ tự nhận diện, không cần tắt tay
- **Ctrl+Shift** để tắt/bật nếu cần

---

## Telex

### Nguyên âm

| Gõ | Ra | | Gõ | Ra |
|----|----|-|----|-----|
| `aa` | â | | `ow` | ơ |
| `aw` | ă | | `uw` | ư |
| `ee` | ê | | `dd` | đ |
| `oo` | ô | | | |

### Dấu thanh

| Phím | Dấu |
|------|-----|
| `z` | xóa dấu |
| `s` | sắc |
| `f` | huyền |
| `r` | hỏi |
| `x` | ngã |
| `j` | nặng |

### Thực tế

| Gõ | Ra |
|----|----|
| `xin chaof` | xin chào |
| `truwowngf` | trường |
| `nguwowif` | người |
| `ddaats` | đất |
| `hoas` | hóa |
| `loading` | loading *(không đụng vào)* |

---

## Yêu cầu

- Windows 7 / 8 / 10 / 11 — **64-bit**
- .NET Framework 4.8 — có sẵn trên Win 10/11, [tải tại đây](https://dotnet.microsoft.com/download/dotnet-framework/net48) nếu dùng Win 7/8

---

## Cấu trúc code

```
src/
├── Program.cs       # Entry point + UI (tray, hooks, toggle)
├── CayEngine.cs     # IME engine: xử lý phím, buffer, trạng thái
├── CayProcessor.cs  # Xử lý nội bộ: dấu, vần, nguyên âm đôi
├── CayData.cs       # Bảng quy tắc Telex + validator
└── NativeInput.cs   # P/Invoke: hooks bàn phím/chuột + gửi phím
```

MIT License
