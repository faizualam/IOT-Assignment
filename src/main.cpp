#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Preferences.h>
#include <WiFi.h>
#include <PubSubClient.h>

Adafruit_MPU6050 mpu;
Preferences preferences;

// ==================================================
// MQTT CONFIGURATION
// ==================================================

WiFiClient espClient;
PubSubClient mqttClient(espClient);

const char *MQTT_SERVER = "broker.hivemq.com";
const int MQTT_PORT = 1883;

const char *DEVICE_ID = "ESP32-001";
const char *FIRMWARE_VERSION = "1.0.0";

const char *MQTT_TOPIC =
    "ispatial/esp32-001/test";

// ==================================================
// MQTT RECONNECT BACKOFF
// ==================================================

unsigned long lastMQTTAttempt = 0;

unsigned long mqttRetryDelay = 2000;

const unsigned long MQTT_MAX_RETRY_DELAY = 30000;

bool mqttTestDisabled = false;

unsigned long lastHealthPublish = 0;

// ==================================================
// STORE-AND-FORWARD BUFFER
// ==================================================

#define MQTT_BUFFER_SIZE 5

struct BufferedRecord
{
    char sensor[16];
    float average;
    float minimum;
    float maximum;
    int validCount;
    unsigned long timestamp;
};

BufferedRecord mqttBuffer[MQTT_BUFFER_SIZE];

int mqttBufferCount = 0;

// ==================================================
// FUNCTION DECLARATION
// ==================================================

void bufferAggregatedRecord(
    const char *sensorName,
    float average,
    float minimum,
    float maximum,
    int validCount
)
{
    if (mqttBufferCount >= MQTT_BUFFER_SIZE)
    {
        Serial.println(
            "[BUFFER] Buffer full - record dropped"
        );

        return;
    }

    strcpy(
        mqttBuffer[mqttBufferCount].sensor,
        sensorName
    );

    mqttBuffer[mqttBufferCount].average =
        average;

    mqttBuffer[mqttBufferCount].minimum =
        minimum;

    mqttBuffer[mqttBufferCount].maximum =
        maximum;

    mqttBuffer[mqttBufferCount].validCount =
        validCount;

    mqttBuffer[mqttBufferCount].timestamp =
        millis();

    mqttBufferCount++;

    Serial.print(
        "[BUFFER] Record stored. Count: "
    );

    Serial.println(
        mqttBufferCount
    );
}

void publishAggregatedRecord(
    const char *sensorName,
    float average,
    float minimum,
    float maximum,
    int validCount
);

// ==================================================
// MQTT HEALTH MESSAGE
// ==================================================

void publishHealthMessage()
{
    if (!mqttClient.connected())
    {
        Serial.println(
            "[MQTT] Health not published - MQTT disconnected"
        );

        

        return;
    }

    String payload = "{";

    payload += "\"device_id\":\"";
    payload += DEVICE_ID;
    payload += "\",";

    payload += "\"status\":\"OK\",";

    payload += "\"mqtt\":true,";

    payload += "\"uptime\":";
    payload += String(millis());

    payload += "}";

    String topic =
        "ispatial/devices/";

    topic += DEVICE_ID;

    topic += "/health";

    if (
        mqttClient.publish(
            topic.c_str(),
            payload.c_str()
        )
    )
    {
        Serial.println(
            "[MQTT] Health message published"
        );

        Serial.print(
            "[MQTT] Health topic: "
        );

        Serial.println(
            topic
        );

        Serial.print(
            "[MQTT] Health payload: "
        );

        Serial.println(
            payload
        );
    }
    else
    {
        Serial.println(
            "[MQTT] Health publish failed"
        );
    }
}

// ==================================================
// WIFI CONFIGURATION
// ==================================================

const char *WIFI_SSID = "Wokwi-GUEST";
const char *WIFI_PASSWORD = "";

// ==================================================
// PIN CONFIGURATION
// ==================================================

#define I2C_SDA 21
#define I2C_SCL 22

#define UART_RX 16
#define UART_TX 17

// ==================================================
// UART TEST MODE
// ==================================================
//
// NORMAL
// NO_RESPONSE
// OUT_OF_RANGE
// FROZEN
//

