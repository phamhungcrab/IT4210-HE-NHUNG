# Radar tầm ngắn - STM32F429I-DISC1 + TouchGFX

> Báo cáo bài tập lớn môn Hệ nhúng: xây dựng mô hình radar tầm ngắn sử dụng STM32F429I-DISC1, cảm biến siêu âm HC-SR04, servo MG90S và giao diện TouchGFX.

## Tóm tắt

Dự án triển khai một hệ thống quét vật cản tầm ngắn theo mô hình radar. Cảm biến HC-SR04 được gắn trên servo MG90S để thay đổi góc quét. STM32F429I-DISC1 phát tín hiệu trigger, đo độ rộng xung echo, tính khoảng cách, xử lý các ngưỡng phát hiện và cập nhật dữ liệu lên màn hình LCD TouchGFX tích hợp của board.

Hệ thống hỗ trợ quét theo góc, đo khoảng cách vật cản, hiển thị góc/khoảng cách/trạng thái, thay đổi chế độ quét, thay đổi tốc độ quét và cảnh báo bằng LED/buzzer khi vật cản nằm trong vùng gần.

Project chính nằm trong thư mục [`radar_short_range_touchgfx`](./radar_short_range_touchgfx).

> [!NOTE]
> Phiên bản hoàn thiện của đề tài sử dụng LCD TouchGFX tích hợp trên STM32F429I-DISC1 làm màn hình hiển thị duy nhất. Báo cáo không mô tả các thiết bị hiển thị ngoài chưa được lắp đặt và kiểm thử thực tế.
>
> Các mục cần minh chứng thực nghiệm như ảnh đấu nối, video demo và bảng đo sai số được giữ dưới dạng placeholder để nhóm bổ sung trước khi nộp.

## Mục lục

