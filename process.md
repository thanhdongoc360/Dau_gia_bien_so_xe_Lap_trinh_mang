# Quy trình hoạt động các chức năng

## Tổng quan kiến trúc
- Server TCP đa luồng, giao tiếp bằng lệnh văn bản; mỗi client kết nối -> thread `handle_client` xử lý.
- Dữ liệu lưu trữ file phẳng: `users.txt`, `rooms.txt`, `items_room*.txt`, `history.txt`, `server.log`.
- Bộ nhớ chạy: mảng `users`, `rooms` (chứa `items`, `members`), `sessions`. Mutex đảm bảo an toàn luồng.
- Tác vụ định kỳ: thread `auction_manager` tick mỗi giây kiểm tra hết giờ, cảnh báo 30s và tự động SOLD.

## Đăng ký / Đăng nhập / Phiên
1) REGISTER
- Lệnh: `REGISTER <user> <pass>`.
- Hàm: `register_user` kiểm tra trùng, thêm user (role user, display_name = username), ghi `users.txt`, log REGISTER.
- Phản hồi: `REGISTER_OK` hoặc `REGISTER_FAIL`.

2) LOGIN
- Lệnh: `LOGIN <user> <pass>`.
- Hàm: `login_user` kiểm tra mật khẩu, online flag; nếu đang online trả `LOGIN_FAIL_ALREADY_ONLINE`.
- Khi OK: cập nhật session (`update_session_login`), set user online, gửi `LOGIN_OK <role>`.

3) LOGOUT
- Lệnh: `LOGOUT`.
- Thoát khỏi room nếu đang ở (broadcast USER_LEAVE), bỏ session, set online=0, log LOGOUT.
- Phản hồi: `LOGOUT_OK`.

4) Đổi thông tin
- `CHANGE_PASS <old> <new>`: kiểm tra old, cập nhật file, log.
- `CHANGE_NAME <newDisplay>`: cập nhật display_name, log.

## Quản lý phòng (admin)
1) CREATE_ROOM
- Lệnh: `CREATE_ROOM <name>`; chỉ admin.
- Hàm: `create_room` tạo id tăng dần, trạng thái OPEN, chủ phòng = admin gọi; lưu `rooms.txt`, log.
- Phản hồi: `ROOM_CREATED <id>` hoặc FAIL.

2) OPEN_ROOM / CLOSE_ROOM
- Lệnh: `OPEN_ROOM <id>` hoặc `CLOSE_ROOM <id>`; chỉ admin của phòng.
- Hàm: bật/tắt `active`, xóa member khi close, lưu file, log.
- Phản hồi: `ROOM_OPENED` / `ROOM_CLOSED` hoặc FAIL.

3) LIST_ROOMS
- Lệnh: `LIST_ROOMS` (cần login).
- Hàm: `list_rooms` dựng danh sách từ mảng `rooms`; status OPEN/CLOSED.

## Tham gia phòng
1) JOIN_ROOM
- Lệnh: `JOIN_ROOM <id>` (cần login).
- Kiểm tra: mỗi user chỉ ở 1 phòng; phòng phải OPEN và còn slot.
- Thành công: thêm vào `members`, broadcast `USER_JOIN`, log, phản hồi `JOIN_OK`.
- Lỗi: `JOIN_ALREADY_IN_ROOM` nếu đã ở phòng này; `JOIN_FAIL_OTHER_ROOM` nếu đang ở phòng khác; `JOIN_FAIL` nếu không tồn tại/đủ chỗ.

2) LEAVE_ROOM
- Lệnh: `LEAVE_ROOM`.
- Xóa khỏi member list phòng hiện tại, broadcast `USER_LEAVE`, log; phản hồi `LEAVE_OK`.

3) Kiểm tra phòng hiện tại
- Hàm `user_current_room` duyệt members để biết user đang ở đâu (dùng cho BID, LIST_ITEMS...).