#define UART_TEST_MODE "NORMAL"

// ==================================================
// CALIBRATION
// ==================================================

float temperatureGain = 1.0;
float temperatureOffset = 0.0;

// ==================================================
// SENSOR STATUS
// ==================================================

enum SensorStatus
{
    SENSOR_WARMING_UP,
    SENSOR_OK,
    SENSOR_NO_RESPONSE,
    SENSOR_OUT_OF_RANGE,
    SENSOR_FROZEN
};

// ==================================================
// SENSOR READING
// ==================================================

struct SensorReading
{
    char sensor[16];

    float value;

    bool valid;

    SensorStatus status;

    unsigned long timestamp;
};

// ==================================================
// QUEUE
// ==================================================

QueueHandle_t sensorQueue;

// ==================================================
// UART
// ==================================================

HardwareSerial uartSensor(2);

// ==================================================
// STATUS TO STRING
// ==================================================

const char *statusToString(SensorStatus status)
{
    switch (status)
    {
        case SENSOR_WARMING_UP:
            return "WARMING_UP";

        case SENSOR_OK:
            return "OK";

        case SENSOR_NO_RESPONSE:
            return "NO_RESPONSE";

        case SENSOR_OUT_OF_RANGE:
            return "OUT_OF_RANGE";

        case SENSOR_FROZEN:
            return "FROZEN";

        default:
            return "UNKNOWN";
    }
}

// ==================================================
// I2C SENSOR TASK
// ==================================================

void i2cSensorTask(void *parameter)
{
    bool sensorReady = false;

    unsigned long startTime = millis();

    while (true)
    {
        SensorReading reading;

        strcpy(
            reading.sensor,
            "MPU6050"
        );

        // ------------------------------------------
        // WARM-UP
        // ------------------------------------------

        if (!sensorReady)
        {
            if (millis() - startTime < 5000)
            {
                reading.value = 0.0;

                reading.valid = false;

                reading.status =
                    SENSOR_WARMING_UP;

                reading.timestamp =
                    millis();

                xQueueSend(
                    sensorQueue,
                    &reading,
                    0
                );

                vTaskDelay(
                    pdMS_TO_TICKS(1000)
                );

                continue;
            }

            sensorReady = true;
        }

        // ------------------------------------------
        // READ MPU6050
        // ------------------------------------------

        sensors_event_t acceleration;
        sensors_event_t gyro;
        sensors_event_t temperature;

        mpu.getEvent(
            &acceleration,
            &gyro,
            &temperature
        );

        // ------------------------------------------
        // RAW TEMPERATURE
        // ------------------------------------------

        float rawValue =
            temperature.temperature;

        // ------------------------------------------
        // CALIBRATION
        // ------------------------------------------

        float value =
            (rawValue * temperatureGain)
            + temperatureOffset;

        Serial.print(
            "[CALIBRATION] Raw: "
        );

        Serial.print(
            rawValue
        );

        Serial.print(
            " | Gain: "
        );

        Serial.print(
            temperatureGain
        );

        Serial.print(
            " | Offset: "
        );

        Serial.print(
            temperatureOffset
        );

        Serial.print(
            " | Calibrated: "
        );

        Serial.println(
            value
        );

        // ------------------------------------------
        // RANGE CHECK
        // ------------------------------------------

        if (
            value < -40.0 ||
            value > 85.0
        )
        {
            reading.value = value;

            reading.valid = false;

            reading.status =
                SENSOR_OUT_OF_RANGE;
        }
        else
        {
            reading.value = value;

            reading.valid = true;

            reading.status =
                SENSOR_OK;
        }

        reading.timestamp =
            millis();

        // ------------------------------------------
        // SEND TO QUEUE
        // ------------------------------------------

        xQueueSend(
            sensorQueue,
            &reading,
            0
        );

        // ------------------------------------------
        // 10 SECOND SAMPLE INTERVAL
        // ------------------------------------------

        vTaskDelay(
            pdMS_TO_TICKS(10000)
        );
    }
}

// ==================================================
// UART SENSOR TASK
// ==================================================

