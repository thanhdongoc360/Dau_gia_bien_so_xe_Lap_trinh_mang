## Hướng dẫn chi tiết chạy trên máy B (Client)

1. Copy toàn bộ thư mục project sang máy B (có thể dùng USB, mạng LAN, hoặc tải từ Git).
2. Cài đặt trình biên dịch C (GCC/MinGW) nếu chưa có.
3. Mở terminal/cmd, chuyển đến thư mục project, chạy lệnh:
	```sh
	make
	```
4. Xác định file thực thi client (thường là `client.exe` hoặc trong thư mục `src/`).
5. Chạy chương trình client:
	```sh
	./client.exe
	```
	hoặc
	```sh
	./src/client.exe
	```
6. Khi chương trình yêu cầu, nhập địa chỉ IP của máy A (máy server) mà bạn đã ghi lại trước đó (ví dụ: 192.168.1.10).
7. Nếu kết nối thành công, làm theo hướng dẫn trên màn hình để tham gia phiên đấu giá.

**Lưu ý:**
- Máy B phải kết nối cùng mạng LAN với máy A hoặc truy cập được IP của máy A.
- Nếu không kết nối được, kiểm tra lại IP, trạng thái server, và firewall trên cả hai máy.
## Hướng dẫn chạy project trên 2 máy khác nhau (Server - Client)

### 1. Chuẩn bị
1. Copy toàn bộ thư mục project sang cả hai máy (hoặc dùng Git clone nếu có repository).
2. Đảm bảo cả hai máy đều đã cài đặt trình biên dịch C (GCC hoặc MinGW trên Windows).

### 2. Biên dịch project
Trên cả hai máy, mở terminal/cmd, chuyển đến thư mục project và chạy:
```sh
make
```

### 3. Thiết lập và chạy Server
1. Chọn một máy làm server.
2. Trên máy server, chạy chương trình server:
	 - Nếu file thực thi là `server.exe` hoặc tương tự:
		 ```sh
		 ./server.exe
		 ```
	 - Hoặc:
		 ```sh
		 ./src/server.exe
		 ```
3. Ghi lại địa chỉ IP của máy server (dùng lệnh `ipconfig` trên Windows để lấy IPv4 Address).
4. Đảm bảo firewall trên máy server đã mở port mà server sử dụng (ví dụ: 8080, 8888,...).

### 4. Thiết lập và chạy Client
1. Trên máy client, chạy chương trình client:
	 - Nếu file thực thi là `client.exe` hoặc tương tự:
		 ```sh
		 ./client.exe
		 ```
	 - Hoặc:
		 ```sh
		 ./src/client.exe
		 ```
2. Khi được hỏi nhập địa chỉ IP server, hãy nhập IPv4 Address của máy server đã ghi ở trên.

### 5. Kết nối và sử dụng
- Client sẽ kết nối đến server qua IP và port đã cấu hình.
- Nếu không kết nối được, kiểm tra lại:
	- Địa chỉ IP nhập đúng chưa.
	- Server đã chạy chưa.
	- Port đã mở trên firewall chưa.

### 6. Một số lưu ý
- Hai máy phải cùng mạng LAN hoặc đảm bảo máy client truy cập được IP của server.
- Nếu chạy qua Internet, cần NAT port trên router hoặc dùng các dịch vụ VPN/mạng riêng.

---
cd "/mnt/d/2025.1 chính thức/Thực hành lập trình mạng project/dau_gia_bien_so_xe/Project 2"


1) Build lại tất cả

Ở root PROJECT1:

