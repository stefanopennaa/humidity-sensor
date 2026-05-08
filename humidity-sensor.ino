/*
 * Project: Humidity Sensor - ESP8266 Plant Monitor
 * File: humidity-sensor.ino
 * Purpose: Main firmware entrypoint (sensing, display, networking, API, OTA, email).
 *
 * Changelog:
 * - 2026-05-08: Hardened Wi-Fi connectivity detection and added disconnect debounce to prevent false infinite "No WiFi. Retry..." loops.
 * - 2026-05-07: Updated web dashboard ambient labels ("Temp. ambiente", "Umidità ambiente").
 * - 2026-05-04: Added DHT11 on D2, extended /api/humidity with ambient fields, and updated OLED layout (Temp/Humi).
 * - 2026-05-03: Reorganized header comments and standardized code formatting (form only).
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <ElegantOTA.h>
#include <LittleFS.h>
#include <time.h>
#include "web_page_template.h"
#include "email_template.h"
#include "secrets.h"

// ============================================================================
// CONFIGURATION
// ============================================================================

// --- Display Configuration ---
constexpr uint8_t SCREEN_WIDTH = 128;
constexpr uint8_t SCREEN_HEIGHT = 64;
constexpr uint8_t OLED_ADDRESS = 0x3C;
constexpr uint8_t OLED_SDA_PIN = D5;
constexpr uint8_t OLED_SCL_PIN = D6;

// --- Soil Moisture Sensor ---
// ESP8266 ADC range: 0-1023
// Calibration: dry soil = high ADC, wet soil = low ADC
constexpr uint8_t SOIL_SENSOR_PIN = A0;
constexpr int ADC_DRY = 1023;  // Adjust based on sensor in air
constexpr int ADC_WET = 127;   // Adjust based on sensor in water

// --- Ambient Sensor (DHT11) ---
constexpr uint8_t DHT_SENSOR_PIN = D2;
constexpr uint8_t DHT_SENSOR_TYPE = DHT11;

// --- Timing Configuration ---
constexpr unsigned long WIFI_RETRY_INTERVAL_MS = 5000;                   // Retry Wi-Fi every 5s
constexpr unsigned long WIFI_CONNECT_TIMEOUT_MS = 15000;                 // Initial connection timeout
constexpr unsigned long WIFI_CONNECT_POLL_MS = 500;                      // Poll interval during connect
constexpr unsigned long WIFI_DISCONNECT_DEBOUNCE_MS = 15000;             // Ignore brief Wi-Fi status glitches
constexpr unsigned long WIFI_DISPLAY_IP_MS = 5000;                       // Alternate IP display every 5s
constexpr unsigned long INTERNET_CHECK_INTERVAL_MS = 30000;              // Verify internet/DNS every 30s
constexpr unsigned long INTERNET_RECONNECT_GRACE_MS = 60000;             // Reconnect if internet is down >60s
constexpr unsigned long SENSOR_UPDATE_INTERVAL_MS = 5000;                // Read sensors every 5s
constexpr unsigned long SENSOR_REFRESH_INTERVAL_MS = 5000;               // Frontend refresh interval
constexpr unsigned long HISTORY_REFRESH_INTERVAL_MS = 60000;             // Refresh history chart every 60s
constexpr unsigned long EMAIL_RETRY_INTERVAL_MS = 10UL * 60UL * 1000UL;  // Retry email after 10min

// --- History Storage ---
constexpr unsigned long HISTORY_SAMPLE_INTERVAL_MS = 5UL * 60UL * 1000UL;  // Sample every 5min
constexpr size_t HISTORY_MAX_SAMPLES = 1000;                               // Max samples in flash (rolling window)
constexpr size_t HISTORY_API_MAX_SAMPLES = 288;                            // Max samples returned by API (~24h at 5min)
constexpr uint32_t HISTORY_API_WINDOW_SECONDS = 24UL * 60UL * 60UL;        // 24 hour window
const char* HISTORY_FILE_PATH = "/humidity_history.csv";
const char* HISTORY_TMP_FILE_PATH = "/humidity_history.tmp";

// --- Email Schedule ---
// Daily summary sent after this time (local timezone)
constexpr int DAILY_EMAIL_HOUR = 8;
constexpr int DAILY_EMAIL_MINUTE = 0;

// --- Time & Timezone ---
const char* NTP_SERVER_1 = "pool.ntp.org";
const char* NTP_SERVER_2 = "time.google.com";
const char* TZ_INFO = "CET-1CEST,M3.5.0/2,M10.5.0/3";  // Italy timezone

// --- Email Service (Resend) ---
const char* RESEND_SUBJECT = "🌱 Lo stato della tua pianta";
const char* RESEND_URGENT_SUBJECT = "⚠️ Umidità sotto 30%: intervento necessario";
const char* RESEND_ENDPOINT = "https://api.resend.com/emails";

// ============================================================================
// HARDWARE INITIALIZATION
// ============================================================================

// Built-in OLED bus.
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
DHT dhtSensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

// Web server on port 80
ESP8266WebServer server(80);

// ============================================================================
// GLOBAL STATE
// ============================================================================

// --- Soil Moisture ---
int gHumidityRaw = 0;
int gHumidityPercent = 0;
float gAirTemperatureC = 0.0f;
float gAirHumidityPercent = 0.0f;
bool gDhtReadingValid = false;

// --- Wi-Fi State ---
String gWifiStatus = "WiFi...";
unsigned long gLastWifiRetryMs = 0;
unsigned long gLastWifiIpDisplayMs = 0;
bool gShowWifiIp = false;
bool gWasWifiConnected = false;
unsigned long gWifiDisconnectedSinceMs = 0;
unsigned long gLastInternetCheckMs = 0;
unsigned long gLastInternetOkMs = 0;
bool gInternetReachable = false;

// --- Email State ---
int gLastDailyEmailDayKey = -1;
unsigned long gLastEmailAttemptMs = 0;
String gEmailStatus = "Email idle";
String gLastEmailErrorDetail = "";
bool gWasBelowLowHumidityThreshold = false;

// --- History State ---
unsigned long gLastHistorySampleMs = 0;
bool gHistoryFsReady = false;
String gHistoryCacheJson = "{\"ok\":false,\"reason\":\"not_ready\",\"points\":[]}";
unsigned long gLastHistoryCacheBuildMs = 0;
bool gHistoryCacheDirty = true;

/**
 * Robust Wi-Fi connectivity check.
 * Some ESP8266 firmware states may transiently report a non-connected status
 * while the station still has a valid IP lease.
 */
