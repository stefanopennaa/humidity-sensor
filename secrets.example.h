#pragma once

/*
 * Project: Humidity Sensor - ESP8266 Plant Monitor
 * File: secrets.example.h
 * Purpose: Public-safe example configuration for local credentials.
 *
 * Changelog:
 * - 2026-05-03: Added template file to prevent publishing real credentials.
 */

// Copy this file to secrets.h and set real values locally.

// Wi-Fi
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// Resend API
const char* RESEND_API_KEY = "YOUR_RESEND_API_KEY";
const char* RESEND_FROM_EMAIL = "Plant Watcher <alerts@example.com>";
const char* RESEND_TO_EMAIL = "your-email@example.com";

// OTA credentials
const char* OTA_USERNAME = "YOUR_OTA_USERNAME";
const char* OTA_PASSWORD = "YOUR_OTA_PASSWORD";

// Restart endpoint credentials (separate from OTA)
const char* RESTART_USERNAME = "YOUR_RESTART_USERNAME";
const char* RESTART_PASSWORD = "YOUR_RESTART_PASSWORD";
