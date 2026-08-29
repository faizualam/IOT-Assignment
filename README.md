# IoT Monitoring Node

An ESP32-based IoT monitoring system developed using PlatformIO and Wokwi. The system collects sensor data from an MPU6050 through I2C, processes data using FreeRTOS tasks, performs calibration and validation, aggregates measurements over 60-second periods, and publishes telemetry and health information through MQTT.

The system also implements MQTT fault handling using a store-and-forward buffer so that aggregated records can be retained during temporary MQTT outages and automatically transmitted after reconnection.

---

## 1. Project Overview

The IoT Monitoring Node provides a complete sensor monitoring and communication pipeline:

- ESP32-based monitoring node
- MPU6050 sensor using I2C
- UART sensor interface
- Sensor warm-up and validation
- Calibration using gain and offset
- Persistent calibration using ESP32 NVS
- FreeRTOS task-based processing
- Queue-based sensor data processing
- 60-second sensor aggregation
- MQTT telemetry publishing
- MQTT health monitoring
- MQTT connection/reconnection
- MQTT fault testing
- Store-and-forward buffering
- Multiple buffered record handling
- Automatic buffered record transmission after MQTT recovery

The project was developed and tested using **PlatformIO** and **Wokwi**.

---

## 2. System Architecture

The overall system follows this processing flow:

```text
                         +----------------------+
                         |        ESP32         |
                         |   IoT Monitoring     |
                         |        Node          |
                         +----------+-----------+
                                    |
                    +---------------+---------------+
                    |                               |
                    v                               v
             +-------------+                 +-------------+
             |   MPU6050   |                 | UART Sensor |
             |     I2C     |                 | Interface   |
             +------+------+                 +------+------+
                    |                               |
                    +---------------+---------------+
                                    |
                                    v
                         +----------------------+
                         | Sensor Validation    |
                         | & Calibration         |
                         +----------+-----------+
                                    |
                                    v
                         +----------------------+
                         | FreeRTOS Processing  |
                         | & Queue              |
                         +----------+-----------+
                                    |
                                    v
                         +----------------------+
                         | 60-Second Aggregation|
                         | Avg / Min / Max      |
                         +----------+-----------+
                                    |
                                    v
                         +----------------------+
                         |    MQTT Publisher    |
                         +----------+-----------+
                                    |
                       +------------+------------+
                       |                         |
                       v                         v
                MQTT Connected           MQTT Unavailable
                       |                         |
                       |                         v
                       |                +----------------+
                       |                | Store-and-     |
                       |                | Forward Buffer |
                       |                +-------+--------+
                       |                        |
                       +<-----------------------+
                              Reconnect

```
## 3. Hardware
Components
| Component      | Purpose                         |
| -------------- | ------------------------------- |
| ESP32 DevKit   | Main microcontroller            |
| MPU6050        | Sensor connected through I2C    |
| UART Interface | Secondary sensor interface      |
| Wokwi          | Hardware simulation and testing |

## MPU6050 Wiring
| MPU6050 Pin | ESP32   |
| ----------- | ------- |
| VCC         | 3.3V    |
| GND         | GND     |
| SDA         | GPIO 21 |
| SCL         | GPIO 22 |

The MPU6050 is detected successfully during startup.

## 4. Software Environment
The project uses:

PlatformIO

ESP32 Arduino framework

Wokwi

FreeRTOS

Adafruit MPU6050 library

PubSubClient MQTT library

ESP32 Preferences / NVS

## MQTT Broker

The MQTT broker used during testing is:

``` Text
broker.hivemq.com

```
MQTT port:
```
1883

```
## 5. Project Structure