bool isWifiConnected() {
  if (WiFi.status() == WL_CONNECTED || WiFi.isConnected()) {
    return true;
  }
  const IPAddress ip = WiFi.localIP();
  return ip[0] != 0 || ip[1] != 0 || ip[2] != 0 || ip[3] != 0;
}

void ensureOledBus() {
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
}

// ============================================================================
// SENSOR READING
// ============================================================================

/**
 * Read analog soil moisture sensor.
 */
void readHumiditySensor() {
  gHumidityRaw = analogRead(SOIL_SENSOR_PIN);
  gHumidityPercent = map(gHumidityRaw, ADC_DRY, ADC_WET, 0, 100);
  gHumidityPercent = constrain(gHumidityPercent, 0, 100);
}

/**
 * Read ambient temperature/humidity from DHT11.
 * On transient read failure, keep previous values and mark current sample invalid.
 */
void readDhtSensor() {
  const float temperatureC = dhtSensor.readTemperature();
  const float humidityPercent = dhtSensor.readHumidity();

  if (isnan(temperatureC) || isnan(humidityPercent)) {
    gDhtReadingValid = false;
    return;
  }

  gAirTemperatureC = temperatureC;
  gAirHumidityPercent = humidityPercent;
  gDhtReadingValid = true;
}

// ============================================================================
// DISPLAY
// ============================================================================

/**
 * Render current readings to OLED display.
 * Layout:
 * - Line 1: Title
 * - Line 2: Soil moisture %
 * - Line 3: Air temperature
 * - Line 4: Air humidity
 * - Line 5: Wi-Fi status
 */
