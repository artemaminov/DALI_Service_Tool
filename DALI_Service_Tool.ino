#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include "esp_timer.h"
#include "hal/gpio_ll.h"
#include "soc/gpio_struct.h"
#include "DALI_Lib.h"
#include "web_page.h"

// ============================================================
// DALI Service Tool
// ESP32-C6 Pico + Waveshare Pico-DALI2
//
// Waveshare mapping verified for this hardware:
//   DALI TX -> GPIO14
//   DALI RX -> GPIO5
//
// Arduino-ESP32: tested target is 3.0.x style timer API.
// ============================================================

#define TX_PIN 14
#define RX_PIN 5

// ---------- Wi-Fi ----------
// The service AP is always created.
// Connect to:
//   SSID: DALI-Service
//   PASS: dali12345
//   URL : http://192.168.4.1
static const char* AP_SSID = "DALI-Service";
static const char* AP_PASS = "dali12345";

// Optional connection to your normal Wi-Fi.
// Leave STA_SSID empty to disable.
static const char* STA_SSID = "";
static const char* STA_PASS = "";

// ---------- Delays ----------
// Intentionally conservative for diagnosis.
static const uint32_t STEP_DELAY_MS       = 1000;
static const uint32_t RANDOMISE_DELAY_MS  = 2000;
static const uint32_t PROGRAM_DELAY_MS    = 1000;
static const uint32_t SEARCH_DELAY_MS     = 40;
static const uint32_t IDENTIFY_DELAY_MS   = 450;

Dali dali;
hw_timer_t* daliTimer = nullptr;
WebServer server(80);

// ============================================================
// DALI hardware callbacks
// ============================================================

// Called from the timer ISR: use the inline GPIO HAL (register access, IRAM-safe).
// digitalRead/digitalWrite and gpio_get/set_level are slower and live in flash.
uint8_t IRAM_ATTR bus_is_high() {
  return gpio_ll_get_level(&GPIO, RX_PIN);
}

void IRAM_ATTR bus_set_low() {
  gpio_ll_set_level(&GPIO, TX_PIN, 0);
}

void IRAM_ATTR bus_set_high() {
  gpio_ll_set_level(&GPIO, TX_PIN, 1);
}

void ARDUINO_ISR_ATTR onDaliTimer() {
  dali.timer();
}

// ============================================================
// State / device model
// ============================================================

struct DeviceInfo {
  bool present = false;
  bool replyValid = false;
  bool inspected = false;

  int16_t status = -999;
  int16_t level = -999;
  int16_t deviceType = -999;
  int16_t version = -999;
  int16_t groups07 = -999;
  int16_t groups815 = -999;

  // Extra information loaded only by Inspect.
  int16_t physicalMin = -999;
  int16_t minLevel = -999;
  int16_t maxLevel = -999;
  int16_t powerOnLevel = -999;
  int16_t systemFailureLevel = -999;
  int16_t fadeTimeRate = -999;
  int16_t operatingMode = -999;
  int16_t lightSourceType = -999;
  int16_t controlGearFailure = -999;
  int16_t lampFailure = -999;
  int16_t lampPowerOn = -999;
  int16_t resetState = -999;
  int16_t powerFailure = -999;
  int16_t nextDeviceType = -999;

  // DT8 colour temperature in mirek (1e6 / Kelvin). 0xFFFF means MASK / unknown.
  int32_t tc = -999;
  int32_t tcCoolest = -999;
  int32_t tcWarmest = -999;

  // 16 scene levels. 0xFF means MASK / scene not programmed.
  int16_t scenes[16] = {
    -999,-999,-999,-999,-999,-999,-999,-999,
    -999,-999,-999,-999,-999,-999,-999,-999
  };
};

DeviceInfo devices[64];
volatile bool busy = false;
volatile uint8_t progressPct = 0;
String currentOperation = "Idle";
String lastResult = "";
String logText = "";
String lastReport = "";
SemaphoreHandle_t stateMutex = nullptr;
SemaphoreHandle_t busMutex = nullptr; // held by DALI operations; the bus monitor reads only while it is free

// Device labels, stored in NVS
Preferences prefs;
String labels[64];

// Scan history and per-address reliability statistics
struct SaStats {
  uint32_t ok = 0;
  uint32_t invalid = 0;
  uint32_t collision = 0;
};
SaStats saStats[64];
uint32_t statRounds = 0;
uint64_t lastScanMask = 0;
bool haveLastScan = false;
String lastScanDiff = "";
uint32_t autoScanMinutes = 0;
uint32_t lastAutoScanMs = 0;

// Bus trace ring buffer (our frames, replies and frames of other bus participants)
#define TRACE_FOREIGN 2      // frame sent by another bus participant
#define TRACE_FOREIGN_ERR 3  // undecodable bus activity
struct TraceEntry {
  uint32_t seq;
  int64_t us;
  uint8_t kind;
  uint8_t bits;
  uint8_t data[4];
  int16_t rv;
};
static const int TRACE_SIZE = 200;
TraceEntry traceBuf[TRACE_SIZE];
uint32_t traceSeq = 0;
SemaphoreHandle_t traceMutex = nullptr;

// Waveform capture of the RX pin
static const uint16_t SCOPE_MAX_SAMPLES = 4096; // 427 ms at 9600 Hz
uint8_t scopeBuf[SCOPE_MAX_SAMPLES / 8];
uint16_t scopeSamples = 0;
uint16_t scopeTxStart = 0xFFFF;
uint16_t scopeTxEnd = 0xFFFF;
uint32_t scopeSeq = 0;
String scopeTitle = "";

// Last memory bank read
struct MemoryDump {
  int sa = -1;
  int bank = -1;
  uint8_t count = 0;
  int16_t bytes[64];
};
MemoryDump memDump;
uint32_t memSeq = 0;

enum OpType {
  OP_NONE,
  OP_SCAN,
  OP_SCAN_DETAILED,
  OP_FIND_UNADDRESSED,
  OP_ASSIGN_UNADDRESSED,
  OP_READDRESS_SINGLE,
  OP_RAW_COMPARE,
  OP_CLEAR_SELECTED,
  OP_CLEAR_ALL,
  OP_RECOMMISSION_ALL,
  OP_IDENTIFY,
  OP_INSPECT,
  OP_ON,
  OP_OFF,
  OP_SELECTED_ON,
  OP_SELECTED_OFF,
  OP_BROADCAST_ON,
  OP_BROADCAST_OFF,
  OP_SET_TC,
  OP_BROADCAST_TC,
  OP_PRESENCE_TEST,
  OP_RAW,
  OP_DIM_TEST,
  OP_TC_TEST,
  OP_MEMORY,
  OP_RESET_DEVICE,
  OP_CONFIG,
  OP_GROUPS,
  OP_SCENES,
  OP_TARGET,
  OP_SCAN_ROUNDS
};

struct PendingOp {
  OpType type = OP_NONE;
  int sa = -1;
  uint64_t mask = 0;
  int value = 0;
  int value2 = 0;
  int16_t p[16];  // per-operation parameters, -1 = not set

  PendingOp() {
    for (int i = 0; i < 16; i++) p[i] = -1;
  }
};

PendingOp pendingOp;
PendingOp requestOp; // filled by HTTP handlers, then started with startRequested()

// ============================================================
// Utility
// ============================================================

String timestamp() {
  uint32_t s = millis() / 1000;
  uint32_t h = s / 3600;
  uint32_t m = (s % 3600) / 60;
  uint32_t sec = s % 60;
  char b[20];
  snprintf(b, sizeof(b), "%02lu:%02lu:%02lu", (unsigned long)h, (unsigned long)m, (unsigned long)sec);
  return String(b);
}

void addLog(const String& s) {
  String line = timestamp() + "  " + s + "\n";
  if (stateMutex) xSemaphoreTake(stateMutex, portMAX_DELAY);
  logText += line;
  if (logText.length() > 18000) {
    logText.remove(0, logText.length() - 15000);
  }
  if (stateMutex) xSemaphoreGive(stateMutex);
  Serial.print(line);
}

String rvName(int16_t rv) {
  if (rv >= 0) return "OK";
  if (rv == -DALI_RESULT_NO_REPLY) return "NO REPLY";
  if (rv == -DALI_RESULT_INVALID_REPLY) return "INVALID REPLY";
  if (rv == -DALI_RESULT_COLLISION) return "COLLISION";
  if (rv == -DALI_RESULT_TIMEOUT) return "TIMEOUT";
  return "ERROR " + String(rv);
}

String jsonEscape(const String& in) {
  String out;
  out.reserve(in.length() + 32);
  for (size_t i = 0; i < in.length(); i++) {
    char c = in[i];
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"':  out += "\\\""; break;
      case '\n': out += "\\n"; break;
      case '\r': break;
      case '\t': out += "\\t"; break;
      default:
        if ((uint8_t)c >= 0x20) out += c;
        break;
    }
  }
  return out;
}

void setOperation(const String& name) {
  if (stateMutex) xSemaphoreTake(stateMutex, portMAX_DELAY);
  currentOperation = name;
  progressPct = 0;
  lastResult = "";
  if (stateMutex) xSemaphoreGive(stateMutex);
}

void finishOperation(const String& result) {
  if (stateMutex) xSemaphoreTake(stateMutex, portMAX_DELAY);
  lastResult = result;
  currentOperation = "Idle";
  progressPct = 100;
  busy = false;
  if (stateMutex) xSemaphoreGive(stateMutex);
  addLog("DONE: " + result);
}

void clearDeviceTable() {
  for (int i = 0; i < 64; i++) devices[i] = DeviceInfo();
}

int countDevices() {
  int n = 0;
  for (int i = 0; i < 64; i++) if (devices[i].present) n++;
  return n;
}

int firstFreeShortAddress(const bool used[64]) {
  for (int i = 0; i < 64; i++) if (!used[i]) return i;
  return -1;
}

// ============================================================
// DALI helpers
// ============================================================

void traceAdd(uint8_t kind, const uint8_t* data, uint8_t bits, int16_t rv) {
  if (!traceMutex) return;
  xSemaphoreTake(traceMutex, portMAX_DELAY);
  TraceEntry& e = traceBuf[traceSeq % TRACE_SIZE];
  e.seq = ++traceSeq;
  e.us = esp_timer_get_time();
  e.kind = kind;
  e.bits = bits;
  e.rv = rv;
  memset(e.data, 0, sizeof(e.data));
  if (data) {
    int n = (bits + 7) / 8;
    memcpy(e.data, data, n > 4 ? 4 : n);
  }
  xSemaphoreGive(traceMutex);
}

