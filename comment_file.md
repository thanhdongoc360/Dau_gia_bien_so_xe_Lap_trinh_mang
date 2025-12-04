1) auction.c
🎯 Tổng kết: Thread này làm gì?

Mỗi giây quét tất cả item đang đấu giá.

Nếu item hết thời gian →

đánh dấu bán

lưu file

ghi lịch sử

gửi thông báo SOLD

Nếu item còn đúng 30s →

gửi cảnh báo 30s

Dùng mutex để tránh race condition.

Không có mutex:

Thread A: user đặt giá

Thread B: auction_manager kết thúc item

Thread C: user khác vào phòng

→ Cả 3 cùng sửa items[]: nát dữ liệu.

Có mutex:

A vào, khóa mutex → B và C phải đợi

A xong → mở mutex → thread tiếp theo được vào

2) handler.c
🎯 Giải thích ngắn gọn handle_client

handle_client() là hàm chạy trong một thread riêng để xử lý từng client kết nối đến server.

Mỗi lệnh client gửi → hàm này đọc → xử lý → trả kết quả.

🔧 Các ý chính dễ hiểu
1. Nhận socket từ client

Lấy socket ra, thiết lập timeout 300 giây để tránh treo.

2. Vòng lặp nhận lệnh

recv() đọc lệnh client gửi như:

REGISTER

LOGIN

JOIN_ROOM

BID
…

3. Xử lý từng lệnh

Một số lệnh quan trọng:

REGISTER / LOGIN: đăng ký và đăng nhập

JOIN_ROOM: vào phòng đấu giá

LEAVE_ROOM: rời phòng

LIST_ITEMS: xem danh sách item

BID: đấu giá

BUY_NOW: mua ngay (kết thúc item)

LOGOUT: đăng xuất

4. Vì sao phải dùng mutex? (rất quan trọng)

Vì nhiều client có thể:

cùng lúc BID một item

cùng lúc JOIN/LEAVE room

admin thêm item trong khi người khác đang xem…

Nên dùng:

pthread_mutex_lock(&rooms_mutex);


để khóa dữ liệu rooms/items, tránh sai lệch hoặc crash.

5. Khi client mất kết nối

Cuối hàm:

tự động rời room

logout

xóa session

đóng socket

Giúp tránh user “ma” còn tồn tại.

🎉 Tóm tắt 1 câu

handle_client() là thread xử lý mọi lệnh mà client gửi đến server, dùng mutex để đảm bảo dữ liệu an toàn khi nhiều client thao tác cùng lúc.

3) history.c:
🔒 Mutex trong đoạn code này dùng để làm gì?

Mutex = khoá cửa.
Nó đảm bảo chỉ 1 thread được phép đọc/ghi file history.txt tại một thời điểm.

Nếu không có mutex:

2 client ghi dữ liệu vào file cùng lúc → file bị loạn.

1 thread đang đọc, thread khác ghi → dữ liệu sai, crash.

📌 Giải thích từng hàm (ngắn – dễ hiểu)
1. append_history(...)

Mục đích: Ghi thêm 1 dòng lịch sử vào file history.txt.

Quy trình:

pthread_mutex_lock → khoá file để thread khác không được chạm vào.

Mở file ở chế độ append "a".

Ghi: thời gian, tên người thắng, room, item, biển số xe, giá, phương thức thắng.

Đóng file.

pthread_mutex_unlock → mở khoá để thread khác có thể dùng.

2. send_history(sock, username)

Mục đích: Gửi toàn bộ lịch sử của 1 user về client.

Quy trình:

Khoá mutex.

Mở file history.txt để đọc.

Với mỗi dòng:

Nếu username trùng → format lại dòng lịch sử đẹp hơn → đưa vào chuỗi msg.

Mở khoá mutex.

Gửi msg qua socket.

Nếu không có dữ liệu → gửi "HISTORY_EMPTY\n".

4) items.c:
Tổng quan

File này quản lý item (biển số xe) trong một phòng đấu giá.
Gồm các chức năng:

Tạo ID mới cho item

Thêm item

Xóa item

Liệt kê item trong phòng

1. Hàm next_item_id(Room *r)

👉 Tìm ID lớn nhất trong phòng, rồi +1
→ Đảm bảo item mới luôn có ID tăng dần.

2. Hàm add_item() – Thêm item vào phòng

Quy trình:

Tìm phòng dựa vào room_id.

