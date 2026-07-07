# CONTEXT TIẾP TỤC PROJECT – Radar tầm ngắn STM32F429I-DISC1

> File này dùng để dán sang đoạn chat mới để tiếp tục project mà không mất ngữ cảnh.  
> Project hiện tại đang ưu tiên **hoàn thiện UI TouchGFX trước**, sau đó mới Generate Code và viết code thật cho servo/SR-04/OLED/buzzer/B1.

---

## 1. Context chung

### Đề tài
- Tên đề tài: **Radar tầm ngắn**
- Nhóm trong file bài tập lớn: **chatgpt**
- Mục tiêu: làm bản hoàn thiện, không phải demo tối giản.

### Board / MCU / workflow
- Board duy nhất đang dùng: **STM32F429I-DISC1**
- MCU: **STM32F429ZIT6**
- LCD: TFT 2.4 inch, QVGA, đang dùng **portrait 240x320**
- TouchGFX Designer: **4.26.1**
- STM32CubeIDE: workflow **CubeMX riêng + CubeIDE riêng**
- Project nền được copy từ project TouchGFX game cũ đã build sạch.
- Project hiện tại: `E:\IT4210\radar_short_range_touchgfx`

### Nguyên tắc hướng dẫn
- Nói tiếng Việt.
- Hướng dẫn từng bước, mỗi bước vừa phải, không quá dài.
- Sau mỗi bước nên dừng chờ “OK”.
- Khi sửa code phải nói rõ file nào, đoạn nào, chèn/thay thế gì.
- Code tự viết ưu tiên file `.c/.h` riêng hoặc vùng USER CODE.
- Không tự đổi board, không tự đổi pin khi chưa hỏi.
- UI dùng **TouchGFX trước**, code phần cứng sau.
- Tận dụng tính năng sẵn có của TouchGFX, đặc biệt **Texture Mapper**.

---

## 2. Linh kiện chính thức sẽ dùng

### Bắt buộc
1. **STM32F429I-DISC1**
2. **SR-04 / HC-SR04** — đo khoảng cách vật cản, dùng Trigger + Echo.
3. **Servo MG90S** — quay cảm biến radar, điều khiển PWM 50Hz.
4. **OLED SH1106 I2C** — màn phụ hiển thị góc, khoảng cách, trạng thái.
5. **Buzzer active** — bật/tắt bằng GPIO.
6. **LED3 / LED4 trên board** — LED3 scan, LED4 detect/alert.
7. **B1 trên board** — bắt buộc theo đề bài, dùng đổi tốc độ servo: `SLOW -> MED -> FAST -> SLOW`.

### Cảnh báo phần cứng phải nhớ
- HC-SR04 thường cấp 5V.
- Echo trả về 5V, **không cắm thẳng vào STM32**.
- Phải chia áp Echo xuống 3.3V.
- GND servo, SR-04, OLED, buzzer và STM32 phải nối chung.
- Servo MG90S nên cấp nguồn 5V đủ dòng, không cấp từ GPIO.
- Có thể thêm tụ 100uF–470uF gần nguồn servo để giảm sụt áp.

---

## 3. Thông số radar chốt cho demo

Dù cảm biến có thể ghi xa hơn, đồ án nên giới hạn thực tế để chạy ổn định:

```c
#define RADAR_MIN_DISTANCE_CM       2
#define RADAR_MAX_DISPLAY_CM        150
#define RADAR_OBJECT_DETECT_CM      150
#define RADAR_NEAR_WARNING_CM       20
```

Ý nghĩa:
- `2–150 cm`: khoảng cách hợp lệ để hiển thị.
- `<=150 cm`: có vật cản.
- `<=20 cm`: cảnh báo gần, sweep đỏ, buzzer kêu, LED4 bật.

---

## 4. Logic hệ thống tổng thể

### Luồng radar thật
```text
RadarTask
  -> Servo_SetAngle(angle)
  -> chờ servo ổn định
  -> HCSR04_ReadDistanceCm(&distance)
  -> lọc nhiễu
  -> xác định detected / near_warning
  -> cập nhật LED/buzzer/OLED
  -> RadarUiBridge_SetData()
  -> TouchGFX Model đọc dữ liệu
  -> ScreenScanView cập nhật sweep, target, text
```