void renderOled() {
  ensureOledBus();
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(0, 2);
  display.print("Umidita terreno");

  display.setCursor(0, 14);
  display.print("Umidita: ");
  display.print(gHumidityPercent);
  display.print("%");

  display.setCursor(0, 26);
  display.print("Temp: ");
  if (gDhtReadingValid) {
    display.print(gAirTemperatureC, 1);
    display.print("C");
  } else {
    display.print("--");
  }

  display.setCursor(0, 38);
  display.print("Humi: ");
  if (gDhtReadingValid) {
    display.print(gAirHumidityPercent, 0);
    display.print("%");
  } else {
    display.print("--");
  }

  display.setCursor(0, 50);
  display.print(gWifiStatus);
  display.display();
}

// ============================================================================
// WEB SERVER
// ============================================================================

/**
 * Build the main web page with dynamic placeholders replaced.
 */
String buildHomePage() {
  String html = FPSTR(HOMEPAGE_TEMPLATE);
  html.replace("__DEVICE_IP__", WiFi.localIP().toString());
  html.replace("__REFRESH_MS__", String(SENSOR_REFRESH_INTERVAL_MS));
  html.replace("__HISTORY_REFRESH_MS__", String(HISTORY_REFRESH_INTERVAL_MS));
  return html;
}

/**
 * Handle GET / → serve main dashboard page.
 */
void handleRoot() {
  server.send(200, "text/html", buildHomePage());
}

/**
 * Handle GET /api/humidity → return current analog sensor readings as JSON.
 * Returns: {raw, humidity, airTempC, airHumidity, airOk}
 */
void handleHumidityApi() {
  const String airTempJson = gDhtReadingValid ? String(gAirTemperatureC, 1) : "null";
  const String airHumidityJson = gDhtReadingValid ? String(gAirHumidityPercent, 0) : "null";
  const String json =
    "{\"raw\":" + String(gHumidityRaw) + ",\"humidity\":" + String(gHumidityPercent) + ",\"airTempC\":" + airTempJson + ",\"airHumidity\":" + airHumidityJson + ",\"airOk\":" + String(gDhtReadingValid ? "true" : "false") + "}";
  server.send(200, "application/json", json);
}

/**
 * Handle POST /api/restart → require HTTP auth, redirect to home, then reboot device.
 */
void handleRestartApi() {
  if (!server.authenticate(RESTART_USERNAME, RESTART_PASSWORD)) {
    server.requestAuthentication(BASIC_AUTH, "Restart ESP");
    return;
  }

  server.sendHeader("Location", "/");
  server.send(303, "text/plain", "");
  delay(200);
  ESP.restart();
}

// ============================================================================
// HISTORY STORAGE (LittleFS)
// ============================================================================

/**
 * Initialize LittleFS and create history file if needed.
 * Returns: true if filesystem ready, false on error.
 */
bool initHistoryStorage() {
  gHistoryFsReady = LittleFS.begin();
  if (!gHistoryFsReady) {
    return false;
  }

  if (!LittleFS.exists(HISTORY_FILE_PATH)) {
    File f = LittleFS.open(HISTORY_FILE_PATH, "w");
    if (!f) {
      gHistoryFsReady = false;
      return false;
    }
    f.close();
  }
  return true;
}

/**
 * Enforce rolling window retention policy on history file.
 * Keeps only the most recent HISTORY_MAX_SAMPLES entries.
 * Uses temporary file for atomic write.
 */
void enforceHistoryRetention() {
  if (!gHistoryFsReady) return;

  // Pass 1: count total lines
  File inputCount = LittleFS.open(HISTORY_FILE_PATH, "r");
  if (!inputCount) return;

  size_t totalLines = 0;
  while (inputCount.available()) {
    String line = inputCount.readStringUntil('\n');
    line.trim();
    if (line.length() > 0) ++totalLines;
    yield();
  }
  inputCount.close();

  if (totalLines <= HISTORY_MAX_SAMPLES) return;  // Within limit

  // Pass 2: copy recent samples to temp file
  const size_t skipLines = totalLines - HISTORY_MAX_SAMPLES;
  File input = LittleFS.open(HISTORY_FILE_PATH, "r");
  File output = LittleFS.open(HISTORY_TMP_FILE_PATH, "w");

  if (!input || !output) {
    if (input) input.close();
    if (output) output.close();
    return;
  }

  size_t lineIndex = 0;
  while (input.available()) {
    String line = input.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    if (lineIndex < skipLines) {
      ++lineIndex;
      yield();
      continue;
    }
    output.println(line);
    yield();
  }
  input.close();
  output.close();

  // Atomic replace: delete old, rename temp
  LittleFS.remove(HISTORY_FILE_PATH);
  LittleFS.rename(HISTORY_TMP_FILE_PATH, HISTORY_FILE_PATH);
}

