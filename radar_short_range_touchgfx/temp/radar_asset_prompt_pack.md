# Prompt pack – Radar tầm ngắn TouchGFX assets

## Chốt phạm vi làm nhanh
- Giữ nguyên background và button images hiện có.
- Không tạo lại background và button assets.
- Tập trung tạo **icon** và **radar support assets** còn thiếu.
- Sweep line và target nên ưu tiên điều khiển bằng code TouchGFX để linh hoạt.
- Màn Scan: sweep xanh lá khi đang quét bình thường; khi đang phát hiện vật cản có thể đổi target sang đỏ, và nếu muốn có thể đổi sweep sang đỏ trong thời điểm cảnh báo.

---

## 1) Master prompt – bộ icon (10 icon)
Dùng prompt này với công cụ tạo ảnh nếu muốn tạo cả bộ icon đồng bộ.

```text
Create a cohesive pack of 10 embedded-system UI icons for a TouchGFX radar project on STM32F429I-DISC1.

Requirements:
- Generate 10 separate icons, each exported as an individual PNG.
- Exact size for each icon: 48x48 px.
- Transparent background.
- Style: clean futuristic embedded UI / cyber radar / neon blue-green line art.
- High contrast, simple shapes, readable at very small size.
- No text labels inside the icons.
- Same visual style across the whole set.
- Use thin glowing outlines, minimal fill, suitable for a 2.4 inch LCD.

Needed icons and filenames:
1. icon_radar.png        -> radar/scope icon
2. icon_play.png         -> play/start icon
3. icon_settings.png     -> settings/gear icon
4. icon_info.png         -> info icon
5. icon_warning.png      -> warning triangle icon
6. icon_buzzer.png       -> buzzer/speaker/alarm icon
7. icon_servo.png        -> servo motor icon
8. icon_distance.png     -> distance/ruler icon
9. icon_oled.png         -> small screen/display icon
10. icon_led.png         -> LED/light indicator icon

Keep all icons centered, with transparent padding, and export-ready for TouchGFX.
```

---

## 2) Master prompt – radar support assets
Dùng prompt này để tạo các asset hỗ trợ riêng cho màn Scan.

```text
Create a small asset pack for a TouchGFX short-range radar interface on STM32F429I-DISC1.
Generate the following assets as separate PNG files.

General style:
- Futuristic embedded radar UI.
- Dark navy / black background when needed.
- Neon green for normal radar visuals.
- Red for alert visuals.
- Clean, sharp, suitable for a 240x320 portrait LCD.
- High contrast, not too detailed.

Assets to generate:

1. radar_grid_180.png
- Size: 200x140 px.
- Transparent background preferred.
- A clean 180-degree radar semicircle grid.
- Concentric arcs and angle division lines.
- Neon green lines.
- No target dot.
- No sweep beam.
- Intended to overlay on the scan screen.

2. radar_grid_90.png
- Size: 160x140 px.
- Transparent background preferred.
- A clean 90-degree radar grid.
- Neon green lines.
- No target dot.
- No sweep beam.

3. target_dot.png
- Size: 20x20 px.
- Transparent background.
- Bright radar target marker.
- Create as a circular target/glow style.
- Main version in red.

4. sweep_glow_green.png
- Size: 200x140 px.
- Transparent background.
- A radar sweep wedge/beam starting from the bottom center, glowing green.
- Soft transparent fade.
- No grid, only the sweep beam.

5. sweep_glow_red.png
- Size: 200x140 px.
- Transparent background.
- Same as sweep_glow_green but in red alert style.

Export each asset as a separate PNG with the exact filenames above.
```

---

## 3) Prompt tối giản hơn – chỉ tạo đúng cái nên tạo ngay
Nếu muốn làm nhanh nhất, chỉ tạo đúng 3 asset sau:
- target_dot.png
- sweep_glow_green.png
- sweep_glow_red.png

```text
Create 3 separate PNG assets for a TouchGFX radar scan screen.

1. target_dot.png
- 20x20 px
- transparent background
- bright red circular target marker with glow

2. sweep_glow_green.png
- 200x140 px
- transparent background
- radar sweep wedge from bottom center, neon green, soft fade

3. sweep_glow_red.png
- 200x140 px
- transparent background
- same sweep wedge but in red alert style

Style: futuristic embedded UI, sharp and simple, readable on STM32 2.4 inch LCD.
```

---

## 4) Kế hoạch UI rút gọn để kịp tiến độ

### ScreenHome
Giữ:
- imgHomeBg
- txtHomeTitle
- txtHomeSub
- btnStart
- btnSettings
- btnInfo

### ScreenScan (ưu tiên cao nhất)
Nên có:
- imgScanBg
- imgRadarGrid (nếu có asset riêng, còn không có thể dùng luôn background)
- imgSweepGreen
- imgSweepRed (ẩn mặc định)
- imgTarget (ẩn mặc định)
- txtAngleLabel + txtAngleValue
- txtDistanceLabel + txtDistanceValue
- txtStatusLabel + txtStatusValue
- txtModeLabel + txtModeValue
- btnBackHome
- btnSettingsSmall

Logic hiển thị gợi ý:
- SCANNING: sweep xanh, target ẩn.
- DETECTED: target đỏ hiện tại vị trí tương đối.
- WARNING NEAR: có thể bật sweep đỏ hoặc chuyển sang ScreenWarning.

### ScreenWarning
- imgWarningBg
- txtWarningTitle
- txtWarningDistance
- txtWarningAngle
- btnBackScan

### ScreenSettings
- imgSettingsBg
- btnSpeedSlow
- btnSpeedMedium
- btnSpeedFast
- btnMode90
- btnMode180
- txtHintB1
- btnBackHome

### ScreenInfo
- imgInfoBg
- txtScanMode
- txtSpeedMode
- txtMinDistance
- txtLastAngle
- txtObjectCount
- txtBuzzerState
- txtLedState
- txtOledState
- btnBackHome

---

## 5) Gợi ý triển khai code UI trước
1. Làm xong navigation 5 screen.
2. Làm xong ScreenScan với dữ liệu giả:
   - Angle = 65 deg
   - Distance = 42 cm
   - Status = SCANNING / DETECTED
3. Tạo imgTarget ẩn/hiện bằng code.
4. Tạo 2 sweep images: xanh và đỏ; đổi visible theo trạng thái.
5. Generate code.
6. Sau đó mới ghép `radar_ui_bridge` và code nhúng.