Kiểm tra phòng còn hoạt động và admin đúng → sai thì trả về:

-1: admin không đúng

-2: quá số lượng item

Tạo item mới:

id = ID mới

license_plate = biển số

current_price = start_price

leader = "None"

status = 0 (đang đấu giá)

auction_active = 1

end_time = now + AUCTION_DURATION

Ghi item vào file bằng save_items()

Ghi log bằng log_event()

Gửi thông báo cho cả phòng (broadcast_to_room_nolock)

Trả về item_id mới tạo.

3. Hàm remove_item() – Xóa item

Quy trình:

Tìm phòng → kiểm tra admin.

Tìm item có id tương ứng.

Nếu tìm thấy:

Lưu biển số để ghi log

Dịch mảng sang trái để xóa item

Giảm item_count

Lưu vào file (save_items)

Ghi log

Thông báo cho phòng

Trả về 1 (thành công)

Nếu không có item → trả về 0

Nếu không tìm được phòng → trả -2

4. Hàm list_items() – Gửi danh sách item cho client

Gửi dạng text:

ID

biển số

giá hiện tại

bước nhảy

giá mua ngay

leader

trạng thái (ACTIVE / SOLD)

thời gian còn lại (remain seconds)

Nếu hết thời gian → rem = 0.

Gửi kết quả về socket client.

Tóm tắt dễ hiểu nhất

add_item: Kiểm tra admin → tạo item → lưu → log → thông báo.

remove_item: Kiểm tra admin → tìm item → xóa → lưu → log → thông báo.

list_items: Gửi danh sách item + thời gian còn lại cho người dùng.

5) log.c:
Chức năng

Ghi một dòng log vào file server.log kèm timestamp, và đảm bảo an toàn khi nhiều thread cùng ghi.

Cách hoạt động

Khóa mutex:

pthread_mutex_lock(&log_mutex);


→ Đảm bảo không có thread nào khác ghi vào log cùng lúc.

Mở file server.log để thêm ("a" = append).

Nếu mở file thất bại → thoát hàm và mở khóa mutex.

Tạo timestamp hiện tại:

time_t now = time(NULL);
struct tm *tm_info = localtime(&now);
strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", tm_info);


→ Ví dụ: [2025-11-29 09:15:00]

Ghi timestamp vào file:

fprintf(f, "[%s] ", ts);


Ghi message log với định dạng biến đổi (printf biến số lượng tham số):

va_list ap;
va_start(ap, fmt);
vfprintf(f, fmt, ap);
va_end(ap);


Xuống dòng và đóng file:

fprintf(f, "\n");
fclose(f);


Mở khóa mutex để các thread khác có thể ghi log:

pthread_mutex_unlock(&log_mutex);

Tóm tắt cực ngắn

Ghi log có thời gian.

Hỗ trợ printf-style format.

An toàn khi đa thread nhờ mutex.

6) main.c:
1. Khai báo và khởi tạo

users[MAX_USERS], rooms[MAX_ROOMS], sessions[MAX_CLIENTS]: lưu thông tin người dùng, phòng đấu giá, và client đang kết nối.

Các mutex (users_mutex, rooms_mutex, ...) để đảm bảo thread-safe khi nhiều thread truy cập cùng lúc.

user_count, room_count, next_room_id: quản lý số lượng và ID phòng tiếp theo.

2. Load dữ liệu từ file
load_users();
load_rooms();
load_items(rooms[i].id);


Load người dùng, phòng, và item trong từng phòng từ file lưu trữ.

rooms_mutex bảo vệ dữ liệu khi load.

3. Khởi tạo thread quản lý đấu giá
pthread_create(&auction_tid, NULL, auction_manager, NULL);
pthread_detach(auction_tid);


auction_manager chạy nền, cập nhật trạng thái đấu giá liên tục.

detach → thread tự giải phóng khi kết thúc, không cần pthread_join.

4. Tạo socket server
server_fd = socket(AF_INET, SOCK_STREAM, 0);
bind(...);
listen(...);


Tạo TCP socket, lắng nghe trên port PORT.

INADDR_ANY → server chấp nhận kết nối từ mọi IP.

5. Vòng lặp chính – chấp nhận client
while (1) {
    int *pclient = malloc(sizeof(int));
    *pclient = accept(...);

    pthread_create(&tid, NULL, handle_client, pclient);
    pthread_detach(tid);
}


