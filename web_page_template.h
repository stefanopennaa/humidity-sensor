#pragma once

#include <pgmspace.h>

/*
 * Project: Humidity Sensor - ESP8266 Plant Monitor
 * File: web_page_template.h
 * Purpose: Web dashboard HTML/CSS/JS template with runtime placeholders.
 *
 * Changelog:
 * - 2026-05-04: Added ambient temperature/humidity fields from DHT11 and refreshed UI labels/icons.
 * - 2026-05-03: Reorganized header comments and standardized formatting (form only).
 */

const char HOMEPAGE_TEMPLATE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width,initial-scale=1">
    <link rel="icon"
        href="data:image/svg+xml,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 100 100'><text y='.9em' font-size='90'>🌱</text></svg>">
    <title>Soil Humidity</title>
    <style>
      :root {
        --bg-1: #0f172a;
        --bg-2: #111827;
        --card: rgba(17, 24, 39, 0.75);
        --text: #e5e7eb;
        --muted: #94a3b8;
        --accent: #22c55e;
        --accent-soft: rgba(34, 197, 94, 0.18);
        --warn: #f59e0b;
        --danger: #ef4444;
      }

      * { box-sizing: border-box; }

      body {
        margin: 0;
        min-height: 100vh;
        padding: 24px;
        display: flex;
        align-items: center;
        justify-content: center;
        font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, sans-serif;
        background:
          radial-gradient(1200px 700px at 10% -10%, #1e293b 0%, transparent 55%),
          radial-gradient(900px 600px at 100% 120%, #064e3b 0%, transparent 50%),
          linear-gradient(135deg, var(--bg-1), var(--bg-2));
        color: var(--text);
      }

      .layout {
        width: min(980px, 100%);
        display: grid;
        grid-template-columns: repeat(2, minmax(0, 1fr));
        grid-template-areas: "soil history";
        gap: 18px;
        align-items: stretch;
        justify-content: center;
      }

      .card-soil { grid-area: soil; }
      .card-history { grid-area: history; }

      .card {
        width: 100%;
        height: 100%;
        display: flex;
        flex-direction: column;
        background: var(--card);
        border: 1px solid rgba(148, 163, 184, 0.18);
        border-radius: 18px;
        padding: 22px;
        backdrop-filter: blur(6px);
        box-shadow: 0 14px 34px rgba(0, 0, 0, 0.35);
      }

      .title-row {
        display: flex;
        align-items: center;
        justify-content: space-between;
        gap: 12px;
      }

      h2 {
        margin: 0;
        font-size: 1.15rem;
        font-weight: 700;
        letter-spacing: 0.2px;
      }

      .badge {
        font-size: 0.75rem;
        color: #bbf7d0;
        background: var(--accent-soft);
        border: 1px solid rgba(34, 197, 94, 0.45);
        border-radius: 999px;
        padding: 4px 10px;
      }

      .value {
        margin-top: 16px;
        display: flex;
        align-items: baseline;
        gap: 6px;
      }

      .value #pct {
        font-size: 3rem;
        font-weight: 800;
        line-height: 1;
      }

      .value .unit {
        font-size: 1.2rem;
        color: var(--muted);
      }

      .progress-wrap {
        margin-top: 14px;
        height: 10px;
        width: 100%;
        background: rgba(148, 163, 184, 0.2);
        border-radius: 999px;
        overflow: hidden;
      }

      .progress {
        height: 100%;
        width: 0%;
        background: linear-gradient(90deg, #60a5fa, var(--accent));
        transition: width 350ms ease;
      }

      .meta {
        margin-top: 16px;
        display: grid;
        gap: 8px;
        color: var(--muted);
        font-size: 0.95rem;
      }

      .meta strong { color: var(--text); }

      .state {
        margin-top: 12px;
        font-size: 0.85rem;
        color: var(--muted);
      }

      .chart-title {
        margin: 0 0 12px 0;
        font-size: 0.88rem;
        color: var(--muted);
      }

      #historyChart {
        width: 100%;
        height: 220px;
        display: block;
      }

      #historyInfo {
        margin-top: 12px;
        font-size: 0.78rem;
        color: var(--muted);
      }

      .actions {
        margin-top: 18px;
        display: flex;
        align-items: center;
        justify-content: flex-start;
        gap: 8px;
        flex-wrap: wrap;
      }

      .btn {
        display: inline-flex;
        align-items: center;
        justify-content: center;
        color: #dcfce7;
        text-decoration: none;
        border: 1px solid rgba(34, 197, 94, 0.5);
        background: rgba(34, 197, 94, 0.18);
        border-radius: 10px;
        padding: 10px 14px;
        min-height: 42px;
        font-weight: 600;
        font-size: 0.9rem;
        cursor: pointer;
        white-space: nowrap;
      }

      #restartBtn {
        color: #fee2e2;
        border: 1px solid rgba(239, 68, 68, 0.6);
        background: rgba(239, 68, 68, 0.2);
      }

      .last-update {
        margin-top: 18px;
        font-size: 0.78rem;
        color: var(--muted);
      }

      .card-history #historyInfo {
        margin-top: auto;
        padding-top: 12px;
      }

      @media (max-width: 640px) {
        body {
          padding: 12px;
          align-items: flex-start;
        }

        .layout {
          grid-template-columns: 1fr;
          grid-template-areas:
            "soil"
            "history";
          gap: 12px;
        }

        .card {
          width: 100%;
          border-radius: 14px;
          padding: 16px;
        }

        .title-row {
          align-items: flex-start;
          flex-direction: column;
          gap: 8px;
        }

        h2 {
          font-size: 1.05rem;
        }

        .badge {
          font-size: 0.72rem;
        }

        .value {
          margin-top: 12px;
          gap: 4px;
        }

        .value #pct {
          font-size: clamp(2.2rem, 13vw, 2.8rem);
        }

        .value .unit {
          font-size: 1rem;
        }

        .meta {
          font-size: 0.9rem;
          gap: 6px;
        }

        .state {
          font-size: 0.82rem;
          line-height: 1.4;
        }

        #historyChart {
          height: 150px;
        }

        .actions {
          margin-top: 14px;
          flex-direction: column;
          align-items: stretch;
          gap: 8px;
        }

        .btn {
          width: 100%;
          text-align: center;
        }

        .last-update {
          width: 100%;
          text-align: center;
          font-size: 0.76rem;
        }
      }
    </style>
  </head>
  <body>
    <div class="layout">
      <div class="card card-soil">
        <div class="title-row">
          <h2>Umidità terreno</h2>
          <span class="badge" id="level">In attesa</span>
        </div>

        <div class="value">
          <span id="pct">--</span>
          <span class="unit">%</span>
        </div>

        <div class="progress-wrap">
          <div class="progress" id="bar"></div>
        </div>

        <div class="meta">
          <div>Temp ambiente: <strong id="airTemp">--</strong></div>
          <div>Umidita ambiente: <strong id="airHumidity">--</strong></div>
          <div>IP: <strong>__DEVICE_IP__</strong></div>
        </div>

        <div class="actions">
          <a class="btn" href="/update">OTA Update</a>
          <button class="btn" id="restartBtn" type="button">Restart ESP</button>
        </div>
        <div class="last-update" id="stateText">Ultimo aggiornamento: in attesa...</div>
      </div>

      <div class="card card-history">
        <p class="chart-title">Storico umidità (ultime 24h)</p>
        <svg id="historyChart" viewBox="0 0 320 160" preserveAspectRatio="none" aria-label="Grafico storico umidità"></svg>
        <div id="historyInfo">Storico: in caricamento...</div>
      </div>
    </div>
    <script>
      function updateLevel(humidity) {
        const badge = document.getElementById('level');
        if (humidity < 30) {
          badge.textContent = 'Secco';
          badge.style.color = '#fecaca';
          badge.style.background = 'rgba(239, 68, 68, 0.18)';
          badge.style.borderColor = 'rgba(239, 68, 68, 0.5)';
          return;
        }
        if (humidity < 65) {
          badge.textContent = 'Normale';
          badge.style.color = '#fef3c7';
          badge.style.background = 'rgba(245, 158, 11, 0.18)';
          badge.style.borderColor = 'rgba(245, 158, 11, 0.5)';
          return;
        }
        badge.textContent = 'Umido';
        badge.style.color = '#bbf7d0';
        badge.style.background = 'rgba(34, 197, 94, 0.18)';
        badge.style.borderColor = 'rgba(34, 197, 94, 0.5)';
      }

      async function refreshHumidity() {
        const stateText = document.getElementById('stateText');
        try {
          const response = await fetch('/api/humidity');
          if (!response.ok) {
            throw new Error('HTTP ' + response.status);
          }

          const data = await response.json();
          const humidity = Number(data.humidity);
          const airTempC = Number(data.airTempC);
          const airHumidity = Number(data.airHumidity);
          const airOk = (data.airOk === true || data.airOk === 1 || data.airOk === 'true');

          document.getElementById('pct').textContent = humidity;
          document.getElementById('airTemp').textContent =
            (airOk && Number.isFinite(airTempC)) ? airTempC.toFixed(1) + ' C' : '--';
          document.getElementById('airHumidity').textContent =
            (airOk && Number.isFinite(airHumidity)) ? airHumidity.toFixed(0) + ' %' : '--';
          document.getElementById('bar').style.width = Math.max(0, Math.min(100, humidity)) + '%';
          updateLevel(humidity);
          stateText.textContent = 'Ultimo aggiornamento: ' + new Date().toLocaleTimeString();

        } catch (error) {
          stateText.textContent = 'Errore aggiornamento dati';
        }
      }

      function drawHistoryChart(points) {
        const svg = document.getElementById('historyChart');
        const info = document.getElementById('historyInfo');
        const width = 320;
        const height = 160;
        // Keep drawing math explicit: margins define a stable plot area for axes + labels.
        const margin = { top: 10, right: 10, bottom: 24, left: 32 };
        const chartWidth = width - margin.left - margin.right;
        const chartHeight = height - margin.top - margin.bottom;

        if (!Array.isArray(points) || points.length < 2) {
          svg.innerHTML = '';
          info.textContent = 'Storico: dati insufficienti';
          return;
        }

        const xs = points.map((_, i) => margin.left + (i * chartWidth / (points.length - 1)));
        const ys = points.map((p) => {
          const h = Math.max(0, Math.min(100, Number(p.h)));
          return margin.top + ((100 - h) / 100) * chartHeight;
        });

        let path = '';
        for (let i = 0; i < xs.length; i++) {
          path += (i === 0 ? 'M' : ' L') + xs[i].toFixed(2) + ' ' + ys[i].toFixed(2);
        }

        const yTicks = [0, 25, 50, 75, 100];
        let yTickSvg = '';
        for (const tick of yTicks) {
          // Y axis inverts values (0 at bottom, 100 at top), so transform with (100 - tick).
          const y = margin.top + ((100 - tick) / 100) * chartHeight;
          yTickSvg +=
            '<line x1="' + margin.left + '" y1="' + y.toFixed(2) + '" x2="' + (width - margin.right) + '" y2="' + y.toFixed(2) + '" stroke="rgba(148,163,184,0.16)" stroke-width="1"/>' +
            '<text x="' + (margin.left - 6) + '" y="' + (y + 3).toFixed(2) + '" text-anchor="end" font-size="8" fill="rgba(148,163,184,0.95)">' + tick + '</text>';
        }

        const first = points[0];
        const last = points[points.length - 1];
        const firstDate = new Date(Number(first.t) * 1000);
        const lastDate = new Date(Number(last.t) * 1000);
        const startLabel = firstDate.toLocaleDateString('it-IT', { day: 'numeric', month: 'numeric' }) + ' ' +
          firstDate.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
        const endLabel = lastDate.toLocaleDateString('it-IT', { day: 'numeric', month: 'numeric' }) + ' ' +
          lastDate.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });

        info.textContent = 'Ultimo campione: ' + lastDate.toLocaleString();

        svg.innerHTML =
          '<rect x="0" y="0" width="' + width + '" height="' + height + '" fill="transparent"></rect>' +
          yTickSvg +
          '<line x1="' + margin.left + '" y1="' + margin.top + '" x2="' + margin.left + '" y2="' + (height - margin.bottom) + '" stroke="rgba(148,163,184,0.45)" stroke-width="1"/>' +
          '<line x1="' + margin.left + '" y1="' + (height - margin.bottom) + '" x2="' + (width - margin.right) + '" y2="' + (height - margin.bottom) + '" stroke="rgba(148,163,184,0.45)" stroke-width="1"/>' +
          '<text x="' + margin.left + '" y="' + (height - 6) + '" font-size="8" fill="rgba(148,163,184,0.95)">' + startLabel + '</text>' +
          '<text x="' + (width - margin.right) + '" y="' + (height - 6) + '" text-anchor="end" font-size="8" fill="rgba(148,163,184,0.95)">' + endLabel + '</text>' +
          '<path d="' + path + '" fill="none" stroke="#22c55e" stroke-width="2.2" stroke-linejoin="round" stroke-linecap="round"/>';
      }

      async function refreshHistory() {
        const info = document.getElementById('historyInfo');
        try {
          const response = await fetch('/api/history');
          if (!response.ok) {
            throw new Error('HTTP ' + response.status);
          }
          const data = await response.json();
          if (!data.ok) {
            throw new Error(data.reason || 'history_unavailable');
          }
          drawHistoryChart(data.points || []);
        } catch (error) {
          info.textContent = 'Storico: errore caricamento';
        }
      }

      async function requestRestart() {
        const stateText = document.getElementById('stateText');
        stateText.textContent = 'Autenticazione richiesta...';

        const form = document.createElement('form');
        form.method = 'POST';
        form.action = '/api/restart';
        form.style.display = 'none';
        document.body.appendChild(form);
        form.submit();
      }

      document.getElementById('restartBtn').addEventListener('click', requestRestart);

      refreshHumidity();
      refreshHistory();
      setInterval(refreshHumidity, __REFRESH_MS__);
      setInterval(refreshHistory, __REFRESH_MS__);
    </script>
  </body>
</html>
)rawliteral";