void uartSensorTask(void *parameter)
{
    float simulatedValue = 25.0;

    while (true)
    {
        SensorReading reading;

        strcpy(
            reading.sensor,
            "UART_SENSOR"
        );

        // ------------------------------------------
        // NORMAL
        // ------------------------------------------

        if (
            strcmp(
                UART_TEST_MODE,
                "NORMAL"
            ) == 0
        )
        {
            simulatedValue += 0.1;

            uartSensor.print(
                "TEMP:"
            );

            uartSensor.println(
                simulatedValue,
                1
            );

            unsigned long start =
                millis();

            while (
                !uartSensor.available() &&
                millis() - start < 1000
            )
            {
                vTaskDelay(
                    pdMS_TO_TICKS(10)
                );
            }

            if (
                uartSensor.available()
            )
            {
                String response =
                    uartSensor.readStringUntil(
                        '\n'
                    );

                response.trim();

                if (
                    response.startsWith(
                        "TEMP:"
                    )
                )
                {
                    reading.value =
                        response
                            .substring(5)
                            .toFloat();

                    reading.valid = true;

                    reading.status =
                        SENSOR_OK;
                }
                else
                {
                    reading.value = 0.0;

                    reading.valid = false;

                    reading.status =
                        SENSOR_NO_RESPONSE;
                }
            }
            else
            {
                reading.value = 0.0;

                reading.valid = false;

                reading.status =
                    SENSOR_NO_RESPONSE;
            }
        }

        // ------------------------------------------
        // NO RESPONSE
        // ------------------------------------------

        else if (
            strcmp(
                UART_TEST_MODE,
                "NO_RESPONSE"
            ) == 0
        )
        {
            reading.value = 0.0;

            reading.valid = false;

            reading.status =
                SENSOR_NO_RESPONSE;
        }

        // ------------------------------------------
        // OUT OF RANGE
        // ------------------------------------------

        else if (
            strcmp(
                UART_TEST_MODE,
                "OUT_OF_RANGE"
            ) == 0
        )
        {
            reading.value = 150.0;

            reading.valid = false;

            reading.status =
                SENSOR_OUT_OF_RANGE;
        }

        // ------------------------------------------
        // FROZEN
        // ------------------------------------------

        else if (
            strcmp(
                UART_TEST_MODE,
                "FROZEN"
            ) == 0
        )
        {
            reading.value = 25.0;

            reading.valid = true;

            reading.status =
                SENSOR_OK;
        }

        reading.timestamp =
            millis();

        // ------------------------------------------
        // SEND TO QUEUE
        // ------------------------------------------

        xQueueSend(
            sensorQueue,
            &reading,
            0
        );

        vTaskDelay(
            pdMS_TO_TICKS(10000)
        );
    }
}

// ==================================================
// PROCESSING + 60 SECOND AGGREGATION TASK
// ==================================================