/**
 * Append a single sample to history file.
 * Format: epoch,humidity\n
 */
void appendHistorySample(time_t epochSeconds, int humidityPercent) {
  if (!gHistoryFsReady) return;

  File history = LittleFS.open(HISTORY_FILE_PATH, "a");
  if (!history) return;

  history.print(static_cast<uint32_t>(epochSeconds));
  history.print(',');
  history.println(humidityPercent);
  history.close();

  enforceHistoryRetention();
  gHistoryCacheDirty = true;
}

/**
 * Store soil humidity sample at fixed interval.
 * Sampling starts only after filesystem + NTP are ready.
 */
void maybeStoreHistorySample() {
  if (!gHistoryFsReady || !isTimeSynced()) {
    return;
  }

  const unsigned long nowMs = millis();
  if (nowMs - gLastHistorySampleMs < HISTORY_SAMPLE_INTERVAL_MS) {
    return;
  }

  gLastHistorySampleMs = nowMs;
  appendHistorySample(time(nullptr), gHumidityPercent);
}

/**
 * Build JSON payload for humidity history chart.
 * Includes up to HISTORY_API_MAX_SAMPLES within last 24h when time is synced.
 */
String buildHistoryJson() {
  if (!gHistoryFsReady) {
    return "{\"ok\":false,\"reason\":\"fs_unavailable\",\"points\":[]}";
  }

  const time_t nowEpoch = time(nullptr);
  // If NTP is not ready yet, we keep serving recent samples without applying a time window.
  const bool canFilterLast24h = (nowEpoch > 0) && isTimeSynced();
  const time_t minEpoch = canFilterLast24h ? (nowEpoch - static_cast<time_t>(HISTORY_API_WINDOW_SECONDS)) : 0;

  File inputCount = LittleFS.open(HISTORY_FILE_PATH, "r");
  if (!inputCount) {
    return "{\"ok\":false,\"reason\":\"history_open_failed\",\"points\":[]}";
  }

  // Pass 1: count eligible rows so we can apply a stable "latest N" cap after 24h filtering.
  size_t eligibleLines = 0;
  while (inputCount.available()) {
    String line = inputCount.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) {
      continue;
    }

    const int comma = line.indexOf(',');
    if (comma <= 0 || comma >= static_cast<int>(line.length() - 1)) {
      continue;
    }

    String epoch = line.substring(0, comma);
    epoch.trim();
    if (epoch.length() == 0) {
      continue;
    }

    const time_t sampleEpoch = static_cast<time_t>(epoch.toInt());
    if (canFilterLast24h && sampleEpoch < minEpoch) {
      yield();
      continue;
    }

    ++eligibleLines;
    yield();
  }
  inputCount.close();

  const size_t skipEligibleLines = (eligibleLines > HISTORY_API_MAX_SAMPLES) ? (eligibleLines - HISTORY_API_MAX_SAMPLES) : 0;

  File input = LittleFS.open(HISTORY_FILE_PATH, "r");
  if (!input) {
    return "{\"ok\":false,\"reason\":\"history_open_failed\",\"points\":[]}";
  }

  // Pass 2: emit only the final eligible slice as JSON points.
  String json = "{\"ok\":true,\"points\":[";
  size_t eligibleIndex = 0;
  size_t emitted = 0;
  while (input.available()) {
    String line = input.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) {
      continue;
    }

    const int comma = line.indexOf(',');
    if (comma <= 0 || comma >= static_cast<int>(line.length() - 1)) {
      continue;
    }

    String epoch = line.substring(0, comma);
    String humidity = line.substring(comma + 1);
    epoch.trim();
    humidity.trim();
    if (epoch.length() == 0 || humidity.length() == 0) {
      continue;
    }

    const time_t sampleEpoch = static_cast<time_t>(epoch.toInt());
    if (canFilterLast24h && sampleEpoch < minEpoch) {
      yield();
      continue;
    }

    if (eligibleIndex < skipEligibleLines) {
      ++eligibleIndex;
      yield();
      continue;
    }
    ++eligibleIndex;

    if (emitted > 0) {
      json += ",";
    }
    json += "{\"t\":";
    json += epoch;
    json += ",\"h\":";
    json += humidity;
    json += "}";
    ++emitted;
    yield();
  }
  input.close();

  json += "],\"count\":";
  json += String(emitted);
  json += "}";
  return json;
}