- [1. GIỚI THIỆU](#1-giới-thiệu)
- [2. TÁC GIẢ](#2-tác-giả)
- [3. MÔI TRƯỜNG HOẠT ĐỘNG](#3-môi-trường-hoạt-động)
- [4. SƠ ĐỒ SCHEMATIC / KẾT NỐI PHẦN CỨNG](#4-sơ-đồ-schematic--kết-nối-phần-cứng)
- [5. NGUYÊN LÝ HOẠT ĐỘNG](#5-nguyên-lý-hoạt-động)
- [6. TÍCH HỢP HỆ THỐNG](#6-tích-hợp-hệ-thống)
- [7. KIẾN TRÚC PHẦN MỀM](#7-kiến-trúc-phần-mềm)
- [8. ĐẶC TẢ HÀM / MODULE QUAN TRỌNG](#8-đặc-tả-hàm--module-quan-trọng)
- [9. KẾT QUẢ](#9-kết-quả)
- [10. ĐÁNH GIÁ THỰC TẾ VÀ GIỚI HẠN](#10-đánh-giá-thực-tế-và-giới-hạn)
- [11. KHÓ KHĂN VÀ KINH NGHIỆM RÚT RA](#11-khó-khăn-và-kinh-nghiệm-rút-ra)
- [12. HƯỚNG PHÁT TRIỂN](#12-hướng-phát-triển)
- [13. TÀI LIỆU THAM KHẢO](#13-tài-liệu-tham-khảo)
- [14. KẾT LUẬN](#14-kết-luận)

---

## 1. GIỚI THIỆU

### 1.1. Tên đề tài

**Radar tầm ngắn**

### 1.2. Mục tiêu

Mục tiêu của đề tài là thiết kế và triển khai một hệ thống nhúng có khả năng:

- Quét khu vực phía trước cảm biến theo một dải góc xác định.
- Đo khoảng cách vật cản bằng cảm biến siêu âm HC-SR04.
- Điều khiển servo MG90S để thay đổi hướng quét.
- Hiển thị trực quan trạng thái radar trên LCD TouchGFX của STM32F429I-DISC1.
- Hiển thị góc quét, khoảng cách, trạng thái và vị trí tương đối của vật cản.
- Cảnh báo bằng LED và buzzer khi phát hiện vật thể trong vùng gần.
- Cho phép thay đổi tốc độ và chế độ quét bằng giao diện cảm ứng hoặc nút USER.
- Hỗ trợ UART debug để theo dõi tín hiệu echo, khoảng cách và trạng thái chương trình.

### 1.3. Ý tưởng hệ thống

Hệ thống không phải radar RF theo nghĩa vật lý mà là một mô hình radar tầm ngắn sử dụng cảm biến siêu âm để mô phỏng quá trình quét và phát hiện vật cản.

Servo MG90S quay cảm biến HC-SR04 qua từng góc. Tại mỗi vị trí, STM32 phát xung trigger, bắt cạnh lên và cạnh xuống của tín hiệu echo, tính khoảng cách rồi gửi dữ liệu cho giao diện TouchGFX.

Luồng xử lý tổng quát:

1. Servo được đặt tới góc quét hiện tại.
2. Hệ thống chờ một khoảng ngắn để cơ cấu servo ổn định.
3. STM32 phát xung trigger 10 us tới HC-SR04.
4. Echo được bắt bằng ngắt ngoài EXTI3 trên chân PG3.
5. Phần mềm tính thời gian echo và suy ra khoảng cách.
6. Logic ứng dụng xác định trạng thái `SCAN`, `DETECT` hoặc `ALERT`.
7. Dữ liệu radar được gửi sang giao diện bằng FreeRTOS message queue.
8. LCD TouchGFX, LED và buzzer được cập nhật theo trạng thái mới.
9. Góc quét được tăng hoặc giảm để tiếp tục chu kỳ quét.

### 1.4. Chức năng chính

| Nhóm chức năng | Mô tả |
| --- | --- |
| Đo khoảng cách | Đo khoảng cách bằng HC-SR04, xử lý echo bằng EXTI và DWT cycle counter |
| Quét góc | Điều khiển servo MG90S bằng PWM TIM3_CH1 trên PB4 |
| Hiển thị LCD | TouchGFX hiển thị góc, khoảng cách, trạng thái và vị trí mục tiêu |
| Chế độ quét | Hỗ trợ chế độ quét 90 độ và 180 độ |
| Tốc độ quét | Hỗ trợ các mức `SLOW`, `MED`, `FAST` |
| Cảnh báo | LED và buzzer phản hồi theo trạng thái phát hiện vật cản |
| Nút USER | Bấm ngắn đổi tốc độ; giữ lâu đổi chế độ quét |
| FreeRTOS queue | Truyền dữ liệu radar và cấu hình điều khiển giữa các task |
| Debug UART | In trạng thái đo, echo, counter và thông tin hệ thống qua USART1 |

### 1.5. Cấu trúc project chính

```text
IT4210-HE-NHUNG/
|-- README.md
|-- REPORT_PLAN.md
|-- docs/
|   `-- images/
`-- radar_short_range_touchgfx/
    |-- Core/
    |-- Drivers/
    |-- Middlewares/
    |-- STM32CubeIDE/
    |-- TouchGFX/
    |-- DesignAssets/
    `-- STM32F429I_DISCO_REV_D01.ioc
```

---

## 2. TÁC GIẢ

### 2.1. Thành viên nhóm

| STT | MSSV | Họ và tên | Email |
| ---: | --- | --- | --- |
| 1 | 20235318 | Nguyễn Minh Giang | giang.nm235318@sis.hust.edu.vn |
| 2 | 20235333 | Bùi Trung Hoàng | hoang.bt235333@sis.hust.edu.vn |
| 3 | 20235342 | Phạm Ngọc Hưng | hung.pn235342@sis.hust.edu.vn |
| 4 | 20235421 | Khương Anh Tài | tai.ka235421@sis.hust.edu.vn |

### 2.2. Phân công công việc

| Thành viên | Công việc chính | Ghi chú |
| --- | --- | --- |
| Nguyễn Minh Giang | TODO: Nhóm bổ sung theo phân công thực tế | TODO |
| Bùi Trung Hoàng | TODO: Nhóm bổ sung theo phân công thực tế | TODO |
| **Phạm Ngọc Hưng** | **Thiết kế giao diện TouchGFX; xây dựng các màn Home, Scan, Settings, Info; tích hợp dữ liệu radar lên UI** | Phụ trách UI |
| Khương Anh Tài | TODO: Nhóm bổ sung theo phân công thực tế | TODO |

> [!IMPORTANT]
> Nhóm cần điền đầy đủ phần việc của ba thành viên còn lại trước khi nộp báo cáo chính thức.

---

## 3. MÔI TRƯỜNG HOẠT ĐỘNG

### 3.1. Board STM32F429I-DISC1

Hệ thống sử dụng board **STM32F429I-DISC1** làm bộ điều khiển trung tâm. Vi điều khiển chính là **STM32F429ZIT6**, thuộc dòng STM32F4, phù hợp với bài toán vừa điều khiển ngoại vi thời gian thực vừa chạy giao diện đồ họa nhúng.

Trong project, board đảm nhiệm các vai trò:

- Khởi tạo và điều phối GPIO, EXTI, timer, UART, LTDC, DMA2D và FMC/SDRAM.
- Chạy **FreeRTOS CMSIS v2** để tách xử lý radar, giao diện và nút USER.
- Chạy **TouchGFX** trên LCD tích hợp của board.
- Điều khiển servo MG90S bằng PWM.
- Đọc cảm biến HC-SR04 bằng ngắt ngoài.
- Điều khiển LED và buzzer.
- Truyền log debug về máy tính qua USART1.

Board STM32F429I-DISC1 có LCD và SDRAM tích hợp. Đây là lợi thế lớn khi triển khai TouchGFX, nhưng cũng làm nhiều chân của MCU đã được sử dụng bởi LCD và bộ nhớ ngoài.

Vì vậy pin mapping phải bám sát:

- [`STM32F429I_DISCO_REV_D01.ioc`](./radar_short_range_touchgfx/STM32F429I_DISCO_REV_D01.ioc)
- [`Core/Inc/main.h`](./radar_short_range_touchgfx/Core/Inc/main.h)

### 3.2. Các module sử dụng

| Nhóm | Module | Vai trò |
| --- | --- | --- |
| Điều khiển trung tâm | STM32F429I-DISC1 / STM32F429ZIT6 | Chạy firmware, FreeRTOS, TouchGFX và điều phối ngoại vi |
| Đo khoảng cách | HC-SR04 | Đo khoảng cách vật cản bằng sóng siêu âm |
| Cơ cấu quét | Servo MG90S | Quay cảm biến theo góc quét |
| Hiển thị | LCD tích hợp trên STM32F429I-DISC1 | Hiển thị giao diện radar bằng TouchGFX |
| Cảnh báo | Buzzer active | Cảnh báo âm thanh khi vật thể ở vùng gần |
| Chỉ thị trạng thái | LED on-board PG13/PG14 | Báo trạng thái quét và cảnh báo |
| Điều khiển nhanh | Nút USER PA0/WKUP | Bấm ngắn đổi tốc độ, giữ lâu đổi chế độ |
| Debug | USART1 PA9/PA10 | In log đo khoảng cách, echo và trạng thái hệ thống |

### 3.3. Bill of Materials

| STT | Linh kiện | Số lượng | Vai trò | Ghi chú kỹ thuật |
| ---: | --- | ---: | --- | --- |
| 1 | STM32F429I-DISC1 | 1 | Board điều khiển trung tâm | Có LCD, SDRAM và ST-LINK |
| 2 | HC-SR04 | 1 | Cảm biến đo khoảng cách | Trigger 10 us, sóng siêu âm khoảng 40 kHz |
| 3 | Servo MG90S | 1 | Quay cảm biến theo góc | Điều khiển PWM trên TIM3_CH1/PB4 |
| 4 | Buzzer active | 1 | Cảnh báo âm thanh | Điều khiển GPIO PC4 |
| 5 | LED on-board | 2 | Báo scan và alert | PG13 và PG14 |
| 6 | LCD tích hợp | 1 | Hiển thị giao diện TouchGFX | LCD có sẵn trên board |
| 7 | Nút USER | 1 | Điều khiển nhanh | PA0/WKUP |
| 8 | Mạch chia áp hoặc level shifter | 1 | Hạ mức tín hiệu echo | Bảo vệ GPIO 3.3 V |
| 9 | Dây jumper và breadboard | 1 bộ | Đấu nối phần cứng | Cần bảo đảm tiếp xúc chắc chắn |
| 10 | Nguồn 5 V đủ dòng | 1 | Cấp nguồn servo và cảm biến | Nên có tụ lọc gần servo |

### 3.4. Phần mềm và công cụ

| Hạng mục | Thông tin |
| --- | --- |
| Project chính | `radar_short_range_touchgfx` |
| File cấu hình CubeMX | `STM32F429I_DISCO_REV_D01.ioc` |
| MCU | STM32F429ZITx |
| STM32CubeMX | 6.17.0 |
| STM32Cube FW_F4 | V1.28.3 |
| TouchGFX | X-CUBE-TOUCHGFX 4.26.1 |
| RTOS | FreeRTOS CMSIS v2 |
| Toolchain | STM32CubeIDE |
| Giao diện | TouchGFX, LTDC, DMA2D, SDRAM |

### 3.5. Cấu hình ngoại vi chính

| Ngoại vi | Cấu hình / vai trò |
| --- | --- |
| TIM3 CH1 | PWM servo trên PB4, Prescaler = 89, Period = 19999, Pulse = 1500 |
| EXTI3 | Bắt tín hiệu echo trên PG3 ở cả cạnh lên và cạnh xuống |
| DWT CYCCNT | Tạo delay micro giây và đo độ rộng echo |
| TIM2 | Có cấu hình input capture nhưng không phải đường đo chính hiện tại |
| USART1 | TX PA9, RX PA10, dùng cho debug UART |
| LTDC | Điều khiển LCD tích hợp, kích thước giao diện 240 x 320 |
| DMA2D | Tăng tốc xử lý đồ họa |
| FMC/SDRAM | Bộ nhớ ngoài phục vụ TouchGFX |
| FreeRTOS | Chạy GUI task, radar task, button task và message queue |

---

## 4. SƠ ĐỒ SCHEMATIC / KẾT NỐI PHẦN CỨNG

### 4.1. Sơ đồ khối hệ thống

<p align="center">
  <img src="docs/images/a.png" width="700" alt="Sơ đồ khối hệ thống radar tầm ngắn">
  <br>
  <em>Hình 1. Sơ đồ khối hệ thống radar tầm ngắn.</em>
</p>

STM32F429I-DISC1 là bộ xử lý trung tâm của hệ thống:

- HC-SR04 nhận trigger và gửi echo về STM32.
- Servo MG90S nhận PWM để thay đổi hướng quét.
- LCD TouchGFX hiển thị dữ liệu radar.
- LED và buzzer phản hồi trạng thái cảnh báo.
- Nút USER thay đổi tốc độ và chế độ quét.
- USART1 gửi dữ liệu debug về máy tính.

### 4.2. Sơ đồ schematic

<p align="center">
  <img src="docs/images/b.png" width="700" alt="Sơ đồ schematic hệ thống radar tầm ngắn">
  <br>
  <em>Hình 2. Sơ đồ schematic kết nối phần cứng.</em>
</p>

Sơ đồ schematic cần thể hiện:

- Nguồn 5 V cấp cho HC-SR04 và servo MG90S.
- Nguồn cấp cho STM32F429I-DISC1.
- GND chung giữa STM32, cảm biến, servo và buzzer.
- PG2 nối với chân TRIG của HC-SR04.
- PG3 nhận tín hiệu ECHO qua mạch chia áp hoặc level shifter.
- PB4/TIM3_CH1 xuất PWM điều khiển servo.
- PC4 điều khiển buzzer.
- PG13 và PG14 điều khiển LED trạng thái.
- PA9 và PA10 dùng cho USART1 debug.

### 4.3. Ảnh đấu nối thực tế

```text
[Chèn ảnh: ảnh đấu nối thực tế]
```

Ảnh đấu nối nên chụp đủ:

- Board STM32F429I-DISC1.
- HC-SR04 được gắn trên servo MG90S.
- Nguồn 5 V cho servo.
- Dây trigger và echo.
- Mạch chia áp echo.
- Buzzer.
- Dây GND chung.

### 4.4. Bảng pin mapping

| Chân STM32 | Tên tín hiệu | Module | Chiều | Giải thích |
| --- | --- | --- | --- | --- |
| PB4 | `SERVO_PWM` / `TIM3_CH1` | Servo MG90S | STM32 → Servo | Xuất PWM 50 Hz để điều khiển góc servo |
| PG2 | `HCSR04_TRIG` | HC-SR04 | STM32 → Sensor | Phát xung trigger 10 us |
| PG3 | `HCSR04_ECHO` / `EXTI3` | HC-SR04 | Sensor → STM32 | Bắt cạnh lên và cạnh xuống của echo |
| PC4 | `BUZZER` | Buzzer active | STM32 → Buzzer | Cảnh báo khi vật nằm trong vùng gần |
| PG13 | `LED3_SCAN` | LED3 on-board | STM32 → LED | Báo hệ thống đang quét |
| PG14 | `LED4_ALERT` | LED4 on-board | STM32 → LED | Báo phát hiện hoặc cảnh báo gần |
| PA0/WKUP | `B1_USER` | Nút USER | Button → STM32 | Bấm ngắn đổi tốc độ, giữ lâu đổi mode |
| PA9 | `USART1_TX` | UART debug | STM32 → PC | Gửi log debug |
| PA10 | `USART1_RX` | UART debug | PC → STM32 | Nhận dữ liệu UART nếu cần mở rộng |

### 4.5. Mạch chia áp cho chân echo

HC-SR04 thường được cấp nguồn 5 V và chân ECHO có thể trả mức cao gần 5 V. STM32F429 hoạt động ở mức logic 3.3 V, vì vậy không nên nối trực tiếp echo vào PG3.

Ví dụ mạch chia áp:

```text
HC-SR04 ECHO ---- R1 = 1 kΩ ----+---- PG3 / EXTI3
                                |
                              R2 = 2 kΩ
                                |
                               GND
```

Điện áp tại điểm giữa được tính gần đúng:

```text
Vout = Vin × R2 / (R1 + R2)
     = 5 × 2 / (1 + 2)
     ≈ 3.33 V
```

### 4.6. Nguyên tắc cấp nguồn

Servo MG90S là tải cơ điện có dòng đỉnh lớn hơn nhiều so với cảm biến hoặc GPIO.

Không nên:

- Cấp servo từ chân GPIO.
- Dùng nguồn yếu cho cả servo và LCD.
- Quên nối chung GND.
- Đưa echo 5 V trực tiếp vào GPIO 3.3 V.

Nên:

- Cấp servo bằng nguồn 5 V đủ dòng.
- Nối GND nguồn servo với GND STM32.
- Bổ sung tụ lọc gần servo nếu nguồn bị sụt.
- Cố định dây nguồn và dây tín hiệu chắc chắn.

### 4.7. Lưu ý khi lắp mạch thực tế

> [!WARNING]
> Ba nguyên nhân lỗi phổ biến nhất là nguồn yếu, sai mức logic echo và đấu nhầm chân so với file `.ioc`.

| Hiện tượng | Nguyên nhân thường gặp | Cách kiểm tra / khắc phục |
| --- | --- | --- |
| LCD trắng hoặc board reset | Servo làm sụt nguồn | Dùng nguồn 5 V đủ dòng, nối GND chung |
| Servo rung hoặc giật | Nguồn yếu, pulse chưa phù hợp | Kiểm tra nguồn, hiệu chỉnh pulse min/max |
| Khoảng cách nhảy | Servo còn rung, vật phản xạ lệch | Chờ servo ổn định, dùng vật phẳng |
| Không có echo | Sai PG3, thiếu GND chung, lỗi chia áp | Kiểm tra dây và tín hiệu echo |
| UART không có log | Sai TX/RX hoặc baudrate | Kiểm tra PA9/PA10, baudrate và GND |
| LED/buzzer không phản hồi | Sai chân hoặc logic điều khiển | Test riêng PC4, PG13, PG14 |
| UI không cập nhật | Queue chưa nhận được data hoặc task bị block | Kiểm tra bridge, queue và UART debug |

Nên kiểm thử từng khối theo thứ tự:

1. Test LED và buzzer.
2. Test PWM servo.
3. Test HC-SR04 khi servo đứng yên.
4. Test UART debug.
5. Ghép servo và HC-SR04.
6. Ghép FreeRTOS message queue.
7. Ghép TouchGFX.
8. Kiểm thử toàn hệ thống.

### 4.8. Video demo phần cứng và vận hành hệ thống

Video dưới đây minh họa quá trình lắp ráp và vận hành thực tế của hệ thống radar tầm ngắn. Nội dung demo gồm:

- Board STM32F429I-DISC1 và các kết nối phần cứng.
- Servo MG90S quay cảm biến HC-SR04.
- HC-SR04 đo khoảng cách vật cản.
- Giao diện TouchGFX hiển thị góc, khoảng cách và trạng thái radar.
- LED và buzzer phản hồi khi phát hiện vật cản gần.
- Thay đổi tốc độ quét và chế độ quét 90°/180°.

<p align="center">
  <a href="YOUTUBE_VIDEO_URL">
    <img src="https://img.youtube.com/vi/YOUTUBE_VIDEO_ID/maxresdefault.jpg"
         width="700"
         alt="Video demo hệ thống radar tầm ngắn">
  </a>
  <br>
  <em>Video 1. Demo phần cứng và vận hành hệ thống radar tầm ngắn.</em>
</p>

<p align="center">
  <a href="YOUTUBE_VIDEO_URL"><strong>▶ Xem video demo trên YouTube</strong></a>
</p>

---

## 5. NGUYÊN LÝ HOẠT ĐỘNG

```text
[Chèn ảnh: lưu đồ hoạt động hệ thống]
```

### 5.1. Luồng hoạt động tổng quát

Vòng xử lý chính nằm trong hàm `RadarApp_TaskLoop()` của module:

[`radar_app.c`](./radar_short_range_touchgfx/STM32CubeIDE/Application/User/radar_app.c)

Trình tự xử lý:

1. Nhận cấu hình điều khiển mới từ `RadarControlQueue`.
2. Kiểm tra radar đang bật hay tắt.
3. Nếu radar tắt:
   - Đưa hệ thống về trạng thái an toàn.
   - Tắt buzzer và LED cảnh báo.
   - Tạo dữ liệu trạng thái dừng cho UI.
4. Nếu radar bật:
   - Đặt servo tới góc hiện tại.
   - Chờ servo ổn định.
   - Phát trigger HC-SR04.
   - Chờ echo hoặc timeout.
   - Tính khoảng cách.
   - Xác định `SCAN`, `DETECT` hoặc `ALERT`.
   - Điều khiển LED/buzzer.
   - Gửi dữ liệu sang `RadarMessageQueue`.
   - Tăng hoặc giảm góc quét.

### 5.2. Nguyên lý đo HC-SR04

Khi chân TRIG nhận xung mức cao tối thiểu khoảng 10 us, HC-SR04 phát một chùm sóng siêu âm khoảng 40 kHz.

Khi nhận được sóng phản xạ, module tạo xung ECHO. Độ rộng xung ECHO biểu diễn thời gian sóng âm đi từ cảm biến tới vật rồi quay trở lại.

Trong project:

- `PG2 = HCSR04_TRIG`
- `PG3 = HCSR04_ECHO`
- `HCSR04_StartMeasure()` phát trigger.
- `HCSR04_GPIO_EXTI_Callback()` bắt cạnh echo.
- DWT cycle counter đo thời gian echo.

Cạnh lên:

```text
Lưu thời điểm bắt đầu echo
```

Cạnh xuống:

```text
Lưu thời điểm kết thúc echo
Tính echo_us
```

Công thức trong driver:

```c
g_echo_us = (g_falling_cycle - g_rising_cycle) / g_cycles_per_us;
```

### 5.3. Công thức tính khoảng cách

Sóng âm phải đi từ cảm biến tới vật và quay về, do đó quãng đường đo được bằng hai lần khoảng cách thực.

```text
distance = speed_of_sound × echo_time / 2
```

Với vận tốc âm thanh xấp xỉ 343 m/s:

```text
343 m/s = 0.0343 cm/us
```

Suy ra:

```text
distance_cm = echo_us × 0.0343 / 2
            ≈ echo_us / 58
```

Trong code:

```c
distance = g_echo_us / 58U;
```

Các ngưỡng đang sử dụng:

| Tham số | Giá trị |
| --- | ---: |
| `RADAR_MIN_DISTANCE_CM` | 2 cm |
| `RADAR_MAX_DISPLAY_CM` | 50 cm |
| `RADAR_OBJECT_DETECT_CM` | 50 cm |
| `RADAR_NEAR_WARNING_CM` | 5 cm |
| `HCSR04_TIMEOUT_MS` | 25 ms |

### 5.4. State của driver HC-SR04

Driver sử dụng các trạng thái nội bộ:

| State | Ý nghĩa |
| --- | --- |
| `IDLE` | Chưa có phép đo |
| `WAIT_RISING` | Đang chờ cạnh lên của echo |
| `WAIT_FALLING` | Đã nhận cạnh lên, đang chờ cạnh xuống |
| `DONE` | Đo hoàn thành |
| `TIMEOUT` | Không nhận đủ echo trong thời gian quy định |
| `ERROR` | Kết quả không hợp lệ |

State machine giúp tránh chờ vô hạn và cho phép UART debug xác định lỗi xảy ra ở giai đoạn nào.

### 5.5. Ảnh hưởng của môi trường

Công thức `echo_us / 58` là công thức gần đúng. Kết quả thực tế bị ảnh hưởng bởi:

- Nhiệt độ và độ ẩm.
- Luồng gió.
- Góc đặt vật.
- Bề mặt phản xạ.
- Kích thước vật.
- Rung cơ khí từ servo.
- Nguồn cấp cảm biến.
- Khoảng cách quá gần.
- Echo chéo từ các bề mặt xung quanh.

HC-SR04 phù hợp cho mô hình phát hiện vật cản và đo khoảng cách tương đối, không nên coi là thiết bị đo chính xác cao trong mọi điều kiện.

### 5.6. Điều khiển servo bằng PWM

Servo MG90S được điều khiển bằng PWM chu kỳ khoảng 20 ms, tương ứng 50 Hz.

Driver chính:

- [`servo_mg90s.c`](./radar_short_range_touchgfx/STM32CubeIDE/Application/User/servo_mg90s.c)
- [`servo_mg90s.h`](./radar_short_range_touchgfx/STM32CubeIDE/Application/User/servo_mg90s.h)

Các hàm chính:

- `Servo_Init()`
- `Servo_SetPulseUs()`
- `Servo_SetAngle()`
- `Servo_Stop()`
- `Servo_GetLastAngle()`
- `Servo_GetLastPulseUs()`

Thông số cấu hình:

| Tham số | Giá trị |
| --- | ---: |
| Góc nhỏ nhất | 0° |
| Góc giữa | 90° |
| Góc lớn nhất | 180° |
| Pulse nhỏ nhất | 550 us |
| Pulse giữa | 1500 us |
| Pulse lớn nhất | 2450 us |

Công thức nội suy:

```text
pulse_us = SERVO_MIN_PULSE_US
         + angle_deg × (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US) / 180
```

### 5.7. Cấu hình TIM3

TIM3_CH1 được đưa ra PB4.

| Cấu hình | Giá trị |
| --- | ---: |
| Timer clock | 90 MHz |
| Prescaler | 89 |
| Period | 19999 |
| Pulse ban đầu | 1500 |

Mỗi tick timer:

```text
tick = (89 + 1) / 90 MHz
     = 1 us
```

Chu kỳ PWM:

```text
period = (19999 + 1) × 1 us
       = 20000 us
       = 20 ms
```

Tần số:

```text
f = 1 / 20 ms
  = 50 Hz
```

Việc cấu hình tick 1 us giúp phần mềm có thể ghi trực tiếp giá trị pulse theo micro giây vào thanh ghi compare.

### 5.8. Hiển thị dữ liệu trên TouchGFX

Giao diện chính sử dụng LCD tích hợp của STM32F429I-DISC1.

Dữ liệu radar không được ghi trực tiếp từ driver cảm biến vào widget TouchGFX. Module `radar_ui_bridge.c/h` làm cầu nối giữa:

```text
RadarTask viết bằng C
          ↕
RadarUiBridge + FreeRTOS queue
          ↕
TouchGFX View viết bằng C++
```

Dữ liệu lõi gồm:

- `angle_deg`
- `distance_cm`
- `distance_valid`
- `object_detected`
- `near_warning`
- `radar_status`
- `object_count`
- `last_object_distance_cm`
- `last_object_angle_deg`
- `buzzer_on`
- `led3_on`
- `led4_on`

Cấu hình điều khiển gồm:

- `radar_enabled`
- `scan_mode_deg`
- `speed_mode`

Trong `ScreenScanView.cpp`, UI cập nhật:

- Góc quét.
- Khoảng cách.
- Trạng thái.
- Tia quét xanh hoặc đỏ.
- Vị trí target dot.
- Trạng thái phát hiện vật cản.

```text
[Chèn ảnh: giao diện màn hình radar]
```

### 5.9. Vai trò buzzer và LED

Module:

- [`buzzer_led.c`](./radar_short_range_touchgfx/STM32CubeIDE/Application/User/buzzer_led.c)
- [`buzzer_led.h`](./radar_short_range_touchgfx/STM32CubeIDE/Application/User/buzzer_led.h)

Pin:

```text
PC4  = BUZZER
PG13 = LED3_SCAN
PG14 = LED4_ALERT
```

Logic:

| Trạng thái | LED scan | LED alert | Buzzer |
| --- | --- | --- | --- |
| Không có vật | Hoạt động | Tắt | Tắt |
| Có vật trong vùng phát hiện | Hoạt động | Bật | Tắt |
| Vật ở vùng rất gần | Hoạt động | Bật | Kêu theo chu kỳ |

---

## 6. TÍCH HỢP HỆ THỐNG

### 6.1. Kiến trúc tổng quát

Project được chia thành các lớp:

| Lớp | File / module | Vai trò |
| --- | --- | --- |
| Nền tảng | `main.c`, `.ioc` | Khởi tạo clock, GPIO, timer, UART, LTDC, DMA2D, SDRAM và FreeRTOS |
| Driver | `hcsr04.c`, `servo_mg90s.c`, `buzzer_led.c`, `radar_debug.c` | Điều khiển phần cứng |
| Cấu hình | `radar_config.h`, `radar_types.h` | Chứa ngưỡng, mode và kiểu dữ liệu |
| Ứng dụng | `radar_app.c` | Điều phối toàn bộ hoạt động radar |
| Giao tiếp task/UI | `radar_ui_bridge.c` | Message queue và snapshot dữ liệu |
| Giao diện | Các file `Screen...View.cpp` | Hiển thị dữ liệu và nhận thao tác |
| Middleware | FreeRTOS, TouchGFX, HAL | Lập lịch, queue, đồ họa và abstraction phần cứng |

### 6.2. Các task chính

| Task | Hàm | Vai trò |
| --- | --- | --- |
| `defaultTask` | `StartDefaultTask()` | Đọc nút USER |
| `GUI_Task` | `TouchGFX_Task()` | Chạy giao diện TouchGFX |
| `radarTask` | `StartRadarTask()` | Khởi tạo và chạy `RadarApp_TaskLoop()` |

### 6.3. Luồng dữ liệu cảm biến

```text
HC-SR04
  -> EXTI3 callback
  -> hcsr04.c
  -> RadarApp_MeasureDistance()
  -> RadarApp_TaskLoop()
  -> RadarCoreData_t
  -> RadarMessageQueue
  -> RadarUiBridge_GetData()
  -> TouchGFX
  -> LCD
```

Luồng cảnh báo:

```text
RadarApp_TaskLoop()
  -> detected / near_warning
  -> Alert_Update()
  -> PG13 / PG14 / PC4
```

### 6.4. FreeRTOS message queue

Project sử dụng hai queue.

#### RadarMessageQueue

Truyền dữ liệu từ radar task sang UI:

```text
RadarTask
  -> RadarUiBridge_SetData()
  -> RadarMessageHandle
  -> RadarUiBridge_GetData()
  -> TouchGFX
```

Kiểu dữ liệu chính:

```c
RadarCoreData_t
```

Queue này chứa dữ liệu đo và trạng thái hệ thống.

#### RadarControlQueue

Truyền lệnh điều khiển từ UI hoặc button task sang radar task:

```text
TouchGFX / ButtonTask
  -> RadarUiBridge_SetRadarEnabled()
  -> RadarUiBridge_SetSpeedMode()
  -> RadarUiBridge_SetScanMode()
  -> RadarControlQueueHandle
  -> RadarApp_TaskLoop()
```

Kiểu dữ liệu chính:

```c
RadarControlConfig_t
```

### 6.5. Snapshot và critical section

Ngoài queue, bridge giữ snapshot gần nhất:

```c
static RadarCoreData_t g_radar_core;
static RadarControlConfig_t g_radar_control;
```

Snapshot giúp UI vẫn có dữ liệu để hiển thị khi chưa nhận được message mới.

Các thao tác copy snapshot được bảo vệ bằng:

```c
__disable_irq();
...
__enable_irq();
```

Critical section này chỉ tồn tại trong thời gian rất ngắn để tránh:

- UI đọc struct khi radar task đang copy dở.
- Radar task đọc cấu hình khi UI đang cập nhật dở.
- Dữ liệu trong cùng một snapshot bị trộn giữa giá trị cũ và mới.

> [!NOTE]
> Cơ chế truyền dữ liệu chính là FreeRTOS message queue. Việc tạm khóa ngắt chỉ dùng để bảo vệ snapshot dữ liệu dùng chung.

### 6.6. Trạng thái ứng dụng

| Trạng thái | Ý nghĩa | Điều kiện |
| --- | --- | --- |
| `RADAR_STATUS_SCAN` | Đang quét, chưa có vật hợp lệ | Không có vật trong ngưỡng |
| `RADAR_STATUS_DETECT` | Đã phát hiện vật | Khoảng cách hợp lệ và ≤ ngưỡng detect |
| `RADAR_STATUS_ALERT` | Vật quá gần | Khoảng cách hợp lệ và ≤ ngưỡng near |

Các biến điều khiển:

```text
radar_enabled
scan_mode_deg
speed_mode
g_angle
g_direction
g_prev_detected
```

### 6.7. Vai trò timer và ngắt

| Thành phần | Vai trò |
| --- | --- |
| TIM3 | PWM servo MG90S |
| EXTI3 | Bắt cạnh echo HC-SR04 |
| DWT CYCCNT | Đo thời gian echo và delay micro giây |
| TIM2 | Cấu hình input capture dự phòng/cũ |
| TIM6 | Time base của HAL |
| USART1 | Gửi log debug |

TIM2 vẫn có trong cấu hình nhưng driver HC-SR04 hiện tại sử dụng EXTI3 kết hợp DWT. Khi mô tả firmware thực tế, cần ưu tiên cơ chế EXTI3 + DWT.

### 6.8. TouchGFX cập nhật theo tick

Trong `ScreenScanView`, `handleTickEvent()` không nhất thiết cập nhật toàn bộ widget ở mọi frame. UI gọi `updateRadarUi()` theo chu kỳ định sẵn để:

- Giảm số lần cập nhật text.
- Giảm số lần invalidate widget.
- Hạn chế tải đồ họa.
- Vẫn bảo đảm giao diện đủ mượt.

```text
[Chèn ảnh: sơ đồ luồng dữ liệu phần mềm]
```

---

## 7. KIẾN TRÚC PHẦN MỀM

| File / module | Vai trò |
| --- | --- |
| `Core/Src/main.c` | Khởi tạo HAL, ngoại vi, FreeRTOS task, queue và EXTI callback |
| `radar_config.h` | Chứa thông số khoảng cách, servo, mode, speed và UI |
| `radar_types.h` | Khai báo enum và struct dữ liệu radar |
| `radar_app.c/h` | Điều phối quét, đo, cảnh báo và truyền dữ liệu |
| `hcsr04.c/h` | Driver HC-SR04 dùng EXTI + DWT |
| `servo_mg90s.c/h` | Driver servo dùng TIM3_CH1 |
| `buzzer_led.c/h` | Driver LED và buzzer |
| `radar_ui_bridge.c/h` | Queue và snapshot giữa radar task với TouchGFX |
| `radar_debug.c/h` | In log qua USART1 |
| `ScreenHomeView.cpp` | Màn hình Home |
| `ScreenScanView.cpp` | Màn hình radar chính |
| `ScreenSettingsView.cpp` | Chọn speed và mode |
| `ScreenInfoView.cpp` | Hiển thị thống kê |

---

## 8. ĐẶC TẢ HÀM / MODULE QUAN TRỌNG

### 8.1. Driver HC-SR04

File:

- [`hcsr04.h`](./radar_short_range_touchgfx/STM32CubeIDE/Application/User/hcsr04.h)
- [`hcsr04.c`](./radar_short_range_touchgfx/STM32CubeIDE/Application/User/hcsr04.c)

| Hàm | Chức năng | Ý nghĩa |
| --- | --- | --- |
| `HCSR04_Init()` | Khởi tạo driver và DWT | Đưa cảm biến về trạng thái ban đầu |
| `HCSR04_StartMeasure()` | Phát trigger 10 us | Bắt đầu một lần đo |
| `HCSR04_GPIO_EXTI_Callback()` | Bắt cạnh echo | Đo độ rộng xung không cần polling liên tục |
| `HCSR04_ProcessTimeout()` | Kiểm tra timeout | Tránh hệ thống chờ vô hạn |
| `HCSR04_GetDistanceCm()` | Đổi echo sang cm | Trả khoảng cách hợp lệ |
| `HCSR04_GetLastEchoUs()` | Trả echo gần nhất | Phục vụ UART debug |
| `HCSR04_GetStartCount()` | Trả số lần bắt đầu đo | Kiểm tra trigger |
| `HCSR04_GetRisingCount()` | Trả số cạnh lên | Debug tín hiệu echo |
| `HCSR04_GetFallingCount()` | Trả số cạnh xuống | Debug tín hiệu echo |
| `HCSR04_GetTimeoutCount()` | Trả số lần timeout | Đánh giá độ ổn định |

### 8.2. Driver servo MG90S

File:

- [`servo_mg90s.h`](./radar_short_range_touchgfx/STM32CubeIDE/Application/User/servo_mg90s.h)
- [`servo_mg90s.c`](./radar_short_range_touchgfx/STM32CubeIDE/Application/User/servo_mg90s.c)

| Hàm | Chức năng |
| --- | --- |
| `Servo_Init()` | Start PWM và đặt servo về vị trí giữa |
| `Servo_SetPulseUs()` | Ghi pulse theo micro giây |
| `Servo_SetAngle()` | Chuyển góc sang pulse |
| `Servo_Stop()` | Đưa servo về trạng thái dừng/an toàn |
| `Servo_GetLastAngle()` | Trả góc cuối |
| `Servo_GetLastPulseUs()` | Trả pulse cuối |

### 8.3. Driver buzzer và LED

File:

- [`buzzer_led.h`](./radar_short_range_touchgfx/STM32CubeIDE/Application/User/buzzer_led.h)
- [`buzzer_led.c`](./radar_short_range_touchgfx/STM32CubeIDE/Application/User/buzzer_led.c)

| Hàm | Chức năng |
| --- | --- |
| `BuzzerLed_Init()` | Tắt toàn bộ output khi khởi tạo |
| `Buzzer_Set()` | Bật/tắt buzzer |
| `LedScan_Set()` | Bật/tắt LED scan |
| `LedAlert_Set()` | Bật/tắt LED alert |
| `Alert_Update()` | Phân cấp cảnh báo theo trạng thái vật cản |

### 8.4. Logic radar

File:

- [`radar_app.h`](./radar_short_range_touchgfx/STM32CubeIDE/Application/User/radar_app.h)
- [`radar_app.c`](./radar_short_range_touchgfx/STM32CubeIDE/Application/User/radar_app.c)

| Hàm | Chức năng |
| --- | --- |
| `RadarApp_Init()` | Khởi tạo toàn bộ module radar |
| `RadarApp_Start()` | Bật radar |
| `RadarApp_Stop()` | Dừng radar và đặt output an toàn |
| `RadarApp_SetSpeedMode()` | Đặt tốc độ SLOW/MED/FAST |
| `RadarApp_NextSpeedMode()` | Chuyển vòng tốc độ |
| `RadarApp_SetScanMode()` | Chọn quét 90° hoặc 180° |
| `RadarApp_ToggleScanMode()` | Đổi qua lại 90°/180° |
| `RadarApp_TaskLoop()` | Vòng xử lý chính của radar |
| `RadarApp_FillStoppedData()` | Tạo snapshot trạng thái dừng cho UI |
| `RadarApp_AdvanceAngle()` | Tăng/giảm góc và đổi hướng ở biên |
| `RadarApp_MeasureDistance()` | Thực hiện một chu kỳ đo HC-SR04 |

`RadarApp_FillStoppedData()` không trực tiếp đo cảm biến hoặc điều khiển widget. Hàm này chuẩn bị dữ liệu an toàn khi radar dừng:

- Khoảng cách không hợp lệ.
- Không phát hiện vật.
- Không có cảnh báo.
- LED/buzzer tắt.
- Góc hiển thị trở về vị trí an toàn.
- Cấu hình speed/mode vẫn được duy trì.

### 8.5. Bridge dữ liệu UI

File:

- [`radar_ui_bridge.h`](./radar_short_range_touchgfx/STM32CubeIDE/Application/User/radar_ui_bridge.h)
- [`radar_ui_bridge.c`](./radar_short_range_touchgfx/STM32CubeIDE/Application/User/radar_ui_bridge.c)

| Hàm | Chức năng |
| --- | --- |
| `RadarUiBridge_Init()` | Khởi tạo snapshot lõi và control |
| `RadarUiBridge_SetData()` | Gửi `RadarCoreData_t` vào radar message queue |
| `RadarUiBridge_GetData()` | Nhận message mới và trả snapshot cho UI |
| `RadarUiBridge_IsControlConfigChange()` | Radar task kiểm tra cấu hình mới |
| `RadarUiBridge_SetRadarEnabled()` | Cập nhật bật/tắt radar và gửi control queue |
| `RadarUiBridge_SetSpeedMode()` | Cập nhật tốc độ và gửi control queue |
| `RadarUiBridge_SetScanMode()` | Cập nhật mode và gửi control queue |
| `RadarUiBridge_NextSpeedMode()` | Chuyển vòng tốc độ |

Cấu trúc dữ liệu:

```c
typedef struct
{
    uint16_t angle_deg;
    uint16_t distance_cm;
    uint8_t distance_valid;
    uint8_t object_detected;
    uint8_t near_warning;
    uint8_t radar_status;
    uint16_t object_count;
    uint16_t last_object_distance_cm;
    uint16_t last_object_angle_deg;
    uint8_t buzzer_on;
    uint8_t led3_on;
    uint8_t led4_on;
} RadarCoreData_t;
```

```c
typedef struct
{
    uint8_t radar_enabled;
    uint8_t scan_mode_deg;
    uint8_t speed_mode;
} RadarControlConfig_t;
```

```c
typedef struct
{
    RadarCoreData_t core_data;
    RadarControlConfig_t control;
} RadarUiData_t;
```

> [!NOTE]
> Đoạn struct trên mô tả các trường được sử dụng trong báo cáo. Nếu source còn chứa trường dự phòng không gắn với phần cứng thực tế, nhóm nên xóa hoặc không sử dụng trước khi nộp bản cuối.

### 8.6. Giao diện TouchGFX

File:

- [`ScreenHomeView.cpp`](./radar_short_range_touchgfx/TouchGFX/gui/src/screenhome_screen/ScreenHomeView.cpp)
- [`ScreenScanView.cpp`](./radar_short_range_touchgfx/TouchGFX/gui/src/screenscan_screen/ScreenScanView.cpp)
- [`ScreenSettingsView.cpp`](./radar_short_range_touchgfx/TouchGFX/gui/src/screensettings_screen/ScreenSettingsView.cpp)
- [`ScreenInfoView.cpp`](./radar_short_range_touchgfx/TouchGFX/gui/src/screeninfo_screen/ScreenInfoView.cpp)

| Hàm | Chức năng |
| --- | --- |
| `ScreenHomeView::setupScreen()` | Đưa hệ thống về trạng thái Home |
| `ScreenScanView::setupScreen()` | Khởi động radar và reset hiển thị |
| `ScreenScanView::handleTickEvent()` | Cập nhật UI định kỳ |
| `ScreenScanView::updateRadarUi()` | Cập nhật angle, distance, status và sweep |
| `ScreenScanView::updateTarget()` | Tính vị trí target dot |
| `ScreenSettingsView::handleClickEvent()` | Xử lý chọn speed và mode |
| `ScreenInfoView::updateInfoText()` | Hiển thị thống kê hệ thống |

### 8.7. Debug UART

File:

- [`radar_debug.h`](./radar_short_range_touchgfx/STM32CubeIDE/Application/User/radar_debug.h)
- [`radar_debug.c`](./radar_short_range_touchgfx/STM32CubeIDE/Application/User/radar_debug.c)

`RadarDebug_Printf()` là wrapper cho USART1. Hàm tạo chuỗi bằng `vsnprintf()` rồi gọi `HAL_UART_Transmit()`.

Các thông tin debug gồm:

- Góc servo.
- Pulse servo.
- Khoảng cách.
- Echo micro giây.
- State HC-SR04.
- Số lần start.
- Số cạnh rising.
- Số cạnh falling.
- Số lần timeout.
- Delta counter giữa hai lần log.

Các biến:

```c
static uint32_t prev_start;
static uint32_t prev_rise;
static uint32_t prev_fall;
static uint32_t prev_tout;
```

chỉ dùng để lưu counter của lần log trước và tính số sự kiện phát sinh trong khoảng thời gian giữa hai lần in UART. Chúng không tham gia trực tiếp vào thuật toán đo khoảng cách.

---

## 9. KẾT QUẢ

### 9.1. Kết quả đạt được

Project đã triển khai:

- Driver HC-SR04.
- Driver servo MG90S.
- LED và buzzer cảnh báo.
- Logic radar 90°/180°.
- Ba mức tốc độ quét.
- FreeRTOS task.
- Hai FreeRTOS message queue.
- UART debug.
- Giao diện TouchGFX nhiều màn hình.
- Hiển thị sweep và target dot.

### 9.2. Chức năng theo mã nguồn

| Chức năng | Trạng thái | Minh chứng |
| --- | --- | --- |
| Khởi tạo board và ngoại vi | Đã triển khai | `main.c`, `.ioc` |
| Đo HC-SR04 | Đã triển khai | `hcsr04.c` |
| Điều khiển servo | Đã triển khai | `servo_mg90s.c` |
| Quét 90°/180° | Đã triển khai | `radar_app.c`, `radar_config.h` |
| SLOW/MED/FAST | Đã triển khai | `RadarApp_SetSpeedMode()` |
| SCAN/DETECT/ALERT | Đã triển khai | `radar_types.h`, `radar_app.c` |
| LED scan/alert | Đã triển khai | `buzzer_led.c` |
| Buzzer cảnh báo | Đã triển khai | `Alert_Update()` |
| TouchGFX radar UI | Đã triển khai | `ScreenScanView.cpp` |
| Settings UI | Đã triển khai | `ScreenSettingsView.cpp` |
| Info UI | Đã triển khai | `ScreenInfoView.cpp` |
| Radar message queue | Đã triển khai | `radar_ui_bridge.c` |
| Control message queue | Đã triển khai | `radar_ui_bridge.c` |
| UART debug | Đã triển khai | `radar_debug.c` |

### 9.3. Hình ảnh giao diện

```text
[Chèn ảnh: giao diện màn hình Home]
```

```text
[Chèn ảnh: giao diện màn hình Scan khi đang quét]
```

```text
[Chèn ảnh: giao diện màn hình Scan khi phát hiện vật cản]
```

```text
[Chèn ảnh: giao diện màn hình Settings]
```

```text
[Chèn ảnh: giao diện màn hình Info]
```

### 9.4. Video demo

```text
[Chèn video: demo tổng thể sản phẩm]
```

```text
[Chèn video: servo quét và HC-SR04 phát hiện vật cản]
```

```text
[Chèn video: đổi chế độ 90°/180° và tốc độ SLOW/MED/FAST]
```

```text
[Chèn video: cảnh báo LED/buzzer]
```

### 9.5. Bảng kiểm thử cơ bản

| Kiểm thử | Cách thực hiện | Kết quả mong đợi | Kết quả thực tế |
| --- | --- | --- | --- |
| Servo PWM | Vào màn Scan | Servo quét theo mode | TODO |
| HC-SR04 | Đưa vật phẳng trước cảm biến | Có khoảng cách hợp lệ | TODO |
| Detect | Đặt vật trong phạm vi ≤ 50 cm | UI chuyển `DETECT` | TODO |
| Alert | Đặt vật trong phạm vi ≤ 5 cm | UI chuyển `ALERT`, buzzer kêu | TODO |
| Speed | Bấm UI hoặc bấm ngắn USER | Đổi SLOW/MED/FAST | TODO |
| Scan mode | Bấm UI hoặc giữ USER | Đổi 90°/180° | TODO |
| Home | Quay về Home | Radar dừng an toàn | TODO |
| UART | Mở terminal | Có angle, distance, echo và counter | TODO |
| Radar queue | Theo dõi UI khi radar chạy | UI nhận dữ liệu mới | TODO |
| Control queue | Đổi mode/speed từ UI | Radar task nhận cấu hình | TODO |

### 9.6. Bảng đo thực nghiệm

| Điều kiện | Khoảng cách thực (cm) | Khoảng cách hiển thị (cm) | Sai số (cm) | Nhận xét |
| --- | ---: | ---: | ---: | --- |
| Vật phẳng, servo đứng yên | TODO | TODO | TODO | TODO |
| Vật phẳng, quét chậm | TODO | TODO | TODO | TODO |
| Vật phẳng, quét nhanh | TODO | TODO | TODO | TODO |
| Vật đặt lệch góc | TODO | TODO | TODO | TODO |
| Vật nhỏ | TODO | TODO | TODO | TODO |
| Vùng cảnh báo gần | TODO | TODO | TODO | TODO |

---

## 10. ĐÁNH GIÁ THỰC TẾ VÀ GIỚI HẠN

### 10.1. Nhận xét chung

Hệ thống đáp ứng tốt vai trò mô hình radar tầm ngắn phục vụ học tập. Sản phẩm có cơ cấu quét, đo khoảng cách, giao diện trực quan và cảnh báo.

Tuy nhiên, đây không phải radar RF và không phải thiết bị đo khoảng cách chính xác cao trong mọi môi trường.

### 10.2. Giới hạn HC-SR04

Thông số danh định phổ biến của HC-SR04:

- Tầm đo khoảng 2–400 cm.
- Trigger tối thiểu khoảng 10 us.
- Tần số siêu âm khoảng 40 kHz.
- Độ chính xác lý tưởng có thể được công bố ở mức vài milimét.

Trong thực tế, sai số phụ thuộc vào:

- Góc phản xạ.
- Vật liệu bề mặt.
- Kích thước vật.
- Rung servo.
- Nguồn cấp.
- Nhiệt độ và độ ẩm.
- Nhiễu từ môi trường.
- Cách gá cảm biến.

Do đó cần đánh giá bằng số liệu đo thực nghiệm thay vì chỉ dựa vào datasheet.

### 10.3. Giới hạn servo MG90S

MG90S có stall torque tham khảo khoảng:

- 1.8 kg·cm ở 4.8 V.
- 2.2 kg·cm ở khoảng 6 V.

Stall torque không phải tải làm việc liên tục.

Ví dụ, nếu cánh tay đòn dài 1 cm thì lực lý thuyết tại trạng thái stall có thể tương đương khoảng 1.8–2.2 kgf. Nếu cánh tay đòn dài 2 cm, lực giảm còn khoảng một nửa.

Trong sử dụng thực tế:

- Không nên để servo giữ tải gần stall lâu.
- Gá HC-SR04 nên cân bằng và nhẹ.
- Không để servo kẹt ở biên.
- Cần hiệu chỉnh pulse min/max.
- Cần nguồn đủ dòng.

### 10.4. Ảnh hưởng của chuyển động servo

Nếu đo ngay khi servo còn đang di chuyển:

- Góc thực tế có thể khác góc hiển thị.
- Hướng phát siêu âm có thể thay đổi trong lúc đo.
- Echo dễ bị mất hoặc nhiễu.
- Target dot có thể xuất hiện sai vị trí.

Vì vậy cần có thời gian chờ servo ổn định trước khi phát trigger.

### 10.5. Giới hạn STM32F429I-DISC1

Board có LCD và SDRAM tích hợp, thuận lợi cho TouchGFX nhưng nhiều chân đã được sử dụng.

Hệ thống phải chạy đồng thời:

- GUI task.
- Radar task.
- Button task.
- EXTI echo.
- PWM servo.
- UART debug.
- Message queue.
- DMA2D/LTDC.

Nếu một task blocking quá lâu hoặc UI cập nhật quá nhiều, độ mượt và tốc độ quét có thể bị ảnh hưởng.

### 10.6. Giới hạn message queue và snapshot

Message queue giúp luồng dữ liệu rõ ràng hơn nhưng vẫn cần chú ý:

- Queue có thể đầy nếu bên gửi nhanh hơn bên nhận.
- UI có thể hiển thị dữ liệu cũ nếu không đọc queue đủ thường xuyên.
- Với dữ liệu trạng thái liên tục, đôi khi chỉ cần giữ mẫu mới nhất thay vì mọi mẫu.
- Critical section toàn cục cần được giữ thật ngắn.
- Không nên thực hiện xử lý nặng trong khi đang khóa ngắt.

---

## 11. KHÓ KHĂN VÀ KINH NGHIỆM RÚT RA

### 11.1. Khó khăn và giải pháp

| Hiện tượng | Nguyên nhân khả dĩ | Giải pháp |
| --- | --- | --- |
| Màn hình trắng hoặc reset | Servo làm sụt nguồn | Dùng nguồn servo riêng, GND chung |
| Servo rung | Nguồn yếu hoặc pulse sai | Kiểm tra nguồn và hiệu chỉnh pulse |
| Khoảng cách nhảy | Servo rung hoặc phản xạ kém | Tăng settle time, dùng vật phẳng |
| Không có echo | Sai dây hoặc mức logic | Kiểm tra PG3 và mạch chia áp |
| Có rising nhưng không có falling | Echo kẹt cao hoặc callback lỗi | Kiểm tra tín hiệu và EXTI |
| Timeout liên tục | Không có vật phản xạ hoặc dây lỗi | Kiểm tra trigger, echo và GND |
| UI không đổi | Queue không có message mới | Kiểm tra `osMessageQueuePut/Get` |
| UI trễ | Queue tồn nhiều mẫu cũ | Giảm tần số gửi hoặc chỉ giữ mẫu mới |
| Speed/mode không đổi | Control queue không được nhận | Kiểm tra `RadarUiBridge_IsControlConfigChange()` |
| Buzzer kêu sai | Điều kiện `near_warning` sai | Log distance và trạng thái alert |
| UART không có dữ liệu | Sai baudrate hoặc TX | Kiểm tra PA9 và terminal |

### 11.2. Kinh nghiệm rút ra

- **Kiểm thử từng module riêng.** Không ghép toàn bộ hệ thống ngay từ đầu.
- **Kiểm tra nguồn trước khi sửa thuật toán.** Nhiều lỗi cảm biến thực chất do sụt áp.
- **Chờ servo ổn định trước khi đo.**
- **Không tin tuyệt đối thông số danh định.**
- **Dùng UART counter để xác định lỗi trigger/rising/falling/timeout.**
- **Tách dữ liệu lõi và dữ liệu điều khiển.**
- **Message queue phù hợp cho giao tiếp giữa các task.**
- **Snapshot giúp UI luôn có trạng thái gần nhất để hiển thị.**
- **Critical section phải ngắn.**
- **Bridge giúp UI không phụ thuộc trực tiếp vào driver phần cứng.**
- **Pin mapping phải thống nhất giữa README, `.ioc`, `main.h` và mạch thật.**
- **Chỉ mô tả các phần cứng đã thực sự lắp và kiểm thử.**

---

## 12. HƯỚNG PHÁT TRIỂN

### 12.1. Cải thiện độ ổn định đo

| Hướng phát triển | Lợi ích |
| --- | --- |
| Median filter | Loại bỏ số đo đột biến |
| Moving average | Làm mượt dữ liệu |
| Giới hạn độ nhảy | Tránh target dot nhảy mạnh |
| Tăng settle time | Giảm nhiễu do servo |
| Đo nhiều lần mỗi góc | Cải thiện độ tin cậy |
| Hiệu chỉnh nhiệt độ | Cải thiện công thức vận tốc âm thanh |

### 12.2. Cải thiện giao diện

- Cho phép chỉnh ngưỡng `DETECT` và `ALERT`.
- Hiển thị số lần timeout.
- Hiển thị echo micro giây.
- Thêm pause/resume.
- Lưu lịch sử phát hiện.
- Hiển thị biểu đồ khoảng cách.
- Hiển thị trạng thái queue.
- Thêm trang chẩn đoán cảm biến.

### 12.3. Cải thiện phần cứng

- Dùng level shifter chuyên dụng cho echo.
- Dùng nguồn servo riêng.
- Bổ sung tụ lọc.
- Thiết kế PCB/shield.
- Làm giá đỡ cảm biến chắc chắn hơn.
- Giảm khối lượng và độ lệch tâm trên trục servo.

### 12.4. Cải thiện phần mềm

- Dùng queue length phù hợp.
- Sử dụng cơ chế overwrite/mailbox cho dữ liệu mới nhất.
- Tách state machine thành module riêng.
- Bổ sung mutex hoặc critical section theo API FreeRTOS.
- Dùng `taskENTER_CRITICAL()` thay cho thao tác khóa ngắt trực tiếp nếu phù hợp.
- Bổ sung unit test cho hàm map góc và tọa độ.
- Phân loại mức log UART.
- Giảm log khi demo để tránh blocking.

---

## 13. TÀI LIỆU THAM KHẢO

| Nhóm tài liệu | Tài liệu | Vai trò |
| --- | --- | --- |
| STM32 MCU | STMicroelectronics, STM32F429xx Datasheet | Thông số MCU |
| STM32 peripheral | STMicroelectronics, RM0090 Reference Manual | GPIO, EXTI, TIM, USART, DMA, FMC |
| Board | STM32F429I-DISC1 User Manual | LCD, SDRAM, pinout và ST-LINK |
| TouchGFX | TouchGFX Documentation | Thiết kế và cập nhật giao diện |
| FreeRTOS | FreeRTOS/CMSIS-RTOS2 Documentation | Task và message queue |
| HC-SR04 | HC-SR04 technical documentation | Trigger, echo và công thức đo |
| MG90S | TowerPro MG90S specifications | PWM, tốc độ và torque |
| Repo mẫu | <https://github.com/neittien0110/ProjectSample> | Tham khảo cấu trúc báo cáo |
| Repo tham khảo | <https://github.com/nguyenha-meiii/RadarMonitor> | Tham khảo ý tưởng radar |

> [!NOTE]
> Trước khi nộp, nhóm nên bổ sung URL chính xác hoặc tài liệu PDF chính thức cho các datasheet đã sử dụng.

---

## 14. KẾT LUẬN

Đề tài đã xây dựng được một mô hình radar tầm ngắn trên STM32F429I-DISC1 sử dụng HC-SR04, servo MG90S, LED, buzzer, UART debug, FreeRTOS và TouchGFX.

Hệ thống có khả năng:

- Quét theo góc.
- Đo khoảng cách.
- Phát hiện vật cản.
- Cảnh báo vật ở gần.
- Thay đổi tốc độ.
- Thay đổi chế độ 90°/180°.
- Hiển thị dữ liệu trực quan trên LCD tích hợp.

Firmware được chia thành driver phần cứng, logic ứng dụng, bridge dữ liệu và lớp giao diện. Hai FreeRTOS message queue được sử dụng để truyền dữ liệu radar và cấu hình điều khiển giữa các task. Snapshot dữ liệu được bảo vệ bằng critical section ngắn để tránh đọc/ghi struct không nhất quán.

Điểm quan trọng khi đánh giá sản phẩm là nhìn nhận đúng giới hạn thực tế. HC-SR04 phụ thuộc vào góc và bề mặt phản xạ, servo có thể gây rung và nhiễu nguồn, còn STM32F429I-DISC1 phải đồng thời xử lý nhiều ngoại vi và giao diện đồ họa.

Trước khi nộp bản cuối, nhóm cần bổ sung:

- Ảnh đấu nối thực tế.
- Ảnh các màn hình TouchGFX.
- Video demo.
- Bảng đo sai số.
- Phân công đầy đủ cho các thành viên.
- Sơ đồ khối và schematic đúng với phần cứng thực tế.