accept() → chấp nhận client mới, trả về socket.

Mỗi client được xử lý bằng 1 thread riêng (handle_client).

detach → thread tự hủy khi kết thúc, không cần join.

malloc cho socket, tránh biến cục bộ bị ghi đè.

6. Logging và hiển thị
printf("Server listening on port %d...\n", PORT);
log_event("SERVER_START port=%d", PORT);


In ra console + ghi log sự kiện server bắt đầu.

Tóm tắt cực ngắn

Khởi tạo dữ liệu và mutex.

Load users, rooms, items từ file.

Tạo thread quản lý đấu giá.

Tạo server TCP socket lắng nghe port.

Chấp nhận client → tạo thread handle client riêng.

Ghi log server start.

7) persistence.c:
1. Hàm items_filename()
static void items_filename(int room_id, char *out, size_t n)


Tạo tên file lưu item của phòng dựa vào room_id.

Ví dụ: room_id = 3 → items_room3.txt

2. save_rooms()

Mở file "rooms.txt" để ghi tất cả phòng.

Ghi thông tin mỗi phòng:

id, name, active, admin

Đóng file.

Chức năng: lưu trạng thái phòng hiện tại vào file.

3. load_rooms()

Mở file "rooms.txt" đọc lại các phòng.

Nếu file không tồn tại → room_count = 0, next_room_id = 1

Với mỗi dòng:

Đọc id, name, active, admin

Khởi tạo member_count = 0, item_count = 0

Thêm vào mảng rooms[]

Tính next_room_id = max_id + 1 để tạo phòng mới.

4. save_items(int room_id)

Tạo file riêng cho phòng: items_room<room_id>.txt

Ghi tất cả item trong phòng:

id, license_plate, current_price, step_price, buy_now_price

leader, status, end_time, auction_active, warned30

Mỗi item → 1 dòng trong file.

5. load_items(int room_id)

Mở file items_room<room_id>.txt đọc item.

Với mỗi dòng:

Đọc thông tin item → gán vào r->items[]

Chuyển end_time từ long sang time_t

Cập nhật item_count cho phòng.

Tóm tắt cực ngắn

save_rooms / load_rooms → lưu/đọc danh sách phòng (rooms.txt)

save_items / load_items → lưu/đọc item riêng từng phòng (items_room<ID>.txt)

items_filename → tạo tên file theo room_id

Đảm bảo dữ liệu giữa server và file luôn đồng bộ.

8) rooms.c:
1. create_room(name, admin) – Tạo phòng mới

Kiểm tra room_count < MAX_ROOMS.

Tạo phòng mới:

id = next_room_id++

name, admin, active = 1

member_count = 0, item_count = 0

Tăng room_count.

Lưu vào file (save_rooms) và ghi log.

Trả về room_id mới.

2. close_room(id, admin) – Đóng phòng

Tìm phòng theo id và đang active.

Kiểm tra admin đúng.

Đặt active = 0 và member_count = 0.

Lưu file + log.

Trả về 1 nếu thành công, -1 admin sai, 0 không tìm thấy.

3. open_room(id, admin) – Mở lại phòng

Tìm phòng theo id.

Kiểm tra admin đúng.

Đặt active = 1.

Lưu file + log.

Trả về 1 nếu thành công, -1 admin sai, 0 không tìm thấy.

4. list_rooms(sock) – Gửi danh sách phòng cho client

Tạo chuỗi "ROOM_LIST\n"

Thêm từng phòng:

ID, name, trạng thái [OPEN/CLOSED]

Gửi qua socket sock.

5. user_current_room(username) – Tìm phòng hiện tại của user

Duyệt tất cả phòng và members.

Nếu tìm thấy username → trả về room_id.

Nếu không → trả về 0.

6. join_room(username, id) – Tham gia phòng

Kiểm tra user đang ở phòng khác:

Nếu đang ở phòng khác → trả về -3

Kiểm tra user đã ở phòng này → -2

Kiểm tra số lượng member vượt MAX_ROOM_USERS → -4

Nếu OK → thêm user vào members[], tăng member_count, trả về 1

Nếu phòng không tồn tại hoặc không active → trả về 0

7. leave_room_and_getid(username) – Rời phòng

Tìm username trong tất cả phòng.

Xóa user khỏi members[] bằng cách dịch mảng.

Giảm member_count.

Trả về room_id của phòng vừa rời.

Nếu không tìm thấy → trả về 0.