### Biến trạng thái lõi
```c
uint8_t  radar_enabled;
uint16_t current_angle_deg;
uint16_t current_distance_cm;
uint8_t  object_detected;
uint8_t  near_warning;
uint8_t  scan_mode_deg;      // 90 hoặc 180
uint8_t  speed_mode;         // SLOW / MED / FAST
uint16_t object_count;
uint16_t min_distance_cm;
uint16_t last_detect_angle;
uint8_t  buzzer_on;
uint8_t  led3_on;
uint8_t  led4_on;
uint8_t  oled_connected;
```

### State hoạt động
```text
Home:
- radar_enabled = 0
- servo về 90°
- buzzer off
- LED cảnh báo off

Bấm Start:
- radar_enabled = 1
- chuyển ScreenScan
- bắt đầu quét

ScreenScan:
- radar_enabled = 1
- servo quét liên tục
- SR-04 đo liên tục
- sweep quay theo servo
- target dot hiện theo vật cản

ScreenSettings:
- radar vẫn chạy nền nếu trước đó đã Start
- dùng để đổi speed/mode
- B1 vẫn đổi speed

ScreenInfo:
- radar vẫn chạy nền nếu trước đó đã Start
- chỉ hiển thị thống kê/trạng thái

Back Home:
- radar_enabled = 0
- servo về 90°
- buzzer off
- LED off
```

---

## 5. Logic hiển thị radar trên ScreenScan

### Sweep
- Không dùng nhiều frame ảnh.
- Dùng **Texture Mapper** có sẵn trong TouchGFX.
- Có 2 object:
  - `txtrSweepGreen`
  - `txtrSweepRed`
- Cả 2 đều là Texture Mapper quay theo góc servo bằng code.
- Phải bật: `Animation Texture Mapper = ON`
- Không bật Animation Texture Mapper cho image thường.

### Target dot
- `imgTarget` là Image thường.
- Không cần Texture Mapper.
- Không cần MoveAnimator lúc đầu.
- Update vị trí liên tục bằng:
```cpp
imgTarget.setXY(targetX, targetY);
imgTarget.setVisible(true_or_false);
imgTarget.invalidate();
```

### Công thức target dot
Giả sử vùng radar:
```text
Radar image X = 20
Radar image Y = 55
Radar W = 200
Radar H = 140
Tâm radar local: cx = 100, cy = 140
Tâm màn hình: screen_cx = 20 + 100 = 120
              screen_cy = 55 + 140 = 195
```

Bán kính hiển thị:
```c
#define RADAR_UI_RADIUS_PX 95
#define RADAR_MAX_DISPLAY_CM 150
```

Ý tưởng:
```text
góc servo -> hướng dot
khoảng cách -> dot gần/xa tâm radar
```

Công thức dự kiến:
```cpp
float normalized = distance_cm / 150.0f;
float r = normalized * 95.0f;

// map 0° trái, 90° giữa, 180° phải
float rad = angle_deg * PI / 180.0f;

targetX = screen_cx - 10 + r * cosf(PI - rad);
targetY = screen_cy - 10 - r * sinf(rad);
```

Trừ `10` vì target dot kích thước 20x20, cần đặt tâm dot đúng vị trí.

### Trạng thái hiển thị
```text
Không có vật:
- txtrSweepGreen visible = true
- txtrSweepRed visible = false
- imgTarget visible = false
- txtStatusValue = SCAN

Có vật trong 2–150 cm:
- txtrSweepGreen visible = false hoặc true tùy style
- txtrSweepRed visible = true
- imgTarget visible = true
- txtStatusValue = DETECT

Vật gần <=20 cm:
- txtrSweepGreen visible = false
- txtrSweepRed visible = true
- imgTarget visible = true
- txtStatusValue = ALERT
- buzzer bật
- LED4 bật
```

---

## 6. Assets đã có

Thư mục ảnh:
`E:\IT4210\radar_short_range_touchgfx\DesignAssets\generated_by_gpt`

### Background đã có
- `bg_home_radar.png`
- `bg_scan_radar.png`
- `bg_settings_radar.png`
- `bg_warning_radar.png`
- `bg_info_radar.png`

Ảnh đã resize đúng màn portrait:
- **Width = 240**
- **Height = 320**
- Tỉ lệ đúng: **3:4 portrait**
- Không dùng 9:16.

