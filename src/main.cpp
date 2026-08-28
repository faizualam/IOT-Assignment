#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Preferences.h>

Adafruit_MPU6050 mpu;

Preferences preferences;

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
// Available modes:
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

// Formula:
// calibrated value = (raw value × gain) + offset

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
// SENSOR READING STRUCTURE
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
// FREE RTOS QUEUE
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
        // SENSOR WARM-UP
        // ------------------------------------------

        if (!sensorReady)
        {
            if (millis() - startTime < 5000)
            {
                reading.value = 0;

                reading.valid = false;

                reading.status =
                    SENSOR_WARMING_UP;

                reading.timestamp = millis();

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
        // RAW VALUE
        // ------------------------------------------

        float rawValue =
            temperature.temperature;

        // ------------------------------------------
        // CALIBRATION
        // ------------------------------------------

        float value =
            (rawValue * temperatureGain)
            + temperatureOffset;

        // ------------------------------------------
        // CALIBRATION LOG
        // ------------------------------------------

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
        // RANGE VALIDATION
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

        reading.timestamp = millis();

        // ------------------------------------------
        // SEND TO QUEUE
        // ------------------------------------------

        xQueueSend(
            sensorQueue,
            &reading,
            0
        );

        // ------------------------------------------
        // SAMPLE EVERY 10 SECONDS
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
        // NORMAL MODE
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

            // Wait maximum 1 second
            // for UART response.

            while (
                !uartSensor.available() &&
                millis() - start < 1000
            )
            {
                vTaskDelay(
                    pdMS_TO_TICKS(10)
                );
            }

            // --------------------------------------
            // UART RESPONSE RECEIVED
            // --------------------------------------

            if (uartSensor.available())
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
                    reading.value = 0;

                    reading.valid = false;

                    reading.status =
                        SENSOR_NO_RESPONSE;
                }
            }

            // --------------------------------------
            // NO UART RESPONSE
            // --------------------------------------

            else
            {
                reading.value = 0;

                reading.valid = false;

                reading.status =
                    SENSOR_NO_RESPONSE;
            }
        }

        // ------------------------------------------
        // NO RESPONSE MODE
        // ------------------------------------------

        else if (
            strcmp(
                UART_TEST_MODE,
                "NO_RESPONSE"
            ) == 0
        )
        {
            // Do not send anything.

            reading.value = 0;

            reading.valid = false;

            reading.status =
                SENSOR_NO_RESPONSE;
        }

        // ------------------------------------------
        // OUT OF RANGE MODE
        // ------------------------------------------

        else if (
            strcmp(
                UART_TEST_MODE,
                "OUT_OF_RANGE"
            ) == 0
        )
        {
            uartSensor.println(
                "TEMP:150.0"
            );

            vTaskDelay(
                pdMS_TO_TICKS(20)
            );

            if (uartSensor.available())
            {
                String response =
                    uartSensor.readStringUntil(
                        '\n'
                    );

                response.trim();

                reading.value =
                    response
                        .substring(5)
                        .toFloat();

                reading.valid = false;

                reading.status =
                    SENSOR_OUT_OF_RANGE;
            }
            else
            {
                reading.value = 150.0;

                reading.valid = false;

                reading.status =
                    SENSOR_OUT_OF_RANGE;
            }
        }

        // ------------------------------------------
        // FROZEN MODE
        // ------------------------------------------

        else if (
            strcmp(
                UART_TEST_MODE,
                "FROZEN"
            ) == 0
        )
        {
            uartSensor.println(
                "TEMP:25.0"
            );

            vTaskDelay(
                pdMS_TO_TICKS(20)
            );

            if (uartSensor.available())
            {
                String response =
                    uartSensor.readStringUntil(
                        '\n'
                    );

                response.trim();

                reading.value =
                    response
                        .substring(5)
                        .toFloat();

                // Preliminary frozen test.
                // Full time-based detection
                // will be improved later.

                reading.valid = true;

                reading.status =
                    SENSOR_OK;
            }
            else
            {
                reading.value = 25.0;

                reading.valid = true;

                reading.status =
                    SENSOR_OK;
            }
        }

        // ------------------------------------------
        // TIMESTAMP
        // ------------------------------------------

        reading.timestamp = millis();

        // ------------------------------------------
        // SEND TO QUEUE
        // ------------------------------------------

        xQueueSend(
            sensorQueue,
            &reading,
            0
        );

        // ------------------------------------------
        // SAMPLE EVERY 10 SECONDS
        // ------------------------------------------

        vTaskDelay(
            pdMS_TO_TICKS(10000)
        );
    }
}

// ==================================================
// PROCESSING TASK
// ==================================================

void processingTask(void *parameter)
{
    SensorReading reading;

    while (true)
    {
        if (
            xQueueReceive(
                sensorQueue,
                &reading,
                portMAX_DELAY
            )
        )
        {
            Serial.print(
                "[DATA] "
            );

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
void saveCalibration(float gain, float offset)
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

    temperatureGain = gain;
    temperatureOffset = offset;

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
// SETUP
// ==================================================

void setup()
{
    // ------------------------------------------
    // SERIAL MONITOR
    // ------------------------------------------

    Serial.begin(115200);

    delay(1000);

    loadCalibration();


    Serial.println();

    Serial.println("=================================");

    Serial.println(" IoT Monitoring Node ");

    Serial.println(" Phase 4 - Calibration");

    Serial.println("=================================");

    // ------------------------------------------
    // I2C INITIALIZATION
    // ------------------------------------------

    Wire.begin(
        I2C_SDA,
        I2C_SCL
    );

    Serial.println(
        "[I2C] Initializing MPU6050..."
    );

    if (!mpu.begin())
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
    // UART INITIALIZATION
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
    // QUEUE CREATION
    // ------------------------------------------

    sensorQueue =
        xQueueCreate(
            10,
            sizeof(SensorReading)
        );

    if (sensorQueue == NULL)
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
    // CREATE I2C TASK
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
    // CREATE UART TASK
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
    // CREATE PROCESSING TASK
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
    if (Serial.available())
    {
        String command =
            Serial.readStringUntil('\n');

        command.trim();

        if (command.startsWith("SETCAL"))
        {
            float gain;
            float offset;

            int firstSpace =
                command.indexOf(' ');

            int secondSpace =
                command.indexOf(
                    ' ',
                    firstSpace + 1
                );

            if (
                firstSpace > 0 &&
                secondSpace > firstSpace
            )
            {
                gain =
                    command.substring(
                        firstSpace + 1,
                        secondSpace
                    ).toFloat();

                offset =
                    command.substring(
                        secondSpace + 1
                    ).toFloat();

                saveCalibration(
                    gain,
                    offset
                );
            }
            else
            {
                Serial.println(
                    "[ERROR] Use: SETCAL gain offset"
                );
            }
        }
    }

    vTaskDelay(
        pdMS_TO_TICKS(100)
    );
}