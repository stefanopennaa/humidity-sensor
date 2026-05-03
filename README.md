# Humidity Sensor (ESP8266 + SSD1306)

<p align="center">
  <img src="https://img.shields.io/badge/version-2026.05.03-blue.svg" alt="Version">
  <img src="https://img.shields.io/badge/platform-ESP8266-green.svg" alt="Platform">
  <img src="https://img.shields.io/badge/status-hobby-orange.svg" alt="Status">
</p>

Arduino firmware to monitor soil humidity with **ESP8266**, display it on an **SSD1306 OLED** and a web dashboard, keep a 24h history, and send automatic emails.

> Hobby project (AI-assisted), use at your own risk.

## Overview

- **Soil humidity reading** from a capacitive sensor on `A0`.
- **Local OLED display** (SSD1306, I2C on `D5`/`D6`).
- **Responsive web dashboard** with live value and history chart.
- **24h history** stored on LittleFS with rolling retention.
- **Automatic email notifications**: daily summary + low-humidity alert.
- **OTA update** via `/update` endpoint.

## Requirements

- ESP8266 (e.g. NodeMCU / compatible D1 mini)
- Capacitive soil humidity sensor (analog output)
- Display OLED SSD1306 128x64
- 2.4 GHz WiFi network
- Arduino libraries: `Adafruit_GFX`, `Adafruit_SSD1306`, `ESP8266WebServer`, `ESP8266HTTPClient`, `ElegantOTA`, `LittleFS`

## Quick Start

1. Copy the example secrets file:
   ```bash
   cp secrets.example.h secrets.h
   ```
2. Fill `secrets.h` with your WiFi credentials, Resend settings, and OTA/restart credentials.
3. Verify calibration values in `humidity-sensor.ino`:
   - `ADC_DRY` (sensor in dry/air condition)
   - `ADC_WET` (sensor in wet/water condition)
4. Open `humidity-sensor.ino` in Arduino IDE, select board/port, and upload the firmware.

## Web Endpoints

| Endpoint | Description |
| --- | --- |
| `/` | Main dashboard |
| `/api/humidity` | Current reading (`raw`, `humidity`) |
| `/api/history` | History samples (24h window) |
| `/api/restart` (POST) | Device reboot (Basic Auth) |
| `/update` | OTA page |

## Main Configuration (`humidity-sensor.ino`)

- Sensor calibration: `ADC_DRY`, `ADC_WET`
- Intervals: `SENSOR_UPDATE_INTERVAL_MS`, `HISTORY_SAMPLE_INTERVAL_MS`, `EMAIL_RETRY_INTERVAL_MS`
- Email scheduling: `DAILY_EMAIL_HOUR`, `DAILY_EMAIL_MINUTE`
- Timezone/NTP: `TZ_INFO`, `NTP_SERVER_1`, `NTP_SERVER_2`
- Soil status thresholds: logic `<30`, `<65`, `>=65`

## Known Limitations

- Email delivery depends on internet connectivity and DNS resolution for `api.resend.com`.
- History retention is limited by `HISTORY_MAX_SAMPLES` and the 24h API window.
- Measurement quality depends on real-world sensor calibration in your setup.

## Changelog

### 2026.05.03
- Formal reorganization of source file headers/changelog comments.
- Code formatting normalized (no logic changes).
- README rewritten and aligned to a cleaner presentation.
- Sensitive data removed from the repository and `secrets.example.h` added.

## Security

- `secrets.h` is ignored by Git via `.gitignore`.
- Never commit real credentials.
- If credentials were previously exposed, always rotate passwords and API keys.

## File Structure

| File | Role |
| --- | --- |
| `humidity-sensor.ino` | Main firmware logic |
| `web_page_template.h` | Web dashboard HTML/CSS/JS template |
| `email_template.h` | HTML email template |
| `secrets.example.h` | Public configuration template |
| `secrets.h` | Local secrets (not versioned) |
