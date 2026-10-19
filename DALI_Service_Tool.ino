#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
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
  OP_BROADCAST_OFF
};

struct PendingOp {
  OpType type = OP_NONE;
  int sa = -1;
  uint64_t mask = 0;
};

PendingOp pendingOp;

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

    addLog(line);
    report += line + "\n";
    progressPct = (uint8_t)(((sa + 1) * 100) / 64);
    delay(10);
  }

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

  d.inspected = true;
  devices[sa] = d;

  addLog("SA" + String(sa) +
         " inspect: level=" + String(d.level) +
         " min=" + String(d.minLevel) +
         " max=" + String(d.maxLevel) +
         " physicalMin=" + String(d.physicalMin) +
         " type=" + String(d.deviceType) +
         " nextType=" + String(d.nextDeviceType));

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
  delay(200);
  finishOperation("SA" + String(sa) + (level ? " ON" : " OFF"));
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
// Worker task
// ============================================================

void operationTask(void* parameter) {
  PendingOp op = pendingOp;

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
    default:
      finishOperation("No operation");
      break;
  }

  vTaskDelete(nullptr);
}

bool startOperation(OpType type, int sa = -1, uint64_t mask = 0) {
  if (busy) return false;

  busy = true;
  progressPct = 0;
  pendingOp.type = type;
  pendingOp.sa = sa;
  pendingOp.mask = mask;

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
    j += ",\"replyValid\":"; j += d.replyValid ? "true" : "false";
    j += ",\"inspected\":"; j += d.inspected ? "true" : "false";

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

  setupWifi();
  setupHttp();

  addLog("DALI initialized");
  addLog("Web: http://192.168.4.1");
  addLog("Ready");
}

void loop() {
  server.handleClient();
  delay(2);
}
