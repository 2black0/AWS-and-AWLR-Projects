# 🌦️ AWS and AWLR Projects

This repository contains three embedded systems projects designed for remote environmental monitoring using microcontrollers. The projects collect data such as water levels and weather parameters, then transmit the data over GPRS or Wi-Fi to an online server.

## 📁 Project Structure

```
.
├── LICENSE
├── README.md
├── stm32-awlr/
│   └── stm32-awlr.ino          # STM32-based Automatic Water Level Recorder
├── wemos-awlr/
│   └── wemos-awlr.ino          # Wemos D1 Mini-based AWLR with LCD & SD Logging
└── wemos32-aws/
    └── wemos32-aws.ino         # Wemos32-based Automatic Weather Station
```

---

## 📡 Project Overview

### 1. STM32-AWLR (Automatic Water Level Recorder)

A lightweight water level monitoring system using **STM32** microcontroller and **SIM800L GPRS module** for remote HTTP data posting.

* **Microcontroller**: STM32 (e.g., STM32F103C8T6 / Bluepill)
* **Sensor**: HC-SR04 Ultrasonic Sensor
* **Communication**: SIM800L GPRS module
* **Power Monitoring**: Analog voltage read for battery monitoring
* **Key Libraries**:

  * `TaskScheduler`: Manages loop timing
  * `SIM800L`: Controls SIM800L AT commands
  * `HCSR04`: Distance measurement
  * `IWatchdog`: Watchdog timer reset

#### 🔁 Process Workflow:

1. Initialize hardware and establish GPRS connection.
2. Measure water level using HC-SR04.
3. Monitor battery voltage.
4. Construct HTTP request with sensor data.
5. Send data to API endpoint.
6. If data is sent 10 times or fails twice, system resets using watchdog.

---

### 2. Wemos-AWLR

An upgraded AWLR using **ESP8266 Wemos D1 Mini** with:

* Dual Internet options: **GPRS (SIM800L)** or **Wi-Fi**

* Local logging to **microSD**

* Real-time display on **I2C LCD**

* Time management via **DS1307 RTC**

* **Microcontroller**: ESP8266 Wemos D1 Mini

* **Sensors**:

  * HC-SR04 (Ultrasonic)
  * INA219 (Voltage sensor)

* **Connectivity**:

  * SIM800L (GPRS)
  * Wi-Fi (configurable via SD)

* **Display**: 20x4 I2C LCD

* **Data Logging**: microSD card

* **Libraries**:

  * `SIM800L`, `HCSR04`, `LiquidCrystal_I2C`, `RtcDS1307`, `Adafruit_INA219`
  * `ESP8266WiFi`, `ESP8266HTTPClient`, `SD`, `SDConfigFile`

#### 🔁 Process Workflow:

1. Read configuration from SD card.
2. Initialize modules (LCD, RTC, SD, sensors).
3. Establish internet connection (GPRS or Wi-Fi).
4. Measure water level and voltage.
5. Log data to SD and display to LCD.
6. Send data to API server.
7. Reset device after 10 sends or 2 consecutive failures.

---

### 3. Wemos32-AWS (Automatic Weather Station)

A comprehensive weather monitoring station built on **ESP32**, capable of measuring:

* Voltage

* Temperature (Indoor/Outdoor)

* Humidity (Indoor/Outdoor)

* Wind direction & speed

* Rainfall (hourly & daily)

* Barometric pressure

* UV Index

* **Microcontroller**: ESP32 (Wemos32)

* **Sensors**:

  * SHT11 (Temp & Humidity Outdoor)
  * INA219 (Voltage)
  * UV sensor (Analog)
  * External Weather Sensor (via Serial1)

* **Connectivity**:

  * SIM800L (GPRS)
  * Wi-Fi

* **Display**: 20x4 I2C LCD

* **Storage**: microSD

* **Libraries**:

  * `SHT1x-ESP`, `Adafruit_INA219`, `LiquidCrystal_I2C`, `RtcDS1307`
  * `SIM800L`, `WiFi`, `HTTPClient`, `SD`, `SDConfigFile`

#### 🔁 Process Workflow:

1. Load configuration from SD card.
2. Sync time with RTC.
3. Connect to the internet via GPRS or Wi-Fi.
4. Measure all weather parameters.
5. Save readings to SD card with timestamp.
6. Format and send data to cloud API.
7. Display summary on LCD.
8. Reset system if needed.

---

## 🔌 Requirements

### Common Dependencies

* Arduino IDE with STM32, ESP8266, or ESP32 board support
* Libraries:

  * `TaskScheduler`, `SIM800L`, `HCSR04`, `SD`, `Wire`
  * `LiquidCrystal_I2C`, `RtcDS1307`, `Adafruit_INA219`
  * `ESP8266WiFi` or `WiFi` and `HTTPClient` for ESP-based boards

---

## 🛠️ Hardware Summary

| Project     | MCU     | Connectivity    | Storage | Display | RTC    | Sensors Included                  |
| ----------- | ------- | --------------- | ------- | ------- | ------ | --------------------------------- |
| STM32-AWLR  | STM32F1 | SIM800L (GPRS)  | None    | None    | None   | HC-SR04, ADC                      |
| Wemos-AWLR  | ESP8266 | SIM800L / Wi-Fi | microSD | I2C LCD | DS1307 | HC-SR04, INA219                   |
| Wemos32-AWS | ESP32   | SIM800L / Wi-Fi | microSD | I2C LCD | DS1307 | UV, SHT11, Weather Serial, INA219 |

---

## 🌐 Data Endpoint

All projects send data to the following API:

```
http://website.online/?key=API_KEY&C0=...&C1=...&...
```

Each sensor value is passed via query strings (e.g., `C0` for voltage, `C1` for distance, etc.).

---

## 🔒 License

This project is licensed under the [MIT License](LICENSE).

---

## 🙋‍♂️ Author

Developed and maintained by [Ardy Seto Priambodo](https://github.com/2black0)

---