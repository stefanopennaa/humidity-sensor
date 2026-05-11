# Humidity Sensor (ESP8266 + SSD1306)

<p align="center">
  <img src="https://img.shields.io/badge/version-2026.05.11-blue.svg" alt="Version">
  <img src="https://img.shields.io/badge/platform-ESP8266-green.svg" alt="Platform">
  <img src="https://img.shields.io/badge/license-MIT-yellow.svg" alt="License">
</p>

Firmware for an **ESP8266 soil humidity monitor** with capacitive soil sensor (`A0`), DHT11 ambient sensor (`D2`), SSD1306 OLED output, responsive web dashboard, 24h history, automatic email alerts, and OTA updates.

> Hobby project (AI-assisted), use at your own risk.

## Overview

- **Soil humidity reading** from a capacitive sensor on `A0`.
- **Ambient reading** from DHT11 on `D2` (temperature + RH%).
- **Local OLED display** (SSD1306, I2C on `D5`/`D6`).
- **Responsive web dashboard** with live value and history chart.
- **24h history** stored on LittleFS with rolling retention.
- **Automatic email notifications**: daily summary + low-humidity alert, with daily Wi-Fi reconnect + internet preflight before email attempts.
- **OTA update** via `/update` endpoint.

## Requirements

- ESP8266 (e.g. NodeMCU / compatible D1 mini)
- Capacitive soil humidity sensor (analog output)
- DHT11 temperature/humidity sensor (digital output on `D2`)
- Display OLED SSD1306 128x64
- 2.4 GHz WiFi network
- Arduino libraries: `Adafruit_GFX`, `Adafruit_SSD1306`, `DHT sensor library`, `ESP8266WebServer`, `ESP8266HTTPClient`, `ElegantOTA`, `LittleFS`

> Note: resistive soil moisture sensors tend to degrade quickly in continuous use (often within a few weeks), which is why this project uses a capacitive sensor on `A0`.

## Quick start

1. Clone the repository.
   ```bash
   git clone https://github.com/stefanopennaa/humidity-sensor.git
   cd humidity-sensor
   ```
2. Copy the example secrets file:
   ```bash
   cp secrets.example.h secrets.h
   ```
3. Fill `secrets.h` with your WiFi credentials, Resend settings, and OTA/restart credentials.
4. Verify calibration values in `humidity-sensor.ino`:
    - `ADC_DRY` (sensor in dry/air condition)
    - `ADC_WET` (sensor in wet/water condition)
5. Open `humidity-sensor.ino` in Arduino IDE, select board/port, and upload the firmware.

## File structure

| File | Role |
| --- | --- |
| `humidity-sensor.ino` | Main firmware logic |
| `web_page_template.h` | Web dashboard HTML/CSS/JS template |
| `email_template.h` | HTML email template |
| `secrets.example.h` | Public configuration template |
| `secrets.h` | Local secrets (not versioned) |

## Web endpoints

| Endpoint | Description |
| --- | --- |
| `/` | Main dashboard |
| `/api/humidity` | Current readings (`raw`, `humidity`, `airTempC`, `airHumidity`, `airOk`) |
| `/api/history` | History samples (24h window) |
| `/api/restart` (POST) | Device reboot (Basic Auth) |
| `/update` | OTA page |

## Main configuration (`humidity-sensor.ino`)

- Sensor calibration: `ADC_DRY`, `ADC_WET`
- DHT setup: `DHT_SENSOR_PIN`, `DHT_SENSOR_TYPE`
- Network intervals: `WIFI_RETRY_INTERVAL_MS`, `WIFI_CONNECT_TIMEOUT_MS`, `WIFI_DISCONNECT_DEBOUNCE_MS`, `INTERNET_CHECK_INTERVAL_MS`, `INTERNET_RECONNECT_GRACE_MS`
- Other intervals: `SENSOR_UPDATE_INTERVAL_MS`, `SENSOR_REFRESH_INTERVAL_MS`, `HISTORY_REFRESH_INTERVAL_MS`, `HISTORY_SAMPLE_INTERVAL_MS`, `EMAIL_RETRY_INTERVAL_MS`
- Email scheduling: `DAILY_EMAIL_HOUR`, `DAILY_EMAIL_MINUTE` (default `10:00` local time)
- Timezone/NTP: `TZ_INFO`, `NTP_SERVER_1`, `NTP_SERVER_2`
- Soil status thresholds: `<30` dry, `<65` medium, `>=65` wet

## Known limitations

- Email delivery depends on internet connectivity and DNS resolution for `api.resend.com`.
- History retention is limited by `HISTORY_MAX_SAMPLES` and the 24h API window.
- Measurement quality depends on real-world sensor calibration in your setup.

## Quick troubleshooting

| Problem | Check |
| --- | --- |
| WiFi not connected | Verify SSID/password in `secrets.h` and use a 2.4 GHz network |
| Device reachable only intermittently after WiFi reconnects | Update to firmware `2026.05.11+` (unified reconnect flow + periodic DNS checks with anti-flap threshold) |
| WiFi status stuck on "No WiFi. Retry..." | Update to firmware `2026.05.08+` (Wi-Fi state detection and disconnect debounce hardened) |
| Email not sent after overnight router/internet glitches | Firmware `2026.05.11+` forces a daily Wi-Fi reconnect and checks internet reachability before each email attempt |
| Humidity values look wrong | Recalibrate `ADC_DRY` and `ADC_WET` for your sensor |
| No email notifications | Check Resend credentials, sender/recipient, and internet connectivity |
| Dashboard empty/unreachable | Confirm device IP and that the ESP8266 is connected to WiFi |
| OTA update unavailable | Open `/update` and verify OTA credentials in `secrets.h` |

## Changelog

### 2026.05.11
- Added an email network preflight that runs before delivery attempts.
- First email attempt of each local day now forces a fresh Wi-Fi reconnect.
- Low-humidity alert email now uses the same reconnect + internet check flow before sending.
- Simplified and hardened runtime Wi-Fi/Internet management with a unified reconnect flow.
- Added periodic DNS health checks with anti-flap threshold to reduce false "No internet" states.
- Daily scheduled email time changed from 08:00 to 10:00 (local timezone).
- Email template now includes ambient temperature and ambient humidity values.

### 2026.05.08
- Hardened Wi-Fi connectivity detection to avoid false negatives from transient ESP8266 status states.
- Added a Wi-Fi disconnect debounce window to prevent infinite "No WiFi. Retry..." loops during brief glitches or blocking phases.
- Improved runtime Wi-Fi status text consistency after reconnection.

### 2026.05.07
- Web dashboard labels updated to "Temp. ambiente" and "Umidità ambiente".

### 2026.05.03
- Formal reorganization of source file headers/changelog comments.
- Code formatting normalized (no logic changes).
- README rewritten and aligned to a cleaner presentation.
- Sensitive data removed from the repository and `secrets.example.h` added.

### 2026.05.04
- Added DHT11 support on `D2` (ambient temperature + humidity).
- OLED now shows ambient values in addition to soil values.
- `/api/humidity` extended with DHT fields while keeping existing soil fields.
- Web dashboard now shows ambient temperature and humidity values.
- Added a note explaining why resistive probes were replaced by a capacitive sensor.

## Security

- `secrets.h` is ignored by Git via `.gitignore`.
- Never commit real credentials.
- If credentials were previously exposed, always rotate passwords and API keys.

## License

MIT, see [LICENSE](LICENSE).

## Support

- Issue tracker: [GitHub Issues](https://github.com/stefanopennaa/humidity-sensor/issues)
- Email: stefano@stefanopenna.it
