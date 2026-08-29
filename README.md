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