```
IOT Assignment/
│
├── .vscode/
├── include/
├── lib/
├── src/
│   └── main.cpp
├── test/
│
├── .gitignore
├── README.md
├── platformio.ini
│
├── architecture.png
├── Startup + sensor detection.png
├── Calibration + valid sensor data.png
├── wifi & MQTT connection.png
├── 60-second aggregation + JSON.png
├── Health message.png
├── MQTT OFF 60s.png
├── MQTT ON.png
├── GIT Evidence.png
└── Terminal.png
```
## 6. Sensor Acquisition
6.1 MPU6050

The MPU6050 is connected to the ESP32 using the I2C interface.

The system initializes the sensor during startup:
```
[I2C] Initializing MPU6050...
[I2C] MPU6050 detected

```
## 6.2 UART Sensor

A UART sensor interface is also implemented.

When the UART sensor does not provide a valid response, the system reports:

```
[DATA] UART_SENSOR | Value: 0.00 | Valid: NO | Status: NO_RESPONSE

```
This allows the processing system to distinguish valid sensor readings from invalid or missing readings.

## 7. Sensor Validation

Sensor readings are assigned a status according to their condition.

The implemented status categories include:
```
WARMING_UP
OK
NO_RESPONSE
OUT_OF_RANGE
FROZEN
```
During startup, the MPU6050 enters a warm-up period:
```
[DATA] MPU6050 | Value: 0.00 | Valid: NO | Status: WARMING_UP
```
After warm-up, valid readings are produced:
```
[DATA] MPU6050 | Value: 24.00 | Valid: YES | Status: OK
```
## 8. Calibration

The system applies calibration using a gain and offset.

The calibration process produces output similar to:
```
[CALIBRATION] Raw: 24.00 | Gain: 1.00 | Offset: 0.00 | Calibrated: 24.00
```
The calibration parameters are stored using ESP32 NVS.

At startup, stored calibration information is loaded:
```
[CALIBRATION] Loaded from NVS
[CALIBRATION] Gain: 1.00
[CALIBRATION] Offset: 0.00
```
## 9. FreeRTOS Processing

The system uses FreeRTOS tasks to separate sensor acquisition and processing.

A queue is used for transferring sensor readings between system components.

The startup output confirms:
```
[QUEUE] Queue created
[SYSTEM] FreeRTOS tasks started
```
This architecture allows sensor acquisition and processing to operate independently.

## 10. 60-Second Data Aggregation

The system aggregates valid sensor readings over a 60-second period.

The following values are calculated:

Average
Minimum
Maximum
Valid sample count

Example:
```
========== 60 SECOND RECORD ==========
[AGG] MPU6050
Average: 24.00
Minimum: 24.00
Maximum: 24.00
Valid Samples: 6
```
The UART sensor is also included in the aggregation system. If it has no valid samples, the system reports:
```
[AGG] UART_SENSOR: No valid samples
```
## 11. Wi-Fi Connectivity

The ESP32 connects to Wi-Fi before establishing MQTT communication.

Example successful connection:
```
[WIFI] Connecting...
[WIFI] Connected
[WIFI] IP Address: 10.10.0.2
[WIFI] RSSI: -77
```
## 12. MQTT Communication

MQTT is used to transmit aggregated sensor data and device health information.

Successful MQTT connection:
```
[MQTT] Connecting...
[MQTT] Connected
[MQTT] Broker: broker.hivemq.com
```
## 12.1 MQTT Telemetry Topic

The MPU6050 telemetry topic is:
```
ispatial/devices/ESP32-001/telemetry/MPU6050

```
## 12.2 MQTT Telemetry Payload

The system publishes aggregated sensor data as JSON.

Example:
```
{
  "device_id": "ESP32-001",
  "firmware": "1.0.0",
  "sensor": "MPU6050",
  "timestamp": 127251,
  "average": 24.00,
  "min": 24.00,
  "max": 24.00,
  "valid_count": 6
}
```
## 13. MQTT Health Monitoring

The device periodically publishes a health message.

## Health Topic
```
ispatial/devices/ESP32-001/health
```
Health Payload