// Records frames that nobody asked for: other masters, input devices, late or stray replies.
void monitorTask(void* parameter) {
  uint8_t buf[8]; // a garbled frame can decode to more than 32 bits
  for (;;) {
    if (xSemaphoreTake(busMutex, 0) == pdTRUE) {
      uint8_t rv = dali.rx(buf);
      xSemaphoreGive(busMutex);
      if (rv == 2) traceAdd(TRACE_FOREIGN_ERR, nullptr, 0, -DALI_RESULT_COLLISION);
      else if (rv > 2) traceAdd(TRACE_FOREIGN, buf, rv, 0);
    }
    vTaskDelay(pdMS_TO_TICKS(2));
  }
}

void startScope(uint16_t samples) {
  if (samples > SCOPE_MAX_SAMPLES) samples = SCOPE_MAX_SAMPLES;
  dali.capture_start(scopeBuf, samples);
}

void finishScope(const String& title) {
  uint32_t t0 = millis();
  while (dali.cap_active && millis() - t0 < 1000) delay(5);
  dali.cap_active = 0;
  scopeSamples = dali.cap_pos;
  scopeTxStart = dali.cap_tx_start;
  scopeTxEnd = dali.cap_tx_end;
  scopeTitle = title;
  scopeSeq++;
}

String hexBytes(const uint8_t* data, int n) {
  String s;
  char b[4];
  for (int i = 0; i < n; i++) {
    snprintf(b, sizeof(b), "%02X", data[i]);
    s += b;
  }
  return s;
}

void resetRxDiag() {
  dali.diag_tx_samples = 0;
  dali.diag_tx_low_samples = 0;
  dali.diag_rx_starts = 0;
}

// While transmitting, the RX pin must echo our own frame: a 16-bit forward frame has
// 17 low half-bits out of 38 (start + one per data bit; stop bits are high) = ~45%.
// No echo means RX does not see the bus at all: wrong pin, inverted level or hardware.
String rxDiagLine() {
  uint32_t tx = dali.diag_tx_samples;
  uint32_t low = dali.diag_tx_low_samples;
  uint32_t pct = tx ? (low * 100 / tx) : 0;
  String s = "RX DIAG: TX samples=" + String(tx) +
             ", RX low during TX=" + String(low) + " (" + String(pct) + "%, expect ~45%)" +
             ", RX starts while idle=" + String(dali.diag_rx_starts) +
             ", RX pin now=" + String(bus_is_high());
  return s;
}

void readDevice(int sa, bool detailed) {
  DeviceInfo d;
  d.status = dali.cmd(DALI_QUERY_STATUS, sa);

  d.replyValid = d.status >= 0;
  d.present = d.replyValid ||
              d.status == -DALI_RESULT_INVALID_REPLY ||
              d.status == -DALI_RESULT_COLLISION;

  // An INVALID/COLLISION reply still proves there was backward-bus activity
  // at this short address, but we must not pretend the returned byte is valid.
  if (d.replyValid && detailed) {
    delay(20);
    d.level = dali.cmd(DALI_QUERY_ACTUAL_LEVEL, sa);
    delay(20);
    d.deviceType = dali.cmd(DALI_QUERY_DEVICE_TYPE, sa);
    delay(20);
    d.version = dali.cmd(DALI_QUERY_VERSION_NUMBER, sa);
    delay(20);
    d.groups07 = dali.cmd(DALI_QUERY_GROUPS_0_7, sa);
    delay(20);
    d.groups815 = dali.cmd(DALI_QUERY_GROUPS_8_15, sa);
  }
  devices[sa] = d;
}

int scanBus(bool detailed, bool finalize = true) {
  setOperation(detailed ? "Detailed scan" : "Scan");
  clearDeviceTable();
  resetRxDiag();
  addLog(detailed ? "Detailed scan SA0..SA63" : "Scan SA0..SA63");

  String report = "DALI SERVICE TOOL REPORT\n";
  report += detailed ? "Mode: detailed scan\n\n" : "Mode: scan\n\n";

  int found = 0;
  for (int sa = 0; sa < 64; sa++) {
    readDevice(sa, detailed);
    DeviceInfo &d = devices[sa];

    String line = "SA" + String(sa) + " -> ";
    if (d.present) {
      found++;
      if (d.replyValid) {
        line += "FOUND status=0x";
        char b[8];
        snprintf(b, sizeof(b), "%02X", (uint8_t)d.status);
        line += b;

        if (detailed) {
          line += " level=" + String(d.level);
          line += " type=" + String(d.deviceType);
          line += " version=" + String(d.version);
          line += " groups0-7=" + String(d.groups07);
          line += " groups8-15=" + String(d.groups815);
        }
      } else {
        line += "BUS ACTIVITY, " + rvName(d.status) + " rv=" + String(d.status);
      }
    } else {
      line += rvName(d.status) + " rv=" + String(d.status);
    }

    if (d.replyValid) saStats[sa].ok++;
    else if (d.status == -DALI_RESULT_INVALID_REPLY) saStats[sa].invalid++;
    else if (d.status == -DALI_RESULT_COLLISION) saStats[sa].collision++;

    addLog(line);
    report += line + "\n";
    progressPct = (uint8_t)(((sa + 1) * 100) / 64);
    delay(10);
  }
  statRounds++;

  // Compare with the previous scan
  uint64_t mask = 0;
  for (int sa = 0; sa < 64; sa++) if (devices[sa].present) mask |= (1ULL << sa);
  String diff;
  if (!haveLastScan) {
    diff = "first scan";
  } else {
    for (int sa = 0; sa < 64; sa++) {
      bool now = mask & (1ULL << sa);
      bool before = lastScanMask & (1ULL << sa);
      if (now && !before) diff += (diff.length() ? " " : "") + String("+SA") + String(sa);
      if (!now && before) diff += (diff.length() ? " " : "") + String("-SA") + String(sa);
    }
    if (!diff.length()) diff = "no changes";
  }
  lastScanMask = mask;
  haveLastScan = true;
  lastScanDiff = diff;
  addLog("Compared with previous scan: " + diff);
  report += "Compared with previous scan: " + diff + "\n";

  addLog(rxDiagLine());
  report += rxDiagLine() + "\n";
  report += "\nFound: " + String(found) + "\n";
  lastReport = report;
  if (finalize) finishOperation("Scan complete. Found " + String(found) + " device(s)");
  return found;
}

// Slow, observable variant of the library's binary random-address search.
uint32_t findRandomAddressSlow(bool verbose) {
  uint32_t adr = 0x800000;
  uint32_t addsub = 0x400000;
  uint32_t adrLast = adr;

  dali.set_searchaddr(adr);
  delay(SEARCH_DELAY_MS);

  while (addsub) {
    dali.set_searchaddr_diff(adr, adrLast);
    adrLast = adr;
    delay(SEARCH_DELAY_MS);

    uint8_t cmp = dali.compare();

    if (verbose) {
      char line[96];
      snprintf(line, sizeof(line), "SEARCH 0x%06lX -> COMPARE=%u",
               (unsigned long)adr, (unsigned int)cmp);
      addLog(line);
    }

    if (cmp) adr -= addsub;
    else     adr += addsub;

    addsub >>= 1;
  }

  dali.set_searchaddr_diff(adr, adrLast);
  adrLast = adr;
  delay(SEARCH_DELAY_MS);

  if (!dali.compare()) {
    adr++;
    dali.set_searchaddr_diff(adr, adrLast);
    delay(SEARCH_DELAY_MS);
  }

  return adr;
}

void findUnaddressed() {
  setOperation("Find unaddressed");
  addLog("INITIALISE 0xFF (unaddressed only)");
  int16_t rv = dali.cmd(DALI_INITIALISE, 0xFF);
  addLog("INITIALISE rv=" + String(rv) + " (" + rvName(rv) + ")");
  delay(STEP_DELAY_MS);

  addLog("RANDOMISE");
  rv = dali.cmd(DALI_RANDOMISE, 0x00);
  addLog("RANDOMISE rv=" + String(rv) + " (" + rvName(rv) + ")");
  delay(RANDOMISE_DELAY_MS);

  addLog("SEARCHADDR = 0xFFFFFF");
  dali.set_searchaddr(0xFFFFFF);
  delay(STEP_DELAY_MS);

  uint8_t cmp = dali.compare();
  addLog("COMPARE=" + String(cmp));

  delay(STEP_DELAY_MS);
  dali.cmd(DALI_TERMINATE, 0x00);

  lastReport = "Find unaddressed\nCOMPARE=" + String(cmp) + "\n";
  finishOperation(cmp ? "Unaddressed device(s) detected" : "No unaddressed response detected");
}

int commissionSlow(uint8_t initArg, bool reassignAll) {
  bool used[64] = {false};

  // Preserve occupied short addresses only when assigning previously-unaddressed gear.
  if (!reassignAll) {
    addLog("Reading currently used short addresses...");
    for (int sa = 0; sa < 64; sa++) {
      int16_t rv = dali.cmd(DALI_QUERY_STATUS, sa);
      if (rv >= 0 ||
          rv == -DALI_RESULT_INVALID_REPLY ||
          rv == -DALI_RESULT_COLLISION) used[sa] = true;
      progressPct = (uint8_t)((sa + 1) * 20 / 64);
      delay(10);
    }
  }

  char initLine[48];
  snprintf(initLine, sizeof(initLine), "INITIALISE 0x%02X", initArg);
  addLog(initLine);
  dali.cmd(DALI_INITIALISE, initArg);
  delay(STEP_DELAY_MS);

  addLog("RANDOMISE");
  dali.cmd(DALI_RANDOMISE, 0x00);
  delay(RANDOMISE_DELAY_MS);

  int assigned = 0;
  int verifyFailures = 0;

  while (assigned < 64) {
    uint32_t randomAdr = findRandomAddressSlow(true);

    if (randomAdr > 0xFFFFFF) {
      addLog("No more random addresses found");
      break;
    }

    int sa = firstFreeShortAddress(used);
    if (sa < 0) {
      addLog("No free short addresses");
      break;
    }

    char line[120];
    snprintf(line, sizeof(line),
             "Random 0x%06lX -> programming SA%d",
             (unsigned long)randomAdr, sa);
    addLog(line);

    // findRandomAddressSlow leaves SEARCHADDR on the selected random address.
    dali.program_short_address((uint8_t)sa);
    delay(PROGRAM_DELAY_MS);

    // Do not use query_short_address() here: that helper returns uint8_t,
    // so negative DALI errors get shifted/cast into misleading values such as 203.
    int16_t qraw = dali.cmd(DALI_QUERY_SHORT_ADDRESS, 0x00);
    addLog("QUERY SHORT ADDRESS raw -> " + String(qraw) + " (" + rvName(qraw) + ")");

    int16_t vraw = dali.cmd(DALI_VERIFY_SHORT_ADDRESS, (sa << 1) | 0x01);
    addLog("VERIFY SHORT ADDRESS SA" + String(sa) + " -> " +
           String(vraw) + " (" + rvName(vraw) + ")");

    bool verified = vraw == 0xFF;
    if (qraw >= 0) {
      int decodedSa = ((uint8_t)qraw) >> 1;
      addLog("QUERY SHORT ADDRESS decoded -> SA" + String(decodedSa));
      if (decodedSa == sa) {
        addLog("VERIFY CLEAN");
        verified = true;
      } else {
        addLog("VERIFY WARNING: decoded address differs");
      }
    } else if (qraw == -DALI_RESULT_INVALID_REPLY ||
               qraw == -DALI_RESULT_COLLISION ||
               vraw == -DALI_RESULT_INVALID_REPLY ||
               vraw == -DALI_RESULT_COLLISION) {
      addLog("VERIFY: backward activity detected, but frame decoding is not clean");
      verified = true;
    } else {
      addLog("VERIFY WARNING: no clean backward confirmation");
    }

    // No confirmation usually means a lost COMPARE reply sent the binary search
    // to a wrong random address and no gear was selected. Keep SA free and
    // search again instead of withdrawing nothing.
    if (!verified) {
      if (++verifyFailures >= 3) {
        addLog("Giving up: 3 consecutive unverified assignments");
        break;
      }
      addLog("SA" + String(sa) + " not confirmed, repeating search");
      continue;
    }
    verifyFailures = 0;

    used[sa] = true;
    assigned++;

    addLog("WITHDRAW selected gear");
    dali.cmd(DALI_WITHDRAW, 0x00);
    delay(STEP_DELAY_MS);

    progressPct = min(95, 20 + assigned * 10);
  }

  addLog("TERMINATE");
  dali.cmd(DALI_TERMINATE, 0x00);
  delay(STEP_DELAY_MS);
  return assigned;
}

