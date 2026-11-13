# FuelGuard: Smart Anti-Fuel-Theft Cap

A smart fuel cap system for heavy vehicles. A reed switch detects when the cap is opened, and an ESP32 with a GPS module immediately sends an alert with the vehicle's location. A periodic heartbeat confirms the device is alive and reports its position.

Built as an ECE college project at the Faculty of Technology, University of Delhi.

**Live dashboard demo:** https://maybedron.github.io/fuel-guard/dashboard/

## Features

| Feature | What it does |
|---|---|
| Cap-open detection | Magnetic reed switch triggers an instant alert when the cap is opened |
| Live GPS location | NEO-6M module gives coordinates, sent as a Google Maps link |
| Phone alert | Telegram bot message within seconds of the cap opening or closing |
| Heartbeat | Automatic location update on a timer (30 s in the demo, 6 h in the real product) |
| Fleet dashboard | Web app with map, cap status, heartbeat countdown, event log and demo simulator (simulated data) |

## Architecture

```
Reed switch -> ESP32 <- NEO-6M GPS
                |
        WiFi / hotspot (demo)
        4G module (production design)
                |
         Telegram Bot API -> Phone alert
                |
         Web dashboard (fleet view)
```

## Hardware

| Part | Notes |
|---|---|
| ESP32 Dev Module | Main controller |
| NEO-6M GPS | Location |
| Reed switch + magnet | Switch on the cap neck, magnet on the cap |
| LED | Cap-open indicator (built-in LED, GPIO 2) |
| Phone hotspot (2.4 GHz) | Internet link for the demo |

### Wiring

| From | To ESP32 |
|---|---|
| NEO-6M VCC | 3V3 |
| NEO-6M GND | GND |
| NEO-6M TX | GPIO 16 (RX2) |
| NEO-6M RX | GPIO 17 (TX2) |
| Reed switch wire 1 | GPIO 4 |
| Reed switch wire 2 | GND |

The reed switch uses the ESP32's internal pull-up, so no external resistor is needed.

## Setup

1. Install Arduino IDE 2.x and the **esp32 by Espressif Systems** board package.
2. Install the **TinyGPSPlus** library (Mikal Hart).
3. Create a Telegram bot with @BotFather and note the bot token and your chat ID.
4. In `firmware/fuelguard/`, copy `secrets.example.h` to `secrets.h` and fill in your values:
   ```cpp
   const char* WIFI_SSID = "YOUR_HOTSPOT_NAME";
   const char* WIFI_PASS = "YOUR_HOTSPOT_PASSWORD";
   const char* BOT_TOKEN = "YOUR_BOT_TOKEN";
   const char* CHAT_ID   = "YOUR_CHAT_ID";
   ```
   `secrets.h` is git-ignored and must never be committed.
5. Open `firmware/fuelguard/fuelguard.ino`, select **ESP32 Dev Module** and your COM port, set Upload Speed to 115200, and upload.
6. Open Serial Monitor at 115200 baud.

## Demo flow

1. Power the board. An "armed and online" message arrives on Telegram.
2. Pull the magnet away from the reed switch. The LED turns on and an alert with a Maps link arrives.
3. Return the magnet. A "cap closed" message arrives.
4. Wait for the heartbeat message, sent every 30 s in the demo.

## Design notes

- **Heartbeat:** the real product sends a location update every 6 hours. The demo interval is shortened so it can be seen live.
- **Battery life (design estimate, not measured):** with deep sleep between events, GPS and the cellular module powered only when needed, and about 4 heartbeats a day, roughly 1.5 to 2 months on one 2000 mAh cell is expected. A dev board left always on would last only days.
- **Production connectivity:** a 4G module (A7670C or SIM7600) with SMS fallback and store-and-forward for dead zones. The prototype uses WiFi instead, because 2G modules such as the SIM800L are unreliable in India.
- **Tamper detection:** wiring the reed switch as a supervised loop would also flag a cut wire.
- **Safety:** keep all electronics outside the tank and away from fuel vapour. Only the switch and magnet sit at the cap.

## Roadmap

1. Deep-sleep firmware with reed-switch wake-up and a 6-hour timer wake-up.
2. 4G module integration with SMS fallback and offline event storage.
3. Backend (for example Firebase) so the dashboard shows live device data.
4. Supervised-loop tamper detection and weatherproof enclosure.
5. Power from the vehicle battery with a backup cell.

## Repository layout

```
fuel-guard/
├── firmware/fuelguard/   ESP32 sketch (secrets.h is not committed)
├── dashboard/            Web dashboard (simulated data)
├── hardware/             Wiring and parts notes
└── README.md
```

## Team

- Dron Pathak, ECE-B1, Faculty of Technology, University of Delhi
- [Add teammates]
- Supervisor: [Add name]

## License

[Add a license, for example MIT]