/**
 * Handle GET /api/history → return 24h history as JSON.
 */
void handleHistoryApi() {
  const unsigned long nowMs = millis();
  const bool cacheExpired = (nowMs - gLastHistoryCacheBuildMs >= HISTORY_REFRESH_INTERVAL_MS);
  if (gHistoryCacheDirty || cacheExpired) {
    gHistoryCacheJson = buildHistoryJson();
    gLastHistoryCacheBuildMs = nowMs;
    gHistoryCacheDirty = false;
  }
  server.send(200, "application/json", gHistoryCacheJson);
}

// ============================================================================
// TIME & TIMEZONE
// ============================================================================

/**
 * Initialize NTP time sync with Italy timezone.
 */
void initTimeSync() {
  // Use timezone-aware NTP setup directly to avoid UTC/local offset mismatches.
  configTime(TZ_INFO, NTP_SERVER_1, NTP_SERVER_2);
}

/**
 * Check if NTP time has been synchronized.
 * Returns: true if time is synced (after Nov 2023).
 */
bool isTimeSynced() {
  return time(nullptr) > 1700000000;
}

/**
 * Resolve Resend host and return DNS result code from WiFi.hostByName.
 * Result 1 means success and outIp contains a valid address.
 */
int resolveResendHost(IPAddress& outIp) {
  return WiFi.hostByName("api.resend.com", outIp);
}

/**
 * Build unique day key for deduplication.
 * Format: YYYY*1000 + day_of_year
 * Returns: -1 on error.
 */
int buildLocalDayKey(time_t nowEpoch) {
  struct tm localTimeInfo;
  if (!localtime_r(&nowEpoch, &localTimeInfo)) {
    return -1;
  }
  return (localTimeInfo.tm_year + 1900) * 1000 + localTimeInfo.tm_yday;
}

// ============================================================================
// EMAIL NOTIFICATIONS
// ============================================================================

/**
 * Escape special characters for JSON strings.
 */
String jsonEscape(const String& input) {
  String out;
  out.reserve(input.length() + 16);
  for (size_t i = 0; i < input.length(); ++i) {
    const char c = input.charAt(i);
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"': out += "\\\""; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default: out += c; break;
    }
  }
  return out;
}

/**
 * Send email via Resend API with soil moisture data.
 * Retries once on failure for network resilience.
 * Returns: true if email sent successfully.
 */
