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
button{border:1px solid #3b4552;background:#232a34;color:var(--txt);padding:9px 12px;border-radius:9px;cursor:pointer;font-weight:600}
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
@media(max-width:900px){.device-grid{grid-template-columns:repeat(2,minmax(140px,1fr))}}

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
      <span class="badge" id="busyBadge">Idle</span>
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

    <section class="card full">
      <div class="row" style="justify-content:space-between">
        <h2 style="margin:0">Log</h2>
        <button onclick="clearLog()">Clear log</button>
      </div>
      <pre class="log" id="log"></pre>
      <div class="small muted" id="lastResult"></div>
    </section>
  </div>
</main>
<div id="tooltip" class="tooltip"></div>

<script>
let lastLog = "";
const selectedSAs = new Set();
const expandedSAs = new Set();
const autoExpandAfterInspect = new Set();

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
  if(sa<0||sa>63) return alert("SA must be between 0 and 63.");
  if(!confirm("Is exactly ONE physical luminaire connected to the DALI bus?")) return;
  start("/api/readdress?sa="+sa);
}

function clearOne(){
  const sa = Number(document.getElementById("clearSa").value);
  if(sa<0||sa>63) return alert("SA must be between 0 and 63.");
  if(!confirm("Clear short address SA"+sa+"? The gear will become unaddressed.")) return;
  start("/api/clear-address?sa="+sa);
}

function clearAll(){
  if(!confirm("CLEAR the short address of ALL responding DALI control gear?")) return;
  if(prompt('Type CLEAR to confirm:') !== 'CLEAR') return;
  start("/api/clear-all?confirm=CLEAR");
}

function recommissionAll(){
  if(!confirm("REASSIGN addresses for ALL connected DALI control gear?")) return;
  if(prompt('Type READDRESS to confirm:') !== 'READDRESS') return;
  start("/api/recommission-all?confirm=READDRESS");
}

function deviceAction(sa, cmd){
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
  if(!sas.length) return alert("Select at least one device first.");
  start("/api/selected?cmd="+cmd+"&sas="+encodeURIComponent(sas.join(",")));
}

function broadcastAction(cmd){
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
  return `${hexByte(v)} · time=${fadeTime}, rate=${fadeRate}`;
}

function sceneHtml(scenes){
  if(!Array.isArray(scenes)) return "—";
  return scenes.map((v,i)=>{
    const text=(v===255) ? "MASK" : (v<0 ? "—" : v);
    return `<span class="scene-chip">S${i}: ${text}</span>`;
  }).join("");
}

function detailCard(d){
  if(!d.inspected){
    return `<div class="device-card ${expandedSAs.has(d.sa)?"open":""}" id="deviceCard${d.sa}">
      <div class="muted small">No extended data loaded. Click <b>Inspect</b>.</div>
    </div>`;
  }

  return `<div class="device-card ${expandedSAs.has(d.sa)?"open":""}" id="deviceCard${d.sa}">
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
      <div class="device-field"><span class="k">Status</span><span class="v">${statusText(d)}</span></div>
    </div>
    <div style="margin-top:15px">
      <div class="small muted">Scenes 0–15 <span class="help" data-tip="Scene value is the stored arc level for that DALI scene. MASK means the scene is not programmed.">?</span></div>
      <div class="scene-list">${sceneHtml(d.scenes)}</div>
    </div>
    <div class="small muted" style="margin-top:14px">
      DT8 colour-temperature capabilities and Memory Bank 0 identity are not decoded in this build yet.
    </div>
  </div>`;
}

function renderDevices(ds){
  const tb=document.getElementById("devices");
  tb.innerHTML="";

  for(const d of ds){
    if(!d.present) continue;

    if(d.inspected && autoExpandAfterInspect.has(d.sa)){
      expandedSAs.add(d.sa);
      autoExpandAfterInspect.delete(d.sa);
    }

    const tr=document.createElement("tr");
    tr.className="device-main";
    tr.innerHTML=`
      <td><input class="deviceSelect" data-sa="${d.sa}" type="checkbox"
          ${selectedSAs.has(d.sa) ? "checked" : ""}
          onchange="toggleSelected(${d.sa},this.checked)"></td>
      <td><b>SA${d.sa}</b></td>
      <td>${statusText(d)}</td>
      <td>${val(d.level)}</td>
      <td>${val(d.type)}</td>
      <td>${val(d.version)}</td>
      <td>${d.groups}</td>
      <td style="white-space:nowrap">
        <button class="op" onclick="inspectDeviceUI(${d.sa})">Inspect <span class="help" data-tip="Loads extended information for this device: configured levels, flags, device information, groups and scenes.">?</span></button>
        <button onclick="deviceAction(${d.sa},'identify')">Identify <span class="help" data-tip="Flashes this addressed luminaire three times so you can match its physical location to its short address.">?</span></button>
        <button onclick="deviceAction(${d.sa},'off')">Off</button>
        <button onclick="deviceAction(${d.sa},'on')">On</button>
        <button id="expandBtn${d.sa}" class="expand-btn" onclick="toggleDeviceCard(${d.sa})">${expandedSAs.has(d.sa)?"▴":"▾"}</button>
      </td>`;
    tb.appendChild(tr);

    const detail=document.createElement("tr");
    detail.className="device-detail";
    const td=document.createElement("td");
    td.colSpan=8;
    td.innerHTML=detailCard(d);
    detail.appendChild(td);
    tb.appendChild(detail);
  }

  updateSelectedCount();
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
    renderDevices(s.devices);

    if(s.log!==lastLog){
      const el=document.getElementById("log");
      el.textContent=s.log;
      el.scrollTop=el.scrollHeight;
      lastLog=s.log;
    }
    document.querySelectorAll(".op").forEach(b=>b.disabled=s.busy);
  }catch(e){
    document.getElementById("opText").textContent="Connection error: "+e.message;
  }
}

refresh();
setInterval(refresh,800);
</script>
</body>
</html>
)DALIWEB";