void assignUnaddressed() {
  setOperation("Assign unaddressed");
  int n = commissionSlow(0xFF, false);
  int found = scanBus(false, false);
  finishOperation("Assigned " + String(n) + " previously-unaddressed device(s); bus now has " + String(found));
}

void recommissionAll() {
  setOperation("Recommission all");
  addLog("WARNING: reassigning ALL control gear");
  int n = commissionSlow(0x00, true);
  int found = scanBus(false, false);
  finishOperation("Recommission complete. Assigned " + String(n) + " device(s); bus now has " + String(found));
}

void readdressSingle(int newSa) {
  setOperation("Readdress single");
  addLog("ONLY ONE physical luminaire should be connected");
  addLog("Target new address: SA" + String(newSa));

  dali.cmd(DALI_INITIALISE, 0x00);
  addLog("INITIALISE 0x00");
  delay(STEP_DELAY_MS);

  dali.cmd(DALI_RANDOMISE, 0x00);
  addLog("RANDOMISE");
  delay(RANDOMISE_DELAY_MS);

  dali.set_searchaddr(0xFFFFFF);
  delay(STEP_DELAY_MS);
  uint8_t cmp = dali.compare();
  addLog("COMPARE @ FFFFFF = " + String(cmp));

  if (!cmp) {
    dali.cmd(DALI_TERMINATE, 0x00);
    finishOperation("FAILED: no commissioning response");
    return;
  }

  uint32_t randomAdr = findRandomAddressSlow(true);
  char b[64];
  snprintf(b, sizeof(b), "find random -> 0x%08lX", (unsigned long)randomAdr);
  addLog(b);

  if (randomAdr > 0xFFFFFF) {
    dali.cmd(DALI_TERMINATE, 0x00);
    finishOperation("FAILED: random address not found");
    return;
  }

  delay(STEP_DELAY_MS);
  dali.set_searchaddr(randomAdr);
  delay(STEP_DELAY_MS);

  cmp = dali.compare();
  addLog("COMPARE exact = " + String(cmp));
  if (!cmp) {
    dali.cmd(DALI_TERMINATE, 0x00);
    finishOperation("FAILED: exact device not selected");
    return;
  }

  addLog("PROGRAM SHORT ADDRESS SA" + String(newSa));
  dali.program_short_address((uint8_t)newSa);
  delay(PROGRAM_DELAY_MS);

  int16_t qraw = dali.cmd(DALI_QUERY_SHORT_ADDRESS, 0x00);
  addLog("QUERY SHORT ADDRESS raw = " + String(qraw) + " (" + rvName(qraw) + ")");

  int16_t vraw = dali.cmd(DALI_VERIFY_SHORT_ADDRESS, (newSa << 1) | 0x01);
  addLog("VERIFY SHORT ADDRESS SA" + String(newSa) + " -> " +
         String(vraw) + " (" + rvName(vraw) + ")");

  bool queryClean = false;
  if (qraw >= 0) {
    int decodedSa = ((uint8_t)qraw) >> 1;
    addLog("QUERY SHORT ADDRESS decoded = SA" + String(decodedSa));
    queryClean = decodedSa == newSa;
  }

  delay(STEP_DELAY_MS);
  dali.cmd(DALI_TERMINATE, 0x00);
  delay(STEP_DELAY_MS);

  int16_t st = dali.cmd(DALI_QUERY_STATUS, newSa);
  addLog("QUERY STATUS SA" + String(newSa) + " -> " + String(st) +
         " (" + rvName(st) + ")");

  bool addressedActivity =
      st >= 0 ||
      st == -DALI_RESULT_INVALID_REPLY ||
      st == -DALI_RESULT_COLLISION;

  if (queryClean && st >= 0) {
    addLog("Address change succeeded; starting automatic scan");
    int found = scanBus(false, false);
    finishOperation("SUCCESS: device is now SA" + String(newSa) +
                    "; scan found " + String(found) + " device(s)");
  } else if (addressedActivity) {
    addLog("Addressed bus activity detected; starting automatic scan");
    int found = scanBus(false, false);
    finishOperation("LIKELY SUCCESS: SA" + String(newSa) +
                    " responds, but backward-frame decoding is not clean; scan found " +
                    String(found) + " device(s)");
  } else {
    finishOperation("WARNING: address programming was sent, but no addressed response was detected");
  }
}

void rawCompare20() {
  setOperation("RAW COMPARE x20");
  resetRxDiag();

  addLog("INITIALISE 0xFF");
  dali.cmd(DALI_INITIALISE, 0xFF);
  delay(STEP_DELAY_MS);

  addLog("RANDOMISE");
  dali.cmd(DALI_RANDOMISE, 0x00);
  delay(RANDOMISE_DELAY_MS);

  addLog("SEARCHADDR = 0xFFFFFF");
  dali.set_searchaddr(0xFFFFFF);
  delay(STEP_DELAY_MS);

  int yes = 0, noReply = 0, invalid = 0, collision = 0, other = 0;
  String report = "RAW COMPARE x20\n";

  for (int i = 0; i < 20; i++) {
    int16_t rv = dali.cmd(DALI_COMPARE, 0x00);
    String tag;

    if (rv == 0xFF) {
      yes++; tag = "YES 0xFF";
    } else if (rv == -DALI_RESULT_NO_REPLY) {
      noReply++; tag = "NO REPLY";
    } else if (rv == -DALI_RESULT_INVALID_REPLY) {
      invalid++; tag = "INVALID";
    } else if (rv == -DALI_RESULT_COLLISION) {
      collision++; tag = "COLLISION";
    } else {
      other++; tag = "OTHER";
    }

    String line = String(i + 1) + ": rv=" + String(rv) + " " + tag;
    addLog(line);
    report += line + "\n";

    progressPct = (uint8_t)((i + 1) * 100 / 20);
    delay(100);
  }

  dali.cmd(DALI_TERMINATE, 0x00);

  addLog(rxDiagLine());
  report += rxDiagLine() + "\n";
  report += "\nYES=" + String(yes);
  report += "\nNO_REPLY=" + String(noReply);
  report += "\nINVALID=" + String(invalid);
  report += "\nCOLLISION=" + String(collision);
  report += "\nOTHER=" + String(other) + "\n";
  lastReport = report;

  finishOperation("COMPARE: YES " + String(yes) +
                  ", NO REPLY " + String(noReply) +
                  ", INVALID " + String(invalid) +
                  ", COLLISION " + String(collision));
}


void inspectDevice(int sa) {
  if (sa < 0 || sa > 63 || !devices[sa].present) {
    finishOperation("Inspect failed: SA" + String(sa) + " is not in the current scan");
    return;
  }

  setOperation("Inspect SA" + String(sa));
  addLog("Inspecting SA" + String(sa));

  DeviceInfo d = devices[sa];

  // Keep the operation observable and conservative: one query at a time.
  d.status = dali.cmd(DALI_QUERY_STATUS, sa); delay(20);
  d.level = dali.cmd(DALI_QUERY_ACTUAL_LEVEL, sa); delay(20);
  d.version = dali.cmd(DALI_QUERY_VERSION_NUMBER, sa); delay(20);
  d.deviceType = dali.cmd(DALI_QUERY_DEVICE_TYPE, sa); delay(20);

  d.physicalMin = dali.cmd(DALI_QUERY_PHYSICAL_MINIMUM_LEVEL, sa); delay(20);
  d.minLevel = dali.cmd(DALI_QUERY_MIN_LEVEL, sa); delay(20);
  d.maxLevel = dali.cmd(DALI_QUERY_MAX_LEVEL, sa); delay(20);
  d.powerOnLevel = dali.cmd(DALI_QUERY_POWER_ON_LEVEL, sa); delay(20);
  d.systemFailureLevel = dali.cmd(DALI_QUERY_SYSTEM_FAILURE_LEVEL, sa); delay(20);
  d.fadeTimeRate = dali.cmd(DALI_QUERY_FADE_TIME_FADE_RATE, sa); delay(20);

  d.operatingMode = dali.cmd(DALI_QUERY_OPERATING_MODE_DALI2, sa); delay(20);
  d.lightSourceType = dali.cmd(DALI_QUERY_LIGHT_SOURCE_TYPE, sa); delay(20);
  d.controlGearFailure = dali.cmd(DALI_QUERY_CONTROL_GEAR_FAILURE, sa); delay(20);
  d.lampFailure = dali.cmd(DALI_QUERY_LAMP_FAILURE, sa); delay(20);
  d.lampPowerOn = dali.cmd(DALI_QUERY_LAMP_POWER_ON, sa); delay(20);
  d.resetState = dali.cmd(DALI_QUERY_RESET_STATE, sa); delay(20);
  d.powerFailure = dali.cmd(DALI_QUERY_POWER_FAILURE, sa); delay(20);

  d.groups07 = dali.cmd(DALI_QUERY_GROUPS_0_7, sa); delay(20);
  d.groups815 = dali.cmd(DALI_QUERY_GROUPS_8_15, sa); delay(20);

  // DALI-2 may expose additional device types. Keep raw reply visible rather
  // than guessing capabilities from it.
  d.nextDeviceType = dali.cmd(DALI_QUERY_NEXT_DEVICE_TYPE, sa); delay(20);

  for (int i = 0; i < 16; i++) {
    d.scenes[i] = dali.cmd(DALI_QUERY_SCENE0_LEVEL + i, sa);
    delay(18);
    progressPct = 55 + (uint8_t)(((i + 1) * 40) / 16);
  }

  // DT8 colour temperature. Gear without DT8 does not answer and keeps -101.
  d.tc = dali.query_colour_value(DALI_DT8_SELECTOR_TC, sa); delay(20);
  d.tcCoolest = dali.query_colour_value(DALI_DT8_SELECTOR_TC_COOLEST, sa); delay(20);
  d.tcWarmest = dali.query_colour_value(DALI_DT8_SELECTOR_TC_WARMEST, sa); delay(20);

  d.inspected = true;
  devices[sa] = d;

  addLog("SA" + String(sa) +
         " inspect: level=" + String(d.level) +
         " min=" + String(d.minLevel) +
         " max=" + String(d.maxLevel) +
         " physicalMin=" + String(d.physicalMin) +
         " type=" + String(d.deviceType) +
         " nextType=" + String(d.nextDeviceType) +
         " tc=" + String(d.tc) +
         " tcCoolest=" + String(d.tcCoolest) +
         " tcWarmest=" + String(d.tcWarmest));

  finishOperation("Inspect complete for SA" + String(sa));
}