bool sendDailyUpdateEmail(time_t nowEpoch, const char* subject, const char* emailHeadline, const char* deliveryContext) {
  if (strlen(RESEND_API_KEY) == 0) {
    gEmailStatus = "Email err: no key";
    gLastEmailErrorDetail = "";
    return false;
  }

  struct tm localTimeInfo;
  if (!localtime_r(&nowEpoch, &localTimeInfo)) {
    gEmailStatus = "Email err: time";
    gLastEmailErrorDetail = "";
    return false;
  }

  char timeHmBuffer[6];
  strftime(timeHmBuffer, sizeof(timeHmBuffer), "%H:%M", &localTimeInfo);

  // Build plain text email body.
  String textBody = String(emailHeadline) + ".\n";
  textBody += "Umidità terreno: " + String(gHumidityPercent) + "%\n";
  textBody += "Ora locale: " + String(timeHmBuffer);

  // Determine soil moisture level badge styling.
  String levelText = "Umido";
  String levelColor = "#166534";
  if (gHumidityPercent < 30) {
    levelText = "Secco";
    levelColor = "#b91c1c";
  } else if (gHumidityPercent < 65) {
    levelText = "Normale";
    levelColor = "#92400e";
  }

  // Build HTML email body.
  String htmlBody = FPSTR(EMAIL_TEMPLATE);
  htmlBody.replace("__LEVEL_TEXT__", levelText);
  htmlBody.replace("__LEVEL_COLOR__", levelColor);
  htmlBody.replace("__HUMIDITY__", String(gHumidityPercent));
  htmlBody.replace("__TIME_HM__", String(timeHmBuffer));
  htmlBody.replace("__EMAIL_HEADLINE__", String(emailHeadline));
  htmlBody.replace("__DELIVERY_CONTEXT__", String(deliveryContext));

  String payload = "{";
  payload += "\"from\":\"" + jsonEscape(String(RESEND_FROM_EMAIL)) + "\",";
  payload += "\"to\":[\"" + jsonEscape(String(RESEND_TO_EMAIL)) + "\"],";
  payload += "\"subject\":\"" + jsonEscape(String(subject)) + "\",";
  payload += "\"text\":\"" + jsonEscape(textBody) + "\",";
  payload += "\"html\":\"" + jsonEscape(htmlBody) + "\"";
  payload += "}";

  IPAddress resendIp;
  const int dnsResult = resolveResendHost(resendIp);
  const String dnsInfo = (dnsResult == 1) ? resendIp.toString() : ("lookup_failed(" + String(dnsResult) + ")");

  int httpCode = -1;
  String responseBody = "";
  bool beginFailed = false;
  for (uint8_t attempt = 0; attempt < 2; ++attempt) {
    BearSSL::WiFiClientSecure client;
    client.setInsecure();
    client.setBufferSizes(512, 512);
    client.setTimeout(15000);

    HTTPClient http;
    http.setTimeout(15000);
    http.setReuse(false);
    if (!http.begin(client, RESEND_ENDPOINT)) {
      beginFailed = true;
      httpCode = -1;
    } else {
      beginFailed = false;
      http.addHeader("Content-Type", "application/json");
      http.addHeader("Authorization", "Bearer " + String(RESEND_API_KEY));
      http.addHeader("User-Agent", "humidity-sensor-esp8266/1.0");
      httpCode = http.POST(payload);
      responseBody = http.getString();
      http.end();
    }

    if (httpCode > 0 || attempt == 1) {
      break;
    }
    delay(300);
  }

  const bool ok = (httpCode >= 200 && httpCode < 300);
  if (ok) {
    gEmailStatus = "Email sent";
    gLastEmailErrorDetail = responseBody.substring(0, 220);
  } else if (beginFailed) {
    gEmailStatus = "Email err: begin";
    gLastEmailErrorDetail = "dns:" + dnsInfo + " | rssi:" + String(WiFi.RSSI()) + " | heap:" + String(ESP.getFreeHeap());
  } else if (httpCode <= 0) {
    gEmailStatus = "Email fail " + String(httpCode);
    gLastEmailErrorDetail = HTTPClient::errorToString(httpCode) + " | dns:" + dnsInfo + " | rssi:" + String(WiFi.RSSI()) + " | heap:" + String(ESP.getFreeHeap());
  } else {
    gEmailStatus = "Email fail " + String(httpCode);
    gLastEmailErrorDetail = responseBody.substring(0, 220);
  }
  return ok;
}

/**
 * Send low-humidity alert once per dry event.
 * Edge-triggered: fires only on transition from >=30% to <30%.
 */
