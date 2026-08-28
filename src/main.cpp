#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

Adafruit_MPU6050 mpu;

// -----------------------------
// Pin configuration
// -----------------------------

#define I2C_SDA 21
#define I2C_SCL 22

#define UART_RX 16
#define UART_TX 17

// -----------------------------
// Sensor states
// -----------------------------

enum SensorStatus
{
    SENSOR_OK,
    SENSOR_NO_RESPONSE
};

// -----------------------------
// Reading structure
// -----------------------------

struct SensorReading
{
    char sensor[16];
    float value;
    bool valid;
    SensorStatus status;
    unsigned long timestamp;
};

// -----------------------------
// Queue
// -----------------------------

QueueHandle_t sensorQueue;

// -----------------------------
// I2C SENSOR TASK
// -----------------------------

void i2cSensorTask(void *parameter)
{
    while (true)
    {
        SensorReading reading;

        strcpy(reading.sensor, "MPU6050");

        sensors_event_t acceleration;
        sensors_event_t gyro;
        sensors_event_t temperature;

        mpu.getEvent(
            &acceleration,
            &gyro,
            &temperature
        );

        reading.value = temperature.temperature;
        reading.valid = true;
        reading.status = SENSOR_OK;
        reading.timestamp = millis();

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

// -----------------------------
// UART SENSOR TASK
// -----------------------------

void uartSensorTask(void *parameter)
{
    float simulatedValue = 25.0;

    while (true)
    {
        SensorReading reading;

        strcpy(reading.sensor, "UART_SENSOR");

        simulatedValue += 0.1;

        reading.value = simulatedValue;
        reading.valid = true;
        reading.status = SENSOR_OK;
        reading.timestamp = millis();

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

// -----------------------------
// PROCESSING TASK
// -----------------------------

void processingTask(void *parameter)
{
    SensorReading reading;

    while (true)
    {
        if (xQueueReceive(
                sensorQueue,
                &reading,
                portMAX_DELAY))
        {
            Serial.print("[DATA] ");

            Serial.print(reading.sensor);

            Serial.print(" | Value: ");

            Serial.print(reading.value);

            Serial.print(" | Valid: ");

            Serial.print(reading.valid ? "YES" : "NO");

            Serial.print(" | Time: ");

            Serial.println(reading.timestamp);
        }
    }
}

// -----------------------------
// SETUP
// -----------------------------

void setup()
{
    Serial.begin(115200);

    delay(1000);

    Serial.println();
    Serial.println("=================================");
    Serial.println(" IoT Monitoring Node");
    Serial.println(" Phase 2 - FreeRTOS");
    Serial.println("=================================");

    // I2C
    Wire.begin(
        I2C_SDA,
        I2C_SCL
    );

    Serial.println("[I2C] Initializing MPU6050...");

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

    // Queue
    sensorQueue = xQueueCreate(
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
        "[QUEUE] Sensor queue created"
    );

    // Create I2C task
    xTaskCreate(
        i2cSensorTask,
        "I2C Sensor",
        4096,
        NULL,
        2,
        NULL
    );

    // Create UART task
    xTaskCreate(
        uartSensorTask,
        "UART Sensor",
        4096,
        NULL,
        2,
        NULL
    );

    // Create processing task
    xTaskCreate(
        processingTask,
        "Processing",
        4096,
        NULL,
        1,
        NULL
    );

    Serial.println(
        "[SYSTEM] FreeRTOS tasks started"
    );
}

// -----------------------------
// MAIN LOOP
// -----------------------------

void loop()
{
    // Main loop remains responsive.
    // No blocking sensor operations here.

    vTaskDelay(
        pdMS_TO_TICKS(1000)
    );
}