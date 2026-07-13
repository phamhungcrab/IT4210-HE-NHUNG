# Radar tầm ngắn - STM32F429I-DISC1 + TouchGFX

> Báo cáo bài tập lớn môn Hệ nhúng: xây dựng mô hình radar tầm ngắn sử dụng STM32F429I-DISC1, cảm biến siêu âm HC-SR04, servo MG90S và giao diện TouchGFX.

## Tóm tắt

Dự án triển khai một hệ thống quét vật cản tầm ngắn theo mô hình radar. Cảm biến HC-SR04 được gắn trên servo MG90S để thay đổi góc quét, STM32F429I-DISC1 đọc tín hiệu echo, tính khoảng cách, xử lý ngưỡng phát hiện và hiển thị trạng thái trên màn hình LCD TouchGFX tích hợp của board.

Hệ thống có các chức năng chính: quét theo góc, đo khoảng cách vật cản, hiển thị góc/khoảng cách/trạng thái, đổi chế độ quét, đổi tốc độ quét và cảnh báo bằng LED/buzzer khi vật ở vùng gần. Project chính nằm trong thư mục [`radar_short_range_touchgfx`](./radar_short_range_touchgfx).

> [!NOTE]
> Báo cáo ưu tiên thông tin đã kiểm chứng từ repo và file cấu hình. Các mục cần minh chứng thực nghiệm như ảnh, video, bảng đo và phần OLED SH1106 vẫn được giữ `TODO` để nhóm bổ sung sau, tránh mô tả vượt quá dữ liệu hiện có.

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
- [14. KẾT LUẬN NGẮN](#14-kết-luận-ngắn)

## 1. GIỚI THIỆU

### 1.1. Tên đề tài

**Radar tầm ngắn**

### 1.2. Mục tiêu

Mục tiêu của đề tài là thiết kế và triển khai một sản phẩm hệ thống nhúng có khả năng:

- Quét khu vực phía trước cảm biến theo một dải góc xác định.
- Đo khoảng cách vật cản bằng cảm biến siêu âm HC-SR04.
- Điều khiển servo MG90S để thay đổi hướng quét.
- Hiển thị trực quan trạng thái radar trên LCD TouchGFX của STM32F429I-DISC1.
- Cảnh báo bằng LED và buzzer khi phát hiện vật thể trong vùng gần.
- Cho phép thay đổi tốc độ quét và chế độ quét thông qua giao diện hoặc nút USER.

### 1.3. Ý tưởng hệ thống

Hệ thống không phải radar RF theo nghĩa vật lý, mà là một mô hình radar tầm ngắn dùng cảm biến siêu âm để mô phỏng quá trình quét và phát hiện vật cản. Servo quay cảm biến HC-SR04 qua từng góc; tại mỗi góc, STM32 phát xung trigger, đo độ rộng xung echo, tính khoảng cách và cập nhật dữ liệu cho giao diện TouchGFX.

Luồng xử lý tổng quát:

1. Servo được đặt tới góc quét hiện tại.
2. STM32 phát xung trigger 10 us tới HC-SR04.
3. Echo được bắt bằng ngắt ngoài EXTI3 trên chân PG3.
4. Phần mềm tính thời gian echo và suy ra khoảng cách.
5. Logic ứng dụng xác định trạng thái `SCAN`, `DETECT` hoặc `ALERT`.
6. LCD TouchGFX, LED và buzzer được cập nhật theo trạng thái mới.
7. Góc quét được tăng/giảm để tiếp tục chu kỳ quét.

### 1.4. Chức năng chính

| Nhóm chức năng      | Mô tả                                                                        |
| ------------------- | ---------------------------------------------------------------------------- |
| Đo khoảng cách      | Đo khoảng cách bằng HC-SR04, xử lý echo bằng EXTI và DWT cycle counter.      |
| Quét góc            | Điều khiển servo MG90S bằng PWM TIM3_CH1 trên PB4.                           |
| Hiển thị LCD        | Giao diện TouchGFX hiển thị góc, khoảng cách, trạng thái và vị trí mục tiêu. |
| Chế độ quét         | Hỗ trợ chế độ quét 90 độ và 180 độ theo cấu hình trong `radar_config.h`.     |
| Tốc độ quét         | Hỗ trợ các mức `SLOW`, `MED`, `FAST`.                                        |
| Cảnh báo            | LED/buzzer phản hồi theo trạng thái phát hiện vật cản và vật cản gần.        |
| Điều khiển nút USER | Bấm ngắn đổi tốc độ; giữ lâu đổi chế độ quét.                                |
| Debug UART          | In trạng thái đo, echo, counter và dữ liệu UI qua USART1.                    |

### 1.5. Cấu trúc project chính

```text
IT4210/
|-- README.md
|-- REPORT_PLAN.md
`-- radar_short_range_touchgfx/
    |-- Core/
    |-- Drivers/
    |-- Middlewares/
    |-- STM32CubeIDE/
    |-- TouchGFX/
    |-- DesignAssets/
    `-- STM32F429I_DISCO_REV_D01.ioc
```

## 2. TÁC GIẢ

### 2.1. Thành viên nhóm

| STT | MSSV     | Họ và tên         | Email                          |
| --: | -------- | ----------------- | ------------------------------ |
|   1 | 20235318 | Nguyễn Minh Giang | giang.nm235318@sis.hust.edu.vn |
|   2 | 20235333 | Bùi Trung Hoàng   | hoang.bt235333@sis.hust.edu.vn |
|   3 | 20235342 | Phạm Ngọc Hưng    | hung.pn235342@sis.hust.edu.vn  |
|   4 | 20235421 | Khương Anh Tài    | tai.ka235421@sis.hust.edu.vn   |

### 2.2. Phân công công việc

> [!IMPORTANT]
> Repo hiện chưa có dữ liệu đủ chắc để kết luận chi tiết phân công từng thành viên. Bảng dưới đây được tạo sẵn để nhóm điền trước khi nộp báo cáo.

| Thành viên        | Công việc chính                | Ghi chú |
| ----------------- | ------------------------------ | ------- |
| Nguyễn Minh Giang | TODO: Điền phần việc thực hiện | TODO    |
| Bùi Trung Hoàng   | TODO: Điền phần việc thực hiện | TODO    |
| Phạm Ngọc Hưng    | TODO: Điền phần việc thực hiện | TODO    |
| Khương Anh Tài    | TODO: Điền phần việc thực hiện | TODO    |

## 3. MÔI TRƯỜNG HOẠT ĐỘNG

### 3.1. Board STM32F429I-DISC1

Hệ thống sử dụng board **STM32F429I-DISC1** làm nền tảng điều khiển trung tâm. Vi điều khiển chính là **STM32F429ZIT6**, thuộc dòng STM32F4, phù hợp với bài toán vừa điều khiển ngoại vi thời gian thực vừa chạy giao diện đồ họa nhúng.

Trong project này, board đảm nhiệm các vai trò:

- Khởi tạo và điều phối toàn bộ ngoại vi: GPIO, EXTI, TIM, I2C, UART, LTDC, DMA2D và FMC/SDRAM.
- Chạy **FreeRTOS CMSIS v2** để tách logic giao diện, logic radar và xử lý nút USER.
- Chạy **TouchGFX** trên LCD tích hợp của board để hiển thị giao diện radar.
- Điều khiển servo MG90S bằng PWM, đọc cảm biến HC-SR04 bằng ngắt ngoài và phát tín hiệu cảnh báo qua LED/buzzer.

Board STM32F429I-DISC1 có LCD và SDRAM tích hợp, đây là lợi thế cho giao diện TouchGFX nhưng cũng làm số chân trống bị hạn chế. Vì vậy pin mapping trong báo cáo phải bám sát file cấu hình [`STM32F429I_DISCO_REV_D01.ioc`](./radar_short_range_touchgfx/STM32F429I_DISCO_REV_D01.ioc) và file [`Core/Inc/main.h`](./radar_short_range_touchgfx/Core/Inc/main.h).

### 3.2. Các module sử dụng

| Nhóm                 | Module                             | Vai trò                                                  |
| -------------------- | ---------------------------------- | -------------------------------------------------------- |
| Điều khiển trung tâm | STM32F429I-DISC1 / STM32F429ZIT6   | Chạy firmware, FreeRTOS, TouchGFX và điều phối ngoại vi  |
| Đo khoảng cách       | HC-SR04                            | Đo khoảng cách vật cản bằng sóng siêu âm                 |
| Cơ cấu quét          | Servo MG90S                        | Quay cảm biến theo góc quét                              |
| Hiển thị chính       | LCD tích hợp trên STM32F429I-DISC1 | Hiển thị giao diện radar bằng TouchGFX                   |
| Hiển thị phụ         | OLED SH1106 I2C                    | TODO: xác nhận mức độ hoàn thiện driver/hình ảnh thực tế |
| Cảnh báo             | Buzzer active                      | Phát cảnh báo âm thanh khi vật thể ở vùng gần            |
| Chỉ thị trạng thái   | LED on-board PG13/PG14             | Báo trạng thái quét và cảnh báo                          |
| Điều khiển nhanh     | Nút USER PA0/WKUP                  | Bấm ngắn đổi tốc độ, giữ lâu đổi chế độ quét             |
| Debug                | USART1 PA9/PA10                    | In log đo khoảng cách, echo, counter và dữ liệu UI       |

### 3.3. Bill of Materials (BOM)

| STT | Linh kiện                                | Số lượng | Vai trò                             | Ghi chú kỹ thuật                                                                                 |
| --: | ---------------------------------------- | -------: | ----------------------------------- | ------------------------------------------------------------------------------------------------ |
|   1 | STM32F429I-DISC1 / STM32F429ZIT6         |        1 | Board điều khiển trung tâm          | Có LCD, SDRAM, ST-LINK; project cấu hình TouchGFX, LTDC, DMA2D, FMC/SDRAM                        |
|   2 | HC-SR04                                  |        1 | Cảm biến đo khoảng cách             | Trigger 10 us; echo đo bằng EXTI3 trên PG3; sóng siêu âm 40 kHz; cần lưu ý echo có thể ở mức 5 V |
|   3 | Servo MG90S                              |        1 | Quay cảm biến HC-SR04 theo góc quét | Điều khiển bằng PWM TIM3_CH1 trên PB4; cần nguồn đủ dòng, GND chung với STM32                    |
|   4 | OLED SH1106 I2C                          |        1 | Màn hình phụ theo yêu cầu phần cứng | I2C3 SCL/SDA trên PA8/PC9; TODO: xác nhận driver OLED trong firmware                             |
|   5 | Buzzer active                            |        1 | Cảnh báo âm thanh                   | Điều khiển GPIO PC4; code hiện bật/tắt theo trạng thái cảnh báo gần                              |
|   6 | LED on-board                             |        2 | Chỉ thị scan và alert               | PG13 là `LED3_SCAN`, PG14 là `LED4_ALERT`                                                        |
|   7 | LCD tích hợp trên board                  |        1 | Giao diện người dùng chính          | TouchGFX 4.26.1, LTDC 240 x 320, RGB565                                                          |
|   8 | Nút USER trên board                      |        1 | Điều khiển nhanh                    | PA0/WKUP; bấm ngắn đổi tốc độ, giữ lâu đổi mode 90/180 độ                                        |
|   9 | Dây jumper / breadboard / dây nguồn      |     TODO | Đấu nối phần cứng                   | TODO: cập nhật theo ảnh đấu nối thực tế                                                          |
|  10 | Mạch chia áp hoặc level shifter cho Echo |     TODO | Bảo vệ GPIO 3.3 V                   | Khuyến nghị khi module HC-SR04 xuất echo 5 V                                                     |

### 3.4. Phần mềm và công cụ

| Hạng mục             | Thông tin đã xác định                                     |
| -------------------- | --------------------------------------------------------- |
| Project chính        | `radar_short_range_touchgfx`                              |
| File cấu hình CubeMX | `radar_short_range_touchgfx/STM32F429I_DISCO_REV_D01.ioc` |
| MCU                  | STM32F429ZITx                                             |
| STM32CubeMX          | 6.17.0                                                    |
| STM32Cube FW_F4      | V1.28.3                                                   |
| TouchGFX             | X-CUBE-TOUCHGFX 4.26.1                                    |
| RTOS                 | FreeRTOS CMSIS v2                                         |
| Toolchain mục tiêu   | STM32CubeIDE                                              |
| Giao diện đồ họa     | TouchGFX, LTDC, DMA2D, SDRAM                              |

### 3.5. Cấu hình ngoại vi chính

| Ngoại vi  | Cấu hình / vai trò                                                        |
| --------- | ------------------------------------------------------------------------- |
| TIM3 CH1  | PWM servo trên PB4, Prescaler = 89, Period = 19999, Pulse = 1500          |
| EXTI3     | Bắt tín hiệu echo của HC-SR04 trên PG3, rising/falling edge               |
| TIM2      | Có cấu hình input capture trong `.ioc`; code HC-SR04 hiện dùng EXTI + DWT |
| I2C3      | SCL PA8, SDA PC9, tốc độ 100 kHz                                          |
| USART1    | TX PA9, RX PA10, dùng cho debug UART                                      |
| LTDC      | Điều khiển LCD tích hợp, 240 x 320, RGB565                                |
| DMA2D     | Tăng tốc đồ họa cho TouchGFX                                              |
| FMC/SDRAM | Bộ nhớ ngoài phục vụ framebuffer/TouchGFX                                 |
| FreeRTOS  | Tách task GUI, task radar và task xử lý nút USER                          |

## 4. SƠ ĐỒ SCHEMATIC / KẾT NỐI PHẦN CỨNG

### 4.1. Sơ đồ khối hệ thống

<p align="center">
  <img src="docs/images/a.png" width="700" alt="Sơ đồ khối hệ thống">
  <br>
  <em>Hình 1. Sơ đồ khối hệ thống.</em>
</p>

Sơ đồ khối cần thể hiện STM32F429I-DISC1 là trung tâm. HC-SR04 gửi tín hiệu echo về STM32, servo nhận PWM để thay đổi góc quét, LCD TouchGFX hiển thị dữ liệu, LED/buzzer phản hồi trạng thái cảnh báo, OLED SH1106 kết nối qua I2C3 nếu phần hiển thị phụ được hoàn thiện.

### 4.2. Sơ đồ schematic

<p align="center">
  <img src="docs/images/b.png" width="700" alt="Sơ đồ schematic">
  <br>
  <em>Hình 2. Sơ đồ schematic.</em>
</p>

Sơ đồ schematic cần thể hiện rõ nguồn cấp, GND chung, đường tín hiệu trigger/echo của HC-SR04, đường PWM servo, đường I2C OLED, LED/buzzer và UART debug. Nếu dùng mạch chia áp hoặc level shifter cho chân echo, cần vẽ trực tiếp vào schematic.

### 4.3. Ảnh đấu nối thực tế

`[Chèn ảnh: ảnh đấu nối thực tế]`

Ảnh đấu nối thực tế nên chụp đủ board STM32F429I-DISC1, cảm biến HC-SR04 gắn trên servo, dây nguồn servo, dây trigger/echo, buzzer và OLED nếu có. Nên đánh dấu màu dây hoặc chú thích để người đọc đối chiếu được với bảng pin mapping bên dưới.

### 4.4. Bảng pin mapping

| Chân STM32 | Tên tín hiệu trong project | Ngoại vi / module  | Chiều tín hiệu        | Giải thích sử dụng                                                                                                                                    |
| ---------- | -------------------------- | ------------------ | --------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------- |
| PB4        | `SERVO_PWM` / `TIM3_CH1`   | Servo MG90S        | STM32 -> Servo        | Xuất PWM 50 Hz để đặt góc servo. Cấu hình TIM3 dùng Prescaler = 89, Period = 19999, Pulse ban đầu = 1500, tương ứng tick khoảng 1 us và chu kỳ 20 ms. |
| PG2        | `HCSR04_TRIG`              | HC-SR04            | STM32 -> Sensor       | Chân trigger. Firmware kéo mức cao trong 10 us để yêu cầu HC-SR04 phát burst siêu âm.                                                                 |
| PG3        | `HCSR04_ECHO` / `EXTI3`    | HC-SR04            | Sensor -> STM32       | Chân echo. Được cấu hình ngắt ngoài EXTI3 ở cả cạnh lên và cạnh xuống để đo độ rộng xung echo.                                                        |
| PC4        | `BUZZER`                   | Buzzer active      | STM32 -> Buzzer       | GPIO output điều khiển buzzer. Code chỉ bật cảnh báo âm thanh khi trạng thái `near_warning` được kích hoạt.                                           |
| PG13       | `LED3_SCAN`                | LED on-board       | STM32 -> LED          | LED báo hệ thống đang quét. Trong `radar_app.c`, LED này được toggle theo chu kỳ xử lý radar.                                                         |
| PG14       | `LED4_ALERT`               | LED on-board       | STM32 -> LED          | LED cảnh báo khi phát hiện vật thể hoặc vật thể ở vùng gần.                                                                                           |
| PA0/WKUP   | `B1_USER`                  | Nút USER           | Button -> STM32       | Input đọc nút USER. Bấm ngắn đổi tốc độ quét; giữ từ khoảng 800 ms đổi mode quét 90/180 độ.                                                           |
| PA8        | `I2C3_SCL`                 | OLED SH1106 / I2C3 | STM32 -> I2C bus      | Clock I2C3, cấu hình pull-up trong `.ioc`. Dự kiến dùng cho OLED SH1106 hoặc thiết bị I2C liên quan.                                                  |
| PC9        | `I2C3_SDA`                 | OLED SH1106 / I2C3 | Hai chiều             | Data I2C3, cấu hình pull-up trong `.ioc`. TODO: xác nhận driver OLED đã được tích hợp đầy đủ hay chưa.                                                |
| PA9        | `USART1_TX`                | UART debug         | STM32 -> UART adapter | Truyền log debug qua USART1, ví dụ trạng thái echo, khoảng cách, counter và dữ liệu UI.                                                               |
| PA10       | `USART1_RX`                | UART debug         | UART adapter -> STM32 | Nhận UART USART1. Project hiện chủ yếu dùng TX để in debug; RX giữ cho giao tiếp UART đầy đủ.                                                         |

### 4.5. Nguyên tắc nối dây

Khi lắp mạch, cần xem STM32 là mốc logic 3.3 V và các module ngoại vi là tải/nguồn tín hiệu bên ngoài. Các đường tín hiệu điều khiển từ STM32 như `TRIG`, `SERVO_PWM`, `BUZZER`, `LED3_SCAN`, `LED4_ALERT` phải đi đúng chân đã cấu hình trong `.ioc`; nếu đổi chân ngoài phần cứng mà không cập nhật CubeMX và code thì firmware sẽ không hoạt động đúng.

Nguồn cấp cần được tách theo tải. Board STM32 có thể cấp nguồn cho logic nhẹ, nhưng servo MG90S là tải cơ điện có dòng đỉnh cao hơn nhiều so với GPIO hoặc cảm biến. Nếu servo lấy nguồn không đủ dòng, điện áp có thể sụt khi servo khởi động hoặc đổi chiều, kéo theo LCD trắng màn, board reset, servo rung hoặc số đo HC-SR04 nhiễu. Cách lắp an toàn hơn là cấp servo bằng nguồn 5 V đủ dòng, nhưng **bắt buộc nối chung GND** giữa nguồn servo và GND của STM32.

Với HC-SR04, chân `TRIG` nhận mức điều khiển từ STM32 thường không phải vấn đề lớn, nhưng chân `ECHO` của nhiều module HC-SR04 xuất mức 5 V. GPIO của STM32F429 hoạt động logic 3.3 V, vì vậy cần kiểm tra module cụ thể và nên dùng mạch chia áp hoặc level shifter trước khi đưa echo vào PG3/EXTI3.

Đường I2C3 cho OLED SH1106 cần có pull-up phù hợp về 3.3 V. File `.ioc` đã cấu hình pull-up cho PA8/PC9, nhưng khi dùng module OLED thực tế vẫn cần kiểm tra module có pull-up sẵn hay không và bảo đảm bus không bị kéo lên 5 V.

<!-- ### 4.6. Lưu ý khi lắp mạch thực tế


| Hiện tượng                                    | Nguyên nhân thường gặp                                                     | Cách tránh / kiểm tra                                                                                            |
| --------------------------------------------- | -------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------- |
| LCD trắng màn hoặc board reset khi servo quay | Servo kéo dòng lớn làm sụt nguồn                                           | Dùng nguồn 5 V đủ dòng cho servo, nối GND chung, tránh lấy toàn bộ dòng servo qua chân cấp yếu                   |
| Servo rung, giật hoặc không tới góc           | Nguồn servo yếu, dây nguồn dài, pulse biên chưa phù hợp                    | Kiểm tra nguồn bằng đồng hồ, rút ngắn dây nguồn, hiệu chỉnh `SERVO_MIN_PULSE_US` và `SERVO_MAX_PULSE_US` nếu cần |
| Khoảng cách HC-SR04 nhảy bất thường           | Đo khi servo còn rung, vật phản xạ lệch góc, nguồn nhiễu                   | Chờ servo ổn định trước khi đo, test với mặt phẳng lớn, cố định cảm biến chắc chắn                               |
| Không bắt được echo                           | Sai chân PG3, chưa có GND chung, echo vượt/không đạt mức logic             | Đối chiếu pin mapping, đo tín hiệu bằng oscilloscope/logic analyzer nếu có, dùng chia áp cho echo 5 V            |
| OLED không hiển thị                           | Sai địa chỉ I2C, thiếu pull-up, bus bị kéo lên 5 V, driver chưa hoàn thiện | Scan I2C, kiểm tra PA8/PC9, xác nhận driver SH1106 trong firmware                                                |
| UART không có log                             | Sai TX/RX, sai baudrate, chưa nối GND chung với USB-UART                   | Đối chiếu PA9/PA10, kiểm tra cấu hình USART1 và terminal                                                         |
| LED/buzzer không phản hồi                     | Sai chân hoặc trạng thái logic ngược                                       | Đối chiếu `PC4`, `PG13`, `PG14`; test GPIO độc lập trước khi tích hợp radar                                      |

Về quy trình, nên kiểm thử từng khối trước khi chạy toàn hệ thống: test PWM servo, test trigger/echo HC-SR04, test LED/buzzer, test UART debug, sau đó mới ghép với TouchGFX và FreeRTOS. Cách làm này giúp cô lập lỗi phần cứng, tránh nhầm lỗi nguồn hoặc đấu dây thành lỗi phần mềm. -->

## 5. NGUYÊN LÝ HOẠT ĐỘNG

`[Chèn ảnh: lưu đồ hoạt động hệ thống]`

### 5.1. Luồng hoạt động tổng quát

Về mặt chức năng, hệ thống hoạt động theo một vòng lặp quét lặp lại liên tục. Servo đưa cảm biến HC-SR04 tới một góc xác định, cảm biến đo khoảng cách tại hướng đó, phần mềm xử lý kết quả đo rồi cập nhật giao diện và cảnh báo. Sau đó góc quét được tăng hoặc giảm để chuyển sang hướng tiếp theo.

Trong mã nguồn, vòng lặp này nằm chủ yếu trong hàm `RadarApp_TaskLoop()` của module [`radar_app.c`](./radar_short_range_touchgfx/STM32CubeIDE/Application/User/radar_app.c). Trình tự xử lý chính:

1. Đọc trạng thái điều khiển từ `RadarUiBridge`: radar đang bật/tắt, tốc độ quét và chế độ quét.
2. Nếu radar đang tắt, đưa servo về giữa, tắt buzzer/LED cảnh báo và cập nhật dữ liệu UI ở trạng thái dừng.
3. Nếu radar đang bật, đặt servo tới góc hiện tại bằng `Servo_SetAngle()`.
4. Chờ một khoảng ngắn theo tốc độ quét để servo kịp ổn định.
5. Gọi `RadarApp_MeasureDistance()` để phát trigger HC-SR04 và chờ kết quả echo.
6. Phân loại kết quả thành `SCAN`, `DETECT` hoặc `ALERT`.
7. Cập nhật LED scan, LED alert, buzzer và struct `RadarUiData_t`.
8. Ghi dữ liệu mới vào `RadarUiBridge_SetData()` để TouchGFX đọc và hiển thị.
9. Tăng hoặc giảm góc quét bằng `RadarApp_AdvanceAngle()`.

### 5.2. Nguyên lý đo khoảng cách bằng HC-SR04

HC-SR04 đo khoảng cách bằng sóng siêu âm. Khi chân `TRIG` nhận một xung mức cao tối thiểu khoảng 10 us, module phát một chùm sóng siêu âm tần số khoảng 40 kHz. Sóng truyền trong không khí, gặp vật cản thì phản xạ trở lại đầu thu. Module giữ chân `ECHO` ở mức cao trong khoảng thời gian tương ứng với thời gian sóng đi từ cảm biến tới vật rồi quay lại.

Trong project này:

- `PG2 = HCSR04_TRIG`: STM32 phát xung trigger.
- `PG3 = HCSR04_ECHO`: STM32 đo xung echo qua ngắt ngoài `EXTI3`.
- `HCSR04_StartMeasure()` phát trigger 10 us.
- `HCSR04_GPIO_EXTI_Callback()` bắt cạnh lên và cạnh xuống của echo.
- DWT cycle counter được dùng để đo thời gian echo với độ phân giải micro giây.

Code hiện tại không polling echo bằng vòng lặp dài. Thay vào đó, cạnh lên của echo lưu thời điểm bắt đầu, cạnh xuống lưu thời điểm kết thúc, sau đó tính:

```c
g_echo_us = (g_falling_cycle - g_rising_cycle) / g_cycles_per_us;
```

### 5.3. Công thức tính khoảng cách

Về nguyên lý vật lý:

```text
quãng đường sóng đi được = vận tốc âm thanh x thời gian echo
```

Tuy nhiên, thời gian echo là thời gian cho cả hành trình **đi từ cảm biến tới vật cản** và **quay từ vật cản về cảm biến**. Khoảng cách cần tìm chỉ là một chiều, nên phải chia đôi:

```text
khoảng cách = (vận tốc âm thanh x thời gian echo) / 2
```

Nếu lấy vận tốc âm thanh trong không khí xấp xỉ 343 m/s ở khoảng 20 độ C:

```text
343 m/s = 0.0343 cm/us
khoảng cách_cm = echo_us x 0.0343 / 2
              ~= echo_us / 58
```

Trong mã nguồn, `HCSR04_GetDistanceCm()` dùng đúng dạng gần đúng này:

```c
distance = g_echo_us / 58U;
```

Giá trị đo còn được kiểm tra hợp lệ theo ngưỡng cấu hình. Với project này, `RADAR_MIN_DISTANCE_CM = 2`, `RADAR_OBJECT_DETECT_CM = 50`, `RADAR_NEAR_WARNING_CM = 5`, và timeout HC-SR04 là `HCSR04_TIMEOUT_MS = 25`.

### 5.4. Ảnh hưởng của môi trường tới kết quả đo

Công thức `echo_us / 58` là xấp xỉ thuận tiện, nhưng vận tốc âm thanh không cố định tuyệt đối. Nhiệt độ, độ ẩm, luồng gió và điều kiện môi trường đều có thể làm vận tốc âm thanh thay đổi. Vì vậy cùng một vật ở cùng vị trí có thể cho số đo hơi khác nhau giữa các lần đo.

Ngoài môi trường, HC-SR04 còn bị ảnh hưởng bởi:

- Bề mặt vật cản: mặt phẳng cứng phản xạ tốt hơn vải, xốp hoặc bề mặt hấp thụ âm.
- Góc đặt vật: nếu sóng phản xạ lệch khỏi đầu thu, echo yếu hoặc mất.
- Rung cơ khí: servo đang rung có thể làm hướng phát siêu âm thay đổi trong lúc đo.
- Nguồn cấp: nguồn yếu hoặc nhiễu có thể làm echo sai, servo rung và màn hình hoạt động không ổn định.

Vì các lý do này, hệ thống nên được hiểu là mô hình radar tầm ngắn phục vụ phát hiện và minh họa, không phải thiết bị đo khoảng cách chính xác cao trong mọi điều kiện.

### 5.5. Nguyên lý điều khiển servo bằng PWM

Servo MG90S được điều khiển bằng xung PWM chu kỳ khoảng 20 ms, tương đương tần số 50 Hz. Độ rộng xung mức cao quyết định góc servo. Trong project, driver [`servo_mg90s.c`](./radar_short_range_touchgfx/STM32CubeIDE/Application/User/servo_mg90s.c) dùng:

- `Servo_Init()`: start PWM TIM3 CH1 và đưa servo về góc giữa.
- `Servo_SetPulseUs()`: đặt độ rộng xung PWM theo micro giây.
- `Servo_SetAngle()`: chuyển góc 0-180 độ thành pulse.
- `Servo_Stop()`: đưa servo về góc center 90 độ.

Các ngưỡng trong [`radar_config.h`](./radar_short_range_touchgfx/STM32CubeIDE/Application/User/radar_config.h):

| Tham số                  | Giá trị | Ý nghĩa               |
| ------------------------ | ------: | --------------------- |
| `SERVO_MIN_ANGLE_DEG`    |       0 | Góc nhỏ nhất          |
| `SERVO_CENTER_ANGLE_DEG` |      90 | Góc giữa              |
| `SERVO_MAX_ANGLE_DEG`    |     180 | Góc lớn nhất          |
| `SERVO_MIN_PULSE_US`     |  550 us | Pulse ứng với góc nhỏ |
| `SERVO_CENTER_PULSE_US`  | 1500 us | Pulse trung tâm       |
| `SERVO_MAX_PULSE_US`     | 2450 us | Pulse ứng với góc lớn |

Hàm `Servo_SetAngle()` nội suy tuyến tính từ góc sang độ rộng xung:

```text
pulse_us = SERVO_MIN_PULSE_US
         + angle_deg x (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US) / 180
```

### 5.6. Ý nghĩa cấu hình TIM3 cho servo

TIM3 CH1 được gán ra chân `PB4 = SERVO_PWM`. Cấu hình trong project:

| Cấu hình TIM3 | Giá trị | Ý nghĩa                                                            |
| ------------- | ------: | ------------------------------------------------------------------ |
| Prescaler     |      89 | Chia clock timer để mỗi tick xấp xỉ 1 us khi timer clock là 90 MHz |
| Period        |   19999 | Bộ đếm chạy 20000 tick, tương đương chu kỳ 20 ms                   |
| Pulse ban đầu |    1500 | Xung cao 1500 us, thường tương ứng vị trí giữa của servo           |

Với tick 1 us và chu kỳ 20000 us:

```text
T_PWM = 20 ms
f_PWM = 1 / 20 ms = 50 Hz
```

Đây là tần số điều khiển phổ biến cho servo hobby. Việc dùng tick 1 us cũng giúp phần mềm đặt pulse trực tiếp theo đơn vị micro giây, ví dụ `550`, `1500`, `2450`, thay vì phải quy đổi phức tạp sang giá trị compare.

### 5.7. Hiển thị lên TouchGFX và OLED

Giao diện chính của hệ thống dùng TouchGFX trên LCD tích hợp của STM32F429I-DISC1. Luồng dữ liệu không được truyền trực tiếp từ driver cảm biến sang UI, mà đi qua lớp trung gian [`radar_ui_bridge.c`](./radar_short_range_touchgfx/STM32CubeIDE/Application/User/radar_ui_bridge.c).

Struct trung tâm là `RadarUiData_t`, chứa các thông tin như:

- `angle_deg`: góc quét hiện tại.
- `distance_cm`: khoảng cách đo được.
- `distance_valid`: kết quả đo có hợp lệ hay không.
- `object_detected`: có vật trong vùng phát hiện hay không.
- `near_warning`: vật có nằm trong vùng cảnh báo gần hay không.
- `radar_status`: trạng thái `SCAN`, `DETECT`, `ALERT`.
- `speed_mode`, `scan_mode_deg`: cấu hình tốc độ và góc quét.
- `object_count`, `last_object_distance_cm`, `last_object_angle_deg`: thống kê phát hiện.

Trong màn hình [`ScreenScanView.cpp`](./radar_short_range_touchgfx/TouchGFX/gui/src/screenscan_screen/ScreenScanView.cpp), TouchGFX đọc dữ liệu bằng `RadarUiBridge_GetData()` rồi:

- Cập nhật text góc quét.
- Cập nhật text khoảng cách hoặc hiển thị `--- cm` nếu đo không hợp lệ.
- Hiển thị trạng thái `SCAN`, `DETECT`, `ALERT`.
- Đổi sweep xanh/đỏ theo trạng thái phát hiện.
- Tính vị trí target dot từ góc và khoảng cách.

`[Chèn ảnh: giao diện màn hình radar]`

Với OLED SH1106, repo hiện có cấu hình I2C3 trên `PA8/PC9` và struct `RadarUiData_t` có trường `oled_connected`. Tuy nhiên trong mã nguồn user hiện chưa thấy driver SH1106 riêng; `radar_app.c` đang gán `oled_connected = 0`. Vì vậy phần README chỉ mô tả OLED ở mức phần cứng/kế hoạch tích hợp, chưa khẳng định nội dung hiển thị OLED đã hoàn thiện.

### 5.8. Vai trò buzzer và LED cảnh báo

Module [`buzzer_led.c`](./radar_short_range_touchgfx/STM32CubeIDE/Application/User/buzzer_led.c) điều khiển:

- `PC4 = BUZZER`
- `PG13 = LED3_SCAN`
- `PG14 = LED4_ALERT`

Trong `RadarApp_TaskLoop()`, `LED3_SCAN` được toggle để báo hệ thống đang quét. Hàm `Alert_Update(detected, near_warning)` xử lý cảnh báo:

- Không phát hiện vật: tắt LED alert và buzzer.
- Có vật trong vùng phát hiện: bật LED alert, chưa bật buzzer.
- Vật ở vùng rất gần (`near_warning`): bật LED alert và toggle buzzer theo chu kỳ khoảng 120 ms.

Cách phân cấp này giúp người dùng phân biệt giữa phát hiện vật bình thường và tình huống vật ở quá gần cần cảnh báo mạnh hơn.

## 6. TÍCH HỢP HỆ THỐNG

### 6.1. Kiến trúc phần mềm tổng quát

Project được chia thành nhiều lớp để tách phần cứng, logic xử lý và giao diện. Cách tổ chức này giúp việc debug từng khối dễ hơn, đồng thời giảm phụ thuộc trực tiếp giữa TouchGFX C++ và driver C.

| Lớp               | File / module chính                                                                        | Vai trò                                                                            |
| ----------------- | ------------------------------------------------------------------------------------------ | ---------------------------------------------------------------------------------- |
| Cấu hình nền tảng | `Core/Src/main.c`, `.ioc`                                                                  | Khởi tạo clock, GPIO, TIM, I2C, UART, LTDC, DMA2D, FMC/SDRAM, FreeRTOS             |
| Driver phần cứng  | `hcsr04.c`, `servo_mg90s.c`, `buzzer_led.c`, `radar_debug.c`                               | Làm việc trực tiếp với GPIO, EXTI, PWM, UART                                       |
| Cấu hình ứng dụng | `radar_config.h`, `radar_types.h`                                                          | Chứa ngưỡng khoảng cách, tham số servo, scan mode, speed mode và struct dữ liệu UI |
| Logic ứng dụng    | `radar_app.c`                                                                              | Điều phối quét, đo khoảng cách, phát hiện vật, cảnh báo và cập nhật dữ liệu        |
| Cầu nối UI        | `radar_ui_bridge.c`                                                                        | Chia sẻ dữ liệu giữa radar task C và TouchGFX C++ bằng critical section ngắn       |
| UI TouchGFX       | `ScreenHomeView.cpp`, `ScreenScanView.cpp`, `ScreenSettingsView.cpp`, `ScreenInfoView.cpp` | Hiển thị và điều khiển radar từ giao diện                                          |
| Middleware        | FreeRTOS CMSIS v2, TouchGFX, HAL                                                           | Lập lịch task, giao diện đồ họa, abstraction ngoại vi STM32                        |

### 6.2. Luồng dữ liệu cảm biến - xử lý - hiển thị - cảnh báo

Luồng dữ liệu chính:

```text
HC-SR04
  -> EXTI3/DWT trong hcsr04.c
  -> RadarApp_MeasureDistance()
  -> RadarApp_TaskLoop()
  -> RadarUiData_t
  -> RadarUiBridge_SetData()
  -> TouchGFX ScreenScan/Info/Settings
  -> LCD radar UI

RadarApp_TaskLoop()
  -> Alert_Update()
  -> LED3_SCAN / LED4_ALERT / BUZZER
```

Diễn giải theo từng bước:

1. `RadarApp_TaskLoop()` đặt servo tới góc hiện tại bằng `Servo_SetAngle()`.
2. Sau thời gian chờ ngắn, `RadarApp_MeasureDistance()` gọi `HCSR04_StartMeasure()`.
3. HC-SR04 phản hồi bằng xung echo trên PG3.
4. `HAL_GPIO_EXTI_Callback()` trong `main.c` chuyển xử lý sang `HCSR04_GPIO_EXTI_Callback()`.
5. `hcsr04.c` tính `echo_us`, sau đó `HCSR04_GetDistanceCm()` trả khoảng cách cm nếu hợp lệ.
6. `radar_app.c` so sánh khoảng cách với các ngưỡng `RADAR_OBJECT_DETECT_CM` và `RADAR_NEAR_WARNING_CM`.
7. Kết quả được đóng gói vào `RadarUiData_t`.
8. TouchGFX đọc dữ liệu qua `RadarUiBridge_GetData()` để cập nhật text, sweep và target dot.
9. `Alert_Update()` cập nhật LED/buzzer theo trạng thái phát hiện.

### 6.3. Task, loop và trạng thái trong chương trình

Trong `main.c`, project tạo ba luồng xử lý chính:

| Task / loop   | Nguồn                | Vai trò                                                                     |
| ------------- | -------------------- | --------------------------------------------------------------------------- |
| `defaultTask` | `StartDefaultTask()` | Đọc nút USER PA0. Bấm ngắn đổi speed mode, giữ lâu đổi scan mode 90/180 độ. |
| `GUI_Task`    | `TouchGFX_Task()`    | Chạy TouchGFX, render giao diện và xử lý màn hình.                          |
| `radarTask`   | `StartRadarTask()`   | Gọi `RadarApp_Init()` rồi lặp vô hạn `RadarApp_TaskLoop()`.                 |

`RadarApp_TaskLoop()` có thể xem như state machine mức ứng dụng, dù không được viết dưới dạng bảng state riêng. Các trạng thái chính nằm trong `RadarUiData_t`:

| Trạng thái            | Ý nghĩa                              | Điều kiện chính                                               |
| --------------------- | ------------------------------------ | ------------------------------------------------------------- |
| `RADAR_STATUS_SCAN`   | Đang quét, chưa phát hiện vật hợp lệ | Không có vật trong ngưỡng phát hiện                           |
| `RADAR_STATUS_DETECT` | Phát hiện vật trong vùng quan sát    | Khoảng cách hợp lệ và `distance_cm <= RADAR_OBJECT_DETECT_CM` |
| `RADAR_STATUS_ALERT`  | Vật nằm quá gần                      | Khoảng cách hợp lệ và `distance_cm <= RADAR_NEAR_WARNING_CM`  |

Ngoài trạng thái radar, phần mềm còn quản lý:

- `radar_enabled`: radar bật/tắt.
- `scan_mode_deg`: quét 90 độ hoặc 180 độ.
- `speed_mode`: `SLOW`, `MED`, `FAST`.
- `g_angle`, `g_direction`: góc quét hiện tại và hướng quét.
- `g_prev_detected`: dùng để tăng `object_count` khi có vật mới xuất hiện.

### 6.4. Vai trò của timer và ngắt

| Thành phần   | Vai trò trong project                                                                                                                                                                                                           |
| ------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `TIM3`       | Timer PWM điều khiển servo MG90S trên kênh CH1/PB4. Đây là timer quan trọng cho cơ cấu quét.                                                                                                                                    |
| `EXTI3`      | Ngắt ngoài trên PG3 để bắt cạnh lên/xuống của echo HC-SR04. Đây là cơ chế đo echo hiện đang được code sử dụng.                                                                                                                  |
| DWT `CYCCNT` | Bộ đếm chu kỳ CPU dùng trong `hcsr04.c` để tạo delay micro giây cho trigger và đo độ rộng xung echo.                                                                                                                            |
| `TIM2`       | Được cấu hình input capture trong `.ioc` và `main.c`, Prescaler = 89, Period = 0xFFFFFFFF. Tuy nhiên `hcsr04.c` hiện ghi rõ không dùng TIM2 input capture nữa; hàm `HCSR04_TIM_IC_CaptureCallback()` chỉ giữ stub `(void)htim`. |
| `TIM6`       | Được dùng làm time base HAL trong project CubeMX, phục vụ tick hệ thống.                                                                                                                                                        |

Điểm cần lưu ý là project có dấu vết cấu hình TIM2 input capture, nhưng logic đo HC-SR04 hiện tại đã chuyển sang EXTI3 + DWT. Vì vậy khi mô tả hoạt động thực tế của firmware, cần ưu tiên mô tả EXTI3/DWT; TIM2 chỉ nên được nhắc là ngoại vi còn cấu hình, chưa phải đường đo chính trong code hiện tại.

### 6.5. Tích hợp TouchGFX với logic C

TouchGFX chạy ở phía C++, trong khi phần lớn logic radar và driver viết bằng C. Repo giải quyết việc trao đổi dữ liệu bằng `radar_ui_bridge.c/h`. Bridge này giữ một biến static kiểu `RadarUiData_t` và dùng critical section ngắn bằng `__disable_irq()` / `__enable_irq()` khi copy dữ liệu.

Cách làm này giúp:

- Radar task cập nhật dữ liệu đo mà không cần biết chi tiết widget TouchGFX.
- TouchGFX đọc dữ liệu hiển thị mà không gọi trực tiếp driver cảm biến.
- Các biến điều khiển như `radar_enabled`, `speed_mode`, `scan_mode_deg` được giữ lại khi `RadarApp_TaskLoop()` cập nhật sensor data, tránh lỗi UI vừa set trạng thái mới nhưng task ghi đè bằng dữ liệu cũ.

Ở màn `ScreenScanView`, `handleTickEvent()` không cập nhật UI ở mọi frame mà cứ 3 tick mới gọi `updateRadarUi()`. Đây là cách giảm tải cập nhật text/texture trong khi vẫn giữ giao diện đủ mượt.

`[Chèn ảnh: sơ đồ luồng dữ liệu phần mềm]`

## 7. KIẾN TRÚC PHẦN MỀM

Các module phần mềm được tách theo trách nhiệm rõ ràng: phần khởi tạo hệ thống nằm trong `Core`, các driver và logic radar nằm trong `STM32CubeIDE/Application/User`, còn giao diện người dùng nằm trong `TouchGFX/gui`. Bảng dưới đây tóm tắt các file nên đọc khi cần hiểu hoặc mở rộng project.

| File / module            | Vai trò                                                            |
| ------------------------ | ------------------------------------------------------------------ |
| `Core/Src/main.c`        | Khởi tạo HAL, ngoại vi, FreeRTOS task và callback EXTI.            |
| `radar_app.c/h`          | Điều phối logic quét, đo, phát hiện, cảnh báo và cập nhật UI data. |
| `hcsr04.c/h`             | Driver đo khoảng cách bằng HC-SR04 qua EXTI + DWT.                 |
| `servo_mg90s.c/h`        | Driver điều khiển servo bằng PWM TIM3_CH1.                         |
| `buzzer_led.c/h`         | Điều khiển buzzer và LED trạng thái.                               |
| `radar_ui_bridge.c/h`    | Vùng dữ liệu chia sẻ giữa radar task C và TouchGFX C++.            |
| `radar_debug.c/h`        | In log debug qua USART1.                                           |
| `ScreenScanView.cpp`     | Màn hình radar chính, hiển thị sweep, target, góc, khoảng cách.    |
| `ScreenSettingsView.cpp` | Chọn tốc độ quét và chế độ quét.                                   |
| `ScreenInfoView.cpp`     | Hiển thị thông tin thống kê.                                       |
| `ScreenHomeView.cpp`     | Màn hình Home, dừng radar khi quay về Home.                        |

## 8. ĐẶC TẢ HÀM / MODULE QUAN TRỌNG

Phần này đặc tả các module và hàm quan trọng theo mã nguồn thực tế trong thư mục [`STM32CubeIDE/Application/User`](./radar_short_range_touchgfx/STM32CubeIDE/Application/User) và phần UI trong [`TouchGFX/gui/src`](./radar_short_range_touchgfx/TouchGFX/gui/src). Các hàm được chọn theo vai trò trong luồng đo khoảng cách, điều khiển servo, cảnh báo và cập nhật giao diện.

### 8.1. Nhóm đo khoảng cách HC-SR04

File chính:

- [`hcsr04.h`](./radar_short_range_touchgfx/STM32CubeIDE/Application/User/hcsr04.h)
- [`hcsr04.c`](./radar_short_range_touchgfx/STM32CubeIDE/Application/User/hcsr04.c)

Driver HC-SR04 sử dụng state machine, tự định nghĩa các biến trạng thái để xử lý logic một cách tường minh và rành mạch

```C
/**
 * @brief Các trạng thái hoạt động của driver HC-SR04.
 *
 * Enum HCSR04_State_t
 *
 * - IDLE         : Chưa thực hiện phép đo.
 * - WAIT_RISING  : Đã phát Trigger, đang chờ cạnh lên của Echo.
 * - WAIT_FALLING : Đã nhận cạnh lên, đang chờ cạnh xuống để kết thúc phép đo.
 * - DONE         : Đã đo xong và có dữ liệu khoảng cách hợp lệ.
 * - TIMEOUT      : Không nhận được Echo trong thời gian quy định.
 * - ERROR        : Xuất hiện lỗi trong quá trình đo.
 */
```

```C
/**
 * @brief Khởi tạo driver HC-SR04.
 *
 * Hàm bật bộ đếm chu kỳ DWT để đo thời gian có độ phân giải micro giây,
 * khởi tạo các biến trạng thái nội bộ của driver và đưa chân TRIG về mức thấp.
 * Đây là bước chuẩn bị trước khi radar bắt đầu thực hiện các phép đo khoảng cách.
 *
 * @param None.
 * @return None.
 */
void HCSR04_Init(void);
```

```C
/**
 * @brief Bắt đầu một lần đo khoảng cách bằng HC-SR04.
 *
 * Hàm phát xung Trigger có độ rộng khoảng 10 us trên chân TRIG và chuyển
 * trạng thái driver sang chờ tín hiệu Echo. Hàm chỉ khởi động quá trình đo,
 * không chờ kết quả hoàn thành.
 *
 * @param None.
 * @return None.
 */
void HCSR04_StartMeasure(void);
```

```C
/**
 * @brief Xử lý ngắt tín hiệu Echo của HC-SR04.
 *
 * Khi phát hiện cạnh lên của chân Echo, hàm lưu thời điểm bắt đầu.
 * Khi phát hiện cạnh xuống, hàm lưu thời điểm kết thúc, tính độ rộng xung
 * Echo theo micro giây và cập nhật trạng thái hoàn thành phép đo.
 *
 * @param GPIO_Pin: Chân GPIO phát sinh ngắt.
 * @return None.
 */
void HCSR04_GPIO_EXTI_Callback(uint16_t GPIO_Pin);
```

```C
/**
 * @brief Kiểm tra timeout của quá trình đo.
 *
 * Hàm kiểm tra xem thời gian chờ Echo có vượt quá ngưỡng cho phép hay không.
 * Nếu quá thời gian, trạng thái driver được chuyển sang TIMEOUT nhằm tránh
 * chương trình bị chờ vô hạn khi cảm biến không nhận được Echo.
 *
 * @param None.
 * @return None.
 */
void HCSR04_ProcessTimeout(void);
```

```C
/**
 * @brief Lấy khoảng cách đo được theo đơn vị centimet.
 *
 * Hàm chuyển đổi độ rộng xung Echo sang khoảng cách bằng công thức:
 *
 *      distance = echo_us / 58
 *
 * Sau khi chuyển đổi, hàm kiểm tra tính hợp lệ của kết quả (timeout,
 * khoảng cách tối thiểu, dữ liệu lỗi...) trước khi trả về.
 *
 * @param[out] distance_cm Con trỏ lưu khoảng cách đo được (cm).
 * @return
 *      - 1: Giá trị hợp lệ.
 *      - 0: Giá trị không hợp lệ hoặc phép đo thất bại.
 */
uint8_t HCSR04_GetDistanceCm(uint16_t *distance_cm);
```

```C
/**
 * @brief Lấy độ rộng xung Echo gần nhất.
 *
 * Hàm trả về giá trị Echo đã đo được gần nhất theo đơn vị micro giây,
 * phục vụ việc ghi log hoặc đánh giá chất lượng tín hiệu cảm biến.
 *
 * @param None.
 * @return Độ rộng xung Echo (us).
 */
uint32_t HCSR04_GetLastEchoUs(void);
```

### 8.2. Nhóm điều khiển servo MG90S

File chính:

- [`servo_mg90s.h`](./radar_short_range_touchgfx/STM32CubeIDE/Application/User/servo_mg90s.h)
- [`servo_mg90s.c`](./radar_short_range_touchgfx/STM32CubeIDE/Application/User/servo_mg90s.c)

Module servo MG90S chịu trách nhiệm tạo tín hiệu PWM để điều khiển góc quay của servo. Các hàm trong module cho phép khởi tạo PWM, điều khiển góc hoặc độ rộng xung trực tiếp, đồng thời lưu lại trạng thái gần nhất phục vụ việc kiểm tra và debug.

```C
/**
 * @brief Khởi tạo driver servo MG90S.
 *
 * Hàm khởi động kênh PWM TIM3 Channel 1 và đưa servo về vị trí
 * trung tâm mặc định. Đây là bước chuẩn bị trước khi radar bắt đầu
 * thực hiện quá trình quét.
 *
 * @param None.
 * @return None.
 */
void Servo_Init(void);
```

```C
/**
 * @brief Đặt độ rộng xung PWM cho servo.
 *
 * Hàm cập nhật độ rộng xung PWM theo đơn vị micro giây
 *
 * @param pulse_us Độ rộng xung PWM (us).
 * @return None.
 */
void Servo_SetPulseUs(uint16_t pulse_us);
```

```C
/**
 * @brief Điều khiển servo theo góc quay.
 *
 * Đặt góc quay của servo đế vị trí mong muốn trong khoảng 0 đến 180 độ
 *
 * @param angle_deg Góc cần đặt (độ).
 * @return None.
 */
void Servo_SetAngle(uint16_t angle_deg);
```

```C
/**
 * @brief Đưa servo về vị trí trung tâm.
 *
 * Hàm điều khiển servo quay về góc giữa (90 độ), tạo trạng thái
 * an toàn khi hệ thống dừng hoạt động hoặc cần reset vị trí quét.
 *
 * @param None.
 * @return None.
 */
void Servo_Stop(void);
```

```C
/**
 * @brief Lấy góc quay gần nhất của servo.
 *
 * Hàm trả về giá trị góc cuối cùng đã được đặt cho servo.
 * Giá trị này hữu ích cho việc theo dõi trạng thái hoặc debug.
 *
 * @param None.
 * @return Góc quay gần nhất (độ).
 */
uint16_t Servo_GetLastAngle(void);
```

```C
/**
 * @brief Lấy độ rộng xung PWM gần nhất.
 *
 * Hàm trả về giá trị độ rộng xung PWM cuối cùng đã được cấu hình
 * cho servo theo đơn vị micro giây.
 *
 * @param None.
 * @return Độ rộng xung PWM gần nhất (us).
 */
uint16_t Servo_GetLastPulseUs(void);
```

Các tham số cấu hình quan trọng của servo được định nghĩa trong [`radar_config.h`](./radar_short_range_touchgfx/STM32CubeIDE/Application/User/radar_config.h).

| Tham số                 | Giá trị mặc định | Ý nghĩa                                                                      |
| ----------------------- | ---------------: | ---------------------------------------------------------------------------- |
| `SERVO_MIN_PULSE_US`    |            `550` | Độ rộng xung PWM nhỏ nhất, tương ứng với góc quay nhỏ nhất của servo. (0°)   |
| `SERVO_CENTER_PULSE_US` |           `1500` | Độ rộng xung PWM tại vị trí trung tâm (90°).                                 |
| `SERVO_MAX_PULSE_US`    |           `2450` | Độ rộng xung PWM lớn nhất, tương ứng với góc quay lớn nhất của servo. (180°) |

### 8.3. Nhóm điều khiển buzzer và LED

File chính:

- [`buzzer_led.h`](./radar_short_range_touchgfx/STM32CubeIDE/Application/User/buzzer_led.h)
- [`buzzer_led.c`](./radar_short_range_touchgfx/STM32CubeIDE/Application/User/buzzer_led.c)

Module buzzer và LED chịu trách nhiệm điều khiển các thiết bị cảnh báo của hệ thống. Nhóm hàm này cung cấp giao diện hàm bật/tắt từng thiết bị LED và buzzer.

```C
/**
 * @brief Khởi tạo driver buzzer và LED.
 *
 * @param None.
 * @return None.
 */
void BuzzerLed_Init(void);
```

```C
/**
 * @brief Điều khiển trạng thái buzzer.
 *
 * Hàm bật hoặc tắt buzzer active theo trạng thái đầu vào.
 *
 * @param on Trạng thái buzzer (1: bật, 0: tắt).
 * @return None.
 */
void Buzzer_Set(uint8_t on);
```

```C
/**
 * @brief Điều khiển LED Scan.
 *
 * Hàm bật hoặc tắt LED Scan để biểu thị trạng thái hoạt động
 * của quá trình quét radar.
 *
 * @param on Trạng thái LED (1: bật, 0: tắt).
 * @return None.
 */
void LedScan_Set(uint8_t on);
```

```C
/**
 * @brief Điều khiển LED Alert.
 *
 * Hàm bật hoặc tắt LED Alert nhằm thông báo trạng thái
 * phát hiện vật cản hoặc cảnh báo khoảng cách gần.
 *
 * @param on Trạng thái LED (1: bật, 0: tắt).
 * @return None.
 */
void LedAlert_Set(uint8_t on);
```

```C
/**
 * @brief Cập nhật trạng thái cảnh báo của hệ thống.
 *
 * Hàm quyết định trạng thái LED Alert và buzzer dựa trên
 * kết quả phát hiện vật cản và mức cảnh báo khoảng cách.
 *
 * @param detected Cờ phát hiện vật cản.
 * @param near_warning Cờ cảnh báo vật ở khoảng cách gần.
 * @return None.
 */
void Alert_Update(uint8_t detected, uint8_t near_warning);
```

Logic cảnh báo hiện tại được chia thành ba mức như sau:

| Trạng thái          | Điều kiện                          | LED Alert | Buzzer                      |
| ------------------- | ---------------------------------- | --------- | --------------------------- |
| Không phát hiện vật | `detected = 0`                     | Tắt       | Tắt                         |
| Phát hiện vật       | `detected = 1`, `near_warning = 0` | Bật       | Tắt                         |
| Cảnh báo gần        | `detected = 1`, `near_warning = 1` | Bật       | Nhấp nháy khoảng mỗi 120 ms |

### 8.4. Nhóm logic điều phối radar

File chính:

- [`radar_app.h`](./radar_short_range_touchgfx/STM32CubeIDE/Application/User/radar_app.h)
- [`radar_app.c`](./radar_short_range_touchgfx/STM32CubeIDE/Application/User/radar_app.c)
- [`radar_config.h`](./radar_short_range_touchgfx/STM32CubeIDE/Application/User/radar_config.h)
- [`radar_types.h`](./radar_short_range_touchgfx/STM32CubeIDE/Application/User/radar_types.h)

Module `radar_app` là trung tâm điều phối toàn bộ hệ thống radar. Module chịu trách nhiệm quản lý trạng thái hoạt động, điều khiển servo, thực hiện phép đo khoảng cách, phân loại kết quả, cập nhật dữ liệu cho giao diện và điều khiển các thiết bị cảnh báo.

```C
/**
 * @brief Khởi tạo module RadarApp.
 *
 * Hàm khởi tạo toàn bộ các module thành phần của hệ thống,
 * bao gồm UI Bridge, servo MG90S, buzzer, LED, cảm biến
 * HC-SR04 và các trạng thái mặc định của radar.
 *
 * @param None.
 * @return None.
 */
void RadarApp_Init(void);
```

```C
/**
 * @brief Bắt đầu hoạt động của radar.
 *
 * Hàm chuyển trạng thái radar sang hoạt động, cho phép
 * vòng quét và quá trình đo khoảng cách được thực hiện.
 *
 * @param None.
 * @return None.
 */
void RadarApp_Start(void);
```

```C
/**
 * @brief Dừng hoạt động của radar.
 *
 * Hàm đưa radar về trạng thái an toàn bằng cách dừng
 * quá trình quét, đưa servo về vị trí trung tâm và
 * tắt toàn bộ tín hiệu cảnh báo.
 *
 * @param None.
 * @return None.
 */
void RadarApp_Stop(void);
```

```C
/**
 * @brief Thiết lập chế độ tốc độ quét.
 *
 * Hàm thay đổi tốc độ quét của radar theo chế độ được
 * lựa chọn, bao gồm SLOW, MED và FAST.
 *
 * @param mode Chế độ tốc độ quét.
 * @return None.
 */
void RadarApp_SetSpeedMode(RadarSpeedMode_t mode);
```

```C
/**
 * @brief Chuyển sang chế độ tốc độ quét tiếp theo.
 *
 * Hàm thay đổi tuần tự giữa các chế độ tốc độ quét
 * SLOW → MED → FAST → SLOW.
 *
 * @param None.
 * @return None.
 */
void RadarApp_NextSpeedMode(void);
```

```C
/**
 * @brief Thiết lập chế độ quét.
 *
 * Hàm cấu hình vùng quét của radar theo góc 90 độ
 * hoặc 180 độ. Góc hiện tại sẽ được giới hạn để
 * luôn nằm trong vùng quét mới.
 *
 * @param scan_mode_deg Góc quét (90 hoặc 180).
 * @return None.
 */
void RadarApp_SetScanMode(uint8_t scan_mode_deg);
```

```C
/**
 * @brief Chuyển đổi chế độ quét.
 *
 * Hàm chuyển đổi qua lại giữa chế độ quét
 * 90 độ và 180 độ.
 *
 * @param None.
 * @return None.
 */
void RadarApp_ToggleScanMode(void);
```

```C
/**
 * @brief Vòng xử lý chính của radar.
 *
 * Hàm thực hiện toàn bộ chu trình hoạt động của radar,
 * bao gồm đọc trạng thái điều khiển, đặt góc servo,
 * đo khoảng cách bằng HC-SR04, phân loại trạng thái
 * phát hiện vật cản, cập nhật dữ liệu giao diện,
 * điều khiển cảnh báo và chuyển sang góc quét tiếp theo.
 *
 * Hàm được gọi lặp liên tục trong radar task để duy trì
 * hoạt động của hệ thống.
 *
 * @param None.
 * @return None.
 */
void RadarApp_TaskLoop(void);
```

Các chế độ tốc độ quét được định nghĩa trong `RadarSpeedMode_t`.

| Chế độ             | Ý nghĩa                                                                     |
| ------------------ | --------------------------------------------------------------------------- |
| `RADAR_SPEED_SLOW` | Quét chậm với thời gian dừng lớn hơn tại mỗi góc, giúp phép đo ổn định hơn. |
| `RADAR_SPEED_MED`  | Tốc độ quét trung bình, cân bằng giữa tốc độ và độ ổn định.                 |
| `RADAR_SPEED_FAST` | Quét nhanh, giảm thời gian dừng tại mỗi góc để tăng tốc độ cập nhật.        |

Các trạng thái của rader được định nghĩa trong `RadarStatus_t`
| Giá trị | Ý nghĩa |
| --------------------- | --------------------------------------------------- |
| `RADAR_STATUS_SCAN` | Radar đang quét và chưa phát hiện vật cản. |
| `RADAR_STATUS_DETECT` | Radar đã phát hiện vật cản trong vùng quét. |
| `RADAR_STATUS_ALERT` | Radar phát hiện vật cản ở khoảng cách cảnh báo gần. |

Các biến data khi quét của radar được định nghĩa trong struct `RadarCoreData_t`.

| Thành viên                | Kiểu dữ liệu | Ý nghĩa                                                    |
| ------------------------- | ------------ | ---------------------------------------------------------- |
| `angle_deg`               | `uint16_t`   | Góc quét hiện tại của servo (độ).                          |
| `distance_cm`             | `uint16_t`   | Khoảng cách đo được từ cảm biến HC-SR04 (cm).              |
| `distance_valid`          | `uint8_t`    | Cờ xác nhận kết quả đo hợp lệ.                             |
| `object_detected`         | `uint8_t`    | Cờ cho biết có phát hiện vật cản hay không.                |
| `near_warning`            | `uint8_t`    | Cờ cảnh báo khi vật ở khoảng cách nguy hiểm.               |
| `radar_status`            | `uint8_t`    | Trạng thái hiện tại của radar (`SCAN`, `DETECT`, `ALERT`). |
| `object_count`            | `uint16_t`   | Tổng số lần phát hiện vật cản.                             |
| `last_object_distance_cm` | `uint16_t`   | Khoảng cách của vật cản được phát hiện gần nhất.           |
| `last_object_angle_deg`   | `uint16_t`   | Góc của vật cản được phát hiện gần nhất.                   |
| `buzzer_on`               | `uint8_t`    | Trạng thái hoạt động của buzzer.                           |
| `led3_on`                 | `uint8_t`    | Trạng thái LED Scan.                                       |
| `led4_on`                 | `uint8_t`    | Trạng thái LED Alert.                                      |
| `oled_connected`          | `uint8_t`    | Trạng thái kết nối của màn hình OLED.                      |

Đóng gói dữ liệu thành một struct duy nhất `RadarUiData_t`

| Thành viên  | Kiểu dữ liệu           | Ý nghĩa                                                                                            |
| ----------- | ---------------------- | -------------------------------------------------------------------------------------------------- |
| `core_data` | `RadarCoreData_t`      | Nhóm dữ liệu hoạt động của radar dùng để hiển thị trên giao diện.                                  |
| `control`   | `RadarControlConfig_t` | Nhóm thông tin cấu hình điều khiển của radar như trạng thái hoạt động, chế độ quét và tốc độ quét. |

Radar hỗ trợ hai chế độ vùng quét.

| Chế độ | Phạm vi góc                  |
| ------ | ---------------------------- |
| `90°`  | Quét từ 45° đến 135°.        |
| `180°` | Quét toàn bộ từ 0° đến 180°. |

### 8.5. Nhóm bridge dữ liệu UI

File chính:

- [`radar_ui_bridge.h`](./radar_short_range_touchgfx/STM32CubeIDE/Application/User/radar_ui_bridge.h)
- [`radar_ui_bridge.c`](./radar_short_range_touchgfx/STM32CubeIDE/Application/User/radar_ui_bridge.c)

Module UI Bridge đóng vai trò cầu nối giữa radar task và giao diện TouchGFX. Nhóm hàm này quản lý dữ liệu dùng chung, đồng bộ các tham số cấu hình và cung cấp cơ chế trao đổi dữ liệu an toàn giữa các task.

```C
/**
 * @brief Khởi tạo module Radar UI Bridge.
 *
 * Hàm khởi tạo cấu trúc RadarUiData_t với các giá trị mặc định,
 * đảm bảo giao diện có dữ liệu hợp lệ trước khi radar bắt đầu hoạt động.
 *
 * @param None.
 * @return None.
 */
void RadarUiBridge_Init(void);
```

```C
/**
 * @brief Cập nhật dữ liệu dùng chung cho giao diện.
 *
 * Hàm sao chép dữ liệu từ radar task vào vùng nhớ dùng chung để
 * giap diện co thể hiển thị
 *
 * @param data Con trỏ tới cấu trúc RadarCoreData_t cần cập nhật.
 * @return None.
 */
void RadarUiBridge_SetData(const RadarCoreData_t *data);
```

```C
/**
 * @brief Đọc dữ liệu hiện tại của radar.
 *
 * Hàm sao chép dữ liệu từ vùng nhớ dùng chung ra cấu trúc do
 * chương trình gọi cung cấp.
 *
 * @param[out] data Con trỏ lưu dữ liệu RadarUiData_t.
 * @return None.
 */
void RadarUiBridge_GetData(RadarUiData_t *data);
```

```C
/**
 * @brief Cập nhật trạng thái bật/tắt của radar.
 *
 * Hàm thay đổi trạng thái hoạt động của radar
 *
 * @param enabled Trạng thái radar (1: bật, 0: tắt).
 * @return None.
 */
void RadarUiBridge_SetRadarEnabled(uint8_t enabled);
```

```C
/**
 * @brief Cập nhật chế độ tốc độ quét.
 *
 * Hàm lưu chế độ tốc độ quét hiện tại vào vùng dữ liệu dùng chung
 *
 * @param speed_mode Chế độ tốc độ quét.
 * @return None.
 */
void RadarUiBridge_SetSpeedMode(uint8_t speed_mode);
```

```C
/**
 * @brief Cập nhật chế độ quét.
 *
 * Hàm lưu chế độ quét hiện tại (90° hoặc 180°) vào vùng dữ liệu
 * dùng chung
 *
 * @param scan_mode_deg Góc quét của radar.
 * @return None.
 */
void RadarUiBridge_SetScanMode(uint8_t scan_mode_deg);
```

```C
/**
 * @brief Chuyển sang chế độ tốc độ quét tiếp theo.
 *
 * Hàm thay đổi tuần tự giữa các chế độ tốc độ quét được hỗ trợ
 * và cập nhật kết quả vào vùng dữ liệu dùng chung.
 *
 * @param None.
 * @return None.
 */
void RadarUiBridge_NextSpeedMode(void);
```

### 8.6. Nhóm cập nhật giao diện TouchGFX

File chính:

- [`ScreenHomeView.cpp`](./radar_short_range_touchgfx/TouchGFX/gui/src/screenhome_screen/ScreenHomeView.cpp)
- [`ScreenScanView.cpp`](./radar_short_range_touchgfx/TouchGFX/gui/src/screenscan_screen/ScreenScanView.cpp)
- [`ScreenSettingsView.cpp`](./radar_short_range_touchgfx/TouchGFX/gui/src/screensettings_screen/ScreenSettingsView.cpp)
- [`ScreenInfoView.cpp`](./radar_short_range_touchgfx/TouchGFX/gui/src/screeninfo_screen/ScreenInfoView.cpp)

Nhóm các màn hình TouchGFX chịu trách nhiệm hiển thị trạng thái hoạt động của radar và tiếp nhận thao tác từ người dùng. Các màn hình trao đổi dữ liệu với radar thông qua `RadarUiBridge`, bảo đảm tách biệt giữa giao diện và logic xử lý.

```C
/**
 * @brief Cập nhật giao diện radar.
 *
 * Hàm đọc dữ liệu từ RadarUiBridge và cập nhật các thành
 * phần hiển thị như góc quét, khoảng cách đo được, trạng
 * thái phát hiện vật cản và hiệu ứng quét radar.
 *
 * @param None.
 * @return None.
 */
void ScreenScanView::updateRadarUi(void);
```

```C
/**
 * @brief Cập nhật vị trí mục tiêu trên giao diện radar.
 *
 * Hàm chuyển đổi góc quét và khoảng cách đo được sang tọa
 * độ hiển thị trên màn hình, đồng thời quyết định có hiển
 * thị mục tiêu hay không.
 *
 * @param angleDeg Góc phát hiện vật cản (độ).
 * @param distanceCm Khoảng cách đo được (cm).
 * @param visible Cờ hiển thị mục tiêu.
 * @return None.
 */
void ScreenScanView::updateTarget(uint16_t angleDeg,
                                  uint16_t distanceCm,
                                  uint8_t visible);
```

```C
/**
 * @brief Xử lý sự kiện nhấn trên màn hình Settings.
 *
 * Hàm tiếp nhận các thao tác của người dùng trên giao diện
 * để thay đổi tốc độ quét hoặc chế độ quét của radar.
 *
 * @param event Sự kiện Click của TouchGFX.
 * @return None.
 */
void ScreenSettingsView::handleClickEvent(const touchgfx::ClickEvent& event);
```

```C
/**
 * @brief Cập nhật thông tin trên màn hình Info.
 *
 * Hàm đọc dữ liệu hiện tại của radar và cập nhật các thông
 * tin thống kê như chế độ quét, tốc độ quét, khoảng cách,
 * góc quét gần nhất và số lượng vật thể phát hiện.
 *
 * @param None.
 * @return None.
 */
void ScreenInfoView::updateInfoText(void);
```

Các màn hình TouchGFX đảm nhiệm các chức năng chính như sau.

| Màn hình     | Chức năng                                                                                                     |
| ------------ | ------------------------------------------------------------------------------------------------------------- |
| **Home**     | Hiển thị màn hình chính và đưa radar về trạng thái dừng an toàn.                                              |
| **Scan**     | Hiển thị hiệu ứng quét radar, mục tiêu phát hiện, góc quét, khoảng cách và trạng thái hoạt động của hệ thống. |
| **Settings** | Cho phép người dùng thay đổi tốc độ quét và vùng quét của radar.                                              |
| **Info**     | Hiển thị các thông tin thống kê và trạng thái hoạt động hiện tại của hệ thống.                                |

## 9. KẾT QUẢ

### 9.1. Mô tả kết quả đạt được

Dựa trên mã nguồn trong repo, project đã hình thành đầy đủ khung chức năng của một hệ thống radar tầm ngắn: có driver đo HC-SR04, driver servo MG90S, logic điều phối radar, cảnh báo LED/buzzer, bridge dữ liệu và giao diện TouchGFX nhiều màn hình. Các file build/debug trong `STM32CubeIDE/Debug` cũng cho thấy project đã được build trong môi trường STM32CubeIDE.

Các kết quả thực nghiệm như ảnh demo, video demo và bảng sai số đo chưa có sẵn trong repo. Vì vậy phần dưới đây tạo sẵn khung để nhóm bổ sung minh chứng khi hoàn thiện báo cáo nộp.

### 9.2. Chức năng đã hoàn thành theo mã nguồn

| Chức năng                    | Trạng thái theo repo     | Minh chứng trong mã nguồn                     | Ghi chú                                                          |
| ---------------------------- | ------------------------ | --------------------------------------------- | ---------------------------------------------------------------- |
| Khởi tạo board và ngoại vi   | Đã có                    | `Core/Src/main.c`, `.ioc`                     | GPIO, TIM2, TIM3, I2C3, USART1, LTDC, DMA2D, FMC/SDRAM, FreeRTOS |
| Đo HC-SR04 bằng trigger/echo | Đã có code               | `hcsr04.c`                                    | Dùng PG2/PG3, EXTI3 + DWT                                        |
| Điều khiển servo MG90S       | Đã có code               | `servo_mg90s.c`                               | PWM TIM3_CH1 trên PB4                                            |
| Quét 90/180 độ               | Đã có code               | `radar_config.h`, `radar_app.c`               | 90 độ: 45-135; 180 độ: 0-180                                     |
| Tốc độ SLOW/MED/FAST         | Đã có code               | `radar_config.h`, `RadarApp_SetSpeedMode()`   | Step/delay cấu hình trong `radar_config.h`                       |
| Phân loại SCAN/DETECT/ALERT  | Đã có code               | `radar_types.h`, `RadarApp_TaskLoop()`        | Detect <= 50 cm, near warning <= 5 cm                            |
| LED scan và LED alert        | Đã có code               | `buzzer_led.c`, `radar_app.c`                 | PG13 toggle scan, PG14 alert                                     |
| Buzzer cảnh báo gần          | Đã có code               | `Alert_Update()`                              | Toggle khoảng 120 ms khi `near_warning`                          |
| TouchGFX radar UI            | Đã có code               | `ScreenScanView.cpp`                          | Hiển thị góc, khoảng cách, status, sweep, target dot             |
| Settings UI                  | Đã có code               | `ScreenSettingsView.cpp`                      | Chọn speed và mode                                               |
| Info UI                      | Đã có code               | `ScreenInfoView.cpp`                          | Hiển thị mode, speed, range, last angle, object count            |
| OLED SH1106                  | Chưa đủ dữ liệu kết luận | I2C3 có cấu hình, chưa thấy driver OLED riêng | Để TODO nếu chưa có demo thực tế                                 |

### 9.3. Hình ảnh giao diện

`[Chèn ảnh: giao diện màn hình Home]`

`[Chèn ảnh: giao diện màn hình Scan khi đang quét]`

`[Chèn ảnh: giao diện màn hình Scan khi phát hiện vật cản]`

`[Chèn ảnh: giao diện màn hình Settings]`

`[Chèn ảnh: giao diện màn hình Info]`

`[Chèn ảnh: OLED SH1106 nếu đã hoàn thiện]`

### 9.4. Video demo

`[Chèn video: demo sản phẩm]`

`[Chèn video: demo servo quét và HC-SR04 phát hiện vật cản]`

`[Chèn video: demo đổi chế độ quét 90/180 độ và đổi tốc độ SLOW/MED/FAST]`

`[Chèn video: demo cảnh báo bằng LED/buzzer khi vật ở gần]`

### 9.5. Bảng kiểm thử cơ bản

| Nhóm kiểm thử        | Cách kiểm thử                                                | Kết quả mong đợi                              | Kết quả thực tế |
| -------------------- | ------------------------------------------------------------ | --------------------------------------------- | --------------- |
| Servo PWM            | Vào màn Scan, quan sát servo quay theo góc                   | Servo quét trong vùng 90/180 độ theo mode     | TODO            |
| HC-SR04 trigger/echo | Đưa vật phẳng trước cảm biến, quan sát distance trên UI/UART | Có khoảng cách hợp lệ, không timeout liên tục | TODO            |
| Detect threshold     | Đặt vật trong khoảng <= 50 cm                                | UI chuyển `DETECT`, target dot xuất hiện      | TODO            |
| Near warning         | Đặt vật trong khoảng <= 5 cm                                 | UI chuyển `ALERT`, LED alert bật, buzzer kêu  | TODO            |
| Speed mode           | Bấm nút UI hoặc bấm ngắn USER                                | Speed đổi giữa SLOW/MED/FAST                  | TODO            |
| Scan mode            | Bấm nút UI hoặc giữ lâu USER                                 | Mode đổi 90/180 độ                            | TODO            |
| Home stop            | Quay về Home                                                 | Radar dừng, output về trạng thái an toàn      | TODO            |
| UART debug           | Mở terminal USART1                                           | Có log angle, distance, echo, counter         | TODO            |
| OLED                 | Quan sát OLED SH1106                                         | TODO theo trạng thái driver thực tế           | TODO            |

### 9.6. Bảng đo thực nghiệm cần bổ sung

Repo hiện chưa có số liệu đo thực nghiệm. Nhóm nên đo ở nhiều điều kiện khác nhau: vật phẳng, vật lệch góc, vật nhỏ, khoảng cách gần/xa, servo đứng yên và servo đang quét.

| Điều kiện thử                    | Khoảng cách thực (cm) | Khoảng cách hiển thị (cm) | Sai số (cm) | Nhận xét |
| -------------------------------- | --------------------: | ------------------------: | ----------: | -------- |
| Vật phẳng, servo đứng yên        |                  TODO |                      TODO |        TODO | TODO     |
| Vật phẳng, servo đang quét chậm  |                  TODO |                      TODO |        TODO | TODO     |
| Vật phẳng, servo đang quét nhanh |                  TODO |                      TODO |        TODO | TODO     |
| Vật lệch góc so với cảm biến     |                  TODO |                      TODO |        TODO | TODO     |
| Vật nhỏ hoặc bề mặt hấp thụ âm   |                  TODO |                      TODO |        TODO | TODO     |
| Khoảng cách gần vùng cảnh báo    |                  TODO |                      TODO |        TODO | TODO     |

## 10. ĐÁNH GIÁ THỰC TẾ VÀ GIỚI HẠN

### 10.1. Nhận xét chung

Hệ thống đáp ứng tốt vai trò một mô hình radar tầm ngắn phục vụ học tập: có quét góc, đo khoảng cách, hiển thị trực quan và cảnh báo. Tuy nhiên, không nên hiểu đây là radar theo nghĩa RF hoặc thiết bị đo khoảng cách có độ chính xác ổn định trong mọi môi trường. Độ tin cậy phụ thuộc đồng thời vào cảm biến siêu âm, cơ cấu servo, nguồn cấp, cách gá cơ khí và cách lấy mẫu khi quét.

### 10.2. Giới hạn của HC-SR04

HC-SR04 có thông số danh định tương đối đẹp: tầm đo thường được nêu khoảng 2-400 cm, trigger 10 us, sóng siêu âm 40 kHz và độ chính xác lý tưởng khoảng 3 mm. Trong thực tế, các con số này chỉ đạt gần đúng trong điều kiện thuận lợi: vật phản xạ tốt, đặt vuông góc, nguồn ổn định và môi trường ít nhiễu.

Các yếu tố ảnh hưởng mạnh tới kết quả:

- Góc phản xạ: vật nghiêng có thể làm sóng phản xạ lệch khỏi đầu thu.
- Bề mặt vật: vật mềm, xốp, vải hoặc bề mặt hấp thụ âm cho echo yếu hơn.
- Vùng chết gần cảm biến: khoảng cách quá gần có thể không ổn định.
- Nhiễu cơ khí: servo rung làm hướng phát/thu thay đổi trong lúc đo.
- Nguồn cấp: nguồn yếu hoặc nhiễu có thể làm xung echo sai hoặc timeout.
- Môi trường: nhiệt độ và độ ẩm làm vận tốc âm thanh thay đổi, kéo theo sai số.

Vì vậy kết quả đo nên được đánh giá bằng bảng thực nghiệm thay vì chỉ dựa vào thông số danh định.

### 10.3. Giới hạn của servo MG90S

MG90S có stall torque tham khảo khoảng 1.8 kgcm ở 4.8 V và 2.2 kgcm ở 6 V, tốc độ khoảng 0.1 s/60 độ. Đây là thông số quan trọng để chọn cơ cấu, nhưng **stall torque không phải tải làm việc liên tục**. Nếu gá cảm biến nặng, lệch tâm hoặc để servo giữ tải ở biên lâu, servo có thể nóng, rung, tiêu thụ dòng lớn và làm sụt nguồn.

Khi quét nhanh, servo chưa kịp ổn định tại góc mới thì hệ thống đã đo khoảng cách. Khi đó hướng cảm biến có thể lệch so với góc hiển thị, làm target dot trên UI không phản ánh đúng vị trí vật. Đây là lý do trong `RadarApp_TaskLoop()` có delay ngắn theo speed mode trước khi đo, nhưng nếu cơ khí rung nhiều thì delay này vẫn có thể chưa đủ.

### 10.4. Giới hạn của board STM32F429I-DISC1

STM32F429I-DISC1 thuận lợi cho bài toán giao diện vì có LCD, SDRAM, LTDC và TouchGFX. Đổi lại, nhiều chân đã được sử dụng cho LCD, SDRAM và các ngoại vi tích hợp. Quỹ chân trống thực tế bị hạn chế, nên pin mapping phải cẩn thận để không xung đột.

Việc chạy TouchGFX, FreeRTOS, đo echo, điều khiển servo và cập nhật cảnh báo cùng lúc cũng làm hệ thống phức tạp hơn một bài đo cảm biến đơn giản. Nếu cập nhật UI quá dày hoặc blocking trong task quá lâu, cảm giác giao diện và chu kỳ quét đều có thể bị ảnh hưởng.

### 10.5. Nhận xét kỹ thuật thực tế

Điểm khó nhất của hệ thống không nằm ở từng module riêng lẻ, mà nằm ở tích hợp. HC-SR04 cần timing micro giây, servo tạo nhiễu nguồn và rung cơ khí, TouchGFX cần tài nguyên đồ họa, FreeRTOS chia thời gian cho nhiều task. Khi một lỗi xuất hiện, ví dụ khoảng cách nhảy hoặc màn hình reset, nguyên nhân có thể nằm ở nguồn, dây nối, mức logic, task timing hoặc thuật toán đọc echo.

Do đó cách đánh giá hợp lý là kiểm thử theo lớp: phần cứng nguồn trước, sau đó GPIO/servo, sau đó HC-SR04 đứng yên, sau đó quét servo, cuối cùng mới ghép UI và cảnh báo.

## 11. KHÓ KHĂN VÀ KINH NGHIỆM RÚT RA

### 11.1. Khó khăn / lỗi thường gặp / giải pháp

| Hiện tượng                                     | Nguyên nhân                                                                           | Cách khắc phục                                                                                               |
| ---------------------------------------------- | ------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------ | --- |
| Màn hình trắng, board reset khi servo quay     | Nguồn 5 V yếu, servo kéo dòng đỉnh lớn, GND không chắc                                | Dùng nguồn riêng đủ dòng cho servo, nối GND chung, kiểm tra sụt áp khi servo đổi hướng                       |
| Servo rung hoặc quay không đều                 | Nguồn yếu, dây nguồn dài, pulse min/max chưa phù hợp, cơ khí gá lệch                  | Rút ngắn dây nguồn, tăng ổn định nguồn, chỉnh `SERVO_MIN_PULSE_US`/`SERVO_MAX_PULSE_US`, gá cảm biến cân hơn |
| Khoảng cách HC-SR04 nhiễu khi đang quét        | Servo chưa ổn định, cảm biến rung, vật phản xạ lệch góc                               | Tăng thời gian chờ trước khi đo, giảm tốc độ quét, cố định cảm biến chắc hơn, test với vật phẳng             |
| Không bắt được echo                            | Sai dây PG3, không chung GND, echo 5 V chưa xử lý đúng, cảm biến không được cấp nguồn | Đối chiếu pin mapping, đo chân echo bằng logic analyzer/oscilloscope nếu có, dùng chia áp hoặc level shifter |
| Số đo lúc có lúc không                         | Nguồn nhiễu, dây jumper lỏng, echo yếu do bề mặt vật                                  | Kiểm tra lại dây, dùng nguồn ổn định, thử vật phẳng lớn đặt vuông góc                                        |
| UI TouchGFX không phản ánh đúng dữ liệu mới    | Bridge dữ liệu chưa đồng bộ, update UI quá nhanh/chậm, task radar đang bị block       | Đọc/ghi qua `RadarUiBridge`, cập nhật UI theo tick hợp lý, tránh blocking dài trong radar task               |
| Radar tự dừng hoặc không chạy khi vào màn Scan | `radar_enabled` bị ghi đè hoặc chưa gọi `RadarApp_Start()`                            | Kiểm tra `ScreenScanView::setupScreen()`, `RadarUiBridge_SetData()` và log UART                              |     |

### 11.2. Kinh nghiệm rút ra

- **Tách bài toán thành từng lớp để test.** Không nên ghép TouchGFX, servo và HC-SR04 ngay từ đầu. Nên test PWM servo, test trigger/echo, test UART log, rồi mới tích hợp radar task và UI.
- **Nguồn là phần phải kiểm tra sớm.** Servo nhỏ như MG90S vẫn có thể tạo dòng đỉnh đủ lớn để làm sụt áp, gây reset hoặc nhiễu cảm biến. Nối GND chung và cấp nguồn servo đúng cách quan trọng không kém code.
- **Không tin tuyệt đối vào thông số danh định của cảm biến.** HC-SR04 cho kết quả tốt khi điều kiện phản xạ thuận lợi, nhưng dễ sai khi vật nghiêng, mềm, nhỏ hoặc khi cảm biến rung.
- **Đo khi servo đã ổn định.** Nếu đo ngay lúc servo đang di chuyển, khoảng cách có thể đúng nhưng góc tương ứng trên UI lại sai, hoặc echo bị mất do hướng phát thay đổi.
- **Bridge dữ liệu giúp UI và logic độc lập hơn.** `RadarUiBridge` là điểm trung gian hợp lý giữa C task và TouchGFX C++. Nhờ đó UI không cần biết chi tiết driver, còn radar task không cần thao tác trực tiếp widget.
- **Log UART rất hữu ích khi hệ thống phức tạp.** Khi UI chỉ hiện `--- cm`, log echo/counter giúp phân biệt lỗi cảm biến, lỗi timeout, lỗi dây echo hay lỗi xử lý UI.
- **Pin mapping phải được quản lý như tài liệu kỹ thuật.** Với board tích hợp LCD/SDRAM như STM32F429I-DISC1, đổi chân tùy tiện rất dễ xung đột ngoại vi. README, `.ioc` và `main.h` cần thống nhất.

## 12. HƯỚNG PHÁT TRIỂN

Các hướng phát triển dưới đây được chia theo mức độ ưu tiên thực tế. Một số mục có thể làm ngay trên nền code hiện tại, một số mục cần bổ sung phần cứng hoặc thay đổi kiến trúc phần mềm.

### 12.1. Cải thiện độ ổn định đo

| Hướng phát triển                                 | Lợi ích                            | Ghi chú triển khai                                                                   |
| ------------------------------------------------ | ---------------------------------- | ------------------------------------------------------------------------------------ |
| Lọc số đo bằng median filter hoặc moving average | Giảm nhiễu do echo nhảy bất thường | Nên giữ raw value để debug, chỉ lọc giá trị hiển thị/quyết định cảnh báo             |
| Loại bỏ outlier theo ngưỡng nhảy khoảng cách     | Tránh target dot nhảy mạnh trên UI | Có thể dùng `HCSR04_MAX_JUMP_CM` đã khai báo trong `radar_config.h` nếu muốn mở rộng |
| Tăng thời gian chờ servo ổn định trước khi đo    | Giảm nhiễu do rung cơ khí          | Cần cân bằng giữa độ ổn định và tốc độ quét                                          |
| Hiệu chuẩn pulse servo theo cơ khí thực tế       | Giảm kẹt biên, giảm rung           | Điều chỉnh `SERVO_MIN_PULSE_US` và `SERVO_MAX_PULSE_US`                              |

### 12.2. Cải thiện giao diện và trải nghiệm sử dụng

- Cho phép chỉnh ngưỡng `DETECT` và `ALERT` trực tiếp trên màn Settings.
- Hiển thị thêm chất lượng phép đo: valid/timeout, echo us, số lần timeout.
- Thêm chế độ pause/resume radar thay vì chỉ start khi vào Scan và stop khi về Home.
- Bổ sung biểu đồ hoặc log các lần phát hiện gần nhất trên màn Info.
- Hoàn thiện hiển thị OLED SH1106 nếu nhóm muốn có màn phụ độc lập với LCD TouchGFX.

### 12.3. Cải thiện phần cứng và an toàn mạch

- Bổ sung mạch chia áp hoặc level shifter rõ ràng cho đường echo HC-SR04.
- Dùng nguồn 5 V riêng đủ dòng cho servo, có GND chung với STM32.
- Cố định cơ khí cảm biến chắc hơn để giảm rung khi servo đổi hướng.
- Thiết kế PCB nhỏ hoặc shield đấu nối để tránh dây jumper lỏng.
- Bổ sung tụ lọc gần servo và cảm biến nếu nguồn bị nhiễu.

### 12.4. Cải thiện kiến trúc phần mềm

- Nếu dữ liệu UI mở rộng, cân nhắc dùng FreeRTOS mutex hoặc queue thay cho critical section thủ công.
- Tách phần state machine radar thành module rõ hơn để dễ test.
- Bổ sung unit test hoặc test giả lập cho các hàm tính góc, map tọa độ target và lọc số đo.
- Ghi log qua UART theo mức debug để tránh spam terminal khi demo.

## 13. TÀI LIỆU THAM KHẢO

| Nhóm tài liệu    | Tài liệu / nguồn tham khảo                                              | Vai trò                                                     |
| ---------------- | ----------------------------------------------------------------------- | ----------------------------------------------------------- |
| STM32 MCU        | STMicroelectronics, `STM32F429xx Datasheet`                             | Thông số vi điều khiển STM32F429ZIT6                        |
| STM32 peripheral | STMicroelectronics, `RM0090 Reference manual - STM32F4 series`          | Tham khảo GPIO, EXTI, TIM, I2C, USART, DMA, FMC             |
| Board            | STMicroelectronics, user manual cho STM32F429I-DISC1 / STM32F429I-DISCO | Pinout, LCD, SDRAM, ST-LINK, phần cứng tích hợp trên board  |
| TouchGFX         | STMicroelectronics, tài liệu TouchGFX và X-CUBE-TOUCHGFX                | Cấu trúc GUI task, asset, màn hình, render UI               |
| HC-SR04          | Datasheet/tài liệu module HC-SR04                                       | Trigger 10 us, echo, nguyên lý đo siêu âm, tầm đo danh định |
| MG90S            | Datasheet/tài liệu servo MG90S                                          | PWM servo, torque, tốc độ, điện áp hoạt động                |
| SH1106           | Datasheet/tài liệu SH1106 OLED controller                               | Controller OLED 132x64, giao tiếp I2C/SPI tùy module        |
| Repo mẫu báo cáo | <https://github.com/neittien0110/ProjectSample>                         | Tham khảo khung báo cáo theo yêu cầu môn học                |