void identifyDevice(int sa) {
  setOperation("Identify SA" + String(sa));

  int16_t oldLevel = dali.cmd(DALI_QUERY_ACTUAL_LEVEL, sa);
  if (oldLevel < 0) oldLevel = 200;

  for (int i = 0; i < 3; i++) {
    dali.set_level(0, sa);
    delay(IDENTIFY_DELAY_MS);
    dali.set_level(200, sa);
    delay(IDENTIFY_DELAY_MS);
  }

  dali.set_level((uint8_t)oldLevel, sa);
  finishOperation("Identify SA" + String(sa) + " complete");
}

void setDeviceLevel(int sa, uint8_t level) {
  setOperation(level ? "ON" : "OFF");
  dali.set_level(level, sa);
  devices[sa].level = level;
  delay(200);
  finishOperation("SA" + String(sa) + (level ? " ON" : " OFF"));
}


void setDeviceTc(int sa, uint16_t mirek) {
  setOperation("Colour temperature SA" + String(sa));
  dali.set_colour_temperature(mirek, (uint8_t)sa);
  devices[sa].tc = mirek;
  addLog("SA" + String(sa) + " Tc -> " + String(mirek) + " mirek (" +
         String(1000000UL / mirek) + " K)");
  finishOperation("SA" + String(sa) + " colour temperature " + String(1000000UL / mirek) + " K");
}

void setBroadcastTc(uint16_t mirek) {
  setOperation("Broadcast colour temperature");

  // 0x7F is the DALI broadcast address for regular (and DT8) commands.
  dali.set_colour_temperature(mirek, 0x7F);
  for (int sa = 0; sa < 64; sa++) {
    if (devices[sa].present) devices[sa].tc = mirek;
  }
  addLog("BROADCAST Tc -> " + String(mirek) + " mirek (" + String(1000000UL / mirek) + " K)");

  finishOperation("Broadcast colour temperature " + String(1000000UL / mirek) + " K sent");
}

void setSelectedLevel(uint64_t mask, uint8_t level) {
  setOperation(level ? "Selected ON" : "Selected OFF");

  int count = 0;
  for (int sa = 0; sa < 64; sa++) {
    if (mask & (1ULL << sa)) {
      dali.set_level(level, (uint8_t)sa);
      addLog("SA" + String(sa) + (level ? " -> ON" : " -> OFF"));
      count++;
      delay(80);
    }
  }

  if (count == 0) {
    finishOperation("No devices selected");
    return;
  }

  finishOperation("Selected " + String(count) + " device(s) " + (level ? "ON" : "OFF"));
}

void setBroadcastLevel(uint8_t level) {
  setOperation(level ? "Broadcast ON" : "Broadcast OFF");

  // 0xFF is DALI broadcast address for Direct Arc Power Control.
  dali.set_level(level, 0xFF);
  for (int sa = 0; sa < 64; sa++) if (devices[sa].present) devices[sa].level = level;
  addLog(String("BROADCAST -> ") + (level ? "ON (level 200)" : "OFF (level 0)"));
  delay(200);

  finishOperation(level ? "Broadcast ON sent" : "Broadcast OFF sent");
}

// Clear one existing short address by writing MASK (0xFF) to DTR0
// and executing SET SHORT ADDRESS for the addressed gear.
void clearSelectedAddress(int sa) {
  setOperation("Clear selected address");
  addLog("Clearing short address of SA" + String(sa));

  int16_t before = dali.cmd(DALI_QUERY_STATUS, sa);
  addLog("QUERY STATUS before clear -> " + String(before) + " (" + rvName(before) + ")");

  // -101 means there was no backward frame at all.
  if (before == -DALI_RESULT_NO_REPLY) {
    finishOperation("FAILED: SA" + String(sa) + " produced NO REPLY");
    return;
  }

  if (before >= 0) {
    // Normal path: verify DTR0 through the addressed gear first.
    uint8_t dtrResult = dali.set_dtr0(0xFF, sa);
    addLog("set_dtr0(0xFF) result=" + String(dtrResult));

    if (dtrResult != 0) {
      finishOperation("FAILED: could not verify DTR0 on SA" + String(sa));
      return;
    }
  } else {
    // INVALID/COLLISION still means there was bus activity.
    // This can happen when two gears share one SA or the backward frame is malformed.
    // Do not require an 8-bit query reply: write DTR0 directly and send the
    // addressed SET SHORT ADDRESS command.
    addLog("Backward reply is invalid/colliding; using FORCE CLEAR without DTR verification");
    int16_t dtrWrite = dali.cmd(DALI_DATA_TRANSFER_REGISTER0, 0xFF);
    addLog("DTR0 direct write rv=" + String(dtrWrite));
  }

  delay(150);

  int16_t setRv = dali.cmd(DALI_SET_SHORT_ADDRESS, sa);
  addLog("DALI_SET_SHORT_ADDRESS(MASK) sent to SA" + String(sa) +
         ", rv=" + String(setRv));
  delay(PROGRAM_DELAY_MS);

  // Verify several times. If duplicates existed, both should have lost SA.
  bool stillActivity = false;
  for (int n = 0; n < 3; n++) {
    int16_t after = dali.cmd(DALI_QUERY_STATUS, sa);
    addLog("Verify " + String(n + 1) + " SA" + String(sa) +
           " -> " + String(after) + " (" + rvName(after) + ")");
    if (after != -DALI_RESULT_NO_REPLY) stillActivity = true;
    delay(100);
  }

  if (!stillActivity) {
    addLog("Address clear succeeded; starting automatic scan");
    int found = scanBus(false, false);
    finishOperation("SA" + String(sa) +
                    " cleared; gear is now unaddressed; scan found " +
                    String(found) + " device(s)");
  } else {
    finishOperation("WARNING: bus activity remains on SA" + String(sa) +
                    "; try power-cycle, then RAW COMPARE / Readdress single");
  }
}

void clearAllAddresses() {
  setOperation("Clear ALL addresses");
  addLog("DANGER: clearing every responding short address");

  int cleared = 0;
  for (int sa = 0; sa < 64; sa++) {
    int16_t rv = dali.cmd(DALI_QUERY_STATUS, sa);
    if (rv >= 0) {
      addLog("Clearing SA" + String(sa));
      uint8_t dtrResult = dali.set_dtr0(0xFF, sa);
      if (dtrResult == 0) {
        delay(100);
        dali.cmd(DALI_SET_SHORT_ADDRESS, sa);
        delay(PROGRAM_DELAY_MS);
        int16_t check = dali.cmd(DALI_QUERY_STATUS, sa);
        if (check < 0) {
          cleared++;
          addLog("SA" + String(sa) + " cleared");
        } else {
          addLog("SA" + String(sa) + " STILL RESPONDS");
        }
      } else {
        addLog("Failed to set DTR0 for SA" + String(sa));
      }
    }
    progressPct = (uint8_t)(((sa + 1) * 100) / 64);
  }

  addLog("Clear-all pass complete; starting automatic scan");
  int found = scanBus(false, false);
  finishOperation("Cleared " + String(cleared) +
                  " short address(es); scan found " +
                  String(found) + " device(s)");
}

// ============================================================
// Diagnostics and device tools
// ============================================================

bool isActivity(int16_t rv) {
  return rv >= 0 || rv == -DALI_RESULT_INVALID_REPLY || rv == -DALI_RESULT_COLLISION;
}

// Broadcast queries that every gear answers, with or without a short address.
void presenceTest() {
  setOperation("Bus presence test");
  resetRxDiag();
  String report = "BUS PRESENCE TEST\n";
  int present = 0, missing = 0;

  for (int i = 0; i < 5; i++) {
    int16_t rv = dali.cmd(DALI_QUERY_CONTROL_GEAR_PRESENT, 0x7F);
    if (isActivity(rv)) present++;
    String line = "QUERY CONTROL GEAR PRESENT (broadcast) #" + String(i + 1) + ": " + rvName(rv) + " rv=" + String(rv);
    addLog(line);
    report += line + "\n";
    progressPct = (uint8_t)((i + 1) * 10);
    delay(50);
  }
  for (int i = 0; i < 5; i++) {
    int16_t rv = dali.cmd(DALI_QUERY_MISSING_SHORT_ADDRESS, 0x7F);
    if (isActivity(rv)) missing++;
    String line = "QUERY MISSING SHORT ADDRESS (broadcast) #" + String(i + 1) + ": " + rvName(rv) + " rv=" + String(rv);
    addLog(line);
    report += line + "\n";
    progressPct = (uint8_t)(50 + (i + 1) * 10);
    delay(50);
  }
  addLog("Several gears answering together give INVALID/COLLISION, this still means presence");
  addLog(rxDiagLine());
  report += rxDiagLine() + "\n";
  lastReport = report;

  String result;
  if (!present) {
    result = "NO gear answers on the bus: check the bus power supply (current limited) and wiring";
  } else {
    result = "Gear answers " + String(present) + "/5";
    result += missing ? ", unaddressed gear present (" + String(missing) + "/5)" : ", no unaddressed gear";
  }
  finishOperation(result);
}

