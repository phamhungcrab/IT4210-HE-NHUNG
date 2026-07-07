# MASTER PLAN – Đề tài Radar tầm ngắn STM32F429I-DISC1

## 1. Mục tiêu đề tài

Tên đề tài: Radar tầm ngắn

Board chính:
- STM32F429I-DISC1
- MCU STM32F429ZIT6
- Có LCD TFT 2.4 inch QVGA 320x240 dùng TouchGFX
- Có LED3 / LED4 trên board
- Có nút B1 trên board

Chức năng cần đạt:
1. Servo quay cảm biến siêu âm để quét góc 90° hoặc 180°.
2. SR-04 / HC-SR04 đo khoảng cách vật cản theo từng góc.
3. LCD TouchGFX hiển thị giao diện radar.
4. B1 dùng để đổi tốc độ quét hoặc chọn menu cài đặt.
5. LED/buzzer cảnh báo khi vật cản ở gần.
6. Có màn hình thống kê trạng thái hệ thống.
7. Có code module rõ ràng, dễ viết báo cáo và đưa lên GitHub.

## 2. Linh kiện dùng

Bắt buộc:
- STM32F429I-DISC1
- SR-04 hoặc HC-SR04
- Servo SG92R
- Dây Dupont
- Nguồn 5V cho servo và SR-04
- Điện trở chia áp cho chân Echo của SR-04

Nên có:
- Buzzer chủ động 3.3V hoặc 5V
- OLED SH1106 0.96 inch I2C nếu mượn được
- LED ngoài nếu muốn demo dễ nhìn hơn

Có sẵn trên board:
- LCD TFT 2.4 inch
- LED3 PG13
- LED4 PG14
- B1 PA0
- ST-Link để nạp/debug

## 3. Cảnh báo phần cứng quan trọng

SR-04 / HC-SR04:
- VCC thường dùng 5V
- Trigger nhận 3.3V từ STM32 vẫn thường chạy được
- Echo trả về 5V, KHÔNG đưa thẳng vào GPIO STM32
- Phải chia áp Echo xuống khoảng 3.3V

Ví dụ chia áp:
- Echo SR-04 -> R1 = 1k -> điểm giữa -> GPIO Echo STM32
- điểm giữa -> R2 = 2k -> GND
- điện áp về GPIO khoảng 3.3V

Servo SG92R:
- Nên cấp 5V riêng hoặc từ nguồn 5V đủ dòng
- GND servo phải nối chung GND với STM32
- Tín hiệu PWM từ STM32 là 3.3V thường điều khiển được servo
- Không nên lấy dòng servo trực tiếp từ chân GPIO

## 4. Kiến trúc phần mềm

Chia module như sau:

Core/Inc:
- hcsr04.h
- servo_sg92r.h
- radar_app.h
- radar_config.h
- buzzer_led.h
- oled_status.h      optional
- radar_ui_bridge.h

Core/Src:
- hcsr04.c
- servo_sg92r.c
- radar_app.c
- buzzer_led.c
- oled_status.c      optional
- radar_ui_bridge.c

TouchGFX:
- Model.hpp / Model.cpp
- Presenter
- View các màn hình

## 5. Module chức năng

### 5.1 hcsr04.c / hcsr04.h
Nhiệm vụ:
- Phát xung Trigger 10 us
- Đo độ rộng xung Echo
- Tính khoảng cách cm
- Có timeout để tránh treo chương trình
- Có lọc nhiễu đơn giản

Công thức:
- distance_cm = echo_time_us / 58

Kiến thức học được:
- GPIO Output
- GPIO Input
- delay microsecond
- đo thời gian xung
- timeout trong hệ nhúng

### 5.2 servo_sg92r.c / servo_sg92r.h
Nhiệm vụ:
- Tạo PWM 50 Hz
- Điều khiển góc servo từ 0 đến 180 độ
- Map góc sang độ rộng xung