Example:
```
{
  "device_id": "ESP32-001",
  "status": "OK",
  "mqtt": true,
  "uptime": 60157
}
```
Successful publication:
```
[MQTT] Health message published
[MQTT] Health topic: ispatial/devices/ESP32-001/health
```
The health message provides:

Device ID

System status

MQTT state

Device uptime

## 14. MQTT Fault Testing

The system supports MQTT fault testing through serial commands.

## Disable MQTT

Enter:
```
MQTT_OFF
```
The system reports:
```
[TEST] MQTT disabled
```
## Enable MQTT

Enter:
```
MQTT_ON
```
The system attempts to reconnect:
```
[TEST] MQTT enabled
[MQTT] Connecting...
[MQTT] Connected
```
## 15. Store-and-Forward Buffer

A local in-memory buffer is used when MQTT is unavailable.

Instead of immediately discarding an aggregated record, the system stores it.

Example:
```
[MQTT] Not connected - storing record
[BUFFER] Record stored. Count: 1
```
This provides temporary data retention during MQTT outages.

## 16. Buffered Record Recovery

When MQTT reconnects, buffered records are automatically published.

Example:
```
[BUFFER] Flushing 1 stored record(s)
[BUFFER] Stored record published
[BUFFER] Topic: ispatial/devices/ESP32-001/telemetry/MPU6050
[BUFFER] Remaining: 0
```
The buffer count is reduced only after successful MQTT publication.

This implements a basic store-and-forward mechanism.

## 17. Multiple Buffered Records

The buffer supports multiple records.

During an MQTT outage, multiple aggregation periods can create multiple buffered records.

Example flow:
```
MQTT_OFF
    |
    +--> Aggregated Record 1
    |       |
    |       +--> Buffer Count: 1
    |
    +--> Aggregated Record 2
            |
            +--> Buffer Count: 2

MQTT_ON
    |
    +--> MQTT reconnects
    |
    +--> Buffered Record 1 published
    |
    +--> Buffered Record 2 published
    |
    +--> Buffer Count: 0
```
This was tested successfully.

## 18. Store-and-Forward Test Evidence

The store-and-forward mechanism was tested by disabling MQTT and allowing the system to generate an aggregated record.

Expected result:
```
========== 60 SECOND RECORD ==========
[AGG] MPU6050
Average: 24.00
Minimum: 24.00
Maximum: 24.00
Valid Samples: 6
[MQTT] Not connected - storing record
[BUFFER] Record stored. Count: 1
```
After enabling MQTT:
```
MQTT_ON
[TEST] MQTT enabled
[MQTT] Connecting...
[MQTT] Connected
[BUFFER] Flushing 1 stored record(s)
[BUFFER] Stored record published
[BUFFER] Remaining: 0
```
This demonstrates:
```
MQTT outage
     ↓
Record generated
     ↓
Record buffered
     ↓
MQTT recovery
     ↓
Record published
     ↓
Buffer cleared
```
## 19. Test Results
| Test                           | Result |
| ------------------------------ | ------ |
| ESP32 startup                  | PASS   |
| MPU6050 detection              | PASS   |
| I2C communication              | PASS   |
| UART interface                 | PASS   |
| Sensor warm-up                 | PASS   |
| Sensor validation              | PASS   |
| Calibration                    | PASS   |
| NVS calibration loading        | PASS   |
| FreeRTOS task startup          | PASS   |
| Queue creation                 | PASS   |
| 60-second aggregation          | PASS   |
| Wi-Fi connection               | PASS   |
| MQTT connection                | PASS   |
| MQTT telemetry JSON            | PASS   |
| MQTT health message            | PASS   |
| MQTT OFF testing               | PASS   |
| Local buffering                | PASS   |
| MQTT recovery                  | PASS   |
| Buffered record publishing     | PASS   |
| Multiple buffered records      | PASS   |
| Git repository synchronization | PASS   |

# 20. Evidence Screenshots

