#pragma once
#include <Arduino.h>

// Kept outside the .ino so Arduino's automatic prototype generator
// never tries to parse JavaScript functions as C++.
static const char INDEX_HTML[] PROGMEM = R"DALIWEB(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>DALI Service Tool</title>
<style>
:root{color-scheme:dark;--bg:#111318;--card:#191d24;--line:#2a303a;--txt:#e9edf3;--muted:#98a2b3;--blue:#579dff;--red:#ff6363;--green:#57c785;--amber:#e3ad4f}
*{box-sizing:border-box} body{margin:0;font-family:Inter,system-ui,-apple-system,Segoe UI,Arial;background:var(--bg);color:var(--txt)}
main{max-width:1180px;margin:auto;padding:22px}.top{display:flex;justify-content:space-between;gap:20px;align-items:flex-start;flex-wrap:wrap}
h1{margin:0 0 5px;font-size:28px}h2{margin:0 0 14px;font-size:18px}.sub,.muted{color:var(--muted)}
.grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:16px;margin-top:16px}.card{background:var(--card);border:1px solid var(--line);border-radius:14px;padding:16px}
.full{grid-column:1/-1}.actions{display:flex;gap:9px;flex-wrap:wrap}.row{display:flex;gap:10px;align-items:center;flex-wrap:wrap}
button{border:1px solid #3b4552;background:#232a34;color:var(--txt);padding:9px 12px;border-radius:9px;cursor:pointer;font-weight:600;text-transform:uppercase;letter-spacing:.02em}
button:hover{border-color:var(--blue)}button.primary{background:#214d85;border-color:#3f7dca}.danger{background:#522626;border-color:#8b4141}.danger:hover{border-color:var(--red)}
button:disabled{opacity:.45;cursor:not-allowed}input,select{background:#101319;color:var(--txt);border:1px solid #37404c;border-radius:8px;padding:9px}
.badge{display:inline-block;padding:5px 9px;border-radius:999px;background:#222a34;border:1px solid #35404d;font-size:13px}.ok{color:var(--green)}.bad{color:var(--red)}
.progress{height:8px;background:#0d0f13;border-radius:99px;overflow:hidden;margin-top:10px}.progress>div{height:100%;background:var(--blue);width:0}
table{width:100%;border-collapse:collapse;font-size:14px}th,td{border-bottom:1px solid var(--line);padding:9px 7px;text-align:left}th{color:var(--muted);font-weight:600}
.log{background:#0b0d10;border:1px solid #262c34;border-radius:10px;padding:12px;white-space:pre-wrap;font:12px ui-monospace,SFMono-Regular,Menlo,monospace;max-height:430px;overflow:auto}
.small{font-size:12px}.warn{color:var(--amber)}.sep{height:1px;background:var(--line);margin:14px 0}
.help{display:inline-flex;align-items:center;justify-content:center;width:17px;height:17px;margin-left:5px;border:1px solid #596577;border-radius:50%;color:#b7c1cf;font-size:11px;font-weight:700;cursor:help;vertical-align:middle;user-select:none}
.tooltip{position:fixed;display:none;width:290px;max-width:70vw;padding:9px 11px;border:1px solid #3b4552;border-radius:8px;background:#0b0e13;color:#e9edf3;font-size:12px;font-weight:400;line-height:1.35;white-space:normal;box-shadow:0 8px 24px rgba(0,0,0,.35);pointer-events:none;z-index:99999}
.help:hover{color:#fff;border-color:var(--blue)}
.device-main{cursor:default}
.device-detail td{padding:0;border-bottom:1px solid var(--line)}
.device-card{display:none;padding:16px 18px 18px;background:#11161d}
.device-card.open{display:block}
.device-grid{display:grid;grid-template-columns:repeat(4,minmax(150px,1fr));gap:10px 20px}
.device-field{padding:7px 0;border-bottom:1px solid #222a35}
.device-field .k{display:block;color:#8f9aaa;font-size:11px;text-transform:uppercase;letter-spacing:.04em}
.device-field .v{display:block;margin-top:3px;color:#edf2f7;font-size:14px}
.scene-list{display:flex;flex-wrap:wrap;gap:6px;margin-top:8px}
.scene-chip{padding:4px 7px;border:1px solid #303a47;border-radius:6px;font-size:11px;color:#b8c1cd}
.expand-btn{min-width:34px;padding-left:10px;padding-right:10px}
.tc{display:inline-flex;align-items:center;gap:7px;margin-left:8px;vertical-align:middle}
.tc input[type=range]{-webkit-appearance:none;appearance:none;width:130px;height:6px;padding:0;border:0;border-radius:99px;background:linear-gradient(90deg,#9ccaff,#f4f1ea,#ffb35c);cursor:pointer}
.tc input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:16px;height:16px;border-radius:50%;background:#e9edf3;border:2px solid #232a34;box-shadow:0 0 0 1px #59657780}
.tc input[type=range]::-moz-range-thumb{width:12px;height:12px;border-radius:50%;background:#e9edf3;border:2px solid #232a34}
.tc input[type=range]:disabled{opacity:.45;cursor:not-allowed}
.tc .tc-k{min-width:52px;font-size:12px;color:var(--muted);font-variant-numeric:tabular-nums}
.tc.lvl input[type=range]{background:linear-gradient(90deg,#1b2029,#6b7686,#fff6dc)}
.tc.lvl .tc-k{min-width:74px}
.sliders{display:flex;flex-wrap:wrap;gap:4px 14px;margin-top:8px}
.sliders .tc{margin-left:0}
@media(max-width:900px){.device-grid{grid-template-columns:repeat(2,minmax(140px,1fr))}}
.lbl{color:var(--muted);font-size:12px;max-width:160px;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.edit-btn{padding:2px 6px;font-size:11px;margin-left:4px}
.flags{display:flex;flex-wrap:wrap;gap:4px;margin-top:4px}
.flag{padding:2px 6px;border-radius:6px;font-size:11px;border:1px solid #3b4552;color:#c6cfdb}
.flag.bad{border-color:#8b4141;color:var(--red)}.flag.warn{border-color:#7a6230;color:var(--amber)}
.tools{display:flex;flex-wrap:wrap;gap:8px;align-items:center;margin-bottom:14px}
.sub-h{margin:16px 0 8px;color:#8f9aaa;font-size:11px;text-transform:uppercase;letter-spacing:.04em}
.form-row{display:flex;flex-wrap:wrap;gap:10px;align-items:flex-end}
.form-row label{display:flex;flex-direction:column;gap:4px;font-size:12px;color:var(--muted)}
.form-row input{width:82px;padding:6px 8px}
.chk-grid{display:grid;grid-template-columns:repeat(8,auto);gap:6px 14px;justify-content:start}
.chk-grid label{display:flex;gap:5px;align-items:center;font-size:13px}
.scene-grid{display:grid;grid-template-columns:repeat(4,minmax(150px,1fr));gap:8px 14px}
.scene-grid label{display:flex;gap:6px;align-items:center;font-size:12px;color:var(--muted)}
.scene-grid input{width:64px;padding:5px 7px}
.scene-grid button{padding:4px 8px}
@media(max-width:900px){.scene-grid{grid-template-columns:repeat(2,minmax(140px,1fr))}.chk-grid{grid-template-columns:repeat(4,auto)}}
.mono{font:12px ui-monospace,SFMono-Regular,Menlo,monospace}
.hexdump{background:#0b0d10;border:1px solid #262c34;border-radius:10px;padding:10px 12px;overflow:auto;white-space:pre}
.scroll-table{max-height:380px;overflow:auto;border:1px solid #262c34;border-radius:10px}
.scroll-table table{font-size:12px}.scroll-table th{position:sticky;top:0;background:var(--card)}
.k-tx{color:#8fc1ff}.k-reply{color:var(--green)}.k-foreign{color:var(--amber)}.k-err{color:var(--red)}
canvas{width:100%;height:150px;background:#0b0d10;border:1px solid #262c34;border-radius:10px;display:block}

@media(max-width:760px){.grid{grid-template-columns:1fr}.full{grid-column:auto}table{display:block;overflow:auto}}
</style>
</head>
<body>
<main>
  <div class="top">
    <div>
      <h1>DALI Service Tool</h1>
      <div class="sub">ESP32-C6 Pico + Waveshare Pico-DALI2 · GPIO14 TX / GPIO5 RX</div>
    </div>
    <div class="row">
      <select id="langSel" aria-label="Language" onchange="setLang(this.value)">
        <option value="en">English</option>
        <option value="ru">Русский</option>
      </select>
      <span class="badge" id="busyBadge">IDLE</span>
      <span class="badge">Found: <b id="foundCount">0</b></span>
    </div>
  </div>

  <div class="progress"><div id="progressBar"></div></div>
  <div class="small muted" id="opText" style="margin-top:6px">Idle</div>

  <div class="grid">
    <section class="card">
      <h2>Scanning <span class="help" data-tip="Scan queries short addresses SA0…SA63 to find DALI control gear that currently responds.">?</span></h2>
      <div class="actions">
        <button class="primary op" onclick="start('/api/scan')">SCAN</button>
        <button class="op" onclick="start('/api/scan?detailed=1')">DETAILED SCAN <span class="help" data-tip="Queries each responding short address for additional data: actual level, device type, version and group membership.">?</span></button>
        <button onclick="downloadReport()">Download report</button>
      </div>
      <p class="muted small">Scan uses QUERY STATUS on SA0…SA63. Detailed Scan also reads level, device type, version and group membership.</p>
      <div class="small">Compared with previous scan: <b id="scanDiff">—</b></div>
      <div class="sep"></div>
      <div class="row">
        <label>Repeat</label>
        <input id="scanRounds" type="number" min="1" max="100" value="10" style="width:75px">
        <button class="op" onclick="scanRounds()">rounds <span class="help" data-tip="Runs the normal scan several times in a row and collects per-address statistics (see Scan statistics). Shows which luminaires disappear intermittently.">?</span></button>
      </div>
      <div class="row" style="margin-top:10px">
        <label>Auto-scan every</label>
        <input id="autoScan" type="number" min="0" max="1440" value="0" style="width:75px">
        <label>min</label>
        <button onclick="setAutoScan()">Set <span class="help" data-tip="Runs a scan periodically while the tool is idle and adds it to the statistics. 0 turns auto-scan off. The interval is stored in the ESP32.">?</span></button>
        <span class="small muted" id="autoScanState"></span>
      </div>
    </section>

    <section class="card">
      <h2>Commissioning <span class="help" data-tip="Commissioning is the DALI process used to discover control gear and assign unique 6-bit short addresses (SA0…SA63).">?</span></h2>
      <div class="actions">
        <button class="op" onclick="start('/api/find-unaddressed')">Find unaddressed <span class="help" data-tip="Unaddressed gear has no short address. This test enters commissioning for unaddressed gear only and checks whether any device responds to COMPARE.">?</span></button>
        <button class="primary op" onclick="start('/api/assign-unaddressed')">Assign unaddressed <span class="help" data-tip="Runs commissioning only for gear without a short address, finds each 24-bit random address, then assigns the first free SA.">?</span></button>
      </div>
      <div class="sep"></div>
      <div class="row">
        <label>New SA <span class="help" data-tip="Short Address (SA) is the normal 6-bit DALI address of control gear. Valid values are 0…63.">?</span>:</label>
        <input id="newSa" type="number" min="0" max="63" value="10" style="width:85px">
        <button class="op" onclick="readdress()">Readdress single connected <span class="help" data-tip="Use with exactly one physical luminaire connected. The tool initialises all gear, randomises it, finds the device and programs the requested new short address.">?</span></button>
      </div>
      <p class="warn small">Readdress single: leave exactly ONE physical luminaire connected to the DALI bus.</p>
    </section>

    <section class="card">
      <h2>Diagnostics</h2>
      <div class="actions">
        <button class="primary op" onclick="start('/api/presence')">BUS PRESENCE TEST <span class="help" data-tip="Broadcasts QUERY CONTROL GEAR PRESENT and QUERY MISSING SHORT ADDRESS five times each. Every powered gear answers, with or without a short address. No answer at all means a bus power or wiring problem, not an addressing problem.">?</span></button>
        <button class="op" onclick="start('/api/raw-compare')">RAW COMPARE ×20 <span class="help" data-tip="COMPARE is a commissioning query. Gear whose random address is at or below the current SEARCHADDR answers YES. Repeating it helps diagnose missing, invalid or colliding backward frames.">?</span></button>
      </div>
      <p class="muted small">INITIALISE 0xFF → RANDOMISE → SEARCHADDR FFFFFF → 20 raw COMPARE queries with NO REPLY / INVALID / COLLISION / YES statistics.</p>
      <div class="small muted" style="margin-top:8px">
        INITIALISE <span class="help" data-tip="Places selected DALI control gear into commissioning mode. 0xFF targets gear without a short address; 0x00 includes all gear.">?</span>
        &nbsp; RANDOMISE <span class="help" data-tip="Makes each initialised gear generate a 24-bit random address used only during commissioning.">?</span>
        &nbsp; SEARCHADDR <span class="help" data-tip="A 24-bit commissioning search value used with COMPARE to binary-search a device's random address.">?</span>
        &nbsp; DTR0 <span class="help" data-tip="Data Transfer Register 0: a temporary byte used by many DALI configuration commands.">?</span>
      </div>
    </section>

    <section class="card">
      <h2>Danger zone</h2>
      <div class="row">
        <label>SA:</label>
        <input id="clearSa" type="number" min="0" max="63" value="0" style="width:85px">
        <button class="danger op" onclick="clearOne()">Clear selected address <span class="help" data-tip="Writes MASK (0xFF) to DTR0 and executes SET SHORT ADDRESS for the selected gear. The gear should become unaddressed.">?</span></button>
      </div>
      <div class="actions" style="margin-top:12px">
        <button class="danger op" onclick="clearAll()">Clear ALL addresses <span class="help" data-tip="Removes the short address from every responding SA. Use only when you intend to commission the whole bus again.">?</span></button>
        <button class="danger op" onclick="recommissionAll()">Recommission entire bus <span class="help" data-tip="Initialises and randomises all connected control gear, then assigns short addresses again from SA0 upward. Existing address mapping may change.">?</span></button>
      </div>
      <p class="warn small">These commands change DALI short addresses and may invalidate your existing device/group mapping.</p>
    </section>

    <section class="card full">
      <div class="row" style="justify-content:space-between">
        <h2 style="margin:0">Devices <span class="help" data-tip="A DALI control gear is shown here when it responds to its short address. Group membership uses DALI groups 0…15.">?</span></h2>
        <div class="actions">
          <button onclick="selectAllDevices()">Select all</button>
          <button onclick="clearSelection()">Clear selection</button>
        </div>
      </div>

      <div class="actions" style="margin-top:14px">
        <button class="primary op" onclick="selectedAction('on')">Selected ON</button>
        <button class="op" onclick="selectedAction('off')">Selected OFF</button>
        <span style="width:12px"></span>
        <button class="primary op" onclick="broadcastAction('on')">BROADCAST ON <span class="help" data-tip="Broadcast is one DALI command addressed to all control gear on the bus. It does not require or use individual short addresses.">?</span></button>
        <button class="op" onclick="broadcastAction('off')">BROADCAST OFF</button>
        <div class="sliders" style="flex-basis:100%;margin-top:2px">
        <span class="small muted">Broadcast:</span>
        <span class="tc lvl">
          <input id="lvlBroadcast" class="op" type="range" min="0" max="254" value="200"
            aria-label="Broadcast brightness"
            oninput="levelBroadcastInput(this)"
            onchange="levelBroadcastCommit(this)">
          <span class="tc-k" id="lvlBroadcastLabel">200 · 23%</span>
          <span class="help" data-tip="Broadcast brightness (Direct Arc Power Control) to all gear at once. DALI levels are logarithmic: 1 = 0.1%, 128 = 3.2%, 200 = 23%, 254 = 100%. 0 switches off.">?</span>
        </span>
        <span class="tc">
          <input id="tcBroadcast" class="op" type="range" min="154" max="370" value="262"
            aria-label="Broadcast colour temperature"
            oninput="tcBroadcastInput(this)"
            onchange="tcBroadcastCommit(this)">
          <span class="tc-k" id="tcBroadcastLabel">3820 K</span>
          <span class="help" data-tip="Broadcast colour temperature (DALI DT8) to all gear at once, cold on the left, warm on the right (6500–2700 K). Gear without DT8 ignores the command.">?</span>
        </span>
        </div>
      </div>

      <div class="small muted" style="margin-top:8px">
        Selected: <b id="selectedCount">0</b>. Selected ON/OFF sends addressed commands to checked devices. Broadcast sends one command to all gear and does not depend on short addresses.
      </div>

      <div style="overflow:auto;margin-top:12px">
      <table>
        <thead><tr><th>✓</th><th>SA <span class="help" data-tip="Short Address: 6-bit DALI address, 0…63.">?</span></th><th>Status</th><th>Level</th><th>Type</th><th>Version</th><th>Groups <span class="help" data-tip="DALI control gear can belong to any of 16 groups, numbered 0…15.">?</span></th><th>Control</th></tr></thead>
        <tbody id="devices"></tbody>
      </table>
      </div>
    </section>

    <section class="card">
      <h2>Group / broadcast control <span class="help" data-tip="Sends one command to every gear of a DALI group or to all gear (broadcast). Use it to check group membership and scenes.">?</span></h2>
      <div class="row">
        <select id="tgtAdr"></select>
        <button class="op" onclick="targetAction('off')">Off</button>
        <button class="op" onclick="targetAction('max')">Max</button>
        <button class="op" onclick="targetAction('min')">Min</button>
      </div>
      <div class="row" style="margin-top:10px">
        <label>Level</label>
        <input id="tgtLevel" type="number" min="0" max="254" value="128" style="width:80px">
        <button class="op" onclick="targetAction('level')">Set</button>
        <label style="margin-left:10px">Scene</label>
        <select id="tgtScene"></select>
        <button class="op" onclick="targetAction('scene')">Recall</button>
      </div>
    </section>

    <section class="card">
      <h2>Scan statistics <span class="help" data-tip="Collected over every scan since the last reset. OK = valid reply, INVALID / COLLISION = bus activity without a clean frame. A luminaire with OK below the number of rounds was missed in some scans.">?</span></h2>
      <div class="row" style="justify-content:space-between">
        <span class="small muted">Rounds: <b id="statRounds">0</b></span>
        <button onclick="resetStats()">Reset statistics</button>
      </div>
      <div class="scroll-table" style="margin-top:10px;max-height:260px">
        <table><thead><tr><th>SA</th><th>Label</th><th>OK</th><th>INVALID</th><th>COLLISION</th><th>Missed</th></tr></thead>
        <tbody id="statsBody"></tbody></table>
      </div>
    </section>

    <section class="card full">
      <h2>Raw command &amp; waveform <span class="help" data-tip="Sends any forward frame and optionally records the RX pin at 9600 samples/s. The shaded area is our own transmission; any low level after it is a reply from the gear.">?</span></h2>
      <div class="row">
        <select id="rawPreset" onchange="applyPreset()"></select>
        <label>Frame (hex)</label>
        <input id="rawFrame" class="mono" value="FF91" style="width:110px">
        <label><input id="rawReply" type="checkbox" checked> expect reply</label>
        <label>Repeat</label>
        <input id="rawRepeat" type="number" min="1" max="50" value="1" style="width:70px">
        <label><input id="rawCapture" type="checkbox" checked> record waveform</label>
        <select id="rawSamples">
          <option value="1024">107 ms</option>
          <option value="2048" selected>213 ms</option>
          <option value="4096">427 ms</option>
        </select>
        <button class="primary op" onclick="sendRaw()">Send</button>
      </div>
      <p class="small muted">4 hex digits = 16-bit frame (address byte + opcode), 6 digits = 24-bit frame. Examples: FF91 broadcast QUERY CONTROL GEAR PRESENT, 0190 QUERY STATUS of SA0, FE80 broadcast level 128.</p>
      <div class="row" style="justify-content:space-between;margin-top:6px">
        <span class="small" id="scopeInfo">No waveform recorded yet.</span>
        <span class="row">
          <label class="small">Window</label>
          <select id="scopeWindow" onchange="drawScope()">
            <option value="25">25 ms</option>
            <option value="50" selected>50 ms</option>
            <option value="100">100 ms</option>
            <option value="0">all</option>
          </select>
          <label class="small">Start</label>
          <input id="scopeOffset" type="range" min="0" max="0" value="0" oninput="drawScope()" style="width:180px;padding:0">
        </span>
      </div>
      <canvas id="scope" style="margin-top:8px"></canvas>
    </section>

    <section class="card full">
      <div class="row" style="justify-content:space-between">
        <h2 style="margin:0">Bus monitor <span class="help" data-tip="Every frame on the bus: TX = sent by this tool, REPLY = answer to our query, BUS = frame from another participant (another controller, sensor, push button) or a late reply. Time is when the frame was finished; Δ is the gap to the previous entry.">?</span></h2>
        <div class="actions">
          <label class="small"><input id="monPause" type="checkbox"> pause</label>
          <label class="small"><input id="monForeign" type="checkbox" onchange="renderMonitor()"> only BUS frames</label>
          <button onclick="clearMonitor()">Clear</button>
        </div>
      </div>
      <div class="scroll-table" style="margin-top:12px" id="monWrap">
        <table><thead><tr><th>#</th><th>Time, ms</th><th>Δ, ms</th><th>Kind</th><th>Frame</th><th>Meaning</th></tr></thead>
        <tbody id="monBody"></tbody></table>
      </div>
    </section>

    <section class="card full" id="memCard" style="display:none">
      <h2 id="memTitle">Memory bank</h2>
      <div id="memDecoded"></div>
      <div class="hexdump mono" id="memDump" style="margin-top:10px"></div>
    </section>

    <section class="card full">
      <div class="row" style="justify-content:space-between">
        <h2 style="margin:0">Log</h2>
        <div class="actions">
          <button onclick="location.href='/api/log'">Download log</button>
          <button onclick="clearLog()">Clear log</button>
        </div>
      </div>
      <pre class="log" id="log"></pre>
      <div class="small muted" id="lastResult"></div>
    </section>
  </div>
</main>
<div id="tooltip" class="tooltip"></div>

<script>
// ---------- Language ----------
// English is the source language. Text nodes and attributes are translated in place by
// looking up their English text, so the HTML templates below stay in English.
// Strings with inserted values go through tr() with {placeholders}.
const RU = {
  // header, common
  "Found:":"Найдено:", "BUSY":"ЗАНЯТ", "IDLE":"ГОТОВ", "Connection error:":"Ошибка связи:",
  "Yes":"Да", "No":"Нет", "off":"выкл", "0 · off":"0 · выкл", "ms":"мс", "min":"мин",

  // scanning
  "Scanning":"Сканирование", "SCAN":"Сканировать", "DETAILED SCAN":"Подробный скан", "Download report":"Скачать отчёт",
  "Scan uses QUERY STATUS on SA0…SA63. Detailed Scan also reads level, device type, version and group membership.":
    "Сканирование отправляет QUERY STATUS на адреса SA0…SA63. Подробный скан дополнительно читает уровень, тип устройства, версию и группы.",
  "Compared with previous scan:":"По сравнению с прошлым сканом:", "first scan":"первый скан", "no changes":"без изменений",
  "Repeat":"Повторить", "rounds":"раз", "Auto-scan every":"Автоскан каждые", "Set":"Задать",
  "active, every":"включено, каждые",
  "Scan queries short addresses SA0…SA63 to find DALI control gear that currently responds.":
    "Опрашивает короткие адреса SA0…SA63 и находит отвечающие устройства DALI.",
  "Queries each responding short address for additional data: actual level, device type, version and group membership.":
    "Для каждого ответившего адреса читает дополнительные данные: текущий уровень, тип устройства, версию и группы.",
  "Runs the normal scan several times in a row and collects per-address statistics (see Scan statistics). Shows which luminaires disappear intermittently.":
    "Запускает обычное сканирование несколько раз подряд и собирает статистику по адресам (см. «Статистика сканов»). Показывает, какие светильники периодически пропадают.",
  "Runs a scan periodically while the tool is idle and adds it to the statistics. 0 turns auto-scan off. The interval is stored in the ESP32.":
    "Периодически запускает сканирование, когда утилита свободна, и добавляет результат в статистику. 0 — выключено. Интервал хранится в ESP32.",

  // commissioning
  "Commissioning":"Адресация", "Find unaddressed":"Найти без адреса", "Assign unaddressed":"Назначить адреса",
  "New SA":"Новый SA", "Readdress single connected":"Переадресовать единственный",
  "Readdress single: leave exactly ONE physical luminaire connected to the DALI bus.":
    "Переадресация единственного: на шине DALI должен остаться ровно ОДИН светильник.",
  "Commissioning is the DALI process used to discover control gear and assign unique 6-bit short addresses (SA0…SA63).":
    "Адресация (commissioning) — процедура DALI, которая находит устройства и назначает им уникальные 6-битные короткие адреса (SA0…SA63).",
  "Unaddressed gear has no short address. This test enters commissioning for unaddressed gear only and checks whether any device responds to COMPARE.":
    "У устройства без адреса нет короткого адреса. Тест запускает адресацию только для таких устройств и проверяет, отвечает ли кто-нибудь на COMPARE.",
  "Runs commissioning only for gear without a short address, finds each 24-bit random address, then assigns the first free SA.":
    "Запускает адресацию только для устройств без короткого адреса: находит 24-битный случайный адрес каждого и назначает первый свободный SA.",
  "Short Address (SA) is the normal 6-bit DALI address of control gear. Valid values are 0…63.":
    "Короткий адрес (SA) — обычный 6-битный адрес устройства DALI. Допустимы значения 0…63.",
  "Use with exactly one physical luminaire connected. The tool initialises all gear, randomises it, finds the device and programs the requested new short address.":
    "Используйте, когда подключён ровно один светильник. Утилита переводит все устройства в режим адресации, генерирует случайные адреса, находит устройство и записывает ему новый короткий адрес.",

  // diagnostics
  "Diagnostics":"Диагностика", "BUS PRESENCE TEST":"Проверка шины",
  "INITIALISE 0xFF → RANDOMISE → SEARCHADDR FFFFFF → 20 raw COMPARE queries with NO REPLY / INVALID / COLLISION / YES statistics.":
    "INITIALISE 0xFF → RANDOMISE → SEARCHADDR FFFFFF → 20 запросов COMPARE со статистикой NO REPLY / INVALID / COLLISION / YES.",
  "Broadcasts QUERY CONTROL GEAR PRESENT and QUERY MISSING SHORT ADDRESS five times each. Every powered gear answers, with or without a short address. No answer at all means a bus power or wiring problem, not an addressing problem.":
    "Пять раз отправляет всем QUERY CONTROL GEAR PRESENT и QUERY MISSING SHORT ADDRESS. Отвечает любое включённое устройство, с адресом или без. Если ответа нет совсем, проблема в питании шины или проводке, а не в адресации.",
  "COMPARE is a commissioning query. Gear whose random address is at or below the current SEARCHADDR answers YES. Repeating it helps diagnose missing, invalid or colliding backward frames.":
    "COMPARE — запрос адресации. Отвечает YES устройство, чей случайный адрес не больше текущего SEARCHADDR. Повторы помогают выявить пропущенные, искажённые или столкнувшиеся ответы.",
  "Places selected DALI control gear into commissioning mode. 0xFF targets gear without a short address; 0x00 includes all gear.":
    "Переводит устройства DALI в режим адресации. 0xFF — только устройства без короткого адреса, 0x00 — все устройства.",
  "Makes each initialised gear generate a 24-bit random address used only during commissioning.":
    "Каждое устройство в режиме адресации генерирует 24-битный случайный адрес, который используется только при адресации.",
  "A 24-bit commissioning search value used with COMPARE to binary-search a device's random address.":
    "24-битное значение поиска, с которым COMPARE двоичным поиском находит случайный адрес устройства.",
  "Data Transfer Register 0: a temporary byte used by many DALI configuration commands.":
    "Регистр передачи данных 0: временный байт, который используют многие команды настройки DALI.",

  // danger zone
  "Danger zone":"Опасная зона", "Clear selected address":"Стереть адрес", "Clear ALL addresses":"Стереть ВСЕ адреса",
  "Recommission entire bus":"Переадресовать всю шину",
  "These commands change DALI short addresses and may invalidate your existing device/group mapping.":
    "Эти команды меняют короткие адреса DALI и могут нарушить существующую привязку устройств и групп.",
  "Writes MASK (0xFF) to DTR0 and executes SET SHORT ADDRESS for the selected gear. The gear should become unaddressed.":
    "Записывает MASK (0xFF) в DTR0 и выполняет SET SHORT ADDRESS для выбранного устройства. Устройство должно остаться без адреса.",
  "Removes the short address from every responding SA. Use only when you intend to commission the whole bus again.":
    "Стирает короткий адрес у всех отвечающих SA. Используйте, только если собираетесь заново адресовать всю шину.",
  "Initialises and randomises all connected control gear, then assigns short addresses again from SA0 upward. Existing address mapping may change.":
    "Переводит все подключённые устройства в режим адресации и заново назначает короткие адреса начиная с SA0. Текущая привязка адресов может измениться.",

  // devices
  "Devices":"Устройства", "Select all":"Выбрать все", "Clear selection":"Снять выбор",
  "Selected ON":"Выбранные вкл", "Selected OFF":"Выбранные выкл", "BROADCAST ON":"Все вкл", "BROADCAST OFF":"Все выкл",
  "Broadcast:":"Все:", "Selected:":"Выбрано:",
  ". Selected ON/OFF sends addressed commands to checked devices. Broadcast sends one command to all gear and does not depend on short addresses.":
    ". «Выбранные вкл/выкл» отправляет адресные команды отмеченным устройствам. «Все» отправляет одну команду всем устройствам и не зависит от коротких адресов.",
  "Status":"Статус", "Level":"Уровень", "Type":"Тип", "Version":"Версия", "Groups":"Группы", "Control":"Управление",
  "A DALI control gear is shown here when it responds to its short address. Group membership uses DALI groups 0…15.":
    "Устройство DALI появляется здесь, когда отвечает на свой короткий адрес. Группы — группы DALI 0…15.",
  "Broadcast is one DALI command addressed to all control gear on the bus. It does not require or use individual short addresses.":
    "Broadcast — одна команда DALI сразу всем устройствам на шине. Короткие адреса для неё не нужны.",
  "Broadcast brightness":"Яркость всех", "Broadcast colour temperature":"Цветовая температура всех",
  "Broadcast brightness (Direct Arc Power Control) to all gear at once. DALI levels are logarithmic: 1 = 0.1%, 128 = 3.2%, 200 = 23%, 254 = 100%. 0 switches off.":
    "Яркость сразу для всех устройств (Direct Arc Power Control). Уровни DALI логарифмические: 1 = 0,1%, 128 = 3,2%, 200 = 23%, 254 = 100%. 0 — выключить.",
  "Broadcast colour temperature (DALI DT8) to all gear at once, cold on the left, warm on the right (6500–2700 K). Gear without DT8 ignores the command.":
    "Цветовая температура сразу для всех (DALI DT8): слева холодный, справа тёплый (6500–2700 K). Устройства без DT8 игнорируют команду.",
  "Short Address: 6-bit DALI address, 0…63.":"Короткий адрес: 6-битный адрес DALI, 0…63.",
  "DALI control gear can belong to any of 16 groups, numbered 0…15.":"Устройство DALI может входить в любые из 16 групп с номерами 0…15.",
  "Inspect":"Опросить", "Identify":"Мигнуть", "Off":"Выкл", "On":"Вкл", "Edit label":"Изменить подпись",
  "Loads extended information for this device: configured levels, flags, device information, groups and scenes.":
    "Загружает расширенные данные устройства: настроенные уровни, флаги, сведения об устройстве, группы и сцены.",
  "Flashes this addressed luminaire three times so you can match its physical location to its short address.":
    "Трижды мигает этим светильником, чтобы сопоставить его место установки с коротким адресом.",
  "Brightness of this luminaire (Direct Arc Power Control). DALI levels are logarithmic: 1 = 0.1%, 128 = 3.2%, 200 = 23%, 254 = 100%. 0 switches off.":
    "Яркость этого светильника (Direct Arc Power Control). Уровни DALI логарифмические: 1 = 0,1%, 128 = 3,2%, 200 = 23%, 254 = 100%. 0 — выключить.",
  "Brightness SA{sa}":"Яркость SA{sa}", "Colour temperature SA{sa}":"Цветовая температура SA{sa}",
  "Colour temperature (DALI DT8), cold on the left, warm on the right. The range is {range} K, {source}. Gear without DT8 ignores the command.":
    "Цветовая температура (DALI DT8): слева холодный, справа тёплый. Диапазон {range} K, {source}. Устройства без DT8 игнорируют команду.",
  "as reported by the gear":"по данным устройства",
  "default; Inspect reads the real limits of DT8 gear":"по умолчанию; «Опросить» прочитает реальные пределы устройства DT8",

  // status flags
  "gear failure":"сбой драйвера", "lamp failure":"сбой лампы", "lamp on":"лампа вкл", "limit error":"вне пределов",
  "fade running":"идёт переход", "reset state":"заводское сост.", "no short address":"нет адреса", "power cycle seen":"был сбой питания",

  // device card
  "Dimming test":"Тест яркости", "Tc test":"Тест Tc", "Identity":"Идентификация", "Read bank":"Прочитать банк", "Reset":"Сброс",
  "Bank 1 (luminaire info)":"Банк 1 (данные светильника)", "Bank 202":"Банк 202", "Bank 203":"Банк 203", "Bank 204":"Банк 204",
  "Bank 205":"Банк 205", "Bank 206":"Банк 206", "Bank 207":"Банк 207",
  "Steps the level from minimum to maximum, waits for each fade to finish and compares QUERY ACTUAL LEVEL with the requested level. Restores the previous level.":
    "Проходит уровни от минимума до максимума, ждёт окончания каждого перехода и сравнивает QUERY ACTUAL LEVEL с заданным уровнем. В конце возвращает прежний уровень.",
  "Reads DT8 colour features and limits, steps the colour temperature from cold to warm and compares the reported value with the requested one. Restores the previous value.":
    "Читает цветовые возможности и пределы DT8, проходит температуру от холодной к тёплой и сравнивает фактическое значение с заданным. В конце возвращает прежнее значение.",
  "Reads memory bank 0: GTIN, firmware and hardware version, serial number, DALI part versions.":
    "Читает банк памяти 0: GTIN, версии прошивки и железа, серийный номер, версии частей DALI.",
  "Raw dump of a memory bank. D4i / DALI-2 drivers keep energy reporting and diagnostics (operating time, start counter, temperature, voltages, failures) in banks 202–207. Banks the gear does not implement are reported as not readable.":
    "Сырой дамп банка памяти. Драйверы D4i / DALI-2 хранят в банках 202–207 энергопотребление и диагностику (наработку, число включений, температуру, напряжения, ошибки). Неподдерживаемые банки помечаются как нечитаемые.",
  "DALI RESET: restores factory levels, fade, groups and scenes. The short address is kept.":
    "DALI RESET: возвращает заводские уровни, переходы, группы и сцены. Короткий адрес сохраняется.",
  "Settings":"Настройки", "Save settings":"Сохранить настройки", "Save groups":"Сохранить группы",
  "Scenes":"Сцены", "Save scenes":"Сохранить сцены", "Recall scene {n}":"Вызвать сцену {n}",
  "Level 0..254 stored per scene. Empty = MASK (luminaire is not part of the scene). ▶ recalls the scene on this luminaire only.":
    "Уровень 0..254 для каждой сцены. Пусто = MASK (светильник не участвует в сцене). ▶ вызывает сцену только на этом светильнике.",
  "Min level":"Мин. уровень", "Max level":"Макс. уровень", "Power-on":"При включении", "System failure":"При сбое шины",
  "Fade time":"Время перехода", "Fade rate":"Скорость перехода",
  "Lowest level the gear will dim to (not below physical minimum).":"Нижний уровень диммирования (не ниже физического минимума).",
  "Highest level the gear will go to.":"Верхний уровень яркости.",
  "Level after mains power is applied. 255 = MASK (last level).":"Уровень после подачи сетевого питания. 255 = MASK (последний уровень).",
  "Level when the DALI bus is lost. 255 = MASK (no change).":"Уровень при пропадании шины DALI. 255 = MASK (без изменений).",
  "0 = no fade, 1..15 = 0.7 s … 90.5 s.":"0 — без перехода, 1..15 — от 0,7 с до 90,5 с.",
  "Steps per second for UP/DOWN commands: 1 = 358, 15 = 2.8.":"Шагов в секунду для команд UP/DOWN: 1 = 358, 15 = 2,8.",
  "No extended data loaded. Click":"Расширенные данные не загружены. Нажмите", "to see and edit settings and scenes.":"чтобы увидеть и изменить настройки и сцены.",
  "Actual level":"Текущий уровень", "Physical minimum":"Физический минимум", "Minimum level":"Минимальный уровень",
  "Maximum level":"Максимальный уровень", "Power-on level":"Уровень при включении", "System failure level":"Уровень при сбое шины",
  "Fade time / rate":"Время / скорость перехода", "Operating mode":"Режим работы", "Device type":"Тип устройства",
  "Next device type":"Следующий тип устройства", "DALI version":"Версия DALI", "Light source type":"Тип источника света",
  "Control gear failure":"Неисправность драйвера", "Lamp failure":"Неисправность лампы", "Lamp power on":"Лампа включена",
  "Reset state":"Заводское состояние", "Power failure flag":"Флаг сбоя питания", "Colour temperature":"Цветовая температура",
  "Tc coolest":"Tc самая холодная", "Tc warmest":"Tc самая тёплая", "time={t}, rate={r}":"время={t}, скорость={r}",

  // group control, statistics
  "Group / broadcast control":"Группа / все", "Broadcast":"Все", "Group {g}":"Группа {g}",
  "Max":"Макс", "Min":"Мин", "Scene":"Сцена", "Recall":"Вызвать",
  "Sends one command to every gear of a DALI group or to all gear (broadcast). Use it to check group membership and scenes.":
    "Отправляет одну команду всем устройствам группы DALI или всем сразу. Помогает проверить состав групп и сцены.",
  "Scan statistics":"Статистика сканов", "Rounds:":"Проходов:", "Reset statistics":"Сбросить статистику",
  "Label":"Подпись", "Missed":"Пропуски", "No address has answered yet.":"Ни один адрес пока не ответил.",
  "Collected over every scan since the last reset. OK = valid reply, INVALID / COLLISION = bus activity without a clean frame. A luminaire with OK below the number of rounds was missed in some scans.":
    "Собирается по всем сканам с последнего сброса. OK — корректный ответ, INVALID / COLLISION — активность на шине без чистого кадра. Если OK меньше числа проходов, светильник пропадал в части сканов.",

  // raw command & waveform
  "Raw command & waveform":"Произвольная команда и сигнал", "Presets…":"Примеры…", "Frame (hex)":"Кадр (hex)",
  "expect reply":"ждать ответ", "record waveform":"записать сигнал", "Send":"Отправить",
  "4 hex digits = 16-bit frame (address byte + opcode), 6 digits = 24-bit frame. Examples: FF91 broadcast QUERY CONTROL GEAR PRESENT, 0190 QUERY STATUS of SA0, FE80 broadcast level 128.":
    "4 hex-цифры — 16-битный кадр (байт адреса + код команды), 6 цифр — 24-битный кадр. Примеры: FF91 — QUERY CONTROL GEAR PRESENT для всех, 0190 — QUERY STATUS для SA0, FE80 — уровень 128 для всех.",
  "No waveform recorded yet.":"Сигнал ещё не записан.", "Window":"Окно", "all":"весь", "Start":"Начало",
  "samples":"отсчётов", "TX starts at":"TX начинается на", "bus activity outside TX:":"активность на шине вне TX:",
  "low samples":"низких отсчётов", "no bus activity outside our transmission":"вне нашей передачи активности на шине нет",
  "Sends any forward frame and optionally records the RX pin at 9600 samples/s. The shaded area is our own transmission; any low level after it is a reply from the gear.":
    "Отправляет любой прямой кадр и при желании записывает линию RX с частотой 9600 отсчётов/с. Затенённая область — наша передача; низкий уровень после неё — ответ устройства.",
  "Broadcast QUERY CONTROL GEAR PRESENT":"Все: QUERY CONTROL GEAR PRESENT", "Broadcast QUERY MISSING SHORT ADDRESS":"Все: QUERY MISSING SHORT ADDRESS",
  "Broadcast OFF":"Все: OFF", "Broadcast RECALL MAX LEVEL":"Все: RECALL MAX LEVEL", "Broadcast level 128":"Все: уровень 128",

  // bus monitor
  "Bus monitor":"Монитор шины", "pause":"пауза", "only BUS frames":"только кадры BUS", "Clear":"Очистить",
  "Time, ms":"Время, мс", "Δ, ms":"Δ, мс", "Kind":"Тип", "Frame":"Кадр", "Meaning":"Значение", "No frames yet.":"Кадров пока нет.",
  "Every frame on the bus: TX = sent by this tool, REPLY = answer to our query, BUS = frame from another participant (another controller, sensor, push button) or a late reply. Time is when the frame was finished; Δ is the gap to the previous entry.":
    "Все кадры на шине: TX — отправлены этой утилитой, REPLY — ответ на наш запрос, BUS — кадр другого участника (другой контроллер, датчик, кнопка) или запоздавший ответ. Время — момент окончания кадра; Δ — интервал до предыдущей записи.",
  "reply 0x{v} ({d}){yes}":"ответ 0x{v} ({d}){yes}",
  "collision (several gears answered or noise)":"коллизия (ответили несколько устройств или помеха)",
  "invalid reply, {b} bit(s)":"искажённый ответ, бит: {b}", "undecodable bus activity":"нераспознанная активность на шине",
  "24-bit frame (input devices / DALI-2 part 103)":"24-битный кадр (устройства ввода / DALI-2 часть 103)",
  "backward frame 0x{v} (late or stray reply)":"обратный кадр 0x{v} (запоздавший или чужой ответ)",
  "{b}-bit frame":"{b}-битный кадр", "{t}: level {v}":"{t}: уровень {v}", "special/reserved":"спец./резерв",

  // memory
  "Memory bank":"Банк памяти",
  "Bank not readable: the gear did not answer or does not implement it.":"Банк не читается: устройство не ответило или не поддерживает его.",
  "Decoded with the DALI-2 layout (IEC 62386-102 ed2). DALI-1 gear may use a different layout, check the raw dump.":
    "Расшифровано по раскладке DALI-2 (IEC 62386-102 ed2). У устройств DALI-1 раскладка может отличаться, сверяйтесь с сырым дампом.",
  "Byte 0 = last accessible location. Layout depends on the bank and manufacturer; D4i banks 202–207 hold energy and diagnostics data.":
    "Байт 0 — адрес последней доступной ячейки. Раскладка зависит от банка и производителя; в банках D4i 202–207 хранятся данные энергопотребления и диагностики.",
  "Firmware version":"Версия прошивки", "Identification number":"Идентификационный номер", "Hardware version":"Версия железа",
  "IEC 62386-101 version":"Версия IEC 62386-101", "IEC 62386-102 version":"Версия IEC 62386-102", "IEC 62386-103 version":"Версия IEC 62386-103",
  "Logical control gear units":"Логических устройств", "Last accessible memory bank":"Последний доступный банк",

  // log
  "Log":"Журнал", "Download log":"Скачать журнал", "Clear log":"Очистить журнал",

  // dialogs
  "SA must be between 0 and 63.":"SA должен быть от 0 до 63.",
  "Is exactly ONE physical luminaire connected to the DALI bus?":"К шине DALI подключён ровно ОДИН светильник?",
  "Clear short address SA{sa}? The gear will become unaddressed.":"Стереть короткий адрес SA{sa}? Устройство останется без адреса.",
  "CLEAR the short address of ALL responding DALI control gear?":"СТЕРЕТЬ короткие адреса ВСЕХ отвечающих устройств DALI?",
  "Type CLEAR to confirm:":"Введите CLEAR для подтверждения:",
  "REASSIGN addresses for ALL connected DALI control gear?":"ПЕРЕНАЗНАЧИТЬ адреса ВСЕМ подключённым устройствам DALI?",
  "Type READDRESS to confirm:":"Введите READDRESS для подтверждения:",
  "Select at least one device first.":"Сначала выберите хотя бы одно устройство.",
  "Label for SA{sa} (max 40 characters, empty to remove):":"Подпись для SA{sa} (до 40 символов, пусто — удалить):",
  "RESET SA{sa}? Levels, fade, groups and scenes return to factory values. The short address is kept.":
    "СБРОСИТЬ SA{sa}? Уровни, переходы, группы и сцены вернутся к заводским. Короткий адрес сохранится.",
  "No scene changes.":"Сцены не изменены.", "Rounds must be 1..100.":"Число проходов: от 1 до 100.",
  "Interval must be 0..1440 minutes.":"Интервал: от 0 до 1440 минут.", "Frame must be 4 or 6 hex digits.":"Кадр: 4 или 6 hex-цифр."
};

let lang = "";
try { lang = localStorage.getItem("lang") || ""; } catch(e) {}
if(lang!=="ru" && lang!=="en") lang = (navigator.language||"").toLowerCase().startsWith("ru") ? "ru" : "en";

function tr(text, vars){
  let s = (lang==="ru" && RU[text]!==undefined) ? RU[text] : text;
  if(vars) s = s.replace(/\{(\w+)\}/g, (m,k) => (k in vars) ? vars[k] : m);
  return s;
}

const I18N_ATTRS = ["data-tip","placeholder","title","aria-label"];
const I18N_SKIP = "script,style,#log,#opText,#lastResult,.mono,.hexdump";
const origText = new WeakMap();  // text node -> English source
const origAttrs = new WeakMap(); // element -> {attribute: English source}

function translateNode(node){
  if(node.nodeType===3){
    const parent = node.parentElement;
    if(!parent || parent.closest(I18N_SKIP)) return;
    let orig = origText.get(node);
    if(orig===undefined){ orig = node.nodeValue; origText.set(node, orig); }
    const key = orig.trim();
    const value = key && RU[key]!==undefined ? orig.replace(key, tr(key)) : orig;
    if(node.nodeValue!==value) node.nodeValue = value;
    return;
  }
  if(node.nodeType!==1 || node.matches(I18N_SKIP)) return;
  let saved = origAttrs.get(node);
  for(const a of I18N_ATTRS){
    if(!node.hasAttribute(a)) continue;
    if(!saved){ saved = {}; origAttrs.set(node, saved); }
    if(!(a in saved)) saved[a] = node.getAttribute(a);
    const value = tr(saved[a]);
    if(node.getAttribute(a)!==value) node.setAttribute(a, value);
  }
  for(const child of node.childNodes) translateNode(child);
}

function setLang(l){
  lang = l==="ru" ? "ru" : "en";
  try { localStorage.setItem("lang", lang); } catch(e) {}
  document.documentElement.lang = lang;
  document.getElementById("langSel").value = lang;
  translateNode(document.body);
  // parts built with tr() and inserted values are rebuilt
  if(typeof initTargets==="function"){ initTargets(); initPresets(); }
  if(typeof rowCache!=="undefined"){
    for(const c of rowCache.values()){ c.tr.remove(); c.detail.remove(); }
    rowCache.clear();
    renderDevices(lastDevices);
    renderMonitor();
  }
}

let lastLog = "";
let lastDevices = [];
const selectedSAs = new Set();
const expandedSAs = new Set();
const autoExpandAfterInspect = new Set();

// Colour temperature slider state. Values are in mirek (1e6 / Kelvin):
// small mirek = cold (left), large mirek = warm (right).
const TC_DEFAULT_COOLEST = 154; // 6500 K, used until Inspect reads the gear limits
const TC_DEFAULT_WARMEST = 370; // 2700 K
const tcValues = new Map();     // SA -> last mirek chosen in the UI
let sliderDragging = false;     // pause table re-render while a slider is being moved
document.addEventListener("pointerup", () => { sliderDragging = false; });

const tooltip = document.getElementById("tooltip");

document.addEventListener("pointerover", e => {
  const help = e.target.closest ? e.target.closest(".help") : null;
  if(!help || !tooltip) return;
  tooltip.textContent = help.dataset.tip || "";
  tooltip.style.display = "block";
  positionTooltip(help);
});

document.addEventListener("pointerout", e => {
  const help = e.target.closest ? e.target.closest(".help") : null;
  if(help && tooltip) tooltip.style.display = "none";
});

document.addEventListener("click", e => {
  const help = e.target.closest ? e.target.closest(".help") : null;
  if(!help || !tooltip) return;
  e.preventDefault();
  e.stopPropagation();
  tooltip.textContent = help.dataset.tip || "";
  tooltip.style.display = tooltip.style.display === "block" ? "none" : "block";
  if(tooltip.style.display === "block") positionTooltip(help);
});

function positionTooltip(help){
  const r = help.getBoundingClientRect();
  const pad = 10;
  const tw = tooltip.offsetWidth;
  const th = tooltip.offsetHeight;

  let left = r.left + r.width/2 - tw/2;
  left = Math.max(pad, Math.min(left, window.innerWidth - tw - pad));

  let top = r.top - th - 10;
  if(top < pad) top = r.bottom + 10;

  tooltip.style.left = left + "px";
  tooltip.style.top = top + "px";
}

window.addEventListener("scroll", () => { if(tooltip) tooltip.style.display="none"; }, true);
window.addEventListener("resize", () => { if(tooltip) tooltip.style.display="none"; });


async function api(url, opts={}) {
  const r = await fetch(url, opts);
  const t = await r.text();
  if (!r.ok) throw new Error(t || ("HTTP "+r.status));
  try { return JSON.parse(t); } catch { return t; }
}

async function start(url) {
  try {
    await api(url, {method:"POST"});
    await refresh();
  } catch(e) { alert(e.message); }
}

function readdress(){
  const sa = Number(document.getElementById("newSa").value);
  if(sa<0||sa>63) return alert(tr("SA must be between 0 and 63."));
  if(!confirm(tr("Is exactly ONE physical luminaire connected to the DALI bus?"))) return;
  start("/api/readdress?sa="+sa);
}

function clearOne(){
  const sa = Number(document.getElementById("clearSa").value);
  if(sa<0||sa>63) return alert(tr("SA must be between 0 and 63."));
  if(!confirm(tr("Clear short address SA{sa}? The gear will become unaddressed.", {sa}))) return;
  start("/api/clear-address?sa="+sa);
}

function clearAll(){
  if(!confirm(tr("CLEAR the short address of ALL responding DALI control gear?"))) return;
  if(prompt(tr("Type CLEAR to confirm:")) !== 'CLEAR') return;
  start("/api/clear-all?confirm=CLEAR");
}

function recommissionAll(){
  if(!confirm(tr("REASSIGN addresses for ALL connected DALI control gear?"))) return;
  if(prompt(tr("Type READDRESS to confirm:")) !== 'READDRESS') return;
  start("/api/recommission-all?confirm=READDRESS");
}

function deviceAction(sa, cmd){
  if(cmd==="on" || cmd==="off") levelValues.delete(sa);
  start("/api/device?sa="+sa+"&cmd="+cmd);
}


function updateSelectedCount(){
  document.getElementById("selectedCount").textContent = selectedSAs.size;
}

function toggleSelected(sa, checked){
  if(checked) selectedSAs.add(sa);
  else selectedSAs.delete(sa);
  updateSelectedCount();
}

function selectAllDevices(){
  document.querySelectorAll('input.deviceSelect').forEach(cb=>{
    cb.checked = true;
    selectedSAs.add(Number(cb.dataset.sa));
  });
  updateSelectedCount();
}

function clearSelection(){
  selectedSAs.clear();
  document.querySelectorAll('input.deviceSelect').forEach(cb=>cb.checked=false);
  updateSelectedCount();
}

function selectedAction(cmd){
  const sas = [...selectedSAs].sort((a,b)=>a-b);
  if(!sas.length) return alert(tr("Select at least one device first."));
  start("/api/selected?cmd="+cmd+"&sas="+encodeURIComponent(sas.join(",")));
}

function broadcastAction(cmd){
  levelValues.clear();
  start("/api/broadcast?cmd="+cmd);
}

function downloadReport(){
  location.href="/api/report";
}

async function clearLog(){
  await api("/api/log/clear",{method:"POST"});
  refresh();
}

function val(v){
  return v===null || v===undefined || v<-255 ? "—" : v;
}


function toggleDeviceCard(sa){
  if(expandedSAs.has(sa)) expandedSAs.delete(sa);
  else expandedSAs.add(sa);
  const card=document.getElementById("deviceCard"+sa);
  const btn=document.getElementById("expandBtn"+sa);
  if(card) card.classList.toggle("open", expandedSAs.has(sa));
  if(btn) btn.textContent=expandedSAs.has(sa) ? "▴" : "▾";
}

function inspectDeviceUI(sa){
  autoExpandAfterInspect.add(sa);
  start("/api/device?sa="+sa+"&cmd=inspect");
}

function yesNo(v){
  if(v===1 || v===255) return "Yes";
  if(v===0) return "No";
  return val(v);
}

function hexByte(v){
  if(v===null || v===undefined || v<0) return "—";
  return "0x"+Number(v).toString(16).padStart(2,"0").toUpperCase();
}

function statusText(d){
  if(d.replyValid) return hexByte(d.status);
  if(d.status === -105) return '<span class="warn">INVALID (-105)</span>';
  if(d.status === -3) return '<span class="warn">COLLISION (-3)</span>';
  return "—";
}

function fadeText(v){
  if(v===null || v===undefined || v<0) return "—";
  const fadeRate=v & 0x0F;
  const fadeTime=(v >> 4) & 0x0F;
  return `${hexByte(v)} · ${tr("time={t}, rate={r}", {t:fadeTime, r:fadeRate})}`;
}

function sceneHtml(scenes){
  if(!Array.isArray(scenes)) return "—";
  return scenes.map((v,i)=>{
    const text=(v===255) ? "MASK" : (v<0 ? "—" : v);
    return `<span class="scene-chip">S${i}: ${text}</span>`;
  }).join("");
}

const STATUS_BITS = [
  ["gear failure","bad"], ["lamp failure","bad"], ["lamp on",""], ["limit error","warn"],
  ["fade running",""], ["reset state","warn"], ["no short address","warn"], ["power cycle seen","warn"]
];

function statusFlags(v){
  if(v===null || v===undefined || v<0) return "";
  return STATUS_BITS.map((f,i)=> (v & (1<<i)) ? `<span class="flag ${f[1]}">${f[0]}</span>` : "").join("");
}

function inputVal(v){
  return (v===null || v===undefined || v<0) ? "" : v;
}

function deviceTools(d){
  const sa=d.sa;
  return `<div class="tools">
    <button class="op" onclick="deviceAction(${sa},'dimtest')">Dimming test <span class="help" data-tip="Steps the level from minimum to maximum, waits for each fade to finish and compares QUERY ACTUAL LEVEL with the requested level. Restores the previous level.">?</span></button>
    <button class="op" onclick="deviceAction(${sa},'tctest')">Tc test <span class="help" data-tip="Reads DT8 colour features and limits, steps the colour temperature from cold to warm and compares the reported value with the requested one. Restores the previous value.">?</span></button>
    <button class="op" onclick="readMemory(${sa},0)">Identity <span class="help" data-tip="Reads memory bank 0: GTIN, firmware and hardware version, serial number, DALI part versions.">?</span></button>
    <select id="bankSel${sa}">
      <option value="1">Bank 1 (luminaire info)</option>
      <option value="202">Bank 202</option><option value="203">Bank 203</option><option value="204">Bank 204</option>
      <option value="205">Bank 205</option><option value="206">Bank 206</option><option value="207">Bank 207</option>
    </select>
    <button class="op" onclick="readMemory(${sa},Number(document.getElementById('bankSel${sa}').value))">Read bank <span class="help" data-tip="Raw dump of a memory bank. D4i / DALI-2 drivers keep energy reporting and diagnostics (operating time, start counter, temperature, voltages, failures) in banks 202–207. Banks the gear does not implement are reported as not readable.">?</span></button>
    <button class="danger op" onclick="resetDevice(${sa})">Reset <span class="help" data-tip="DALI RESET: restores factory levels, fade, groups and scenes. The short address is kept.">?</span></button>
  </div>`;
}

function configForm(d){
  const sa=d.sa;
  const ft = d.fadeTimeRate>=0 ? (d.fadeTimeRate>>4)&15 : "";
  const fr = d.fadeTimeRate>=0 ? d.fadeTimeRate&15 : "";
  const f=(id,label,v,min,max,tip)=>`<label><span>${label} <span class="help" data-tip="${tip}">?</span></span><input id="${id}${sa}" type="number" min="${min}" max="${max}" value="${v}"></label>`;
  return `<div class="sub-h">Settings</div>
    <div class="form-row">
      ${f("cfgMin","Min level",inputVal(d.minLevel),1,254,"Lowest level the gear will dim to (not below physical minimum).")}
      ${f("cfgMax","Max level",inputVal(d.maxLevel),1,254,"Highest level the gear will go to.")}
      ${f("cfgPon","Power-on",inputVal(d.powerOnLevel),0,255,"Level after mains power is applied. 255 = MASK (last level).")}
      ${f("cfgSf","System failure",inputVal(d.systemFailureLevel),0,255,"Level when the DALI bus is lost. 255 = MASK (no change).")}
      ${f("cfgFt","Fade time",ft,0,15,"0 = no fade, 1..15 = 0.7 s … 90.5 s.")}
      ${f("cfgFr","Fade rate",fr,1,15,"Steps per second for UP/DOWN commands: 1 = 358, 15 = 2.8.")}
      <button class="op" onclick="saveConfig(${sa})">Save settings</button>
    </div>`;
}

function groupsForm(d){
  if(d.groups07<0 || d.groups815<0 || d.groups07===undefined) return "";
  const mask = d.groups07 | (d.groups815<<8);
  let boxes="";
  for(let g=0; g<16; g++){
    boxes+=`<label><input type="checkbox" id="grp${d.sa}_${g}" ${mask & (1<<g) ? "checked":""}> G${g}</label>`;
  }
  return `<div class="sub-h">Groups</div>
    <div class="row"><div class="chk-grid">${boxes}</div>
    <button class="op" onclick="saveGroups(${d.sa})">Save groups</button></div>`;
}

function scenesForm(d){
  if(!Array.isArray(d.scenes)) return "";
  let cells="";
  d.scenes.forEach((v,i)=>{
    const shown = (v===255 || v<0) ? "" : v;
    const orig = v===255 ? "mask" : (v<0 ? "" : String(v));
    cells+=`<label>S${i}
      <input id="scn${d.sa}_${i}" data-orig="${orig}" value="${shown}" placeholder="${v===255?"mask":"—"}">
      <button class="op" title="${tr("Recall scene {n}", {n:i})}" onclick="sceneRecall(${d.sa},${i})">▶</button></label>`;
  });
  return `<div class="sub-h">Scenes <span class="help" data-tip="Level 0..254 stored per scene. Empty = MASK (luminaire is not part of the scene). ▶ recalls the scene on this luminaire only.">?</span></div>
    <div class="scene-grid">${cells}</div>
    <div style="margin-top:10px"><button class="op" onclick="saveScenes(${d.sa})">Save scenes</button></div>`;
}

function detailCard(d){
  const open = expandedSAs.has(d.sa) ? "open" : "";
  if(!d.inspected){
    return `<div class="device-card ${open}" id="deviceCard${d.sa}">
      ${deviceTools(d)}
      ${groupsForm(d)}
      <div class="muted small" style="margin-top:10px">No extended data loaded. Click <b>Inspect</b> to see and edit settings and scenes.</div>
    </div>`;
  }

  return `<div class="device-card ${open}" id="deviceCard${d.sa}">
    ${deviceTools(d)}
    <div class="device-grid">
      <div class="device-field"><span class="k">Actual level</span><span class="v">${val(d.level)}</span></div>
      <div class="device-field"><span class="k">Physical minimum</span><span class="v">${val(d.physicalMin)}</span></div>
      <div class="device-field"><span class="k">Minimum level</span><span class="v">${val(d.minLevel)}</span></div>
      <div class="device-field"><span class="k">Maximum level</span><span class="v">${val(d.maxLevel)}</span></div>

      <div class="device-field"><span class="k">Power-on level</span><span class="v">${val(d.powerOnLevel)}</span></div>
      <div class="device-field"><span class="k">System failure level</span><span class="v">${val(d.systemFailureLevel)}</span></div>
      <div class="device-field"><span class="k">Fade time / rate</span><span class="v">${fadeText(d.fadeTimeRate)}</span></div>
      <div class="device-field"><span class="k">Operating mode</span><span class="v">${hexByte(d.operatingMode)}</span></div>

      <div class="device-field"><span class="k">Device type</span><span class="v">${val(d.type)}</span></div>
      <div class="device-field"><span class="k">Next device type</span><span class="v">${val(d.nextDeviceType)}</span></div>
      <div class="device-field"><span class="k">DALI version</span><span class="v">${val(d.version)}</span></div>
      <div class="device-field"><span class="k">Light source type</span><span class="v">${hexByte(d.lightSourceType)}</span></div>

      <div class="device-field"><span class="k">Control gear failure</span><span class="v">${yesNo(d.controlGearFailure)}</span></div>
      <div class="device-field"><span class="k">Lamp failure</span><span class="v">${yesNo(d.lampFailure)}</span></div>
      <div class="device-field"><span class="k">Lamp power on</span><span class="v">${yesNo(d.lampPowerOn)}</span></div>
      <div class="device-field"><span class="k">Reset state</span><span class="v">${yesNo(d.resetState)}</span></div>

      <div class="device-field"><span class="k">Power failure flag</span><span class="v">${yesNo(d.powerFailure)}</span></div>
      <div class="device-field"><span class="k">Groups</span><span class="v">${d.groups}</span></div>
      <div class="device-field"><span class="k">Status</span><span class="v">${statusText(d)}<span class="flags">${statusFlags(d.status)}</span></span></div>

      <div class="device-field"><span class="k">Colour temperature</span><span class="v">${tcText(d.tc)}</span></div>
      <div class="device-field"><span class="k">Tc coolest</span><span class="v">${tcText(d.tcCoolest)}</span></div>
      <div class="device-field"><span class="k">Tc warmest</span><span class="v">${tcText(d.tcWarmest)}</span></div>
    </div>
    ${configForm(d)}
    ${groupsForm(d)}
    ${scenesForm(d)}
  </div>`;
}

function validMirek(v){
  return Number.isInteger(v) && v > 0 && v < 0xFFFF;
}

function kelvin(mirek){
  return Math.round(1e6 / mirek / 10) * 10;
}

function tcRange(d){
  const cool = validMirek(d.tcCoolest) ? d.tcCoolest : TC_DEFAULT_COOLEST;
  const warm = validMirek(d.tcWarmest) ? d.tcWarmest : TC_DEFAULT_WARMEST;
  return cool < warm ? [cool, warm] : [TC_DEFAULT_COOLEST, TC_DEFAULT_WARMEST];
}

// DALI arc power is logarithmic: level 1 = 0.1 %, 254 = 100 %, 0 = off
function levelPercent(l){
  if(l<=0) return "off";
  const p=Math.pow(10, (l-1)*3/253 - 1);
  return (p<10 ? p.toFixed(1) : Math.round(p)) + "%";
}

function levelText(l){
  return l<=0 ? "0 · off" : `${l} · ${levelPercent(l)}`;
}

const levelValues = new Map(); // SA -> last level chosen in the UI

function levelControl(d){
  let l = levelValues.has(d.sa) ? levelValues.get(d.sa) : ((d.level>=0 && d.level<=254) ? d.level : 0);
  return `<span class="tc lvl">
      <input class="op lvl-slider" data-sa="${d.sa}" type="range" min="0" max="254" value="${l}"
        aria-label="${tr("Brightness SA{sa}", {sa:d.sa})}"
        onpointerdown="sliderDragging=true"
        oninput="levelInput(${d.sa},this)"
        onchange="levelCommit(${d.sa},this)">
      <span class="tc-k" id="lvlLabel${d.sa}">${levelText(l)}</span>
      <span class="help" data-tip="Brightness of this luminaire (Direct Arc Power Control). DALI levels are logarithmic: 1 = 0.1%, 128 = 3.2%, 200 = 23%, 254 = 100%. 0 switches off.">?</span>
    </span>`;
}

function levelInput(sa, el){
  sliderDragging = true;
  const label = document.getElementById("lvlLabel"+sa);
  if(label) label.textContent = levelText(Number(el.value));
}

function levelCommit(sa, el){
  sliderDragging = false;
  const l = Number(el.value);
  levelValues.set(sa, l);
  start(`/api/target?adr=${sa}&action=level&value=${l}`);
}

function levelBroadcastInput(el){
  document.getElementById("lvlBroadcastLabel").textContent = levelText(Number(el.value));
}

function levelBroadcastCommit(el){
  const l = Number(el.value);
  // Every gear receives the same level, keep the per-device sliders in sync
  document.querySelectorAll("input.lvl-slider").forEach(s => {
    const sa = Number(s.dataset.sa);
    levelValues.set(sa, l);
    s.value = l;
    levelInput(sa, s);
  });
  sliderDragging = false;
  start(`/api/target?adr=127&action=level&value=${l}`);
}

function tcControl(d){
  const [cool, warm] = tcRange(d);
  let m = tcValues.has(d.sa) ? tcValues.get(d.sa) : (validMirek(d.tc) ? d.tc : Math.round((cool + warm) / 2));
  m = Math.min(warm, Math.max(cool, m));
  return `<span class="tc">
      <input class="op tc-slider" data-sa="${d.sa}" type="range" min="${cool}" max="${warm}" value="${m}"
        aria-label="${tr("Colour temperature SA{sa}", {sa:d.sa})}"
        onpointerdown="sliderDragging=true"
        oninput="tcInput(${d.sa},this)"
        onchange="tcCommit(${d.sa},this)">
      <span class="tc-k" id="tcLabel${d.sa}">${kelvin(m)} K</span>
      <span class="help" data-tip="${tr("Colour temperature (DALI DT8), cold on the left, warm on the right. The range is {range} K, {source}. Gear without DT8 ignores the command.", {
        range: `${kelvin(cool)}–${kelvin(warm)}`,
        source: tr(validMirek(d.tcCoolest) ? "as reported by the gear" : "default; Inspect reads the real limits of DT8 gear")})}">?</span>
    </span>`;
}

function tcInput(sa, el){
  sliderDragging = true;
  const label = document.getElementById("tcLabel"+sa);
  if(label) label.textContent = kelvin(Number(el.value)) + " K";
}

function tcCommit(sa, el){
  sliderDragging = false;
  const mirek = Number(el.value);
  tcValues.set(sa, mirek);
  start("/api/device?sa="+sa+"&cmd=tc&mirek="+mirek);
}

function tcBroadcastInput(el){
  document.getElementById("tcBroadcastLabel").textContent = kelvin(Number(el.value)) + " K";
}

function tcBroadcastCommit(el){
  const mirek = Number(el.value);
  // Every gear receives the same value, keep the per-device sliders in sync
  document.querySelectorAll("input.tc-slider").forEach(s => {
    const sa = Number(s.dataset.sa);
    tcValues.set(sa, mirek);
    s.value = Math.min(Number(s.max), Math.max(Number(s.min), mirek));
    const label = document.getElementById("tcLabel"+sa);
    if(label) label.textContent = kelvin(Number(s.value)) + " K";
  });
  start("/api/broadcast?cmd=tc&mirek="+mirek);
}

function tcText(v){
  if(v===0xFFFF) return "MASK";
  return validMirek(v) ? `${kelvin(v)} K (${v} mirek)` : "—";
}

// Rows are rebuilt only when their data changed, so inputs being edited are kept.
const rowCache = new Map(); // SA -> {sig, tr, detail}

function buildRow(d){
  const tr=document.createElement("tr");
  tr.className="device-main";
  tr.innerHTML=`
    <td><input class="deviceSelect" data-sa="${d.sa}" type="checkbox"
        ${selectedSAs.has(d.sa) ? "checked" : ""}
        onchange="toggleSelected(${d.sa},this.checked)"></td>
    <td><b>SA${d.sa}</b><button class="edit-btn" title="Edit label" onclick="editLabel(${d.sa})">✎</button>
      <div class="lbl" title="${escapeHtml(d.label||"")}">${escapeHtml(d.label||"")}</div></td>
    <td>${statusText(d)}<div class="flags">${statusFlags(d.status)}</div></td>
    <td>${val(d.level)}</td>
    <td>${val(d.type)}</td>
    <td>${val(d.version)}</td>
    <td>${d.groups}</td>
    <td style="white-space:nowrap">
      <button class="op" onclick="inspectDeviceUI(${d.sa})">Inspect <span class="help" data-tip="Loads extended information for this device: configured levels, flags, device information, groups and scenes.">?</span></button>
      <button class="op" onclick="deviceAction(${d.sa},'identify')">Identify <span class="help" data-tip="Flashes this addressed luminaire three times so you can match its physical location to its short address.">?</span></button>
      <button class="op" onclick="deviceAction(${d.sa},'off')">Off</button>
      <button class="op" onclick="deviceAction(${d.sa},'on')">On</button>
      <button id="expandBtn${d.sa}" class="expand-btn" onclick="toggleDeviceCard(${d.sa})">${expandedSAs.has(d.sa)?"▴":"▾"}</button>
      <div class="sliders">${levelControl(d)}${tcControl(d)}</div>
    </td>`;

  const detail=document.createElement("tr");
  detail.className="device-detail";
  const td=document.createElement("td");
  td.colSpan=8;
  td.innerHTML=detailCard(d);
  detail.appendChild(td);
  return {tr, detail};
}

function renderDevices(ds){
  const tb=document.getElementById("devices");
  const seen=new Set();
  let prev=null;

  for(const d of ds){
    if(!d.present) continue;
    seen.add(d.sa);

    if(d.inspected && autoExpandAfterInspect.has(d.sa)){
      expandedSAs.add(d.sa);
      autoExpandAfterInspect.delete(d.sa);
    }

    const sig=JSON.stringify(d);
    let c=rowCache.get(d.sa);
    const editing = c && c.detail.contains(document.activeElement) && document.activeElement.tagName!=="BUTTON";
    if(!c || (c.sig!==sig && !editing)){
      const rows=buildRow(d);
      if(c){ c.tr.replaceWith(rows.tr); c.detail.replaceWith(rows.detail); }
      c={sig, tr:rows.tr, detail:rows.detail};
      rowCache.set(d.sa, c);
    }

    const anchor = prev ? prev.detail.nextSibling : tb.firstChild;
    if(c.tr!==anchor) tb.insertBefore(c.tr, anchor);
    if(c.tr.nextSibling!==c.detail) tb.insertBefore(c.detail, c.tr.nextSibling);
    prev=c;
  }

  for(const [sa,c] of rowCache){
    if(!seen.has(sa)){ c.tr.remove(); c.detail.remove(); rowCache.delete(sa); }
  }
  updateSelectedCount();
}

function escapeHtml(t){
  return String(t).replace(/[&<>"']/g, ch => ({"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;","'":"&#39;"}[ch]));
}

async function editLabel(sa){
  const cur = (lastDevices.find(d=>d.sa===sa)||{}).label || "";
  const text = prompt(tr("Label for SA{sa} (max 40 characters, empty to remove):", {sa}), cur);
  if(text===null) return;
  try{
    await api("/api/label?sa="+sa+"&text="+encodeURIComponent(text), {method:"POST"});
    refresh();
  }catch(e){ alert(e.message); }
}

function readMemory(sa, bank){
  start("/api/device?sa="+sa+"&cmd=memory&bank="+bank);
}

function resetDevice(sa){
  if(!confirm(tr("RESET SA{sa}? Levels, fade, groups and scenes return to factory values. The short address is kept.", {sa}))) return;
  deviceAction(sa,'reset');
}

function saveConfig(sa){
  const g=id=>encodeURIComponent(document.getElementById(id+sa).value.trim());
  start(`/api/device?sa=${sa}&cmd=config&min=${g("cfgMin")}&max=${g("cfgMax")}&pon=${g("cfgPon")}&sf=${g("cfgSf")}&ft=${g("cfgFt")}&fr=${g("cfgFr")}`);
}

function saveGroups(sa){
  let mask=0;
  for(let g=0; g<16; g++) if(document.getElementById(`grp${sa}_${g}`).checked) mask |= (1<<g);
  start(`/api/device?sa=${sa}&cmd=groups&mask=${mask}`);
}

function saveScenes(sa){
  let q="", n=0;
  for(let i=0; i<16; i++){
    const el=document.getElementById(`scn${sa}_${i}`);
    const v=el.value.trim().toLowerCase();
    const now = (v==="" || v==="mask") ? "mask" : v;
    if(now===el.dataset.orig) continue;
    q+=`&s${i}=${encodeURIComponent(now)}`;
    n++;
  }
  if(!n) return alert(tr("No scene changes."));
  start(`/api/device?sa=${sa}&cmd=scenes${q}`);
}

function sceneRecall(sa, n){
  start(`/api/target?adr=${sa}&action=scene&value=${n}`);
}

// ---------- Group / broadcast control ----------
function initTargets(){
  const t=document.getElementById("tgtAdr");
  const selected=t.value || "127";
  t.innerHTML = `<option value="127">${tr("Broadcast")}</option>` +
    Array.from({length:16},(_,g)=>`<option value="${64+g}">${tr("Group {g}", {g})}</option>`).join("");
  t.value=selected;
  const sc=document.getElementById("tgtScene");
  if(!sc.options.length)
    sc.innerHTML = Array.from({length:16},(_,n)=>`<option value="${n}">${n}</option>`).join("");
}
initTargets();

function targetAction(action){
  const adr=document.getElementById("tgtAdr").value;
  let value=0;
  if(action==="level") value=Number(document.getElementById("tgtLevel").value);
  if(action==="scene") value=Number(document.getElementById("tgtScene").value);
  start(`/api/target?adr=${adr}&action=${action}&value=${value}`);
}

// ---------- Scan statistics / auto-scan ----------
function scanRounds(){
  const n=Number(document.getElementById("scanRounds").value);
  if(!(n>=1 && n<=100)) return alert(tr("Rounds must be 1..100."));
  start("/api/scan-rounds?rounds="+n);
}

let autoScanEdited=false;
document.getElementById("autoScan").addEventListener("input", ()=>{ autoScanEdited=true; });

async function setAutoScan(){
  const m=Number(document.getElementById("autoScan").value);
  if(!(m>=0 && m<=1440)) return alert(tr("Interval must be 0..1440 minutes."));
  try{ await api("/api/autoscan?minutes="+m,{method:"POST"}); autoScanEdited=false; refresh(); }
  catch(e){ alert(e.message); }
}

async function resetStats(){
  try{ await api("/api/stats/reset",{method:"POST"}); refresh(); }
  catch(e){ alert(e.message); }
}

function renderStats(s){
  document.getElementById("statRounds").textContent=s.statRounds;
  const rows=(s.stats||[]).map(st=>{
    const missed=s.statRounds-st.ok-st.invalid-st.collision;
    return `<tr><td>SA${st.sa}</td><td>${escapeHtml(st.label||"")}</td><td>${st.ok}</td>
      <td class="${st.invalid?"warn":""}">${st.invalid}</td><td class="${st.collision?"warn":""}">${st.collision}</td>
      <td class="${missed>0?"bad":"ok"}">${missed}</td></tr>`;
  }).join("");
  const body=document.getElementById("statsBody");
  const html = rows || `<tr><td colspan="6" class="muted">No address has answered yet.</td></tr>`;
  if(body.innerHTML!==html) body.innerHTML=html;
}

// ---------- Raw command & waveform ----------
const RAW_PRESETS = [
  ["FF91", true,  "Broadcast QUERY CONTROL GEAR PRESENT"],
  ["FF96", true,  "Broadcast QUERY MISSING SHORT ADDRESS"],
  ["0190", true,  "QUERY STATUS SA0"],
  ["01A0", true,  "QUERY ACTUAL LEVEL SA0"],
  ["FF00", false, "Broadcast OFF"],
  ["FF05", false, "Broadcast RECALL MAX LEVEL"],
  ["FE80", false, "Broadcast level 128"],
];
function initPresets(){
  document.getElementById("rawPreset").innerHTML = `<option value="">${tr("Presets…")}</option>` +
    RAW_PRESETS.map((p,i)=>`<option value="${i}">${p[0]} · ${tr(p[2])}</option>`).join("");
}
initPresets();

function applyPreset(){
  const sel=document.getElementById("rawPreset");
  const p=RAW_PRESETS[Number(sel.value)];
  if(!p) return;
  document.getElementById("rawFrame").value=p[0];
  document.getElementById("rawReply").checked=p[1];
  sel.value="";
}

function sendRaw(){
  const frame=document.getElementById("rawFrame").value.replace(/\s+/g,"");
  if(!/^([0-9a-fA-F]{4}|[0-9a-fA-F]{6})$/.test(frame)) return alert(tr("Frame must be 4 or 6 hex digits."));
  const reply=document.getElementById("rawReply").checked ? 1 : 0;
  const repeat=Number(document.getElementById("rawRepeat").value)||1;
  const capture=document.getElementById("rawCapture").checked ? document.getElementById("rawSamples").value : 0;
  start(`/api/raw?frame=${frame}&reply=${reply}&repeat=${repeat}&capture=${capture}`);
}

let scope=null, scopeSeqSeen=0;

async function loadScope(){
  scope=await api("/api/scope");
  const bits=[];
  for(let i=0;i<scope.samples;i++){
    const byte=parseInt(scope.data.substr((i>>3)*2,2),16);
    bits.push((byte>>(7-(i&7)))&1);
  }
  scope.bits=bits;
  const ms=scope.samples*1000/scope.rate;
  const off=document.getElementById("scopeOffset");
  off.max=Math.max(0,Math.floor(ms));
  off.value = scope.txStart<0xFFFF ? Math.max(0,Math.floor(scope.txStart*1000/scope.rate)-2) : 0;

  // low samples outside our own transmission = activity of other bus participants
  let lowOutside=0;
  bits.forEach((b,i)=>{ if(!b && !(scope.txStart<0xFFFF && i>=scope.txStart && i<=(scope.txEnd<0xFFFF?scope.txEnd:scope.samples))) lowOutside++; });
  // static pieces in their own elements so the language switch can translate them
  const txText = scope.txStart<0xFFFF ? `${(scope.txStart*1000/scope.rate).toFixed(1)} <span>ms</span>` : "—";
  document.getElementById("scopeInfo").innerHTML =
    `<b>${escapeHtml(scope.title)}</b> · ${scope.samples} <span>samples</span> (${ms.toFixed(0)} <span>ms</span>) · <span>TX starts at</span> ${txText} · ` +
    (lowOutside ? `<span class="ok"><span>bus activity outside TX:</span> ${lowOutside} <span>low samples</span> (${(lowOutside*1000/scope.rate).toFixed(1)} <span>ms</span>)</span>`
                : `<span class="bad">no bus activity outside our transmission</span>`);
  drawScope();
}

function drawScope(){
  const c=document.getElementById("scope");
  const dpr=window.devicePixelRatio||1;
  c.width=c.clientWidth*dpr; c.height=c.clientHeight*dpr;
  const g=c.getContext("2d");
  g.scale(dpr,dpr);
  const W=c.clientWidth, H=c.clientHeight;
  g.clearRect(0,0,W,H);
  if(!scope || !scope.bits) return;

  const rate=scope.rate;
  const totalMs=scope.samples*1000/rate;
  let winMs=Number(document.getElementById("scopeWindow").value) || totalMs;
  const startMs=Math.min(Number(document.getElementById("scopeOffset").value), Math.max(0,totalMs-1));
  const s0=Math.floor(startMs*rate/1000);
  const s1=Math.min(scope.samples, Math.ceil((startMs+winMs)*rate/1000));
  const x=i=>(i-s0)/((winMs*rate/1000))*W;
  const yHigh=34, yLow=H-26;

  // our transmission
  if(scope.txStart<0xFFFF){
    const a=x(scope.txStart), b=x(scope.txEnd<0xFFFF?scope.txEnd:scope.samples);
    g.fillStyle="rgba(87,157,255,.13)";
    g.fillRect(Math.max(0,a),0,Math.min(W,b)-Math.max(0,a),H);
  }

  // grid
  g.strokeStyle="#222a35"; g.fillStyle="#7d8796"; g.font="10px ui-monospace,Menlo,monospace"; g.lineWidth=1;
  const step = winMs<=30 ? 1 : winMs<=120 ? 5 : 20;
  for(let t=Math.ceil(startMs/step)*step; t<=startMs+winMs; t+=step){
    const px=(t-startMs)/winMs*W;
    g.beginPath(); g.moveTo(px,0); g.lineTo(px,H); g.stroke();
    g.fillText(t+" ms", px+3, 12);
  }
  g.fillText("high (idle)", 4, yHigh-6);
  g.fillText("low (asserted)", 4, yLow+14);

  // waveform
  g.strokeStyle="#57c785"; g.lineWidth=1.5; g.beginPath();
  for(let i=s0;i<s1;i++){
    const y=scope.bits[i]?yHigh:yLow;
    if(i===s0) g.moveTo(x(i),y);
    else if(scope.bits[i]!==scope.bits[i-1]) g.lineTo(x(i),scope.bits[i-1]?yHigh:yLow), g.lineTo(x(i),y);
  }
  g.lineTo(x(s1),scope.bits[s1-1]?yHigh:yLow);
  g.stroke();
}
window.addEventListener("resize", drawScope);

// ---------- Bus monitor ----------
const SPECIAL = {0xA1:"TERMINATE",0xA3:"DTR0",0xA5:"INITIALISE",0xA7:"RANDOMISE",0xA9:"COMPARE",0xAB:"WITHDRAW",0xAD:"PING",
  0xB1:"SEARCHADDRH",0xB3:"SEARCHADDRM",0xB5:"SEARCHADDRL",0xB7:"PROGRAM SHORT ADDRESS",0xB9:"VERIFY SHORT ADDRESS",
  0xBB:"QUERY SHORT ADDRESS",0xC1:"ENABLE DEVICE TYPE",0xC3:"DTR1",0xC5:"DTR2",0xC7:"WRITE MEMORY LOCATION",0xC9:"WRITE MEMORY LOCATION - NO REPLY"};
const CMDS = {0:"OFF",1:"UP",2:"DOWN",3:"STEP UP",4:"STEP DOWN",5:"RECALL MAX LEVEL",6:"RECALL MIN LEVEL",7:"STEP DOWN AND OFF",
  8:"ON AND STEP UP",9:"ENABLE DAPC SEQUENCE",10:"GO TO LAST ACTIVE LEVEL",32:"RESET",33:"STORE ACTUAL LEVEL IN DTR0",
  34:"SAVE PERSISTENT VARIABLES",35:"SET OPERATING MODE",36:"RESET MEMORY BANK",37:"IDENTIFY DEVICE",42:"SET MAX LEVEL",
  43:"SET MIN LEVEL",44:"SET SYSTEM FAILURE LEVEL",45:"SET POWER ON LEVEL",46:"SET FADE TIME",47:"SET FADE RATE",
  48:"SET EXTENDED FADE TIME",128:"SET SHORT ADDRESS",129:"ENABLE WRITE MEMORY",144:"QUERY STATUS",
  145:"QUERY CONTROL GEAR PRESENT",146:"QUERY LAMP FAILURE",147:"QUERY LAMP POWER ON",148:"QUERY LIMIT ERROR",
  149:"QUERY RESET STATE",150:"QUERY MISSING SHORT ADDRESS",151:"QUERY VERSION NUMBER",152:"QUERY CONTENT DTR0",
  153:"QUERY DEVICE TYPE",154:"QUERY PHYSICAL MINIMUM",155:"QUERY POWER FAILURE",156:"QUERY CONTENT DTR1",
  157:"QUERY CONTENT DTR2",158:"QUERY OPERATING MODE",159:"QUERY LIGHT SOURCE TYPE",160:"QUERY ACTUAL LEVEL",
  161:"QUERY MAX LEVEL",162:"QUERY MIN LEVEL",163:"QUERY POWER ON LEVEL",164:"QUERY SYSTEM FAILURE LEVEL",
  165:"QUERY FADE TIME/FADE RATE",166:"QUERY MANUFACTURER SPECIFIC MODE",167:"QUERY NEXT DEVICE TYPE",
  168:"QUERY EXTENDED FADE TIME",169:"QUERY CONTROL GEAR FAILURE",192:"QUERY GROUPS 0-7",193:"QUERY GROUPS 8-15",
  194:"QUERY RANDOM ADDRESS H",195:"QUERY RANDOM ADDRESS M",196:"QUERY RANDOM ADDRESS L",197:"READ MEMORY LOCATION",
  226:"DT8 ACTIVATE",231:"DT8 SET TEMPORARY Tc",249:"DT8 QUERY COLOUR TYPE FEATURES",250:"DT8 QUERY COLOUR VALUE"};

function cmdName(c){
  if(CMDS[c]) return CMDS[c];
  if(c>=16 && c<=31) return "GO TO SCENE "+(c-16);
  if(c>=64 && c<=79) return "SET SCENE "+(c-64);
  if(c>=80 && c<=95) return "REMOVE FROM SCENE "+(c-80);
  if(c>=96 && c<=111) return "ADD TO GROUP "+(c-96);
  if(c>=112 && c<=127) return "REMOVE FROM GROUP "+(c-112);
  if(c>=176 && c<=191) return "QUERY SCENE "+(c-176)+" LEVEL";
  if(c>=224) return "APPLICATION EXTENDED "+c;
  return "COMMAND "+c;
}

function decodeForward(b0,b1){
  if(SPECIAL[b0]!==undefined) return `${SPECIAL[b0]} ${b1} (0x${hex2(b1)})`;
  let target;
  if(b0<0x80) target="SA"+(b0>>1);
  else if((b0&0xE0)===0x80) target="G"+((b0>>1)&15);
  else if(b0>=0xFE) target="BROADCAST";
  else if(b0===0xFC || b0===0xFD) target="BROADCAST UNADDRESSED";
  else return tr("special/reserved");
  return (b0&1) ? `${target}: ${cmdName(b1)}` : tr("{t}: level {v}", {t:target, v:b1});
}

function hex2(v){ return Number(v).toString(16).padStart(2,"0").toUpperCase(); }

function traceMeaning(e){
  const bytes=(e.d.match(/../g)||[]).map(h=>parseInt(h,16));
  if(e.k===1){
    if(e.rv>=0) return tr("reply 0x{v} ({d}){yes}", {v:hex2(e.rv), d:e.rv, yes:e.rv===255?" = YES":""});
    if(e.rv===-3) return tr("collision (several gears answered or noise)");
    return tr("invalid reply, {b} bit(s)", {b:e.b});
  }
  if(e.k===3) return tr("undecodable bus activity");
  if(e.b===16) return decodeForward(bytes[0],bytes[1]);
  if(e.b===24) return tr("24-bit frame (input devices / DALI-2 part 103)");
  if(e.b===8) return tr("backward frame 0x{v} (late or stray reply)", {v:hex2(bytes[0])});
  return tr("{b}-bit frame", {b:e.b});
}

const KIND = {0:["TX","k-tx"],1:["REPLY","k-reply"],2:["BUS","k-foreign"],3:["BUS ERR","k-err"]};
let monEntries=[], monSeq=0;

let tracePolling=false;

async function pollTrace(){
  if(tracePolling || document.getElementById("monPause").checked) return;
  tracePolling=true;
  try{
    const r=await api("/api/trace?since="+monSeq);
    if(r.entries.length){
      monEntries=monEntries.concat(r.entries).slice(-300);
      renderMonitor();
    }
    monSeq=r.seq;
  } finally { tracePolling=false; }
}

function renderMonitor(){
  const onlyForeign=document.getElementById("monForeign").checked;
  const wrap=document.getElementById("monWrap");
  const atBottom = wrap.scrollTop+wrap.clientHeight >= wrap.scrollHeight-20;
  let prevT=null, html="";
  for(const e of monEntries){
    const delta = prevT===null ? "" : (e.t-prevT).toFixed(1);
    prevT=e.t;
    if(onlyForeign && e.k<2) continue;
    const [kind,cls]=KIND[e.k]||["?",""];
    const frame = e.k===1 ? (e.rv>=0 ? hex2(e.rv) : "—") : (e.d || "—");
    html+=`<tr><td>${e.s}</td><td>${e.t.toFixed(1)}</td><td>${delta}</td><td class="${cls}">${kind}</td>
      <td class="mono">${frame}</td><td>${escapeHtml(traceMeaning(e))}</td></tr>`;
  }
  document.getElementById("monBody").innerHTML = html || `<tr><td colspan="6" class="muted">No frames yet.</td></tr>`;
  if(atBottom) wrap.scrollTop=wrap.scrollHeight;
}

function clearMonitor(){
  monEntries=[];
  renderMonitor();
}

// ---------- Memory bank ----------
let memSeqSeen=0;

function verText(v){ return v>=0 ? `${v>>2}.${v&3}` : "—"; }

function decodeBank0(b){
  const ok=i=>b[i]!==undefined && b[i]>=0;
  const range=(a,z)=>{ for(let i=a;i<=z;i++) if(!ok(i)) return null; return b.slice(a,z+1); };
  const rows=[];
  const gtin=range(3,8);
  if(gtin) rows.push(["GTIN", gtin.reduce((acc,v)=>acc*256n+BigInt(v),0n).toString()]);
  if(ok(9)&&ok(10)) rows.push(["Firmware version", `${b[9]}.${b[10]}`]);
  const serial=range(0x0B,0x12);
  if(serial) rows.push(["Identification number", serial.map(hex2).join("")]);
  if(ok(0x13)&&ok(0x14)) rows.push(["Hardware version", `${b[0x13]}.${b[0x14]}`]);
  if(ok(0x15)) rows.push(["IEC 62386-101 version", verText(b[0x15])]);
  if(ok(0x16)) rows.push(["IEC 62386-102 version", verText(b[0x16])]);
  if(ok(0x17)) rows.push(["IEC 62386-103 version", verText(b[0x17])]);
  if(ok(0x19)) rows.push(["Logical control gear units", b[0x19]]);
  if(ok(2)) rows.push(["Last accessible memory bank", b[2]]);
  return rows;
}

async function loadMemory(){
  const m=await api("/api/memory");
  const card=document.getElementById("memCard");
  card.style.display="block";
  document.getElementById("memTitle").innerHTML=`<span>Memory bank</span> ${m.bank} · SA${m.sa}`;
  if(!m.bytes.length){
    document.getElementById("memDecoded").innerHTML=`<span class="bad">Bank not readable: the gear did not answer or does not implement it.</span>`;
    document.getElementById("memDump").textContent="";
    return;
  }
  let dump="     " + Array.from({length:16},(_,i)=>" "+hex2(i)).join("") + "\n";
  for(let r=0;r<m.bytes.length;r+=16){
    dump+=hex2(r)+":  "+m.bytes.slice(r,r+16).map(v=>v<0?" --":" "+hex2(v)).join("")+"\n";
  }
  document.getElementById("memDump").textContent=dump;
  if(m.bank===0){
    const rows=decodeBank0(m.bytes);
    document.getElementById("memDecoded").innerHTML = `<div class="device-grid">` +
      rows.map(r=>`<div class="device-field"><span class="k">${r[0]}</span><span class="v">${escapeHtml(String(r[1]))}</span></div>`).join("") +
      `</div><div class="small muted" style="margin-top:8px">Decoded with the DALI-2 layout (IEC 62386-102 ed2). DALI-1 gear may use a different layout, check the raw dump.</div>`;
  } else {
    document.getElementById("memDecoded").innerHTML=`<div class="small muted">Byte 0 = last accessible location. Layout depends on the bank and manufacturer; D4i banks 202–207 hold energy and diagnostics data.</div>`;
  }
  card.scrollIntoView({behavior:"smooth", block:"nearest"});
}

async function refresh(){
  try{
    const s=await api("/api/state");
    document.getElementById("foundCount").textContent=s.found;
    document.getElementById("busyBadge").textContent=s.busy ? "BUSY" : "IDLE";
    document.getElementById("busyBadge").className="badge "+(s.busy?"warn":"ok");
    document.getElementById("opText").textContent=s.operation+" · "+s.progress+"%";
    document.getElementById("progressBar").style.width=s.progress+"%";
    document.getElementById("lastResult").textContent=s.lastResult||"";
    lastDevices=s.devices;
    if(!sliderDragging) renderDevices(s.devices);
    renderStats(s);
    document.getElementById("scanDiff").textContent=s.scanDiff||"—";
    const autoHtml = s.autoScan ? `<span>active, every</span> ${s.autoScan} <span>min</span>` : `<span>off</span>`;
    const autoEl = document.getElementById("autoScanState");
    if(autoEl.dataset.html!==autoHtml){ autoEl.dataset.html=autoHtml; autoEl.innerHTML=autoHtml; }
    if(!autoScanEdited && document.activeElement!==document.getElementById("autoScan"))
      document.getElementById("autoScan").value=s.autoScan;
    if(s.traceSeq!==monSeq) pollTrace().catch(()=>{});
    if(s.scopeSeq && s.scopeSeq!==scopeSeqSeen){ scopeSeqSeen=s.scopeSeq; loadScope().catch(()=>{}); }
    if(s.memSeq && s.memSeq!==memSeqSeen){ memSeqSeen=s.memSeq; loadMemory().catch(()=>{}); }

    if(s.log!==lastLog){
      const el=document.getElementById("log");
      el.textContent=s.log;
      el.scrollTop=el.scrollHeight;
      lastLog=s.log;
    }
    document.querySelectorAll(".op").forEach(b=>b.disabled=s.busy);
  }catch(e){
    document.getElementById("opText").textContent=tr("Connection error:")+" "+e.message;
  }
}

new MutationObserver(muts => {
  for(const m of muts) for(const n of m.addedNodes) translateNode(n);
}).observe(document.body, {childList:true, subtree:true});
setLang(lang);

refresh();
setInterval(refresh,800);
</script>
</body>
</html>
)DALIWEB";