## Quản lý item (admin trong phòng)
1) ADD_ITEM
- Lệnh: `ADD_ITEM <room_id> <plate> <start> <step> <buy_now>`; admin của phòng.
- Tạo `Item` mới: id tăng dần trong phòng, set giá hiện tại=start, leader="None", status ACTIVE, thời gian kết thúc = now + AUCTION_DURATION, bật auction_active.
- Lưu `items_room<id>.txt`, log, broadcast `ITEM_ADDED`, phản hồi `ITEM_ADDED_OK` hoặc FAIL.

2) REMOVE_ITEM
- Lệnh: `REMOVE_ITEM <room_id> <item_id>`; admin phòng.
- Xóa item khỏi danh sách, lưu file, log, broadcast `ITEM_REMOVED`; phản hồi `ITEM_REMOVED_OK` hoặc FAIL.

3) LIST_ITEMS
- Lệnh: `LIST_ITEMS <room_id>`; yêu cầu user đang ở đúng phòng.
- Gửi danh sách: id, biển số, giá hiện tại, bước giá, buy-now, leader, status, thời gian còn.

## Đấu giá
1) BID
- Lệnh: `BID <room_id> <item_id> <amount>`; yêu cầu đang ở phòng và item ACTIVE.
- Kiểm tra thời gian: nếu hết giờ, set SOLD, save, báo `AUCTION_ENDED`.
- Kiểm tra giá tối thiểu: `amount >= current_price + step_price`; nếu thấp -> `BID_TOO_LOW`.
- Hợp lệ: cập nhật giá/leader; nếu còn ≤5s thì gia hạn +5s, broadcast `TIME_EXTENDED`; lưu file.
- Broadcast `NEW_BID ... rem=<seconds>` cho cả phòng; log; phản hồi `BID_OK`.

2) BUY_NOW
- Lệnh: `BUY_NOW <room_id> <item_id>`; yêu cầu đang ở phòng.
- Kiểm tra còn thời gian và ACTIVE; đặt giá = buy_now_price, set leader=user, status=SOLD, stop auction, lưu file.
- Ghi history (method BUY_NOW), log, broadcast `SOLD_BUY_NOW ...`, phản hồi `BUY_NOW_OK`.

3) Tick tự động (auction_manager)
- Mỗi giây: duyệt item ACTIVE.
- Nếu rem == 30s và chưa cảnh báo -> broadcast `WARNING_30S`, save.
- Nếu rem <= 0 -> set SOLD, dừng auction, save, log SOLD_TIMEOUT, ghi history (winner nếu khác "None"), broadcast `SOLD ...`.

## Lịch sử và log
- History: `append_history` ghi (timestamp, winner, room, item, plate, price, method) vào `history.txt` khi SOLD timeout hoặc BUY_NOW.
- Lệnh `HISTORY` trả danh sách lịch sử của user: `HISTORY_LIST` ... hoặc `HISTORY_EMPTY`.
- Server log: `log_event` append time-stamped dòng vào `server.log` cho mọi hành động chính (register, login, room/item, bid, sold...).

## Lưu/khôi phục dữ liệu
- Khởi động: `load_users`, `load_rooms`, rồi `load_items` cho từng room, khởi tạo sessions.
- Khi thay đổi phòng hoặc item: `save_rooms` / `save_items(room_id)` ghi ngay file.
- Khi restart, server đọc lại file để giữ trạng thái phòng và item (kể cả SOLD, end_time còn lại).

## Luồng client mẫu
1) Kết nối TCP -> nhập lệnh (client.c) hoặc menu (client_ui.c).
2) Đăng ký/đăng nhập.
3) Admin tạo/điều khiển phòng; user JOIN phòng.
4) Admin thêm item; user xem `LIST_ITEMS`, thực hiện `BID` hoặc `BUY_NOW`.
5) Broadcast hiển thị mọi sự kiện (JOIN/LEAVE/ITEM/BID/TIME_EXTENDED/WARNING_30S/SOLD).
6) Xem lịch sử, đổi mật khẩu/tên hiển thị, logout.