### Button assets
- `btn_radar_65x36_normal.png`
- `btn_radar_65x36_pressed.png`
- `btn_transparent_65x36.png`

UI ưu tiên dùng **Button with Label**, không dùng text riêng + invisible button.

### Icons đã có
- `icon_radar.png`
- `icon_play.png`
- `icon_settings.png`
- `icon_info.png`
- `icon_warning.png`
- `icon_buzzer.png`
- `icon_servo.png`
- `icon_distance.png`
- `icon_oled.png`
- `icon_led.png`
- có thêm icon home/back tùy file đã tạo/import nếu cần.

### Radar assets đã có
- `radar_grid_180.png`
- `radar_grid_90.png`
- `target_dot.png`
- `sweep_glow_green.png`
- `sweep_glow_red.png`

---

## 7. UI TouchGFX hiện tại

### Danh sách screen
Đã tạo:
- `ScreenHome`
- `ScreenScan`
- `ScreenSettings`
- `ScreenInfo`
- `ScreenWarning`

`ScreenHome` là startup screen.

---

## 8. ScreenHome – trạng thái hiện tại

### Object chuẩn
```text
imgHomeBg
txtHomeTitle
txtHomeSub
btnStart
btnSettings
btnInfo
```

### Nội dung
- Title: `RADAR TẦM NGẮN`
- Subtitle: `STM32F429I-DISC1`
- 3 nút dưới: `Start`, `Set`, `Info`

### Interaction chuẩn
```text
btnStart clicked    -> Change screen -> ScreenScan
btnSettings clicked -> Change screen -> ScreenSettings
btnInfo clicked     -> Change screen -> ScreenInfo
```

Transition nên để `None` cho nhẹ, nhưng nếu đang dùng Slide thì vẫn chạy được.

### Logic sau này
- `btnStart`: bật `radar_enabled = 1`, chuyển ScreenScan.
- `btnSettings`: vào Settings, có thể chưa start radar.
- `btnInfo`: xem Info.

---

## 9. ScreenScan – trạng thái hiện tại

Đây là màn quan trọng nhất, đã gần hoàn thiện UI.

### Object chuẩn cần giữ
```text
imgScanBg
imgRadarGrid180
txtrSweepGreen
txtrSweepRed
imgTarget

txtAngleValue
txtDistanceValue
txtStatusValue

btnBackHome
btnSettingsSmall
btnInfoSmall       // nên thêm nếu chưa có

iconServo
iconDistance
iconRadarStatus
```

### Layer từ dưới lên
```text
imgScanBg
imgRadarGrid180
txtrSweepGreen
txtrSweepRed
imgTarget
iconServo
iconDistance
iconRadarStatus
txtAngleValue
txtDistanceValue
txtStatusValue
btnBackHome
btnInfoSmall
btnSettingsSmall
```

### Texture Mapper settings
Cho `txtrSweepGreen` và `txtrSweepRed`:
```text
X = 20
Y = 55
W = 200
H = 140
Bitmap/Image = sweep_glow_green.png hoặc sweep_glow_red.png
Animation Texture Mapper = ON

X Origo = 100
Y Origo = 140
Z Origo = 1000
Camera Distance = 1000
```

Giải thích:
- Origo X=100, Y=140 là đáy giữa ảnh sweep.
- Đây là tâm quay đúng của radar.

### Initial visible
```text
txtrSweepGreen: Visible = true
txtrSweepRed:   Visible = false
imgTarget:      Visible = false
```

### Text mặc định
```text
txtAngleValue    = "065 deg"
txtDistanceValue = "042 cm"
txtStatusValue   = "SCAN"
```

### Icon trong 3 ô dưới
```text
iconServo       -> icon_servo.png
iconDistance    -> icon_distance.png hoặc icon_radar tùy style
iconRadarStatus -> icon_radar.png hoặc icon_distance tùy đang đặt
```

### Navigation trên Scan
Nên có 3 nút:
```text
btnBackHome      góc trái  -> ScreenHome
btnInfoSmall     giữa trên -> ScreenInfo
btnSettingsSmall góc phải  -> ScreenSettings
```

Logic:
```text
btnBackHome:
- về Home
- sau này code dừng radar

btnInfoSmall:
- sang ScreenInfo
- radar vẫn chạy nền

btnSettingsSmall:
- sang ScreenSettings
- radar vẫn chạy nền
```