void rawCommand(uint32_t frame, int bits, bool expectReply, int repeat, int captureSamples) {
  uint8_t data[3];
  int nbytes = bits / 8;
  for (int i = 0; i < nbytes; i++) data[i] = frame >> (8 * (nbytes - 1 - i));
  String hex = hexBytes(data, nbytes);

  setOperation("Raw " + hex);
  String report = "RAW COMMAND " + hex + " (" + String(bits) + " bit)\n";
  int replies = 0;

  for (int i = 0; i < repeat; i++) {
    if (i == 0 && captureSamples > 0) startScope(captureSamples);

    int16_t rv;
    String line = "RAW " + hex + " #" + String(i + 1) + ": ";
    if (expectReply) {
      rv = dali.tx_wait_rx_frame(data, bits);
      if (rv >= 0) {
        char b[8];
        snprintf(b, sizeof(b), "0x%02X", rv);
        line += "REPLY " + String(b);
      } else {
        line += rvName(rv) + " rv=" + String(rv);
      }
      if (isActivity(rv)) replies++;
    } else {
      rv = -(int16_t)dali.tx_wait(data, bits);
      line += rv == 0 ? "SENT" : rvName(rv);
    }

    if (i == 0 && captureSamples > 0) finishScope("RAW " + hex);
    addLog(line);
    report += line + "\n";
    progressPct = (uint8_t)((i + 1) * 100 / repeat);
    if (i + 1 < repeat) delay(50);
  }

  lastReport = report;
  if (expectReply) finishOperation("Raw " + hex + ": bus activity in " + String(replies) + "/" + String(repeat) + " attempt(s)");
  else finishOperation("Raw " + hex + " sent " + String(repeat) + " time(s)");
}

// Waits until the fade finishes (STATUS bit 4 "fade running"), at most 3 s, then reads the level.
int16_t waitFadeAndReadLevel(int sa) {
  uint32_t t0 = millis();
  delay(100);
  while (millis() - t0 < 3000) {
    int16_t st = dali.cmd(DALI_QUERY_STATUS, sa);
    if (st < 0 || !(st & 0x10)) break;
    delay(100);
  }
  delay(50);
  return dali.cmd(DALI_QUERY_ACTUAL_LEVEL, sa);
}

void dimTest(int sa) {
  setOperation("Dimming test SA" + String(sa));
  String report = "DIMMING TEST SA" + String(sa) + "\n";

  int16_t oldLevel = dali.cmd(DALI_QUERY_ACTUAL_LEVEL, sa); delay(20);
  int16_t minL = dali.cmd(DALI_QUERY_MIN_LEVEL, sa); delay(20);
  int16_t maxL = dali.cmd(DALI_QUERY_MAX_LEVEL, sa); delay(20);
  String head = "current=" + String(oldLevel) + " min=" + String(minL) + " max=" + String(maxL);
  if (minL < 1 || minL > 254) minL = 1;
  if (maxL < 1 || maxL > 254 || maxL < minL) maxL = 254;
  addLog("Dimming test SA" + String(sa) + ": " + head);
  report += head + "\n";

  const int steps = 8;
  int mismatches = 0, noReply = 0;
  for (int i = 0; i <= steps; i++) {
    uint8_t target = minL + (maxL - minL) * i / steps;
    dali.set_level(target, sa);
    int16_t actual = waitFadeAndReadLevel(sa);

    String line = "set " + String(target) + " -> actual " + String(actual);
    if (actual < 0) {
      noReply++;
      line += " (" + rvName(actual) + ")";
    } else if (abs(actual - (int)target) > 1) {
      mismatches++;
      line += "  MISMATCH";
    }
    addLog(line);
    report += line + "\n";
    progressPct = (uint8_t)((i + 1) * 100 / (steps + 1));
  }

  if (oldLevel >= 0 && oldLevel <= 254) dali.set_level((uint8_t)oldLevel, sa);
  lastReport = report;
  finishOperation("Dimming test SA" + String(sa) + ": " + String(mismatches) + " mismatch(es), " +
                  String(noReply) + " without reply");
}

bool validMirek(int32_t v) {
  return v > 0 && v < 0xFFFF;
}

void tcTest(int sa) {
  setOperation("Colour temperature test SA" + String(sa));
  String report = "COLOUR TEMPERATURE TEST SA" + String(sa) + "\n";

  dali.cmd(DALI_ENABLE_DEVICE_TYPE_X, 8);
  int16_t features = dali.cmd(DALI_DT8_QUERY_COLOUR_TYPE_FEATURES, sa); delay(20);
  String f = "features=" + String(features);
  if (features >= 0) {
    f += " (xy=" + String(features & 1 ? "yes" : "no") +
         ", Tc=" + String(features & 2 ? "yes" : "no") +
         ", primaries=" + String((features >> 2) & 7) +
         ", RGBWAF channels=" + String((features >> 5) & 7) + ")";
  }
  int32_t cur = dali.query_colour_value(DALI_DT8_SELECTOR_TC, sa); delay(20);
  int32_t cool = dali.query_colour_value(DALI_DT8_SELECTOR_TC_COOLEST, sa); delay(20);
  int32_t warm = dali.query_colour_value(DALI_DT8_SELECTOR_TC_WARMEST, sa); delay(20);
  f += " current=" + String(cur) + " coolest=" + String(cool) + " warmest=" + String(warm) + " mirek";
  addLog("Tc test SA" + String(sa) + ": " + f);
  report += f + "\n";

  if (features < 0 && !validMirek(cur)) {
    lastReport = report;
    finishOperation("Tc test SA" + String(sa) + ": no DT8 reply (" + rvName(features) + ")");
    return;
  }

  int32_t lo = validMirek(cool) ? cool : 154;
  int32_t hi = validMirek(warm) ? warm : 370;
  if (lo >= hi) { lo = 154; hi = 370; }

  const int steps = 4;
  int mismatches = 0, noReply = 0;
  for (int i = 0; i <= steps; i++) {
    uint16_t target = lo + (hi - lo) * i / steps;
    dali.set_colour_temperature(target, sa);
    delay(800);
    int32_t actual = dali.query_colour_value(DALI_DT8_SELECTOR_TC, sa);

    String line = "set " + String(target) + " mirek (" + String(1000000UL / target) + " K) -> actual " + String(actual);
    if (actual < 0) {
      noReply++;
      line += " (" + rvName(actual) + ")";
    } else if (abs(actual - (int32_t)target) > max(2, (int)target / 50)) {
      mismatches++;
      line += "  MISMATCH";
    }
    addLog(line);
    report += line + "\n";
    progressPct = (uint8_t)((i + 1) * 100 / (steps + 1));
  }

  if (validMirek(cur)) dali.set_colour_temperature((uint16_t)cur, sa);
  devices[sa].tc = cur;
  devices[sa].tcCoolest = cool;
  devices[sa].tcWarmest = warm;
  lastReport = report;
  finishOperation("Tc test SA" + String(sa) + ": " + String(mismatches) + " mismatch(es), " +
                  String(noReply) + " without reply");
}

void readMemoryBank(int sa, int bank) {
  setOperation("Memory bank " + String(bank) + " SA" + String(sa));
  MemoryDump m;
  m.sa = sa;
  m.bank = bank;

  // Byte 0 of every bank is the address of its last accessible location.
  int16_t first = -DALI_RESULT_NO_REPLY;
  uint8_t err = dali.read_memory((uint8_t)bank, 0, &first, 1, (uint8_t)sa);
  if (err || first < 0) {
    memDump = m;
    memSeq++;
    finishOperation("Memory bank " + String(bank) + " SA" + String(sa) + " not readable (" +
                    (err ? "DTR" + String(err == 1 ? "1" : "0") + " not confirmed" : rvName(first)) + ")");
    return;
  }

  int count = first + 1;
  if (count > 64) count = 64;
  m.bytes[0] = first;
  if (count > 1) dali.read_memory((uint8_t)bank, 1, m.bytes + 1, count - 1, (uint8_t)sa);
  m.count = count;
  memDump = m;
  memSeq++;

  String report = "MEMORY BANK " + String(bank) + " SA" + String(sa) + "\n";
  for (int row = 0; row < count; row += 16) {
    char b[8];
    snprintf(b, sizeof(b), "%02X:", row);
    String line = b;
    for (int i = row; i < row + 16 && i < count; i++) {
      if (m.bytes[i] < 0) line += " --";
      else { snprintf(b, sizeof(b), " %02X", m.bytes[i]); line += b; }
    }
    addLog(line);
    report += line + "\n";
  }
  lastReport = report;
  finishOperation("Memory bank " + String(bank) + " SA" + String(sa) + ": " + String(count) + " byte(s)");
}

void resetDevice(int sa) {
  setOperation("Reset SA" + String(sa));
  dali.cmd(DALI_RESET, sa); // send-twice command
  delay(400);               // gear may need up to 300 ms after RESET
  int16_t rs = dali.cmd(DALI_QUERY_RESET_STATE, sa);
  addLog("RESET SA" + String(sa) + ", QUERY RESET STATE -> " + String(rs) + " (" + rvName(rs) + ")");
  finishOperation(rs == 0xFF ? "SA" + String(sa) + " reset to factory settings (short address kept)"
                             : "RESET sent to SA" + String(sa) + ", reset state not confirmed");
}

uint8_t setFade(bool time, uint8_t v, int sa) {
  if (dali.set_dtr0(v, sa)) return 1;
  dali.cmd(time ? DALI_SET_FADE_TIME : DALI_SET_FADE_RATE, sa);
  int16_t ftr = dali.cmd(DALI_QUERY_FADE_TIME_FADE_RATE, sa);
  if (ftr < 0) return 2;
  uint8_t got = time ? ((ftr >> 4) & 0x0F) : (ftr & 0x0F);
  return got == v ? 0 : 2;
}

String configResult(const char* name, int value, uint8_t r) {
  String s = String(name) + "=" + String(value) + ": ";
  if (r == 0) s += "OK";
  else if (r == 1) s += "FAILED (DTR0 not confirmed)";
  else s += "FAILED (value not confirmed)";
  addLog(s);
  return s;
}

// p: 0 min, 1 max, 2 power-on, 3 system failure, 4 fade time, 5 fade rate (-1 = unchanged)
void configDevice(int sa, const int16_t* p) {
  setOperation("Configure SA" + String(sa));
  int failed = 0;
  // max first, so that raising min above the old max works
  if (p[1] >= 0 && configResult("max", p[1], dali.set_max_level(p[1], sa)).indexOf("FAILED") >= 0) failed++;
  if (p[0] >= 0 && configResult("min", p[0], dali.set_min_level(p[0], sa)).indexOf("FAILED") >= 0) failed++;
  if (p[2] >= 0 && configResult("power-on", p[2], dali.set_power_on_level(p[2], sa)).indexOf("FAILED") >= 0) failed++;
  if (p[3] >= 0 && configResult("system failure", p[3], dali.set_system_failure_level(p[3], sa)).indexOf("FAILED") >= 0) failed++;
  if (p[4] >= 0 && configResult("fade time", p[4], setFade(true, p[4], sa)).indexOf("FAILED") >= 0) failed++;
  if (p[5] >= 0 && configResult("fade rate", p[5], setFade(false, p[5], sa)).indexOf("FAILED") >= 0) failed++;

  DeviceInfo& d = devices[sa];
  d.minLevel = dali.cmd(DALI_QUERY_MIN_LEVEL, sa);
  d.maxLevel = dali.cmd(DALI_QUERY_MAX_LEVEL, sa);
  d.powerOnLevel = dali.cmd(DALI_QUERY_POWER_ON_LEVEL, sa);
  d.systemFailureLevel = dali.cmd(DALI_QUERY_SYSTEM_FAILURE_LEVEL, sa);
  d.fadeTimeRate = dali.cmd(DALI_QUERY_FADE_TIME_FADE_RATE, sa);
  finishOperation(failed ? "Configure SA" + String(sa) + ": " + String(failed) + " value(s) not applied"
                         : "Configure SA" + String(sa) + ": OK");
}