PWM servo:
- Chu kỳ: 20 ms
- 0° khoảng 0.5 ms hoặc 1.0 ms tùy servo
- 90° khoảng 1.5 ms
- 180° khoảng 2.5 ms hoặc 2.0 ms tùy servo

Kiến thức học được:
- Timer PWM
- Prescaler / ARR / CCR
- điều khiển cơ cấu chấp hành

### 5.3 radar_app.c / radar_app.h
Nhiệm vụ:
- Quản lý logic radar
- Quét từ trái sang phải rồi phải sang trái
- Mỗi góc đo một lần
- Lưu góc hiện tại, khoảng cách hiện tại
- Xác định có vật cản hay không
- Xác định vật cản gần hay không
- Đếm số lần phát hiện vật cản
- Cập nhật dữ liệu cho UI

Biến trạng thái chính:
- current_angle
- current_distance_cm
- scan_mode: 90 hoặc 180
- speed_mode: slow / medium / fast
- object_detected
- near_warning
- object_count
- min_distance_cm
- last_detect_angle

Ngưỡng gợi ý:
- object_detected nếu distance từ 2 đến 80 cm
- near_warning nếu distance <= 15 cm

### 5.4 buzzer_led.c / buzzer_led.h
Nhiệm vụ:
- LED3 bật khi đang quét
- LED4 bật khi phát hiện vật cản
- Buzzer kêu khi vật cản gần
- Có thể nháy theo trạng thái

### 5.5 oled_status.c / oled_status.h optional
Nhiệm vụ:
- Nếu mượn được OLED SH1106 thì hiển thị phụ:
  - Angle
  - Distance
  - Mode
  - Status
- Dùng I2C3 nếu không xung đột

### 5.6 radar_ui_bridge.c / radar_ui_bridge.h
Nhiệm vụ:
- Là cầu nối giữa C/HAL và TouchGFX C++
- Lưu struct dữ liệu radar dùng chung
- TouchGFX đọc dữ liệu này để vẽ màn hình

Struct gợi ý:

typedef struct {
    uint16_t angle_deg;
    uint16_t distance_cm;
    uint8_t object_detected;
    uint8_t near_warning;
    uint8_t scan_mode_deg;
    uint8_t speed_mode;
    uint16_t object_count;
    uint16_t min_distance_cm;
    uint8_t buzzer_on;
    uint8_t led3_on;
    uint8_t led4_on;
} RadarUiData_t;

## 6. Giao diện LCD TouchGFX cần đạt

Có 6 trạng thái / màn hình chính:

### Screen 1: Home
Hiển thị:
- RADAR TẦM NGẮN
- STM32F429I-DISC1
- Bắt đầu quét
- Cài đặt
- Thông tin

### Screen 2: Scan Normal
Hiển thị:
- Radar bán nguyệt 90° hoặc 180°
- Tia quét xanh
- Góc servo hiện tại
- Khoảng cách hiện tại
- Trạng thái: Đang quét

### Screen 3: Object Detected
Hiển thị:
- Radar bán nguyệt
- Chấm đỏ/vàng tại góc phát hiện
- Khoảng cách ví dụ 42 cm
- Góc ví dụ 65°
- Trạng thái: Phát hiện vật cản

### Screen 4: Near Warning
Hiển thị:
- CẢNH BÁO
- Vật cản gần
- Khoảng cách lớn, ví dụ 12 cm
- Buzzer icon
- Nền/viền đỏ

### Screen 5: Settings
Hiển thị:
- Tốc độ servo: Chậm / Trung bình / Nhanh
- Góc quét: 90° / 180°
- Gợi ý: Bấm B1 để chuyển / chọn

### Screen 6: System Info
Hiển thị:
- Chế độ quét
- Tốc độ servo
- Khoảng cách gần nhất
- Góc phát hiện gần nhất
- Số vật cản phát hiện
- LED / Buzzer / OLED status

## 7. FreeRTOS / luồng xử lý

Nên dùng FreeRTOS vì TouchGFX thường chạy cùng RTOS.