---

## 10. ScreenSettings – trạng thái hiện tại

Màn Settings đã setup đẹp và hợp lý.

### Object chuẩn cần giữ/đổi tên
```text
imgSettingsBg
txtSettingsTitle
txtCurrentConfig

btnBackHome
btnBackScan

btnSpeedSlow
btnSpeedMedium
btnSpeedFast

btnMode90
btnMode180
```

Nếu còn tên kiểu:
```text
buttonWithIcon1
btnSpeedSlow_2
btnMode90_1
```
thì cần rename về tên chuẩn trên.

### Nội dung UI hiện tại
- Title: `SETTINGS`
- Buttons: `SLOW`, `MED`, `FAST`, `90 độ`, `180 độ`
- Dòng dưới:
```text
SPEED: MED | MODE: 180
```

### Hai nút góc
```text
btnBackHome:
- góc trái
- Change screen -> ScreenHome
- sau này code dừng radar

btnBackScan:
- góc phải
- Change screen -> ScreenScan
- radar vẫn chạy nền
```

### Interaction trong Designer
Bắt buộc:
```text
btnBackHome -> ScreenHome
btnBackScan -> ScreenScan
```

Các nút speed/mode chưa cần interaction đổi màn:
```text
btnSpeedSlow
btnSpeedMedium
btnSpeedFast
btnMode90
btnMode180
```
Sau này code C++ sẽ bắt event/callback để đổi biến radar.

### Logic code về sau
```text
btnSpeedSlow    -> speed_mode = SLOW
btnSpeedMedium  -> speed_mode = MED
btnSpeedFast    -> speed_mode = FAST
btnMode90       -> scan_mode_deg = 90
btnMode180      -> scan_mode_deg = 180
txtCurrentConfig cập nhật theo state
```

---

## 11. ScreenInfo – cần làm tiếp ở đoạn chat mới

### Mục tiêu
ScreenInfo là màn xem thống kê hệ thống, radar vẫn chạy nền nếu đã Start.

### Object chuẩn đề xuất
```text
imgInfoBg
btnBackHome
btnBackScan

txtInfoTitle
txtScanMode
txtSpeedMode
txtDistanceRange
txtLastAngle
txtObjectCount
txtBuzzerState
txtLedState
txtOledState

iconInfoRadar
iconInfoServo
iconInfoDistance
iconInfoBuzzer
iconInfoLed
iconInfoOled
```

### Layout gợi ý
Background `bg_info_radar.png` có nhiều khung ngang. Có thể đặt text theo từng hàng:

```text
txtInfoTitle
Text: SYSTEM INFO
X: 55
Y: 18
W: 140
H: 24

txtScanMode
Text: Scan: 180 deg
X: 60
Y: 55
W: 160
H: 18

txtSpeedMode
Text: Speed: MED
X: 60
Y: 90
W: 160
H: 18

txtDistanceRange
Text: Range: 2-150 cm
X: 60
Y: 125
W: 160
H: 18

txtLastAngle
Text: Last angle: 065
X: 60
Y: 160
W: 160
H: 18

txtObjectCount
Text: Objects: 000
X: 60
Y: 195
W: 160
H: 18
```

Dưới cùng:
```text
txtBuzzerState
Text: Buzzer: OFF
X: 15
Y: 270
W: 75
H: 18

txtLedState
Text: LED: OK
X: 92
Y: 270
W: 60
H: 18

txtOledState
Text: OLED: OK
X: 155
Y: 270
W: 75
H: 18
```

### Navigation
```text
btnBackHome -> ScreenHome
btnBackScan -> ScreenScan
```

Logic:
- `btnBackHome`: về Home và dừng radar.
- `btnBackScan`: quay lại Scan, radar chạy tiếp.

---

## 12. ScreenWarning – màn phụ

ScreenWarning không phải runtime chính. Runtime chính là ScreenScan đổi đỏ/ALERT.

### Object chuẩn đề xuất
```text
imgWarningBg
btnBackScan
btnBackHome

txtWarningTitle
txtWarningDistance
txtWarningAngle
txtWarningHint

iconWarning
iconBuzzer
```

