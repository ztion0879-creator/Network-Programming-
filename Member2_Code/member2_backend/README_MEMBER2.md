# Thành viên 2 – Database và Backend

## Phạm vi
- SQLite: Users, Foods, Orders, OrderDetails.
- Đăng ký/đăng nhập, phân quyền Customer/Admin.
- CRUD món ăn.
- Tạo đơn hàng, lưu snapshot tên/giá tại thời điểm đặt, địa chỉ và trạng thái.
- Cung cấp `Backend::dispatch()` để Router của Thành viên 1 gọi.

## Thư viện cần cài (Windows)
1. SQLite3: header `sqlite3.h` và library `sqlite3.lib`.
2. nlohmann/json: tải `json.hpp` và đặt tại `external/nlohmann/json.hpp`, hoặc cấu hình include path tương ứng.
3. Windows SDK có BCrypt (dùng để băm mật khẩu). Link thêm `bcrypt.lib`.
4. Biên dịch C++17 trở lên, vì Database.cpp dùng `std::filesystem`.

## Thêm file vào project Visual Studio
Thêm các file:
- `server/Database.h`, `server/Database.cpp`
- `server/Auth.h`, `server/Auth.cpp`
- `server/FoodService.h`, `server/FoodService.cpp`
- `server/OrderService.h`, `server/OrderService.cpp`
- `server/Backend.h`, `server/Backend.cpp`

Cấu hình:
- C/C++ → Language → C++ Language Standard: ISO C++17.
- C/C++ → General → Additional Include Directories: đường dẫn tới thư mục chứa `sqlite3.h` và thư mục gốc `nlohmann`.
- Linker → General → Additional Library Directories: thư mục chứa `sqlite3.lib`.
- Linker → Input → Additional Dependencies: `sqlite3.lib; bcrypt.lib; %(AdditionalDependencies)`.
- Đảm bảo `sqlite3.dll` nằm cạnh file `.exe` nếu dùng SQLite dạng DLL.

## Khởi tạo và tích hợp với Router (Thành viên 1)
Tạo Backend tồn tại suốt vòng đời server, không tạo mới mỗi request:

```cpp
#include "Backend.h"

Backend backend("database/food_order.db");
if (!backend.initialize()) {
    std::cerr << "Khong khoi tao duoc database\n";
    return 1;
}

// Trong Router sau khi đã parse HttpRequest:
std::string token;
auto it = request.headers.find("Authorization");
if (it != request.headers.end()) {
    token = it->second;
    const std::string prefix = "Bearer ";
    if (token.rfind(prefix, 0) == 0) token = token.substr(prefix.size());
}
auto result = backend.dispatch(request.method, request.url, request.body, token);
response.setHeader("Content-Type", "application/json; charset=utf-8");
response.setBody(result.dump());
```

Lưu ý: `HttpRequest.headers` hiện tại giữ nguyên tên header như nhận được. Khi tích hợp, nên tìm `Authorization` không phân biệt hoa thường hoặc chuẩn hóa tên header trong parser.

## API và JSON mẫu

### POST /api/register
```json
{"username":"khoa","password":"123456","full_name":"Nguyen Van A","phone":"0900000000"}
```

### POST /api/login
```json
{"username":"khoa","password":"123456"}
```
Thành công trả `token` và `user`. Client gửi token ở header `Authorization: Bearer <token>` cho API cần đăng nhập.

### GET /api/foods
Có thể tìm kiếm: `GET /api/foods?search=ga`

### POST /api/admin/foods (Admin)
```json
{"name":"Com ga","description":"Com ga sot","price":45000,"category":"Com","image":"","available":true}
```

### PUT /api/admin/foods?id=1 (Admin)
Gửi các trường `name`, `description`, `price`, `category`, `image`, `available`.

### DELETE /api/admin/foods?id=1 (Admin)

### POST /api/orders (Customer/Admin)
```json
{
  "delivery_address":"12 Nguyen Trai, Quan 1",
  "note":"It cay",
  "items":[{"food_id":1,"quantity":2},{"food_id":2,"quantity":1}]
}
```
Server tự lấy giá hiện tại từ database, không tin giá do client gửi. Toàn bộ order và order details được lưu trong một transaction.

### GET /api/orders
Customer chỉ xem đơn của mình; Admin xem tất cả.

### PUT /api/orders/status (Admin)
```json
{"order_id":1,"status":"Confirmed"}
```
Trạng thái hợp lệ: `Pending`, `Confirmed`, `Preparing`, `Delivering`, `Completed`, `Cancelled`.

## Lưu ý quan trọng
- Tạo tài khoản đăng ký luôn có role `Customer`; không cho client tự gửi role Admin.
- Cần tạo tài khoản Admin ban đầu bằng thao tác quản trị DB an toàn (không mở API tự nâng quyền).
- Session token hiện lưu trong RAM, mất khi server khởi động lại; phù hợp bản demo lớp học, chưa phải hệ thống production.
- SHA-256 có salt tránh lưu mật khẩu thuần, nhưng production nên thay bằng Argon2id/bcrypt/PBKDF2 có work factor.
- Các hàm SQLite cần được gọi tuần tự hoặc bổ sung mutex nếu server xử lý nhiều thread dùng chung một connection.
- Đường dẫn database tương đối tính từ working directory của chương trình.
