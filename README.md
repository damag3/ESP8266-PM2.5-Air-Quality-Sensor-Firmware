# ESP8266 PM2.5 Air Quality Sensor Firmware

This firmware runs on an **ESP8266** and reads particulate matter (PM1.0 / PM2.5 / PM10) from a PMS5003‑type sensor using the Adafruit PM25 AQI library.  
It exposes a **JSON API endpoint** used by the MagicMirror module **PM25Monitor**, and optionally displays values on an ST7735 TFT screen.

---

## 🔧 Hardware Requirements

- **ESP8266 (NodeMCU / Wemos D1 Mini)**
- **PMS5003 / PMSA003 PM2.5 sensor**
- **Adafruit PM2.5 AQI library**
- **ST7735 TFT display (optional)**
- **5V power supply for PMS sensor**

### Wiring Summary

| Component | ESP8266 Pin |
|----------|-------------|
| PMS5003 TX | D5 (SoftwareSerial RX) |
| PMS5003 RX | D6 (SoftwareSerial TX) |
| PMS5003 5V | 5V |
| PMS5003 GND | GND |
| ST7735 TFT | As configured in your code |
| ESP8266 WiFi | Required |

---

## 📡 WiFi Configuration

Edit:

```cpp
const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";


````
🌐 Web API
The ESP8266 exposes a JSON endpoint:

GET /json
Example response:
````
{
  "pm1_0": 8,
  "pm2_5": 12,
  "pm10": 20,
  "status": "ok"
}
````
If the sensor fails or WiFi drops:
````
{
  "pm1_0": "--",
  "pm2_5": "--",
  "pm10": "--",
  "status": "error"
}
````
This endpoint is consumed by the MagicMirror module PM25Monitor.

🧠 Firmware Features
✔ Reads PM1.0 / PM2.5 / PM10
Using:
````
#include "Adafruit_PM25AQI.h"
````
✔ Uses SoftwareSerial for PMS5003
Compatible with ESP8266.

✔ Serves JSON via ESP8266WebServer
Used by MagicMirror.

✔ Optional TFT Display
Shows live values using ST7735 + Adafruit GFX.

✔ Auto‑refresh
Sensor is polled continuously.

▶️ Example JSON Output
````
{
  "pm1_0": 5,
  "pm2_5": 11,
  "pm10": 18,
  "status": "ok"
}
````

▶️ Flashing Instructions
1. Install ESP8266 board support
Arduino IDE → Preferences → Additional Boards Manager URLs:
````
http://arduino.esp8266.com/stable/package_esp8266com_index.json
````
2. Install required libraries
Adafruit PM25 AQI

Adafruit GFX

Adafruit ST7735

ESP8266WebServer

🔗 MagicMirror Integration
Use the MagicMirror module:

👉 PM25Monitor  
[(https://github.com/damag3/PM25Monitor/tree/main]
