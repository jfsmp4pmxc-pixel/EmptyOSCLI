# EmptyOSCLI
<img width="1372" height="943" alt="IMG_3684" src="https://github.com/user-attachments/assets/d98ba5f5-ec34-4eb7-b3df-3cf27faf1d23" />


# EmptyOS CLI Edition v0.4:

**EmptyOS** là một hệ điều hành mã nguồn mở siêu nhẹ (hobby OS) được phát triển cho kiến trúc x86. Phiên bản v0.4 đánh dấu bước ngoặt lớn với hệ thống File System chạy hoàn toàn trên RAM (Initrd) và một Shell tương tác mạnh mẽ.

## Tính năng mới trong v0.4
- **RAM Disk Driver**: Loại bỏ hoàn toàn lỗi IDE Timeout trên các thiết bị giả lập yếu (như iPhone 6s Plus qua UTM SE).
- **EFS v2 (Empty File System)**: Hệ thống quản lý file một cấp với cấu trúc Directory Entry 32-byte.
- **Improved Shell**: Hỗ trợ Command Highlight (màu sắc lệnh), hiển thị đường dẫn `root@EmptyOS:/#`.
- **File Management**: Hỗ trợ đầy đủ các lệnh `touch`, `echo` (với redirection `>`), `cat`, `ls`.

## Yêu cầu hệ thống
- **Emulator**: UTM SE (iOS), QEMU, hoặc bất kỳ trình giả lập x86 nào.
- **Architecture**: i386 (32-bit).
- **Memory**: Tối thiểu 1MB RAM.

## Cấu trúc File System (EFS)
Hệ điều hành sử dụng **Sector 100** làm "Bảng mục lục" (Directory Table) và các Sector từ **101-116** để lưu trữ dữ liệu thực tế của từng file.

| Byte Range | Description |
|------------|-------------|
| 0 - 11     | Tên file (Filename) |
| 12         | Cờ tồn tại (Presence Flag) |
| 13         | Chỉ số Sector dữ liệu |
| 14 - 15    | Kích thước file (Size in bytes) |

## Các lệnh được hỗ trợ
- `help`: Hiển thị danh sách lệnh.
- `cls`: Xóa sạch màn hình console.
- `ls`: Danh sách các file hiện có cùng dung lượng và vị trí sector.
- `touch <filename>`: Tạo một file mới trống.
- `echo <content>`
- `cat <filename>`: Đọc nội dung file đã lưu.
- `format`: Khởi tạo lại toàn bộ vùng RAM Disk.
## lưu ý:
một số tính năng không hoạt động.
