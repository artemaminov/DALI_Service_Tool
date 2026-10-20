# DALI Service Tool

A web-based service and diagnostic interface for:

- Waveshare ESP32-C6 Pico
- Waveshare Pico-DALI2 / DALI2 Expansion Board
- DALI control gear (DALI-1, DALI-2, DT8 colour temperature)

The interface is available in English and Russian (selector in the page header).

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

### Scanning

- **Scan** — queries short addresses SA0–SA63 with `QUERY STATUS`. Invalid or colliding backward-frame activity is reported instead of being treated as no device.
- **Detailed Scan** — additionally reads actual level, device type, DALI version and groups.
- **Comparison with the previous scan** — appeared and disappeared addresses, e.g. `+SA5 -SA7`.
- **Repeated scan** — runs the scan N times in a row (1–100).
- **Auto-scan** — runs a scan every N minutes while the tool is idle. The interval is stored in the ESP32.
- **Scan statistics** — per address: valid replies, `INVALID`, `COLLISION` and missed rounds. Shows luminaires that disappear intermittently.
- **RX DIAG** — after Scan and RAW COMPARE the log shows how the RX pin saw our own transmission (expected ~45% low samples for a 16-bit frame) and whether any reception started while the bus was idle. Tells apart a deaf RX path from gear that never answers.

### Devices table

Each discovered short address has a row with:

- Status byte decoded as flags (gear failure, lamp failure, lamp on, limit error, fade running, reset state, no short address, power cycle seen)
- `Inspect`, `Identify`, `Off`, `On`
- Brightness slider (0–254, shown with the logarithmic DALI percentage)
- Colour-temperature slider, cold on the left, warm on the right (DT8)
- Label, edited with ✎ and stored in the ESP32 (NVS)

`Selected ON/OFF` sends addressed commands to the checked rows. The **Broadcast** row has `BROADCAST ON/OFF` and brightness and colour-temperature sliders for all gear at once.

Rows are updated in place, so values typed into the device card are kept while the page refreshes.

### Device card (▾)

Tools:

- **Dimming test** — steps the level from minimum to maximum, waits for every fade to finish and compares `QUERY ACTUAL LEVEL` with the requested level. Restores the previous level.
- **Tc test** — reads DT8 colour type features and Tc limits, steps the colour temperature from cold to warm and compares the reported value with the requested one. Restores the previous value.
- **Identity** — reads memory bank 0 and decodes GTIN, firmware and hardware versions, identification number and DALI part versions (DALI-2 layout, IEC 62386-102 ed2).
- **Read bank** — raw dump of memory bank 1 or 202–207 (D4i / DALI-2 energy reporting and diagnostics).
- **Reset** — DALI `RESET`: factory levels, fade, groups and scenes. The short address is kept.

After **Inspect** the card shows:

- Actual, physical minimum, minimum and maximum level
- Power-on and system-failure level, fade time / fade rate
- Operating mode, device type, DALI version, light-source type
- Control-gear, lamp, reset and power-failure state
- DT8 colour temperature and its coolest / warmest limits
- Group membership and all 16 scene levels

and lets you edit:

- **Settings** — min/max level, power-on level, system-failure level, fade time, fade rate. Every value is read back after writing.
- **Groups** — membership in groups 0–15.
- **Scenes** — level of every scene (empty = MASK); ▶ recalls a scene on this luminaire.

### Identify

Flashes the selected short address three times and then attempts to restore its previous light level.

### Group / broadcast control

Off, max, min, level and scene recall for a DALI group 0–15 or for all gear.

### Diagnostics

- **Bus presence test** — broadcasts `QUERY CONTROL GEAR PRESENT` and `QUERY MISSING SHORT ADDRESS` five times each. Every powered gear answers, with or without a short address. No answer at all points to a bus power or wiring problem rather than an addressing problem.
- **RAW COMPARE x20** — `INITIALISE 0xFF → RANDOMISE → SEARCHADDR 0xFFFFFF` followed by 20 raw `COMPARE` queries with YES / NO REPLY / INVALID / COLLISION statistics.
- **Raw command** — sends any 16-bit (4 hex digits) or 24-bit (6 hex digits) forward frame, with or without waiting for a reply, repeated up to 50 times. Presets are included.
- **Waveform** — records the RX pin at 9600 samples/s (107, 213 or 427 ms) while the raw command is sent. Our own transmission is shaded; any low level outside it is a reply from the gear. The summary line states whether there was bus activity outside our transmission.
- **Bus monitor** — every frame on the bus with time, gap to the previous entry and meaning:
  - `TX` — sent by this tool
  - `REPLY` — answer to our query (or invalid reply / collision)
  - `BUS` — frame from another participant (another controller, sensor, push button) or a late reply

  Forward frames are decoded to address and command name. Frames of other participants are captured while no operation is running.