void processingTask(void *parameter)
{
    SensorReading reading;

    // MPU6050 statistics
    float mpuSum = 0.0;
    float mpuMin = 0.0;
    float mpuMax = 0.0;
    int mpuValidCount = 0;

    // UART statistics
    float uartSum = 0.0;
    float uartMin = 0.0;
    float uartMax = 0.0;
    int uartValidCount = 0;

    unsigned long aggregationStart =
        millis();

    while (true)
    {
        // ------------------------------------------
        // RECEIVE SENSOR DATA
        // ------------------------------------------

        if (
            xQueueReceive(
                sensorQueue,
                &reading,
                pdMS_TO_TICKS(100)
            )
        )
        {
            // --------------------------------------
            // PRINT READING
            // --------------------------------------

            Serial.print("[DATA] ");

            Serial.print(
                reading.sensor
            );

            Serial.print(
                " | Value: "
            );

            Serial.print(
                reading.value
            );

            Serial.print(
                " | Valid: "
            );

            Serial.print(
                reading.valid
                    ? "YES"
                    : "NO"
            );

            Serial.print(
                " | Status: "
            );

            Serial.print(
                statusToString(
                    reading.status
                )
            );

            Serial.print(
                " | Time: "
            );

            Serial.println(
                reading.timestamp
            );

            // --------------------------------------
            // IGNORE INVALID VALUES
            // --------------------------------------

            if (!reading.valid)
            {
                continue;
            }

            // --------------------------------------
            // MPU6050
            // --------------------------------------

            if (
                strcmp(
                    reading.sensor,
                    "MPU6050"
                ) == 0
            )
            {
                if (
                    mpuValidCount == 0
                )
                {
                    mpuMin =
                        reading.value;

                    mpuMax =
                        reading.value;
                }
                else
                {
                    if (
                        reading.value < mpuMin
                    )
                    {
                        mpuMin =
                            reading.value;
                    }

                    if (
                        reading.value > mpuMax
                    )
                    {
                        mpuMax =
                            reading.value;
                    }
                }

                mpuSum +=
                    reading.value;

                mpuValidCount++;
            }

            // --------------------------------------
            // UART SENSOR
            // --------------------------------------

            else if (
                strcmp(
                    reading.sensor,
                    "UART_SENSOR"
                ) == 0
            )
            {
                if (
                    uartValidCount == 0
                )
                {
                    uartMin =
                        reading.value;

                    uartMax =
                        reading.value;
                }
                else
                {
                    if (
                        reading.value < uartMin
                    )
                    {
                        uartMin =
                            reading.value;
                    }

                    if (
                        reading.value > uartMax
                    )
                    {
                        uartMax =
                            reading.value;
                    }
                }

                uartSum +=
                    reading.value;

                uartValidCount++;
            }
        }

        // ------------------------------------------
        // 60 SECOND REPORT
        // ------------------------------------------

        if (
            millis() - aggregationStart
            >= 60000
        )
        {
            Serial.println();

            Serial.println(
                "========== 60 SECOND RECORD =========="
            );

            // --------------------------------------
            // MPU6050 RECORD
            // --------------------------------------

            if (
                mpuValidCount > 0
            )
            {
                float average =
                    mpuSum /
                    mpuValidCount;

                Serial.println(
                    "[AGG] MPU6050"
                );

                Serial.print(
                    "Average: "
                );

                Serial.println(
                    average
                );

                Serial.print(
                    "Minimum: "
                );

                Serial.println(
                    mpuMin
                );

                Serial.print(
                    "Maximum: "
                );

                Serial.println(
                    mpuMax
                );

                Serial.print(
                    "Valid Samples: "
                );

                Serial.println(
                    mpuValidCount
                );

                // Publish MQTT
                publishAggregatedRecord(
                    "MPU6050",
                    average,
                    mpuMin,
                    mpuMax,
                    mpuValidCount
                );
            }
            else
            {
                Serial.println(
                    "[AGG] MPU6050: No valid samples"
                );
            }

            // --------------------------------------
            // UART RECORD
            // --------------------------------------

            if (
                uartValidCount > 0
            )
            {
                float average =
                    uartSum /
                    uartValidCount;

                Serial.println(
                    "[AGG] UART_SENSOR"
                );

                Serial.print(
                    "Average: "
                );

                Serial.println(
                    average
                );

                Serial.print(
                    "Minimum: "
                );

                Serial.println(
                    uartMin
                );

                Serial.print(
                    "Maximum: "
                );

                Serial.println(
                    uartMax
                );

                Serial.print(
                    "Valid Samples: "
                );

                Serial.println(
                    uartValidCount
                );

                // Publish MQTT
                publishAggregatedRecord(
                    "UART_SENSOR",
                    average,
                    uartMin,
                    uartMax,
                    uartValidCount
                );
            }
            else
            {
                Serial.println(
                    "[AGG] UART_SENSOR: No valid samples"
                );
            }

            Serial.println(
                "======================================"
            );

            // --------------------------------------
            // RESET STATISTICS
            // --------------------------------------

            mpuSum = 0.0;
            mpuMin = 0.0;
            mpuMax = 0.0;
            mpuValidCount = 0;

            uartSum = 0.0;
            uartMin = 0.0;
            uartMax = 0.0;
            uartValidCount = 0;

            aggregationStart =
                millis();
        }
    }
}

