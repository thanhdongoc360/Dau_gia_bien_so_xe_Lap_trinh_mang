Project file overview (tóm tắt ngắn gọn)

- readme.md: Hướng dẫn build/chạy nhanh bằng gcc cho server và client.
- process.md: Quy trình kiểm thử các tính năng REGISTER/LOGIN/ROOM/ITEM/BID/BUY_NOW và kỳ vọng phản hồi.
- Makefile: Hiện trống (chưa cấu hình build).

Source (src/)
- main.c: Khởi động server TCP, load dữ liệu, tạo thread auction_manager và tạo thread handle_client cho từng kết nối.
- handler.c: Vòng recv/parse lệnh văn bản, kiểm tra quyền, gọi các hàm nghiệp vụ (user/room/item/bid/buy), quản lý logout và broadcast.
- auction.c: Thread định kỳ 1s kiểm tra thời gian còn lại của item, phát WARNING_30S, SOLD_TIMEOUT, cập nhật file và log.
- users.c: Quản lý users (load/save file, register/login/logout, đổi mật khẩu/tên hiển thị) với mutex.
- rooms.c: Quản lý phòng (tạo/mở/đóng/list), thành viên, và tra cứu phòng hiện tại của user.
- items.c: Quản lý item trong phòng (add/remove/list), đặt thời gian kết thúc, broadcast khi thêm/xóa.
- persistence.c: Đọc/ghi rooms và items ra files phẳng (rooms.txt, items_room*.txt), giữ next_room_id.
- session.c: Lưu phiên socket↔user, hỗ trợ broadcast tới các user trong cùng phòng.
- history.c: Ghi lịch sử giao dịch vào history.txt và gửi lịch sử cho user.
- log.c: Ghi server.log với timestamp an toàn luồng.

Headers (include/)
- common.h: Định nghĩa struct User/Room/Item/ClientSession, hằng số (PORT, BUF_SIZE, giới hạn) và khai báo biến toàn cục/mutex.
- auction.h, handler.h, items.h, rooms.h, session.h, users.h, persistence.h, history.h, log.h: Khai báo hàm công khai tương ứng với mỗi module.

Clients
- client.c: Client dòng lệnh đơn giản; một thread nhận/broadcast, luồng chính gửi lệnh từ stdin.
- client_ui.c: Client menu có trạng thái (logged_in/room/role), cung cấp menu thao tác, vẫn dùng thread nhận để in broadcast.

Data/log files
- users.txt: Danh sách người dùng (username password role display_name).
- rooms.txt: Danh sách phòng (id name active admin).
- items_room*.txt: Danh sách item theo phòng, lưu giá hiện tại, leader, thời gian kết thúc, trạng thái.
- history.txt: Lịch sử giao dịch (timestamp, winner, room, item, plate, price, method).
- server.log: Log sự kiện hệ thống (register/login/room/item/bid/sold...).

Mô tả ngắn hệ thống
- Server TCP đa luồng, giao thức lệnh văn bản; dữ liệu bền trên file phẳng.
- Luồng auction_manager xử lý timeout/gia hạn; luồng handle_client phục vụ từng kết nối và broadcast theo phòng qua session map.






Handler role (xử lý lệnh từ mỗi client) nằm trong handler.c:

Vòng đời kết nối: Thread được spawn từ main nhận socket, thiết lập SO_RCVTIMEO (300s), rồi lặp recv() đến khi lỗi/timeout/đóng; cuối vòng bảo đảm logout, rời phòng, đóng socket.
Phân tích lệnh văn bản: So khớp tiền tố REGISTER/LOGIN/LOGOUT/CHANGE_PASS/CHANGE_NAME/HISTORY/CREATE_ROOM/OPEN_ROOM/CLOSE_ROOM/LIST_ROOMS/JOIN_ROOM/LEAVE_ROOM/ADD_ITEM/REMOVE_ITEM/LIST_ITEMS/BID/BUY_NOW; trích tham số bằng sscanf và phản hồi bằng chuỗi đơn giản với send.
Kiểm tra quyền & trạng thái: Yêu cầu đã login, đúng role admin cho lệnh quản trị; kiểm tra user đã join đúng phòng trước khi BID/BUY/LIST_ITEMS; từ chối nếu đang ở phòng khác, item sold/expired.
Cập nhật phiên & broadcast: Gọi update_session_login khi login, remove_session khi thoát; khi JOIN/LEAVE/LOGOUT/BID/BUY/ADD/REMOVE gửi broadcast tới phòng qua broadcast_to_room_nolock.
Quản lý phòng và item: Dùng mutex rooms_mutex bao quanh thao tác join/leave/add/remove/bid/buy để an toàn luồng; lưu trạng thái ra file bằng save_items khi giá/thời gian thay đổi.
Đấu giá & gia hạn: Trong BID, kiểm tra tối thiểu current + step; nếu còn ≤5s thì cộng thêm 5s và broadcast TIME_EXTENDED; sau cập nhật giá/leader, broadcast NEW_BID.
Kết thúc/timeout: Nếu recv timeout hoặc socket đóng, handler tự rời phòng (broadcast USER_LEAVE) và logout để tránh kẹt trạng thái online.