### Layout gợi ý
```text
txtWarningTitle
Text: ALERT
X: 80
Y: 40
W: 100
H: 35

txtWarningDistance
Text: 012 cm
X: 70
Y: 130
W: 130
H: 40

txtWarningAngle
Text: Angle: 065
X: 65
Y: 180
W: 130
H: 22

txtWarningHint
Text: OBJECT TOO CLOSE
X: 40
Y: 220
W: 170
H: 22
```

Navigation:
```text
btnBackScan -> ScreenScan
btnBackHome -> ScreenHome
```

---

## 13. Code architecture sau khi UI xong

Sau khi hoàn thiện UI và Generate Code, sẽ code theo module:

### Core/Inc
```text
radar_config.h
radar_types.h
radar_ui_bridge.h
radar_app.h
servo_mg90s.h
hcsr04.h
buzzer_led.h
oled_status.h
```

### Core/Src
```text
radar_ui_bridge.c
radar_app.c
servo_mg90s.c
hcsr04.c
buzzer_led.c
oled_status.c
```

### TouchGFX sửa
```text
TouchGFX/gui/include/gui/model/Model.hpp
TouchGFX/gui/src/model/Model.cpp

TouchGFX/gui/include/gui/screenscan_screen/ScreenScanView.hpp
TouchGFX/gui/src/screenscan_screen/ScreenScanView.cpp

TouchGFX/gui/include/gui/screensettings_screen/ScreenSettingsView.hpp
TouchGFX/gui/src/screensettings_screen/ScreenSettingsView.cpp

TouchGFX/gui/include/gui/screeninfo_screen/ScreenInfoView.hpp
TouchGFX/gui/src/screeninfo_screen/ScreenInfoView.cpp
```

Tên folder thực tế có thể do TouchGFX generate, ví dụ:
```text
screen_scan_screen
screen_settings_screen
```
Cần kiểm tra sau Generate Code.

---

## 14. Struct UI bridge dự kiến

```c
typedef enum {
    RADAR_SPEED_SLOW = 0,
    RADAR_SPEED_MEDIUM,
    RADAR_SPEED_FAST
} RadarSpeedMode_t;

typedef enum {
    RADAR_SCAN_90 = 90,
    RADAR_SCAN_180 = 180
} RadarScanMode_t;

typedef struct {
    uint16_t angle_deg;
    uint16_t distance_cm;

    uint8_t object_detected;
    uint8_t near_warning;
    uint8_t radar_enabled;

    uint8_t scan_mode_deg;
    uint8_t speed_mode;

    uint16_t object_count;
    uint16_t min_distance_cm;
    uint16_t last_detect_angle;

    uint8_t buzzer_on;
    uint8_t led3_on;
    uint8_t led4_on;
    uint8_t oled_connected;
} RadarUiData_t;
```

Functions:
```c
void RadarUiBridge_SetData(const RadarUiData_t *data);
void RadarUiBridge_GetData(RadarUiData_t *data);

void RadarUiBridge_SetRadarEnabled(uint8_t enabled);
uint8_t RadarUiBridge_GetRadarEnabled(void);

void RadarUiBridge_SetSpeedMode(uint8_t speed);
uint8_t RadarUiBridge_GetSpeedMode(void);

void RadarUiBridge_SetScanMode(uint8_t mode);
uint8_t RadarUiBridge_GetScanMode(void);
```

---

## 15. Radar app logic dự kiến

```c
void RadarApp_Init(void);
void RadarApp_TaskLoop(void);

void RadarApp_Start(void);
void RadarApp_Stop(void);

void RadarApp_SetSpeedMode(RadarSpeedMode_t mode);
void RadarApp_NextSpeedMode(void);

void RadarApp_SetScanMode(RadarScanMode_t mode);
```

### Speed mode
```c
SLOW:
- angle step = 3°
- delay = 120 ms

MED:
- angle step = 5°
- delay = 80 ms

FAST:
- angle step = 8° hoặc 10°
- delay = 50 ms
```

### Scan mode
```text
180°:
0 -> 180 -> 0

90°:
45 -> 135 -> 45
```

Lý do chọn 45–135 cho 90°:
- Giữ radar ở giữa màn.
- Đẹp hơn 0–90 vì không lệch một bên.

---

## 16. FreeRTOS task dự kiến

### RadarTask
```text
- Nếu radar_enabled = 1:
  gọi RadarApp_TaskLoop()
- Nếu radar_enabled = 0:
  servo giữ 90°
  buzzer off
  vTaskDelay()
```