// ==================================================
// LOAD CALIBRATION FROM NVS
// ==================================================

void loadCalibration()
{
    preferences.begin(
        "calibration",
        false
    );

    temperatureGain =
        preferences.getFloat(
            "temp_gain",
            1.0
        );

    temperatureOffset =
        preferences.getFloat(
            "temp_offset",
            0.0
        );

    preferences.end();

    Serial.println(
        "[CALIBRATION] Loaded from NVS"
    );

    Serial.print(
        "[CALIBRATION] Gain: "
    );

    Serial.println(
        temperatureGain
    );

    Serial.print(
        "[CALIBRATION] Offset: "
    );

    Serial.println(
        temperatureOffset
    );
}

// ==================================================
// SAVE CALIBRATION TO NVS
// ==================================================

void saveCalibration(
    float gain,
    float offset
)
{
    preferences.begin(
        "calibration",
        false
    );

    preferences.putFloat(
        "temp_gain",
        gain
    );

    preferences.putFloat(
        "temp_offset",
        offset
    );

    preferences.end();

    temperatureGain =
        gain;

    temperatureOffset =
        offset;

    Serial.println(
        "[CALIBRATION] Saved to NVS"
    );

    Serial.print(
        "[CALIBRATION] New Gain: "
    );

    Serial.println(
        temperatureGain
    );

    Serial.print(
        "[CALIBRATION] New Offset: "
    );

    Serial.println(
        temperatureOffset
    );
}
// ==================================================
// CONNECT TO WIFI
// ==================================================

void connectWiFi()
{
    Serial.println(
        "[WIFI] Connecting..."
    );

    WiFi.begin(
        WIFI_SSID,
        WIFI_PASSWORD
    );

    unsigned long startTime =
        millis();

    while (
        WiFi.status() != WL_CONNECTED &&
        millis() - startTime < 15000
    )
    {
        Serial.print(".");

        delay(500);
    }

    Serial.println();

    if (
        WiFi.status() == WL_CONNECTED
    )
    {
        Serial.println(
            "[WIFI] Connected"
        );

        Serial.print(
            "[WIFI] IP Address: "
        );

        Serial.println(
            WiFi.localIP()
        );

        Serial.print(
            "[WIFI] RSSI: "
        );

        Serial.println(
            WiFi.RSSI()
        );
    }
    else
    {
        Serial.println(
            "[WIFI] Connection failed"
        );
    }
}

// ==================================================
// CONNECT TO MQTT
// ==================================================

void connectMQTT()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        return;
    }

    unsigned long now = millis();

    // Wait until retry delay has elapsed
    if (
        now - lastMQTTAttempt <
        mqttRetryDelay
    )
    {
        return;
    }

    lastMQTTAttempt = now;

    mqttClient.setServer(
        MQTT_SERVER,
        MQTT_PORT
    );

    Serial.println(
        "[MQTT] Connecting..."
    );

    String clientId =
        "ESP32-001-";

    clientId +=
        String(random(0xffff), HEX);

    if (
        mqttClient.connect(
            clientId.c_str()
        )
    )
    {
        Serial.println(
            "[MQTT] Connected"
        );

        Serial.print(
            "[MQTT] Broker: "
        );

        Serial.println(
            MQTT_SERVER
        );

        // Connection successful:
        // reset backoff
        mqttRetryDelay = 2000;
    }
    else
    {
        Serial.print(
            "[MQTT] Connection failed, state="
        );

        Serial.println(
            mqttClient.state()
        );

        // Double retry delay
        mqttRetryDelay *= 2;

        if (
            mqttRetryDelay >
            MQTT_MAX_RETRY_DELAY
        )
        {
            mqttRetryDelay =
                MQTT_MAX_RETRY_DELAY;
        }

        Serial.print(
            "[MQTT] Next retry in "
        );

        Serial.print(
            mqttRetryDelay / 1000
        );

        Serial.println(
            " seconds"
        );
    }
}
// ==================================================
// PUBLISH TEST JSON
// ==================================================

// ==================================================
// PUBLISH 60-SECOND AGGREGATED RECORD
// ==================================================