Task đề xuất:
1. RadarTask
   - Quay servo
   - Đo SR-04
   - Cập nhật trạng thái
   - Điều khiển LED/buzzer
   - Ghi dữ liệu sang radar_ui_bridge

2. ButtonTask hoặc xử lý trong RadarTask
   - Đọc B1
   - Chống dội nút
   - Đổi speed_mode hoặc menu selection

3. TouchGFX Task
   - Do TouchGFX tạo sẵn
   - Đọc dữ liệu từ Model
   - Cập nhật View

4. UartDebugTask optional
   - In angle, distance, status ra Hercules
   - Giúp debug phần cứng

## 8. Thứ tự triển khai code

### Phase 1: Tạo project nền
- Tạo project mới radar_short_range
- Nên dựa trên project TouchGFX đã chạy sạch trước đó để giữ cấu hình LCD
- Không tận dụng code game racing
- Chỉ tận dụng cấu hình LCD/FreeRTOS/TouchGFX đã ổn

### Phase 2: Tạo UI TouchGFX skeleton
- Tạo các màn hình hoặc ít nhất tạo Home + Scan + Warning + Settings + Info
- Đặt đúng tên object
- Generate code
- Build sạch

### Phase 3: Cấu hình phần cứng trong CubeMX
- Servo PWM
- SR-04 Trigger GPIO Output
- SR-04 Echo GPIO Input
- Buzzer GPIO Output
- B1 PA0 Input
- LED3 PG13 Output
- LED4 PG14 Output
- UART debug nếu cần
- OLED I2C nếu dùng

### Phase 4: Test phần cứng đơn lẻ
- Test LED3/LED4
- Test B1
- Test servo quay 0/90/180
- Test SR-04 đọc khoảng cách
- Test buzzer
- Test OLED nếu có

### Phase 5: Ghép radar logic
- Servo quét từng góc
- Mỗi góc đo khoảng cách
- Phát hiện vật cản
- Cảnh báo vật cản gần
- Cập nhật dữ liệu UI

### Phase 6: Ghép TouchGFX
- Model đọc dữ liệu radar
- Presenter đẩy dữ liệu sang View
- View cập nhật text/đồ họa
- Vẽ sweep line / target dot

### Phase 7: Cài đặt tốc độ / góc quét
- B1 đổi tốc độ servo
- TouchGFX Settings hiển thị mode đang chọn
- Có thể thêm nút cảm ứng nếu muốn

### Phase 8: Hoàn thiện demo
- Có trạng thái đang quét
- Có trạng thái phát hiện
- Có trạng thái cảnh báo gần
- Có thống kê
- Có debug UART
- Chạy ổn định trên board thật

### Phase 9: Chuẩn bị báo cáo GitHub
Theo format ProjectSample:
- Giới thiệu
- Tác giả
- Môi trường hoạt động
- Sơ đồ kết nối
- Nguyên lý hoạt động
- Tích hợp hệ thống
- Đặc tả hàm
- Kết quả
- Video demo / hình ảnh demo
- Hướng phát triển

## 9. Nguyên tắc khi code

- Không viết toàn bộ trong main.c nếu có thể tránh
- Code driver để file .c/.h riêng
- main.c chỉ gọi hàm init/task
- Code tự viết đặt trong USER CODE hoặc file riêng
- Không sửa file generated của TouchGFX nếu không cần
- Mỗi lần thêm module phải build test ngay
- Không ghép tất cả một lần vì rất khó debug

## 10. Tiêu chí bản hoàn thiện

Bản hoàn thiện được coi là đạt nếu:
1. Servo quét ổn định 90° hoặc 180°.
2. SR-04 đo được khoảng cách thực tế.
3. LCD TouchGFX hiển thị được góc và khoảng cách.
4. Khi có vật cản, LCD hiện target.
5. Khi vật cản gần, LCD hiện cảnh báo và buzzer/LED hoạt động.
6. B1 đổi được tốc độ hoặc chế độ.
7. Code chia module rõ ràng.
8. Có ảnh/video kết quả để đưa vào báo cáo.