void setGroups(int sa, uint16_t wanted) {
  setOperation("Groups SA" + String(sa));
  int16_t g07 = dali.cmd(DALI_QUERY_GROUPS_0_7, sa);
  int16_t g815 = dali.cmd(DALI_QUERY_GROUPS_8_15, sa);
  if (g07 < 0 || g815 < 0) {
    finishOperation("Groups SA" + String(sa) + ": cannot read current groups (" + rvName(g07 < 0 ? g07 : g815) + ")");
    return;
  }
  uint16_t current = (uint16_t)g07 | ((uint16_t)g815 << 8);
  for (int g = 0; g < 16; g++) {
    bool want = wanted & (1 << g);
    if (want == (bool)(current & (1 << g))) continue;
    dali.cmd((want ? DALI_ADD_TO_GROUP0 : DALI_REMOVE_FROM_GROUP0) + g, sa);
    addLog("SA" + String(sa) + (want ? " ADD TO GROUP " : " REMOVE FROM GROUP ") + String(g));
  }
  devices[sa].groups07 = dali.cmd(DALI_QUERY_GROUPS_0_7, sa);
  devices[sa].groups815 = dali.cmd(DALI_QUERY_GROUPS_8_15, sa);
  uint16_t now = (uint16_t)devices[sa].groups07 | ((uint16_t)devices[sa].groups815 << 8);
  bool ok = devices[sa].groups07 >= 0 && devices[sa].groups815 >= 0 && now == wanted;
  finishOperation(ok ? "Groups SA" + String(sa) + " saved" : "Groups SA" + String(sa) + ": not confirmed by the gear");
}

// p[n]: -1 unchanged, 255 remove from scene, 0..254 scene level
void setScenes(int sa, const int16_t* p) {
  setOperation("Scenes SA" + String(sa));
  int failed = 0, changed = 0;
  for (int n = 0; n < 16; n++) {
    if (p[n] < 0) continue;
    changed++;
    if (p[n] == 255) {
      dali.cmd(DALI_REMOVE_FROM_SCENE0 + n, sa);
    } else if (dali.set_dtr0(p[n], sa) == 0) {
      dali.cmd(DALI_SET_SCENE0 + n, sa);
    }
    int16_t v = dali.cmd(DALI_QUERY_SCENE0_LEVEL + n, sa);
    devices[sa].scenes[n] = v;
    bool ok = v == p[n];
    if (!ok) failed++;
    addLog("SA" + String(sa) + " scene " + String(n) + " = " + (p[n] == 255 ? String("MASK") : String(p[n])) +
           (ok ? ": OK" : ": FAILED, gear reports " + String(v)));
    progressPct = (uint8_t)((n + 1) * 100 / 16);
  }
  finishOperation("Scenes SA" + String(sa) + ": " + String(changed - failed) + "/" + String(changed) + " saved");
}

String targetName(int adr) {
  if (adr == 0x7F) return "BROADCAST";
  if (adr >= 0x40) return "GROUP " + String(adr & 0x0F);
  return "SA" + String(adr);
}

// adr: 0..63 short address, 0x40|g group, 0x7F broadcast
// action: 0 off, 1 recall max, 2 recall min, 3 level=value, 4 go to scene value
void targetControl(int adr, int action, int value) {
  String name = targetName(adr);
  setOperation(name);
  String what;
  switch (action) {
    case 0: dali.cmd(DALI_OFF, adr); what = "OFF"; break;
    case 1: dali.cmd(DALI_RECALL_MAX_LEVEL, adr); what = "RECALL MAX LEVEL"; break;
    case 2: dali.cmd(DALI_RECALL_MIN_LEVEL, adr); what = "RECALL MIN LEVEL"; break;
    case 3:
      dali.set_level((uint8_t)value, adr);
      what = "LEVEL " + String(value);
      // keep the table in sync for addressed and broadcast levels (groups: membership may be unknown)
      if (adr < 64) devices[adr].level = value;
      else if (adr == 0x7F) for (int sa = 0; sa < 64; sa++) if (devices[sa].present) devices[sa].level = value;
      break;
    case 4: dali.cmd(DALI_GO_TO_SCENE0 + value, adr); what = "GO TO SCENE " + String(value); break;
  }
  addLog(name + " -> " + what);
  finishOperation(name + " " + what + " sent");
}

void scanRounds(int rounds) {
  for (int r = 0; r < rounds; r++) {
    addLog("Repeated scan round " + String(r + 1) + "/" + String(rounds));
    scanBus(false, false);
    if (r + 1 < rounds) delay(1000);
  }
  finishOperation("Repeated scan: " + String(rounds) + " round(s), see Scan statistics");
}

// ============================================================
// Worker task
// ============================================================

void operationTask(void* parameter) {
  PendingOp op = pendingOp;

  xSemaphoreTake(busMutex, portMAX_DELAY);
  switch (op.type) {
    case OP_SCAN:              scanBus(false); break;
    case OP_SCAN_DETAILED:     scanBus(true); break;
    case OP_FIND_UNADDRESSED:  findUnaddressed(); break;
    case OP_ASSIGN_UNADDRESSED:assignUnaddressed(); break;
    case OP_READDRESS_SINGLE:  readdressSingle(op.sa); break;
    case OP_RAW_COMPARE:       rawCompare20(); break;
    case OP_CLEAR_SELECTED:    clearSelectedAddress(op.sa); break;
    case OP_CLEAR_ALL:         clearAllAddresses(); break;
    case OP_RECOMMISSION_ALL:  recommissionAll(); break;
    case OP_IDENTIFY:          identifyDevice(op.sa); break;
    case OP_INSPECT:           inspectDevice(op.sa); break;
    case OP_ON:                setDeviceLevel(op.sa, 200); break;
    case OP_OFF:               setDeviceLevel(op.sa, 0); break;
    case OP_SELECTED_ON:       setSelectedLevel(op.mask, 200); break;
    case OP_SELECTED_OFF:      setSelectedLevel(op.mask, 0); break;
    case OP_BROADCAST_ON:      setBroadcastLevel(200); break;
    case OP_BROADCAST_OFF:     setBroadcastLevel(0); break;
    case OP_SET_TC:            setDeviceTc(op.sa, (uint16_t)op.value); break;
    case OP_BROADCAST_TC:      setBroadcastTc((uint16_t)op.value); break;
    case OP_PRESENCE_TEST:     presenceTest(); break;
    case OP_RAW:               rawCommand((uint32_t)op.value, op.value2, op.p[0] == 1, op.p[1], op.p[2]); break;
    case OP_DIM_TEST:          dimTest(op.sa); break;
    case OP_TC_TEST:           tcTest(op.sa); break;
    case OP_MEMORY:            readMemoryBank(op.sa, op.value); break;
    case OP_RESET_DEVICE:      resetDevice(op.sa); break;
    case OP_CONFIG:            configDevice(op.sa, op.p); break;
    case OP_GROUPS:            setGroups(op.sa, (uint16_t)op.value); break;
    case OP_SCENES:            setScenes(op.sa, op.p); break;
    case OP_TARGET:            targetControl(op.sa, op.value, op.value2); break;
    case OP_SCAN_ROUNDS:       scanRounds(op.value); break;
    default:
      finishOperation("No operation");
      break;
  }
  xSemaphoreGive(busMutex);

  vTaskDelete(nullptr);
}

bool startOperation(OpType type, int sa = -1, uint64_t mask = 0, int value = 0) {
  requestOp = PendingOp();
  requestOp.type = type;
  requestOp.sa = sa;
  requestOp.mask = mask;
  requestOp.value = value;
  return startRequested();
}

// Starts requestOp. Struct parameters are avoided because the Arduino builder
// places generated prototypes above the struct definitions.
bool startRequested() {
  if (busy) return false;

  busy = true;
  progressPct = 0;
  pendingOp = requestOp;

  BaseType_t ok = xTaskCreate(
    operationTask,
    "daliOperation",
    8192,
    nullptr,
    1,
    nullptr
  );

  if (ok != pdPASS) {
    busy = false;
    return false;
  }
  return true;
}

// ============================================================
// Web UI
// ============================================================

// Web UI is stored in web_page.h to avoid Arduino .ino preprocessor issues.


// ============================================================
// HTTP
// ============================================================

void sendBusyOrStarted(bool started) {
  if (!started) {
    server.send(409, "text/plain; charset=utf-8", "DALI operation already running");
  } else {
    server.send(202, "application/json", "{\"started\":true}");
  }
}

int getSaArg() {
  if (!server.hasArg("sa")) return -1;
  int sa = server.arg("sa").toInt();
  if (sa < 0 || sa > 63) return -1;
  return sa;
}

String groupsString(int groups07, int groups815) {
  if (groups07 < 0 || groups815 < 0) return "-";

  String out = "";
  for (int i = 0; i < 8; i++) {
    if (groups07 & (1 << i)) {
      if (out.length()) out += ",";
      out += String(i);
    }
  }
  for (int i = 0; i < 8; i++) {
    if (groups815 & (1 << i)) {
      if (out.length()) out += ",";
      out += String(i + 8);
    }
  }
  if (!out.length()) out = "-";
  return out;
}