## 20.1 System Architecture

## 20.2 Startup and Sensor Detection

## 20.3 Calibration and Valid Sensor Data

## 20.4 Wi-Fi and MQTT Connection

## 20.5 60-Second Aggregation and MQTT JSON

## 20.6 MQTT Health Message

## 20.7 MQTT OFF / Store-and-Forward

## 20.8 MQTT ON / Recovery

## 20.9 Git Evidence

## 20.10 Terminal / Build Evidence 

# 21. Build and Run
## PlatformIO
Open the project in VS Code with PlatformIO installed.

The ESP32 target is:
```
esp32dev
```
Build using:
```
PlatformIO → Project Tasks → esp32dev → General → Build
```
The project was successfully compiled during development and testing.

## Wokwi

The project was tested using Wokwi with:

ESP32 DevKit

MPU6050

I2C connections

Serial Monitor

The serial monitor was used to verify sensor readings, calibration, aggregation, MQTT communication, fault handling, and recovery.

# 22. Serial Test Commands

The following commands are available for MQTT testing:

## Disable MQTT
```
MQTT_OFF
```
## Enable MQTT
```
MQTT_ON
```
These commands allow MQTT failure and recovery behavior to be demonstrated without changing the firmware.

# 23. Example Successful System Output
```
[WIFI] Connected
[MQTT] Connected
[MQTT] Broker: broker.hivemq.com

[I2C] Initializing MPU6050...
[I2C] MPU6050 detected

[UART] UART interface initialized
[QUEUE] Queue created
[SYSTEM] FreeRTOS tasks started

[DATA] MPU6050 | Value: 24.00 | Valid: YES | Status: OK

========== 60 SECOND RECORD ==========
[AGG] MPU6050
Average: 24.00
Minimum: 24.00
Maximum: 24.00
Valid Samples: 6

[MQTT] Aggregated record published

[MQTT] Health message published
```
# 24. Example MQTT Failure and Recovery

## MQTT unavailable
```
[MQTT] Not connected - storing record
[BUFFER] Record stored. Count: 1
```
## MQTT restored
```
[MQTT] Connected
[BUFFER] Flushing 1 stored record(s)
[BUFFER] Stored record published
[BUFFER] Remaining: 0
```
# 25. Git Version Control

The project is maintained using Git and the main branch.

Final repository verification:
```
On branch main
Your branch is up to date with 'origin/main'.

nothing to commit, working tree clean
```

The project history contains commits for the major implementation phases, including MQTT communication, health monitoring, aggregation, and store-and-forward buffering.

# 26. Final Project Status

The IoT Monitoring Node successfully demonstrates a complete monitoring pipeline:
```
Sensor
  ↓
Validation
  ↓
Calibration
  ↓
FreeRTOS Processing
  ↓
60-Second Aggregation
  ↓
MQTT Telemetry
  ↓
Health Monitoring
  ↓
MQTT Failure Detection
  ↓
Local Buffer
  ↓
MQTT Reconnection
  ↓
Buffered Data Published
```
All major implemented and tested functions have passed validation.

# 27. Conclusion

The IoT Monitoring Node provides a reliable ESP32-based monitoring solution combining sensor acquisition, validation, calibration, real-time task processing, aggregation, MQTT communication, health monitoring, and temporary offline data storage.

Testing demonstrated successful MPU6050 detection, calibrated sensor readings, 60-second aggregation, MQTT telemetry publishing, health reporting, MQTT failure handling, store-and-forward buffering, and automatic recovery after MQTT reconnection.

The project has been tested using Wokwi and PlatformIO and is ready for final submission.

```
### Final step

After replacing your README on GitHub, commit it:

```powershell
git add README.md
git commit -m "Complete project documentation and evidence"
git push origin main
```
Then verify:
```
git status
```
You want:
```
Your branch is up to date with 'origin/main'.

nothing to commit, working tree clean

```





