### ButtonTask
```text
- Đọc B1
- Chống dội
- Khi nhấn B1:
  RadarApp_NextSpeedMode()
  cập nhật txtCurrentConfig qua UI bridge
```

### TouchGFX task
Có sẵn.

---

## 17. Driver phần cứng dự kiến

### Servo MG90S
PWM 50Hz:
```text
Period = 20ms
0°   = khoảng 500us
90°  = khoảng 1500us
180° = khoảng 2500us
```

Functions:
```c
void Servo_Init(void);
void Servo_SetAngle(uint16_t angle);
void Servo_SetPulseUs(uint16_t pulse_us);
```

### HC-SR04
Functions:
```c
void HCSR04_Init(void);
uint8_t HCSR04_ReadDistanceCm(uint16_t *distance_cm);
uint8_t HCSR04_ReadRawUs(uint32_t *echo_us);
```

Công thức:
```c
distance_cm = echo_time_us / 58;
```

### Buzzer / LED
Functions:
```c
void BuzzerLed_Init(void);
void Buzzer_Set(uint8_t on);
void LedScan_Set(uint8_t on);
void LedDetect_Set(uint8_t on);
void Alert_Update(uint8_t detected, uint8_t near);
```

### OLED SH1106
Functions:
```c
void OledStatus_Init(void);
void OledStatus_ShowNormal(uint16_t angle, uint16_t distance, uint8_t mode);
void OledStatus_ShowDetected(uint16_t angle, uint16_t distance);
void OledStatus_ShowWarning(uint16_t angle, uint16_t distance);
void OledStatus_ShowError(void);
```

---

## 18. Việc cần làm ngay khi sang chat mới

Tiếp tục từ trạng thái hiện tại:

### Bước tiếp theo A – hoàn thiện ScreenInfo
- Dùng background `imgInfoBg`
- Thêm text thống kê và icon nếu cần
- Thêm `btnBackHome`, `btnBackScan`
- Set interaction

### Bước tiếp theo B – hoàn thiện ScreenWarning
- Dùng background `imgWarningBg`
- Thêm text ALERT / distance / angle
- Thêm back scan/home

### Bước tiếp theo C – kiểm tra object names toàn bộ UI
Đảm bảo không còn tên chung chung:
```text
image1
textArea1
button1
buttonWithIcon1
btnSpeedSlow_2
btnMode90_1
```

### Bước tiếp theo D – Generate Code
- Bấm Generate Code trong TouchGFX.
- Mở STM32CubeIDE.
- Build thử.
- Nếu lỗi do tên object cũ từ game racing thì sửa theo file lỗi.

### Bước tiếp theo E – code TouchGFX trước
- Cho TextureMapper sweep quay thử theo tick.
- Cho target dot chạy thử theo angle/distance.
- Sau đó mới nối bridge và phần cứng.

---

## 19. Các lỗi/điểm phải nhớ

- Không dùng nhiều sweep frame, vì TouchGFX có **Texture Mapper**.
- Không để sweep cố định.
- Không thêm `txtModeValue` trong ScreenScan nếu UI đã đủ gọn.
- Button nên dùng **Button with Label**, không dùng text riêng + invisible button.
- B1 phải đổi tốc độ servo, không được quên.
- ScreenWarning là màn phụ, không phải luồng chính.
- Runtime chính là `ScreenScan` đổi trạng thái SCAN/DETECT/ALERT.
- Khi vào Settings/Info từ Scan, radar vẫn chạy nền.
- Khi về Home, radar dừng.

---

## 20. Tóm tắt cực ngắn cho chat mới

```text
Tôi đang làm project Radar tầm ngắn trên STM32F429I-DISC1, TouchGFX portrait 240x320.
Đã tạo UI gồm ScreenHome, ScreenScan, ScreenSettings, ScreenInfo, ScreenWarning.
ScreenScan dùng Texture Mapper txtrSweepGreen/txtrSweepRed để quay sweep theo servo, không dùng nhiều frame.
imgTarget là Image thường, sẽ setXY theo angle + distance.
B1 đổi speed servo SLOW/MED/FAST.
ScreenSettings đã có SLOW/MED/FAST, 90/180, txtCurrentConfig.
Cần tiếp tục từ ScreenInfo, ScreenWarning, rồi Generate Code và build.
```
