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

    // Aggregation variables
    float sum = 0.0;

    float minimum = 0.0;

    float maximum = 0.0;

    int validCount = 0;

    unsigned long aggregationStart =
        millis();

    while (true)
    {
        // ------------------------------------------
        // RECEIVE SENSOR READING
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
            // PRINT INDIVIDUAL READING
            // --------------------------------------

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

            // --------------------------------------
            // AGGREGATE ONLY VALID VALUES
            // --------------------------------------

            if (
                reading.valid
            )
            {
                if (
                    validCount == 0
                )
                {
                    minimum =
                        reading.value;

                    maximum =
                        reading.value;
                }
                else
                {
                    if (
                        reading.value < minimum
                    )
                    {
                        minimum =
                            reading.value;
                    }

                    if (
                        reading.value > maximum
                    )
                    {
                        maximum =
                            reading.value;
                    }
                }

                sum +=
                    reading.value;

                validCount++;
            }
        }

        // ------------------------------------------
        // CHECK 60 SECOND WINDOW
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
            // IF VALID DATA EXISTS
            // --------------------------------------

            if (
                validCount > 0
            )
            {
                float average =
                    sum / validCount;

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
                    minimum
                );

                Serial.print(
                    "Maximum: "
                );

                Serial.println(
                    maximum
                );

                Serial.print(
                    "Valid Samples: "
                );

                Serial.println(
                    validCount
                );
            }

            // --------------------------------------
            // NO VALID DATA
            // --------------------------------------

            else
            {
                Serial.println(
                    "Average: N/A"
                );

                Serial.println(
                    "Minimum: N/A"
                );

                Serial.println(
                    "Maximum: N/A"
                );

                Serial.println(
                    "Valid Samples: 0"
                );
            }

            Serial.println(
                "======================================"
            );

            // --------------------------------------
            // RESET AGGREGATION
            // --------------------------------------

            sum = 0.0;

            minimum = 0.0;

            maximum = 0.0;

            validCount = 0;

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
    // SERIAL COMMAND HANDLING
    // ------------------------------------------

    if (
        Serial.available()
    )
    {
        String command =
            Serial.readStringUntil(
                '\n'
            );

        command.trim();

        // --------------------------------------
        // SETCAL COMMAND
        // --------------------------------------

        if (
            command.startsWith(
                "SETCAL"
            )
        )
        {
            int firstSpace =
                command.indexOf(
                    ' '
                );

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
                float gain =
                    command
                        .substring(
                            firstSpace + 1,
                            secondSpace
                        )
                        .toFloat();

                float offset =
                    command
                        .substring(
                            secondSpace + 1
                        )
                        .toFloat();

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