void maybeSendLowHumidityAlert() {
  if (gHumidityPercent >= 30) {
    gWasBelowLowHumidityThreshold = false;
    return;
  }

  if (gWasBelowLowHumidityThreshold) {
    return;
  }

  if (!isTimeSynced()) {
    gEmailStatus = "Low humid alert: wait NTP";
    gLastEmailErrorDetail = "";
    return;
  }

  gWasBelowLowHumidityThreshold = true;
  if (!isWifiConnected()) {
    gEmailStatus = "Low humid alert: no wifi";
    gLastEmailErrorDetail = "";
    return;
  }

  const time_t nowEpoch = time(nullptr);
  sendDailyUpdateEmail(
    nowEpoch,
    RESEND_URGENT_SUBJECT,
    "Allerta umidità bassa",
    "Invio automatico per soglia sotto 30%");
}

/**
 * Send daily scheduled email after configured local time.
 * At most one successful scheduled email per local day.
 */
void maybeSendScheduledEmail() {
  if (!isWifiConnected()) {
    return;
  }

  if (!isTimeSynced()) {
    gEmailStatus = "Email wait NTP";
    return;
  }

  const unsigned long nowMs = millis();
  if (nowMs - gLastEmailAttemptMs < EMAIL_RETRY_INTERVAL_MS) {
    return;
  }

  const time_t nowEpoch = time(nullptr);
  struct tm localTimeInfo;
  if (!localtime_r(&nowEpoch, &localTimeInfo)) {
    gEmailStatus = "Email err: time";
    return;
  }

  const bool isAfterTargetTime =
    (localTimeInfo.tm_hour > DAILY_EMAIL_HOUR) || (localTimeInfo.tm_hour == DAILY_EMAIL_HOUR && localTimeInfo.tm_min >= DAILY_EMAIL_MINUTE);

  if (!isAfterTargetTime) {
    return;
  }

  const int dayKey = buildLocalDayKey(nowEpoch);
  // One scheduled email per local day (after target time).
  if (dayKey < 0 || dayKey == gLastDailyEmailDayKey) {
    return;
  }

  IPAddress resendIp;
  const int dnsResult = resolveResendHost(resendIp);
  if (dnsResult != 1) {
    gLastEmailAttemptMs = nowMs;
    gEmailStatus = "Email no internet";
    gLastEmailErrorDetail = "DNS api.resend.com failed (" + String(dnsResult) + ") | rssi:" + String(WiFi.RSSI());
    return;
  }

  gLastEmailAttemptMs = nowMs;
  if (sendDailyUpdateEmail(nowEpoch, RESEND_SUBJECT, "Aggiornamento terreno", "Invio automatico giornaliero")) {
    gLastDailyEmailDayKey = dayKey;
  }
}

/**
 * Perform initial Wi-Fi connection attempt at boot.
 * Blocking but bounded by WIFI_CONNECT_TIMEOUT_MS.
 */
void connectWiFi() {
  gWifiStatus = "WiFi...";
  renderOled();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  const unsigned long startMs = millis();
  while (!isWifiConnected() && millis() - startMs < WIFI_CONNECT_TIMEOUT_MS) {
    delay(WIFI_CONNECT_POLL_MS);
  }

  if (isWifiConnected()) {
    gWasWifiConnected = true;
    gShowWifiIp = false;
    const unsigned long now = millis();
    gLastWifiIpDisplayMs = now;
    gLastInternetCheckMs = now;
    IPAddress resendIp;
    gInternetReachable = (resolveResendHost(resendIp) == 1);
    if (gInternetReachable) {
      gLastInternetOkMs = now;
      gWifiStatus = "Connected";
    } else {
      gLastInternetOkMs = 0;
      gWifiStatus = "No internet";
    }
  } else {
    gWasWifiConnected = false;
    gInternetReachable = false;
    gLastInternetCheckMs = 0;
    gLastInternetOkMs = 0;
    gWifiStatus = "No WiFi";
  }
  renderOled();
}

/**
 * Keep Wi-Fi connected during runtime with non-blocking retries.
 * Also verifies internet reachability via DNS and alternates status text.
 */
