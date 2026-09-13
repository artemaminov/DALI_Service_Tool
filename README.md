````markdown
# DALI Service Tool

A web-based service and diagnostic interface for:

- Waveshare ESP32-C6 Pico
- Waveshare Pico-DALI2 / DALI2 Expansion Board
- DALI control gear

## Pinout

This build uses:

- GPIO14 — DALI TX
- GPIO5 — DALI RX

## Wi-Fi

After flashing, the ESP32 creates its own access point:

- SSID: `DALI-Service`
- Password: `dali12345`
- Web UI: http://192.168.4.1
- mDNS: http://dali-service.local

Optionally, set `STA_SSID` and `STA_PASS` in the `.ino` file to connect the ESP32 to an existing Wi-Fi network while keeping the access point available.

## Project Files

Keep these files in the same Arduino sketch folder:

- `DALI_Service_Tool.ino`
- `DALI_Lib.cpp`
- `DALI_Lib.h`
- `web_page.h` — embedded web interface

Do not include `WS_DALI.cpp/.h` or `WS_WIFI.cpp/.h` from the original Waveshare demo in the same sketch folder. They define duplicate objects/symbols and will cause compilation errors.

## Features

### Scan

Scans short addresses SA0–SA63 using `QUERY STATUS`.

Addresses producing valid replies are shown as discovered devices. Invalid or colliding backward-frame activity is also reported instead of being treated as no device.

### Detailed Scan

Reads additional information from discovered devices:

- Status
- Actual level
- Device type
- DALI version
- Groups 0–7
- Groups 8–15

### Per-device Inspect

Each discovered short address has an `Inspect` button and an expandable device card.

Cards are collapsed by default. After a successful inspection, the corresponding card opens automatically and can then be collapsed or expanded manually.

Inspect currently reads:

- Actual level
- Physical minimum level
- Minimum and maximum level
- Power-on level
- System-failure level
- Fade time / fade rate
- Operating mode
- Device type information
- DALI version
- Light-source type
- Control-gear and lamp status
- Reset and power-failure state
- Group membership
- All 16 scene levels

DT8 colour-temperature capabilities and Memory Bank 0 device identification are not decoded yet.

### Identify

Flashes the selected short address three times and then attempts to restore its previous light level.

### Find Unaddressed

Checks whether unaddressed control gear is present using:

`INITIALISE 0xFF → RANDOMISE → SEARCHADDR 0xFFFFFF → COMPARE`

### Assign Unaddressed

Finds control gear without a short address and assigns available short addresses.

Longer delays are intentionally used during commissioning to make the process more tolerant and easier to diagnose.

### Readdress Single Connected Device

Forces commissioning of one physically connected luminaire:

`INITIALISE 0x00 → RANDOMISE → random-address binary search → PROGRAM SHORT ADDRESS → VERIFY`

**Important:** Only one physical DALI control gear should be connected to the bus when using this operation.

### RAW COMPARE x20

Runs 20 raw `COMPARE` operations and reports:

- YES
- NO REPLY
- INVALID
- COLLISION

Useful for diagnosing commissioning and backward-frame problems.

### Clear Selected Address

Writes `DTR0 = 0xFF (MASK)` and sends `SET SHORT ADDRESS` to the selected short address.

After successful completion, the control gear becomes unaddressed.

`NO REPLY (-101)` is distinguished from malformed or colliding backward replies such as `INVALID REPLY (-105)`.

When backward activity exists but cannot be decoded cleanly, the tool can use a force-clear path without requiring a valid 8-bit backward reply first.

### Clear ALL Addresses

**Dangerous operation.**

Clears the short addresses of responding control gear, leaving the devices unaddressed.

### Recommission Entire Bus

**Dangerous operation.**

Places all control gear into commissioning mode and assigns new short addresses starting from SA0.

### Individual and Group Control

The device table contains a checkbox for every discovered short address.

- `ON` / `OFF` — control one device
- `Selected ON` — turns on selected addresses sequentially
- `Selected OFF` — turns off selected addresses sequentially
- `BROADCAST ON` — sends one DALI broadcast command with level 200
- `BROADCAST OFF` — sends one DALI broadcast command with level 0
- `Select all` / `Clear selection` — manage device selection

Selection is preserved while the web interface periodically refreshes.

## Automatic Scan After Address Changes

A normal bus scan runs automatically after successful address-changing operations:

- Assign Unaddressed
- Readdress Single
- Clear Selected Address
- Clear ALL Addresses
- Recommission Entire Bus

The scan runs inside the same worker operation before it is marked complete, preventing concurrent DALI operations and automatically refreshing the device table.

## Address Verification

Commissioning verification uses the raw `QUERY SHORT ADDRESS` result instead of `Dali::query_short_address()`.

The original helper returns `uint8_t`, which can convert negative receive errors into misleading positive values. For example, a receive error of `-105` may appear as `203` after shifting and type conversion.

The service therefore:

1. Reads the raw `QUERY SHORT ADDRESS` result.
2. Sends `VERIFY SHORT ADDRESS`.
3. Separately checks the addressed device using `QUERY STATUS`.

`INVALID` or `COLLISION` replies are treated as backward-bus activity rather than as `NO REPLY`.

## Arduino

Recommended configuration matches the environment used by the official Waveshare demo:

- Board: `ESP32C6 Dev Module` or corresponding ESP32-C6 Pico target
- Arduino-ESP32: 3.0.x
- Serial: 115200 baud

The project uses only standard Arduino-ESP32 libraries:

- `WiFi`
- `WebServer`
- `ESPmDNS`

## DALI Timer

The sketch follows the timer configuration used by the official Waveshare demo:

```cpp
timerBegin(9600000);
timerAlarm(timer, 1000, true, 0);
```

`DALI_Lib` is therefore serviced at 9600 Hz, providing 8× oversampling for the 1200-baud DALI bus.

## Arduino Preprocessor Note

The complete HTML/CSS/JavaScript interface is stored in `web_page.h` rather than directly inside the `.ino` file.

This is intentional. Arduino's automatic `.ino` prototype generator can otherwise misinterpret JavaScript declarations such as `async function` as C++ code.

## Status

The project is under active development.

Planned improvements include:

- DALI DT8 colour-temperature detection and control
- DT8 capability inspection
- Memory Bank 0 reading
- GTIN / device identification
- Extended control-gear diagnostics
````