### Commissioning

- **Find unaddressed** — `INITIALISE 0xFF → RANDOMISE → SEARCHADDR 0xFFFFFF → COMPARE`.
- **Assign unaddressed** — finds control gear without a short address and assigns free short addresses.
- **Readdress single connected** — `INITIALISE 0x00 → RANDOMISE → random-address binary search → PROGRAM SHORT ADDRESS → VERIFY`.
  **Important:** only one physical DALI control gear should be connected to the bus.

Longer delays are intentionally used during commissioning to make the process more tolerant and easier to diagnose.

### Danger zone

- **Clear selected address** — writes `DTR0 = 0xFF (MASK)` and sends `SET SHORT ADDRESS`. When backward activity exists but cannot be decoded cleanly, a force-clear path is used without requiring a valid 8-bit reply first.
- **Clear ALL addresses** — clears the short addresses of all responding control gear.
- **Recommission entire bus** — places all control gear into commissioning mode and assigns new short addresses starting from SA0.

### Reports and log

- **Download report** — text report of the last scan or test.
- **Download log** — the full operation log.

## Automatic Scan After Address Changes

A normal bus scan runs automatically after successful address-changing operations:

- Assign Unaddressed
- Readdress Single
- Clear Selected Address
- Clear ALL Addresses
- Recommission Entire Bus

The scan runs inside the same worker operation before it is marked complete, preventing concurrent DALI operations and automatically refreshing the device table.

## Address Verification

Every programmed short address is confirmed before it is considered assigned:

1. Read the raw `QUERY SHORT ADDRESS` result.
2. Send `VERIFY SHORT ADDRESS`.
3. After commissioning, check the addressed device with `QUERY STATUS`.

`INVALID` or `COLLISION` replies are treated as backward-bus activity rather than as `NO REPLY`.

If no gear confirms the address — usually a lost `COMPARE` reply sent the binary search to a wrong random address — the short address stays free and the search is repeated instead of withdrawing nothing. After three unconfirmed attempts in a row the process stops. The library's `commission()` works the same way.

The raw `QUERY SHORT ADDRESS` result is used instead of `Dali::query_short_address()`: the helper returns `uint8_t`, which turns negative receive errors into misleading values (for example `-105` appears as `203`).

## DALI Timing

`DALI_Lib` samples the bus at 9600 Hz (8× oversampling of 1200 baud).

```cpp
daliTimer = timerBegin(8000000);
timerAttachInterrupt(daliTimer, &onDaliTimer);
timerAlarm(daliTimer, 833, true, 0);   // 9603.8 Hz
```

The timer prescaler is an integer divider of the 80 MHz clock. The Waveshare demo setting `timerBegin(9600000)` with alarm `1000` silently becomes 80 / 8 = 10 MHz, i.e. a 10 kHz sample rate and 400 µs half-bits instead of 416.7 µs. 8 MHz / 833 is within 0.04% of the nominal rate.

The timer is started after `dali.begin()` because the interrupt uses the bus callbacks. GPIO access in the interrupt uses the inline HAL (`gpio_ll_get_level` / `gpio_ll_set_level`) instead of `digitalRead` / `digitalWrite`.

Before every forward frame the bus must be idle for 100 samples (10.4 ms), which covers the 22 Te (9.17 ms) settling time required by DALI-1 gear. `COMPARE` is retried three times before a missing reply is accepted as NO.

## Arduino

- Board: `ESP32C6 Dev Module` or corresponding ESP32-C6 Pico target
- Arduino-ESP32: 3.x (tested with 3.3.12)
- Serial: 115200 baud

The project uses only standard Arduino-ESP32 libraries:

- `WiFi`
- `WebServer`
- `ESPmDNS`
- `Preferences` — labels and auto-scan interval

The sketch uses about 95% of the default 1.2 MB application partition.

## Arduino Preprocessor Note

The complete HTML/CSS/JavaScript interface is stored in `web_page.h` rather than directly inside the `.ino` file.

This is intentional. Arduino's automatic `.ino` prototype generator can otherwise misinterpret JavaScript declarations such as `async function` as C++ code. For the same reason, functions in the `.ino` take only built-in types as parameters: generated prototypes are placed above the `struct` definitions.

## Translations

English is the source language of the interface. Russian texts live in the `RU` dictionary at the top of the script in `web_page.h`, keyed by the English text. To add a new label, write it in English and add the pair to `RU`. DALI command names, the log and operation results produced by the ESP32 stay in English.