void maintainWiFi() {
  const unsigned long now = millis();

  if (isWifiConnected()) {
    gWifiDisconnectedSinceMs = 0;
    if (!gWasWifiConnected) {
      gWasWifiConnected = true;
      gShowWifiIp = false;
      gLastWifiIpDisplayMs = now;
      gLastInternetCheckMs = 0;
      gLastInternetOkMs = 0;
    }

    if (gLastInternetCheckMs == 0 || now - gLastInternetCheckMs >= INTERNET_CHECK_INTERVAL_MS) {
      gLastInternetCheckMs = now;
      IPAddress resendIp;
      gInternetReachable = (resolveResendHost(resendIp) == 1);
      if (gInternetReachable) {
        gLastInternetOkMs = now;
      }
    }

    if (!gInternetReachable) {
      gWifiStatus = "No internet";
      const bool shouldForceReconnect = (gLastInternetOkMs == 0) || (now - gLastInternetOkMs >= INTERNET_RECONNECT_GRACE_MS);
      if (shouldForceReconnect && now - gLastWifiRetryMs >= WIFI_RETRY_INTERVAL_MS) {
        gLastWifiRetryMs = now;
        WiFi.disconnect();
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      }
      return;
    }

    if (now - gLastWifiIpDisplayMs >= WIFI_DISPLAY_IP_MS) {
      gLastWifiIpDisplayMs = now;
      gShowWifiIp = !gShowWifiIp;
    }
    gWifiStatus = gShowWifiIp ? WiFi.localIP().toString() : "Connected";
    return;
  }

  if (gWasWifiConnected) {
    if (gWifiDisconnectedSinceMs == 0) {
      gWifiDisconnectedSinceMs = now;
      return;
    }
    if (now - gWifiDisconnectedSinceMs < WIFI_DISCONNECT_DEBOUNCE_MS) {
      return;
    }
  }

  gWasWifiConnected = false;
  gInternetReachable = false;
  gLastInternetCheckMs = 0;
  gLastInternetOkMs = 0;
  if (now - gLastWifiRetryMs >= WIFI_RETRY_INTERVAL_MS) {
    gLastWifiRetryMs = now;
    gShowWifiIp = false;
    gLastWifiIpDisplayMs = now;
    gWifiStatus = "WiFi...";
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  } else {
    gWifiStatus = "No WiFi. Retry...";
  }
}

/**
 * Register HTTP routes and start web server + OTA handler.
 */
void setupWebServer() {
  server.on("/", handleRoot);

  server.on("/api/humidity", handleHumidityApi);
  server.on("/api/history", handleHistoryApi);
  server.on("/api/restart", HTTP_POST, handleRestartApi);

  ElegantOTA.begin(&server, OTA_USERNAME, OTA_PASSWORD);
  server.begin();
}

/**
 * Initialize hardware, sensors, storage, network, and web services.
 */
void setup() {
  // Board OLED mapping: D5 (GPIO14) = SDA, D6 (GPIO12) = SCL.
  ensureOledBus();

  // Initialize OLED display.
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    // Halt if display initialization fails (critical hardware issue).
    while (true) {
      delay(1000);
    }
  }
  // Rotate display 180 degrees for proper viewing orientation.
  display.setRotation(2);

  // Initialize soil moisture sensor.
  pinMode(SOIL_SENSOR_PIN, INPUT);
  dhtSensor.begin();
  readHumiditySensor();
  readDhtSensor();
  renderOled();

  // Initialize history storage on LittleFS.
  initHistoryStorage();
  gLastHistorySampleMs = millis() - HISTORY_SAMPLE_INTERVAL_MS;
  gHistoryCacheDirty = true;

  // Connect to Wi-Fi and sync time via NTP.
  connectWiFi();
  initTimeSync();

  setupWebServer();
}

/**
 * Main runtime loop.
 * Keeps networking alive, refreshes sensors/display, persists history,
 * and serves HTTP/OTA requests.
 */
void loop() {
  static unsigned long lastUpdateMs = 0;
  const unsigned long now = millis();

  maintainWiFi();

  if (now - lastUpdateMs >= SENSOR_UPDATE_INTERVAL_MS) {
    lastUpdateMs = now;
    readHumiditySensor();
    readDhtSensor();
    renderOled();
  }

  maybeStoreHistorySample();
  maybeSendLowHumidityAlert();
  maybeSendScheduledEmail();

  server.handleClient();
  ElegantOTA.loop();
  delay(1);
}