void publishAggregatedRecord(
    const char *sensorName,
    float average,
    float minimum,
    float maximum,
    int validCount
)
{
    if (!mqttClient.connected())
    {
        Serial.println(
            "[MQTT] Not connected - storing record"
        );

        bufferAggregatedRecord(
            sensorName,
            average,
            minimum,
            maximum,
            validCount
        );

        return;
    }

    String payload = "{";

    payload += "\"device_id\":\"";
    payload += DEVICE_ID;
    payload += "\",";

    payload += "\"firmware\":\"";
    payload += FIRMWARE_VERSION;
    payload += "\",";

    payload += "\"sensor\":\"";
    payload += sensorName;
    payload += "\",";

    payload += "\"timestamp\":";
    payload += String(millis());
    payload += ",";

    payload += "\"average\":";
    payload += String(average, 2);
    payload += ",";

    payload += "\"min\":";
    payload += String(minimum, 2);
    payload += ",";

    payload += "\"max\":";
    payload += String(maximum, 2);
    payload += ",";

    payload += "\"valid_count\":";
    payload += String(validCount);

    payload += "}";

    String topic =
        "ispatial/devices/";

    topic += DEVICE_ID;

    topic += "/telemetry/";

    topic += sensorName;

    if (
        mqttClient.publish(
            topic.c_str(),
            payload.c_str()
        )
    )
    {
        Serial.println(
            "[MQTT] Aggregated record published"
        );

        Serial.print(
            "[MQTT] Topic: "
        );

        Serial.println(
            topic
        );

        Serial.print(
            "[MQTT] Payload: "
        );

        Serial.println(
            payload
        );
    }
    else
    {
        Serial.println(
            "[MQTT] Aggregated publish failed"
        );
    }
}
// ==================================================
// FLUSH STORED MQTT RECORDS
// ==================================================

void flushMQTTBuffer()
{
    if (!mqttClient.connected() || mqttBufferCount == 0)
    {
        return;
    }

    Serial.print("[BUFFER] Flushing ");
    Serial.print(mqttBufferCount);
    Serial.println(" stored record(s)");

    while (mqttBufferCount > 0 && mqttClient.connected())
    {
        BufferedRecord &record = mqttBuffer[0];

        String payload = "{";
        payload += "\"device_id\":\"";
        payload += DEVICE_ID;
        payload += "\",";
        payload += "\"firmware\":\"";
        payload += FIRMWARE_VERSION;
        payload += "\",";
        payload += "\"sensor\":\"";
        payload += record.sensor;
        payload += "\",";
        payload += "\"timestamp\":";
        payload += String(record.timestamp);
        payload += ",";
        payload += "\"average\":";
        payload += String(record.average, 2);
        payload += ",";
        payload += "\"min\":";
        payload += String(record.minimum, 2);
        payload += ",";
        payload += "\"max\":";
        payload += String(record.maximum, 2);
        payload += ",";
        payload += "\"valid_count\":";
        payload += String(record.validCount);
        payload += "}";

        String topic = "ispatial/devices/";
        topic += DEVICE_ID;
        topic += "/telemetry/";
        topic += record.sensor;

        if (mqttClient.publish(topic.c_str(), payload.c_str()))
        {
            Serial.println("[BUFFER] Stored record published");
            Serial.print("[BUFFER] Topic: ");
            Serial.println(topic);

            for (int i = 1; i < mqttBufferCount; i++)
            {
                mqttBuffer[i - 1] = mqttBuffer[i];
            }

            mqttBufferCount--;

            Serial.print("[BUFFER] Remaining: ");
            Serial.println(mqttBufferCount);
        }
        else
        {
            Serial.println("[BUFFER] Publish failed - keeping record");
            break;
        }
    }
}

// ==================================================
// SETUP
// ==================================================

