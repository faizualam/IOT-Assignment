# ESP32 IoT Monitoring Node

## 1. Project Overview

This project is a prototype IoT monitoring device using an ESP32.

The device reads sensors, checks sensor health, processes data, and will later send data to an IoT platform.

Development is done using:

- ESP32
- PlatformIO
- VS Code
- Wokwi simulation
- Arduino framework

No physical hardware is used.

---

## 2. Current Architecture

```text
             ESP32
               |
       +-------+-------+
       |               |
      I²C             UART
       |               |
    MPU6050       Sensor Emulator
       |
   Sensor Data
       |
   FreeRTOS Queue
       |
   Processing
```
## 3. I²C Sensor

The MPU6050 is used as the I²C sensor.

Connections
MPU6050	ESP32
VCC	3.3V
GND	GND
SDA	GPIO 21
SCL	GPIO 22

The MPU6050 library used is the Adafruit MPU6050 library.

Reference:
https://github.com/adafruit/Adafruit_MPU6050

## 4. FreeRTOS

Three tasks are currently used:

I²C sensor task
UART sensor task
Processing task

Sensor readings are passed through a FreeRTOS queue.

This keeps sensor acquisition separate from processing.

Reference:
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos.html

## 5. Sensor Reading

Each reading contains:

Sensor
Value
Valid
Status
Timestamp

Example:

[DATA] MPU6050
Value: 24.00
Valid: YES
Status: OK

Invalid readings are marked as invalid and will not be used in future calculations.

## 6. Sensor States

The firmware currently supports:

WARMING_UP
OK
NO_RESPONSE
OUT_OF_RANGE
FROZEN

During warm-up, the sensor reading is not trusted.

Example:

Status: WARMING_UP
Valid: NO

After warm-up:

Status: OK
Valid: YES

## 7. Testing
I²C Test

Status: PASS

Wokwi successfully detected the MPU6050 and produced sensor readings.

Example:

[I2C] MPU6050 detected
[DATA] MPU6050 | Value: 24.00 | Valid: YES | Status: OK
UART Test

Status: IN PROGRESS

UART interface initialization works, but the current UART simulation needs further testing.

## 8. Next Steps

The remaining development will include:

Complete UART sensor testing

Sensor fault detection

Calibration

60-second data aggregation

MQTT communication

Flash store-and-forward

Remote commands

OTA update and rollback

Watchdog and reconnection

Final testing and demo