void handleState() {
  String logCopy, opCopy, resultCopy;
  uint8_t prog;
  bool isBusy;

  if (stateMutex) xSemaphoreTake(stateMutex, portMAX_DELAY);
  logCopy = logText;
  opCopy = currentOperation;
  resultCopy = lastResult;
  prog = progressPct;
  isBusy = busy;
  if (stateMutex) xSemaphoreGive(stateMutex);

  String j;
  j.reserve(22000);
  j += "{\"busy\":";
  j += isBusy ? "true" : "false";
  j += ",\"progress\":" + String(prog);
  j += ",\"operation\":\"" + jsonEscape(opCopy) + "\"";
  j += ",\"lastResult\":\"" + jsonEscape(resultCopy) + "\"";
  j += ",\"found\":" + String(countDevices());
  j += ",\"log\":\"" + jsonEscape(logCopy) + "\"";
  j += ",\"scanDiff\":\"" + jsonEscape(lastScanDiff) + "\"";
  j += ",\"autoScan\":" + String(autoScanMinutes);
  j += ",\"statRounds\":" + String(statRounds);
  j += ",\"traceSeq\":" + String(traceSeq);
  j += ",\"scopeSeq\":" + String(scopeSeq);
  j += ",\"memSeq\":" + String(memSeq);

  // Addresses that ever answered during the collected scan rounds
  j += ",\"stats\":[";
  bool firstStat = true;
  for (int sa = 0; sa < 64; sa++) {
    SaStats &st = saStats[sa];
    if (!st.ok && !st.invalid && !st.collision) continue;
    if (!firstStat) j += ",";
    firstStat = false;
    j += "{\"sa\":" + String(sa) + ",\"ok\":" + String(st.ok) +
         ",\"invalid\":" + String(st.invalid) + ",\"collision\":" + String(st.collision) +
         ",\"label\":\"" + jsonEscape(labels[sa]) + "\"}";
  }
  j += "]";

  j += ",\"devices\":[";

  bool first = true;
  for (int sa = 0; sa < 64; sa++) {
    DeviceInfo &d = devices[sa];
    if (!d.present) continue;
    if (!first) j += ",";
    first = false;

    j += "{\"sa\":" + String(sa);
    j += ",\"present\":true";
    j += ",\"status\":" + String(d.status);
    j += ",\"level\":" + String(d.level);
    j += ",\"type\":" + String(d.deviceType);
    j += ",\"version\":" + String(d.version);
    j += ",\"groups\":\"" + groupsString(d.groups07, d.groups815) + "\"";
    j += ",\"groups07\":" + String(d.groups07);
    j += ",\"groups815\":" + String(d.groups815);
    j += ",\"label\":\"" + jsonEscape(labels[sa]) + "\"";
    j += ",\"replyValid\":"; j += d.replyValid ? "true" : "false";
    j += ",\"inspected\":"; j += d.inspected ? "true" : "false";
    j += ",\"tc\":" + String(d.tc);

    if (d.inspected) {
      j += ",\"physicalMin\":" + String(d.physicalMin);
      j += ",\"minLevel\":" + String(d.minLevel);
      j += ",\"maxLevel\":" + String(d.maxLevel);
      j += ",\"powerOnLevel\":" + String(d.powerOnLevel);
      j += ",\"systemFailureLevel\":" + String(d.systemFailureLevel);
      j += ",\"fadeTimeRate\":" + String(d.fadeTimeRate);
      j += ",\"operatingMode\":" + String(d.operatingMode);
      j += ",\"lightSourceType\":" + String(d.lightSourceType);
      j += ",\"controlGearFailure\":" + String(d.controlGearFailure);
      j += ",\"lampFailure\":" + String(d.lampFailure);
      j += ",\"lampPowerOn\":" + String(d.lampPowerOn);
      j += ",\"resetState\":" + String(d.resetState);
      j += ",\"powerFailure\":" + String(d.powerFailure);
      j += ",\"nextDeviceType\":" + String(d.nextDeviceType);
      j += ",\"tcCoolest\":" + String(d.tcCoolest);
      j += ",\"tcWarmest\":" + String(d.tcWarmest);
      j += ",\"scenes\":[";
      for (int si = 0; si < 16; si++) {
        if (si) j += ",";
        j += String(d.scenes[si]);
      }
      j += "]";
    }

    j += "}";
  }
  j += "]}";

  server.send(200, "application/json; charset=utf-8", j);
}


uint64_t parseSaMask(const String& list) {
  uint64_t mask = 0;
  int start = 0;

  while (start < (int)list.length()) {
    int comma = list.indexOf(',', start);
    if (comma < 0) comma = list.length();

    String token = list.substring(start, comma);
    token.trim();

    if (token.length()) {
      int sa = token.toInt();
      if (sa >= 0 && sa <= 63) {
        mask |= (1ULL << sa);
      }
    }

    start = comma + 1;
  }

  return mask;
}