gcc -Iinclude src/*.c -o server -lpthread
gcc client.c -o client -lpthread


Nếu ra server và client là ok.

2) Chạy server

Terminal 1:

./server


Thấy:

Server listening on port 12345...


=> Server sống.

3) Stage 1 — REGISTER

Terminal 2 (Client A):

./client


Gõ:

REGISTER user1 123456
REGISTER user2 123456


Kỳ vọng:

Server: REGISTER_OK
Server: REGISTER_OK


Test trùng username:

REGISTER user4 123456


Kỳ vọng:

Server: REGISTER_FAIL


=> Stage 1 done.

4) Stage 2 — LOGIN / LOGOUT / Online-only-one-place

Terminal 2:

LOGIN user1 123456


Kỳ vọng:

Server: LOGIN_OK user


Mở Terminal 3 (Client B):

./client
LOGIN user1 123456


Kỳ vọng:

Server: LOGIN_FAIL_ALREADY_ONLINE


Quay lại Terminal 2:

LOGOUT


Kỳ vọng:

Server: LOGOUT_OK


Terminal 3 login lại:

LOGIN user1 123456


Kỳ vọng:

Server: LOGIN_OK user


=> Stage 2 done.

5) Stage 3 — ROOM (admin create/open/close/list)

Terminal 2 (Client Admin):

./client
LOGIN admin 123456


Tạo phòng:

CREATE_ROOM room1
CREATE_ROOM room2


Kỳ vọng:

Server: ROOM_CREATED 1
Server: ROOM_CREATED 2


List phòng:

LIST_ROOMS


Kỳ vọng đại loại:

ROOM_LIST
1. room1 [OPEN]
2. room2 [OPEN]


Close room2:

CLOSE_ROOM 2
LIST_ROOMS


Kỳ vọng room2 thành CLOSED.

Open lại:

OPEN_ROOM 2


=> Stage 3 done.

6) Stage 4 — JOIN/LEAVE + mỗi user chỉ ở 1 room

Terminal 3 (u1 đã login ở trên):

JOIN_ROOM 1


Kỳ vọng:

Server: JOIN_OK


Test join lại cùng room:

JOIN_ROOM 1


Kỳ vọng:

Server: JOIN_ALREADY_IN_ROOM


Test join room khác khi đang ở room1:

JOIN_ROOM 2


Kỳ vọng:

Server: JOIN_FAIL_OTHER_ROOM


Leave:

LEAVE_ROOM


Kỳ vọng:

Server: LEAVE_OK


Join room2:

JOIN_ROOM 2


Ok.

=> Stage 4 done.

7) Stage 5 — ITEM + đấu giá realtime
7.1 Admin add item

Terminal 2 (admin):

JOIN_ROOM 1
ADD_ITEM 1 30A12345 100 10 500
ADD_ITEM 1 29B88888 200 20 800
LIST_ITEMS 1


Kỳ vọng list có 2 item, status ACTIVE, remain ~60s.

7.2 User bid

Terminal 3 (u1):

JOIN_ROOM 1
LIST_ITEMS 1
BID 1 1 105


Kỳ vọng:

Server: BID_TOO_LOW


Vì min = current + step = 110

Bid đúng:

BID 1 1 110


Kỳ vọng:

Server: BID_OK


Terminal 2 (admin) sẽ thấy broadcast:

NEW_BID item 1 ... price=110 leader=u1 ...


Bid sát cuối (chờ còn ~3–5s rồi bid):

BID 1 1 120


Kỳ vọng broadcast:

TIME_EXTENDED item 1 +5s ...
NEW_BID ...

7.3 Buy now

Terminal 3:

BUY_NOW 1 2


Kỳ vọng:

Server: BUY_NOW_OK


Broadcast về SOLD_BUY_NOW.

7.4 Hết giờ tự SOLD

Đợi item 1 hết remain.
Kỳ vọng broadcast:

WARNING_30S ...
SOLD item 1 ... winner=u1 price=120


=> Stage 5 done.

8) Stage 6 — Broadcast + Log
8.1 Broadcast kiểm tra

Khi u1 join room:
Terminal 2 thấy:

USER_JOIN u1 room 1


Khi u1 leave:

USER_LEAVE u1 room 1


Add/remove item, bid, sold… đều broadcast.

=> đạt broadcast.

8.2 Log file

Mở server.log:

cat server.log


Phải thấy các event:

REGISTER
LOGIN/LOGOUT

CREATE_ROOM / OPEN_ROOM / CLOSE_ROOM

JOIN/LEAVE

ADD_ITEM / REMOVE_ITEM

BID

TIME_EXTENDED

SOLD_TIMEOUT / BUY_NOW

=> Stage 6 done.

9) Stage 7 — History + Change info
9.1 History

Terminal 3 (u1):

HISTORY


Kỳ vọng có item u1 thắng:

HISTORY_LIST
... Room 1 | Item 1 ...

9.2 Change password
CHANGE_PASS 111 333
LOGOUT
LOGIN u1 333


Kỳ vọng login OK.

9.3 Change display name
CHANGE_NAME vua_daugia


Kỳ vọng:

CHANGE_NAME_OK


Mở users.txt sẽ thấy cột 4 đổi thành vua_daugia.

=> Stage 7 done.

10) Test “persist sau restart”

Tắt server (Terminal 1) bằng Ctrl+C

Chạy lại:

./server


Admin login:

LOGIN admin 123456
LIST_ROOMS
LIST_ITEMS 1


Kỳ vọng:

room1, room2 vẫn còn

item còn (item SOLD vẫn nằm đó, ACTIVE còn timer/active đúng theo file).

=> Persist ok.