void setup()
{
    // ------------------------------------------
    // SERIAL
    // ------------------------------------------

    Serial.begin(
        115200
    );

    delay(1000);

    // ------------------------------------------
    // LOAD CALIBRATION
    // ------------------------------------------

    loadCalibration();

    connectWiFi();

    connectMQTT();

    Serial.println();

    Serial.println(
        "================================="
    );

    Serial.println(
        " IoT Monitoring Node"
    );

    Serial.println(
        " Phase 5 - Aggregation"
    );

    Serial.println(
        "================================="
    );

    // ------------------------------------------
    // I2C
    // ------------------------------------------

    Wire.begin(
        I2C_SDA,
        I2C_SCL
    );

    Serial.println(
        "[I2C] Initializing MPU6050..."
    );

    if (
        !mpu.begin()
    )
    {
        Serial.println(
            "[ERROR] MPU6050 not detected!"
        );
    }
    else
    {
        Serial.println(
            "[I2C] MPU6050 detected"
        );
    }

    // ------------------------------------------
    // UART
    // ------------------------------------------

    uartSensor.begin(
        9600,
        SERIAL_8N1,
        UART_RX,
        UART_TX
    );

    Serial.println(
        "[UART] UART interface initialized"
    );

    // ------------------------------------------
    // QUEUE
    // ------------------------------------------

    sensorQueue =
        xQueueCreate(
            10,
            sizeof(SensorReading)
        );

    if (
        sensorQueue == NULL
    )
    {
        Serial.println(
            "[ERROR] Queue creation failed!"
        );

        while (true)
        {
            delay(1000);
        }
    }

    Serial.println(
        "[QUEUE] Queue created"
    );

    // ------------------------------------------
    // I2C TASK
    // ------------------------------------------

    xTaskCreate(
        i2cSensorTask,
        "I2C Sensor",
        4096,
        NULL,
        2,
        NULL
    );

    // ------------------------------------------
    // UART TASK
    // ------------------------------------------

    xTaskCreate(
        uartSensorTask,
        "UART Sensor",
        4096,
        NULL,
        2,
        NULL
    );

    // ------------------------------------------
    // PROCESSING TASK
    // ------------------------------------------

    xTaskCreate(
        processingTask,
        "Processing",
        4096,
        NULL,
        1,
        NULL
    );

    // ------------------------------------------
    // SYSTEM INFORMATION
    // ------------------------------------------

    Serial.println(
        "[SYSTEM] FreeRTOS tasks started"
    );

    Serial.print(
        "[TEST] UART mode: "
    );

    Serial.println(
        UART_TEST_MODE
    );

    Serial.print(
        "[CALIBRATION] Gain: "
    );

    Serial.println(
        temperatureGain
    );

    Serial.print(
        "[CALIBRATION] Offset: "
    );

    Serial.println(
        temperatureOffset
    );
}

// ==================================================
// MAIN LOOP
// ==================================================

void loop()
{
    // ------------------------------------------
    // MQTT AUTO RECONNECT with Backoff
    // ------------------------------------------

    if (
        WiFi.status() == WL_CONNECTED &&
        !mqttClient.connected() &&
        !mqttTestDisabled
    )
    {
        connectMQTT();
    }

    // ------------------------------------------
    // SERIAL COMMAND HANDLING
    // ------------------------------------------

   if (Serial.available())
{
    String command =
        Serial.readStringUntil('\n');

    command.trim();

    // MQTT fault injection
    if (command == "MQTT_OFF")
    {
        mqttTestDisabled = true;

        mqttClient.disconnect();

        Serial.println(
            "[TEST] MQTT disabled"
        );
    }

    else if (command == "MQTT_ON")
    {
        mqttTestDisabled = false;

        Serial.println(
            "[TEST] MQTT enabled"
        );

        lastMQTTAttempt = 0;
        mqttRetryDelay = 2000;
        connectMQTT();
    }
    }

    // ------------------------------------------
    // MQTT CLIENT LOOP
    // ------------------------------------------

    if (mqttClient.connected())
    {
        mqttClient.loop();

        flushMQTTBuffer();

        // ------------------------------------------
// PERIODIC MQTT HEALTH MESSAGE
// ------------------------------------------

if (
    mqttClient.connected() &&
    millis() - lastHealthPublish >= 30000
)
{
    publishHealthMessage();

    lastHealthPublish = millis();
}
    }

    vTaskDelay(
        pdMS_TO_TICKS(100)
    );
}