void setupHttp() {
  server.on("/", HTTP_GET, [](){
    server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
  });

  server.on("/api/state", HTTP_GET, handleState);

  server.on("/api/scan", HTTP_POST, [](){
    bool detailed = server.hasArg("detailed") && server.arg("detailed") == "1";
    sendBusyOrStarted(startOperation(detailed ? OP_SCAN_DETAILED : OP_SCAN));
  });

  server.on("/api/find-unaddressed", HTTP_POST, [](){
    sendBusyOrStarted(startOperation(OP_FIND_UNADDRESSED));
  });

  server.on("/api/assign-unaddressed", HTTP_POST, [](){
    sendBusyOrStarted(startOperation(OP_ASSIGN_UNADDRESSED));
  });

  server.on("/api/raw-compare", HTTP_POST, [](){
    sendBusyOrStarted(startOperation(OP_RAW_COMPARE));
  });

  server.on("/api/readdress", HTTP_POST, [](){
    int sa = getSaArg();
    if (sa < 0) return server.send(400, "text/plain", "Invalid SA");
    sendBusyOrStarted(startOperation(OP_READDRESS_SINGLE, sa));
  });

  server.on("/api/clear-address", HTTP_POST, [](){
    int sa = getSaArg();
    if (sa < 0) return server.send(400, "text/plain", "Invalid SA");
    sendBusyOrStarted(startOperation(OP_CLEAR_SELECTED, sa));
  });

  server.on("/api/clear-all", HTTP_POST, [](){
    if (!server.hasArg("confirm") || server.arg("confirm") != "CLEAR")
      return server.send(400, "text/plain", "Confirmation required");
    sendBusyOrStarted(startOperation(OP_CLEAR_ALL));
  });

  server.on("/api/recommission-all", HTTP_POST, [](){
    if (!server.hasArg("confirm") || server.arg("confirm") != "READDRESS")
      return server.send(400, "text/plain", "Confirmation required");
    sendBusyOrStarted(startOperation(OP_RECOMMISSION_ALL));
  });

  server.on("/api/device", HTTP_POST, [](){
    int sa = getSaArg();
    if (sa < 0) return server.send(400, "text/plain", "Invalid SA");
    if (!server.hasArg("cmd")) return server.send(400, "text/plain", "Missing cmd");

    String cmd = server.arg("cmd");
    if (cmd == "identify") sendBusyOrStarted(startOperation(OP_IDENTIFY, sa));
    else if (cmd == "inspect") sendBusyOrStarted(startOperation(OP_INSPECT, sa));
    else if (cmd == "on") sendBusyOrStarted(startOperation(OP_ON, sa));
    else if (cmd == "off") sendBusyOrStarted(startOperation(OP_OFF, sa));
    else if (cmd == "tc") {
      // Colour temperature in mirek: 100..1000 mirek = 10000..1000 K
      int mirek = server.hasArg("mirek") ? server.arg("mirek").toInt() : 0;
      if (mirek < 100 || mirek > 1000) return server.send(400, "text/plain", "mirek must be 100..1000");
      sendBusyOrStarted(startOperation(OP_SET_TC, sa, 0, mirek));
    }
    else if (cmd == "dimtest") sendBusyOrStarted(startOperation(OP_DIM_TEST, sa));
    else if (cmd == "tctest") sendBusyOrStarted(startOperation(OP_TC_TEST, sa));
    else if (cmd == "reset") sendBusyOrStarted(startOperation(OP_RESET_DEVICE, sa));
    else if (cmd == "memory") {
      int bank = server.hasArg("bank") ? server.arg("bank").toInt() : -1;
      if (bank < 0 || bank > 255) return server.send(400, "text/plain", "bank must be 0..255");
      sendBusyOrStarted(startOperation(OP_MEMORY, sa, 0, bank));
    }
    else if (cmd == "groups") {
      long mask = server.hasArg("mask") ? server.arg("mask").toInt() : -1;
      if (mask < 0 || mask > 0xFFFF) return server.send(400, "text/plain", "mask must be 0..65535");
      sendBusyOrStarted(startOperation(OP_GROUPS, sa, 0, (int)mask));
    }
    else if (cmd == "config") {
      // empty or missing = unchanged
      static const char* names[6] = {"min", "max", "pon", "sf", "ft", "fr"};
      static const int lo[6] = {1, 1, 0, 0, 0, 1};
      static const int hi[6] = {254, 254, 255, 255, 15, 15};
      requestOp = PendingOp();
      requestOp.type = OP_CONFIG;
      requestOp.sa = sa;
      for (int i = 0; i < 6; i++) {
        String v = server.arg(names[i]);
        v.trim();
        if (!v.length()) continue;
        int n = v.toInt();
        if (n < lo[i] || n > hi[i]) return server.send(400, "text/plain", String(names[i]) + " out of range");
        requestOp.p[i] = n;
      }
      sendBusyOrStarted(startRequested());
    }
    else if (cmd == "scenes") {
      // s0..s15: empty = unchanged, "mask" = remove from scene, 0..254 = level
      requestOp = PendingOp();
      requestOp.type = OP_SCENES;
      requestOp.sa = sa;
      for (int n = 0; n < 16; n++) {
        String v = server.arg("s" + String(n));
        v.trim();
        v.toLowerCase();
        if (!v.length()) continue;
        if (v == "mask") { requestOp.p[n] = 255; continue; }
        int lvl = v.toInt();
        if (lvl < 0 || lvl > 254 || (lvl == 0 && v != "0")) return server.send(400, "text/plain", "scene " + String(n) + " must be 0..254 or mask");
        requestOp.p[n] = lvl;
      }
      sendBusyOrStarted(startRequested());
    }
    else server.send(400, "text/plain", "Unknown cmd");
  });


  server.on("/api/selected", HTTP_POST, [](){
    if (!server.hasArg("cmd")) return server.send(400, "text/plain", "Missing cmd");
    if (!server.hasArg("sas")) return server.send(400, "text/plain", "Missing sas");

    uint64_t mask = parseSaMask(server.arg("sas"));
    if (mask == 0) return server.send(400, "text/plain", "No valid SA selected");

    String cmd = server.arg("cmd");
    if (cmd == "on") sendBusyOrStarted(startOperation(OP_SELECTED_ON, -1, mask));
    else if (cmd == "off") sendBusyOrStarted(startOperation(OP_SELECTED_OFF, -1, mask));
    else server.send(400, "text/plain", "Unknown cmd");
  });

  server.on("/api/broadcast", HTTP_POST, [](){
    if (!server.hasArg("cmd")) return server.send(400, "text/plain", "Missing cmd");

    String cmd = server.arg("cmd");
    if (cmd == "on") sendBusyOrStarted(startOperation(OP_BROADCAST_ON));
    else if (cmd == "off") sendBusyOrStarted(startOperation(OP_BROADCAST_OFF));
    else if (cmd == "tc") {
      // Colour temperature in mirek: 100..1000 mirek = 10000..1000 K
      int mirek = server.hasArg("mirek") ? server.arg("mirek").toInt() : 0;
      if (mirek < 100 || mirek > 1000) return server.send(400, "text/plain", "mirek must be 100..1000");
      sendBusyOrStarted(startOperation(OP_BROADCAST_TC, -1, 0, mirek));
    }
    else server.send(400, "text/plain", "Unknown cmd");
  });

  server.on("/api/log/clear", HTTP_POST, [](){
    if (stateMutex) xSemaphoreTake(stateMutex, portMAX_DELAY);
    logText = "";
    if (stateMutex) xSemaphoreGive(stateMutex);
    server.send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/api/report", HTTP_GET, [](){
    String r = lastReport;
    if (!r.length()) r = "No report yet.\nRun Scan, Detailed Scan or RAW COMPARE first.\n";
    server.sendHeader("Content-Disposition", "attachment; filename=\"dali-report.txt\"");
    server.send(200, "text/plain; charset=utf-8", r);
  });

  server.on("/api/presence", HTTP_POST, [](){
    sendBusyOrStarted(startOperation(OP_PRESENCE_TEST));
  });

  // Raw forward frame: frame=hex (4 digits = 16 bit, 6 digits = 24 bit), reply=0|1, repeat=1..50, capture=0|samples
  server.on("/api/raw", HTTP_POST, [](){
    String hex = server.arg("frame");
    hex.trim();
    hex.replace(" ", "");
    if (hex.length() != 4 && hex.length() != 6) return server.send(400, "text/plain", "Frame must be 4 or 6 hex digits");
    for (size_t i = 0; i < hex.length(); i++) {
      if (!isxdigit((unsigned char)hex[i])) return server.send(400, "text/plain", "Frame must be hexadecimal");
    }
    int repeat = server.hasArg("repeat") ? server.arg("repeat").toInt() : 1;
    if (repeat < 1 || repeat > 50) return server.send(400, "text/plain", "repeat must be 1..50");
    int capture = server.hasArg("capture") ? server.arg("capture").toInt() : 0;
    if (capture < 0 || capture > SCOPE_MAX_SAMPLES) return server.send(400, "text/plain", "capture must be 0..4096");

    requestOp = PendingOp();
    requestOp.type = OP_RAW;
    requestOp.value = (int)strtoul(hex.c_str(), nullptr, 16);
    requestOp.value2 = hex.length() * 4;
    requestOp.p[0] = server.arg("reply") == "1" ? 1 : 0;
    requestOp.p[1] = repeat;
    requestOp.p[2] = capture;
    sendBusyOrStarted(startRequested());
  });

  // Commands to a short address, a group or broadcast
  // adr: 0..63 SA, 64..79 group 0..15, 127 broadcast; action: off|max|min|level|scene; value: level or scene
  server.on("/api/target", HTTP_POST, [](){
    int adr = server.hasArg("adr") ? server.arg("adr").toInt() : -1;
    if (!((adr >= 0 && adr <= 0x4F) || adr == 0x7F)) return server.send(400, "text/plain", "Invalid target");
    String action = server.arg("action");
    int value = server.arg("value").toInt();
    int a;
    if (action == "off") a = 0;
    else if (action == "max") a = 1;
    else if (action == "min") a = 2;
    else if (action == "level") {
      a = 3;
      if (value < 0 || value > 254) return server.send(400, "text/plain", "level must be 0..254");
    } else if (action == "scene") {
      a = 4;
      if (value < 0 || value > 15) return server.send(400, "text/plain", "scene must be 0..15");
    } else return server.send(400, "text/plain", "Unknown action");

    requestOp = PendingOp();
    requestOp.type = OP_TARGET;
    requestOp.sa = adr;
    requestOp.value = a;
    requestOp.value2 = value;
    sendBusyOrStarted(startRequested());
  });

  server.on("/api/scan-rounds", HTTP_POST, [](){
    int rounds = server.arg("rounds").toInt();
    if (rounds < 1 || rounds > 100) return server.send(400, "text/plain", "rounds must be 1..100");
    sendBusyOrStarted(startOperation(OP_SCAN_ROUNDS, -1, 0, rounds));
  });

  server.on("/api/autoscan", HTTP_POST, [](){
    int minutes = server.arg("minutes").toInt();
    if (minutes < 0 || minutes > 1440) return server.send(400, "text/plain", "minutes must be 0..1440");
    autoScanMinutes = minutes;
    lastAutoScanMs = millis();
    prefs.putUInt("autoscan", autoScanMinutes);
    addLog(minutes ? "Auto-scan every " + String(minutes) + " min" : String("Auto-scan off"));
    server.send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/api/stats/reset", HTTP_POST, [](){
    for (int sa = 0; sa < 64; sa++) saStats[sa] = SaStats();
    statRounds = 0;
    addLog("Scan statistics cleared");
    server.send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/api/label", HTTP_POST, [](){
    int sa = getSaArg();
    if (sa < 0) return server.send(400, "text/plain", "Invalid SA");
    String text = server.arg("text");
    text.trim();
    if (text.length() > 40) text = text.substring(0, 40);
    labels[sa] = text;
    String key = "l" + String(sa);
    if (text.length()) prefs.putString(key.c_str(), text);
    else prefs.remove(key.c_str());
    server.send(200, "application/json", "{\"ok\":true}");
  });

  // Bus trace entries newer than ?since=<seq>
  server.on("/api/trace", HTTP_GET, [](){
    uint32_t since = server.arg("since").toInt();
    String j;
    j.reserve(16000);
    xSemaphoreTake(traceMutex, portMAX_DELAY);
    uint32_t last = traceSeq;
    uint32_t from = since + 1;
    if (last >= (uint32_t)TRACE_SIZE && from < last - TRACE_SIZE + 1) from = last - TRACE_SIZE + 1;
    j += "{\"seq\":" + String(last) + ",\"entries\":[";
    for (uint32_t s = from; s <= last && s > 0; s++) {
      TraceEntry &e = traceBuf[(s - 1) % TRACE_SIZE];
      if (s != from) j += ",";
      j += "{\"s\":" + String(e.seq) + ",\"t\":" + String((double)e.us / 1000.0, 3) +
           ",\"k\":" + String(e.kind) + ",\"b\":" + String(e.bits) +
           ",\"d\":\"" + hexBytes(e.data, e.bits > 32 ? 4 : (e.bits + 7) / 8) + "\",\"rv\":" + String(e.rv) + "}";
    }
    xSemaphoreGive(traceMutex);
    j += "]}";
    server.send(200, "application/json", j);
  });

  server.on("/api/scope", HTTP_GET, [](){
    String j;
    j.reserve(1300);
    j += "{\"seq\":" + String(scopeSeq) + ",\"rate\":9604,\"samples\":" + String(scopeSamples);
    j += ",\"txStart\":" + String(scopeTxStart) + ",\"txEnd\":" + String(scopeTxEnd);
    j += ",\"title\":\"" + jsonEscape(scopeTitle) + "\",\"data\":\"" + hexBytes(scopeBuf, (scopeSamples + 7) / 8) + "\"}";
    server.send(200, "application/json", j);
  });

  server.on("/api/memory", HTTP_GET, [](){
    String j = "{\"seq\":" + String(memSeq) + ",\"sa\":" + String(memDump.sa) + ",\"bank\":" + String(memDump.bank) + ",\"bytes\":[";
    for (int i = 0; i < memDump.count; i++) {
      if (i) j += ",";
      j += String(memDump.bytes[i]);
    }
    j += "]}";
    server.send(200, "application/json", j);
  });

  server.on("/api/log", HTTP_GET, [](){
    String copy;
    if (stateMutex) xSemaphoreTake(stateMutex, portMAX_DELAY);
    copy = logText;
    if (stateMutex) xSemaphoreGive(stateMutex);
    server.sendHeader("Content-Disposition", "attachment; filename=\"dali-log.txt\"");
    server.send(200, "text/plain; charset=utf-8", copy);
  });

  server.onNotFound([](){
    server.send(404, "text/plain", "Not found");
  });

  server.begin();
}

// ============================================================
// Setup
// ============================================================

void setupWifi() {
  WiFi.mode(WIFI_AP_STA);

  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  if (strlen(STA_SSID) > 0) {
    WiFi.begin(STA_SSID, STA_PASS);
    Serial.print("Connecting STA");
    for (int i = 0; i < 30 && WiFi.status() != WL_CONNECTED; i++) {
      delay(300);
      Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("STA IP: ");
      Serial.println(WiFi.localIP());
    }
  }

  if (MDNS.begin("dali-service")) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS: http://dali-service.local");
  }
}

void setup() {
  Serial.begin(115200);
  stateMutex = xSemaphoreCreateMutex();
  busMutex = xSemaphoreCreateMutex();
  traceMutex = xSemaphoreCreateMutex();

  prefs.begin("dali-tool", false);
  for (int sa = 0; sa < 64; sa++) labels[sa] = prefs.getString(("l" + String(sa)).c_str(), "");
  autoScanMinutes = prefs.getUInt("autoscan", 0);
  delay(1500);

  Serial.println();
  Serial.println("======================================");
  Serial.println("DALI SERVICE TOOL");
  Serial.println("ESP32-C6 Pico + Waveshare Pico-DALI2");
  Serial.println("======================================");

  pinMode(RX_PIN, INPUT);
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, HIGH);

  // Waveshare TX is inverted: TX pin HIGH pulls the DALI bus low,
  // so the library's "set low" callback drives the pin HIGH.
  dali.begin(bus_is_high, bus_set_high, bus_set_low);

  // 1200 baud * 8 samples = 9600 timer callbacks / second.
  // The timer prescaler is an integer divider of the 80 MHz clock: 9.6 MHz would
  // silently become 10 MHz (10 kHz ISR, Te 400us). 8 MHz / 833 = 9603.8 Hz (+0.04%).
  // Start the timer only after dali.begin(): the ISR uses the bus callbacks.
  daliTimer = timerBegin(8000000);
  timerAttachInterrupt(daliTimer, &onDaliTimer);
  timerAlarm(daliTimer, 833, true, 0);

  dali.trace_cb = traceAdd;
  xTaskCreate(monitorTask, "daliMonitor", 3072, nullptr, 1, nullptr);

  setupWifi();
  setupHttp();

  addLog("DALI initialized");
  addLog("Web: http://192.168.4.1");
  addLog("Ready");
}

void loop() {
  server.handleClient();

  if (autoScanMinutes && !busy && millis() - lastAutoScanMs >= autoScanMinutes * 60000UL) {
    lastAutoScanMs = millis();
    if (startOperation(OP_SCAN)) addLog("Auto-scan started");
  }

  delay(2);
}
