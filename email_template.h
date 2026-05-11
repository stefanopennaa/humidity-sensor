#pragma once

#include <pgmspace.h>

/*
 * Project: Humidity Sensor - ESP8266 Plant Monitor
 * File: email_template.h
 * Purpose: HTML email template with placeholders for runtime sensor data.
 *
 * Changelog:
 * - 2026-05-11: Added ambient temperature/humidity cards to email template.
 * - 2026-05-03: Reorganized header comments and standardized formatting (form only).
 */

const char EMAIL_TEMPLATE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="it" xmlns="http://www.w3.org/1999/xhtml">
  <head>
    <meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <meta name="x-apple-disable-message-reformatting">
    <title>Aggiornamento Umidità</title>
    <style type="text/css">
      :root {
        color-scheme: light dark;
        supported-color-schemes: light dark;
      }
      body,
      table,
      td,
      p,
      a {
        -webkit-text-size-adjust: 100%;
        -ms-text-size-adjust: 100%;
      }
      table {
        border-collapse: collapse !important;
      }
      img {
        border: 0;
        outline: none;
        text-decoration: none;
        -ms-interpolation-mode: bicubic;
      }
      body {
        margin: 0 !important;
        padding: 0 !important;
        width: 100% !important;
        height: 100% !important;
        background-color: #f4f7fb;
      }

      @media screen and (max-width: 600px) {
        .container {
          width: 100% !important;
        }
        .mobile-padding {
          padding-left: 16px !important;
          padding-right: 16px !important;
        }
        .stack-column,
        .stack-column-cell {
          display: block !important;
          width: 100% !important;
        }
        .stack-column-cell {
          padding-left: 0 !important;
          padding-right: 0 !important;
        }
        .stack-column-cell-secondary {
          padding-top: 10px !important;
        }
        .headline {
          font-size: 22px !important;
          line-height: 28px !important;
        }
        .value {
          font-size: 26px !important;
        }
      }

      @media (prefers-color-scheme: dark) {
        body,
        .page-bg {
          background-color: #020617 !important;
        }
        .panel {
          background-color: #0f172a !important;
          border-color: #1e293b !important;
        }
        .headline {
          color: #e2e8f0 !important;
        }
        .soft-card {
          background-color: #111827 !important;
          border-color: #1e293b !important;
        }
        .label {
          color: #94a3b8 !important;
        }
        .value-main {
          color: #86efac !important;
        }
        .value-secondary {
          color: #cbd5e1 !important;
        }
        .footer {
          background-color: #111827 !important;
          border-top-color: #1e293b !important;
        }
        .footer-text {
          color: #94a3b8 !important;
        }
        .link {
          color: #93c5fd !important;
        }
      }
    </style>
  </head>
  <body style="margin: 0; padding: 0; background-color: #f4f7fb; font-family: Arial, Helvetica, sans-serif; color: #1f2937;">
    <div style="display: none; max-height: 0; overflow: hidden; opacity: 0; mso-hide: all; color: transparent;">
      Ecco l'ultimo aggiornamento dell'umidità del terreno.
    </div>

    <table role="presentation" class="page-bg" width="100%" cellspacing="0" cellpadding="0" border="0" style="background-color: #f4f7fb;">
      <tr>
        <td align="center" style="padding: 24px 12px;">
          <table role="presentation" class="container" width="100%" cellspacing="0" cellpadding="0" border="0" style="width: 100%; max-width: 560px; border-collapse: separate !important;">
            <tr>
              <td class="panel" style="background-color: #ffffff; border: 1px solid #d8e3f0; border-radius: 18px;">
                <table role="presentation" width="100%" cellspacing="0" cellpadding="0" border="0">
                  <tr>
                    <td class="mobile-padding" style="padding: 28px 24px 22px 24px;">
                      <h2 class="headline" style="margin: 0 0 20px 0; font-size: 24px; line-height: 30px; color: #0f172a;">
                        __EMAIL_HEADLINE__
                      </h2>
                    </td>
                  </tr>
                  <tr>
                    <td class="mobile-padding" style="padding: 0 24px 18px 24px;">
                      <table role="presentation" width="100%" cellspacing="0" cellpadding="0" border="0" style="margin-bottom: 16px;">
                        <tr class="stack-column">
                          <td class="stack-column-cell" style="width: 50%; vertical-align: top; padding-right: 9px;">
                            <table role="presentation" class="soft-card" width="100%" cellspacing="0" cellpadding="0" border="0" style="border-collapse: separate !important; border-spacing: 0; border: 1px solid #d8e3f0; border-radius: 12px; background-color: #f8fafc;">
                              <tr>
                                <td height="50" style="height: 50px; padding: 12px; vertical-align: top;">
                                  <p class="label" style="margin: 0; font-size: 12px; color: #64748b;">Umidità terreno</p>
                                  <p class="value value-main" style="margin: 6px 0 0 0; font-size: 28px; line-height: 1; font-weight: 700; color: #16a34a;">__HUMIDITY__%</p>
                                </td>
                              </tr>
                            </table>
                          </td>
                          <td class="stack-column-cell stack-column-cell-secondary" style="width: 50%; vertical-align: top; padding-left: 9px;">
                            <table role="presentation" class="soft-card" width="100%" cellspacing="0" cellpadding="0" border="0" style="border-collapse: separate !important; border-spacing: 0; border: 1px solid #d8e3f0; border-radius: 12px; background-color: #f8fafc;">
                              <tr>
                                <td height="50" style="height: 50px; padding: 12px; vertical-align: top;">
                                  <p class="label" style="margin: 0; font-size: 12px; color: #64748b;">Stato terreno</p>
                                  <p class="value value-secondary" style="margin: 6px 0 0 0; font-size: 28px; line-height: 1; font-weight: 700; color: __LEVEL_COLOR__;">__LEVEL_TEXT__</p>
                                </td>
                              </tr>
                            </table>
                          </td>
                        </tr>
                      </table>
                    </td>
                  </tr>
                  <tr>
                    <td class="mobile-padding" style="padding: 0 24px 18px 24px;">
                      <table role="presentation" width="100%" cellspacing="0" cellpadding="0" border="0" style="margin-bottom: 16px;">
                        <tr class="stack-column">
                          <td class="stack-column-cell" style="width: 50%; vertical-align: top; padding-right: 9px;">
                            <table role="presentation" class="soft-card" width="100%" cellspacing="0" cellpadding="0" border="0" style="border-collapse: separate !important; border-spacing: 0; border: 1px solid #d8e3f0; border-radius: 12px; background-color: #f8fafc;">
                              <tr>
                                <td height="50" style="height: 50px; padding: 12px; vertical-align: top;">
                                  <p class="label" style="margin: 0; font-size: 12px; color: #64748b;">Temp. ambiente</p>
                                  <p class="value value-secondary" style="margin: 6px 0 0 0; font-size: 28px; line-height: 1; font-weight: 700; color: #334155;">__AIR_TEMP__</p>
                                </td>
                              </tr>
                            </table>
                          </td>
                          <td class="stack-column-cell stack-column-cell-secondary" style="width: 50%; vertical-align: top; padding-left: 9px;">
                            <table role="presentation" class="soft-card" width="100%" cellspacing="0" cellpadding="0" border="0" style="border-collapse: separate !important; border-spacing: 0; border: 1px solid #d8e3f0; border-radius: 12px; background-color: #f8fafc;">
                              <tr>
                                <td height="50" style="height: 50px; padding: 12px; vertical-align: top;">
                                  <p class="label" style="margin: 0; font-size: 12px; color: #64748b;">Umidità ambiente</p>
                                  <p class="value value-secondary" style="margin: 6px 0 0 0; font-size: 28px; line-height: 1; font-weight: 700; color: #334155;">__AIR_HUMIDITY__</p>
                                </td>
                              </tr>
                            </table>
                          </td>
                        </tr>
                      </table>
                    </td>
                  </tr>
                  <tr>
                    <td class="mobile-padding" style="padding: 0 24px 20px 24px;">
                      <table role="presentation" class="soft-card" width="100%" cellspacing="0" cellpadding="0" border="0" style="border-collapse: separate !important; border-spacing: 0; border: 1px solid #d8e3f0; border-radius: 12px; background-color: #f8fafc;">
                        <tr>
                          <td style="height: 50px; padding: 12px; vertical-align: top;">
                            <p class="label" style="margin: 0; font-size: 12px; color: #64748b;">Ora locale</p>
                            <p class="value value-secondary" style="margin: 6px 0 0 0; font-size: 28px; line-height: 1; font-weight: 700; color: #334155;">__TIME_HM__</p>
                          </td>
                        </tr>
                      </table>
                    </td>
                  </tr>
                  <tr>
                    <td class="mobile-padding footer" style="padding: 14px 24px; border-top: 1px solid #d8e3f0; background-color: #f8fafc; border-radius: 0 0 18px 18px;">
                      <p class="footer-text" style="margin: 0; font-size: 12px; line-height: 18px; color: #64748b;">
                        __DELIVERY_CONTEXT__ •
                        <a class="link" href="mailto:alerts@example.com" style="color: #2c7be5; text-decoration: none;">alerts@example.com</a>
                      </p>
                    </td>
                  </tr>
                </table>
              </td>
            </tr>
          </table>
        </td>
      </tr>
    </table>
  </body>
</html>
)rawliteral";
