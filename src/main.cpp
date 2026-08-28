#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

Adafruit_MPU6050 mpu;

void setup()
{
    Serial.begin(115200);

    delay(1000);

    Serial.println();
    Serial.println("=================================");
    Serial.println(" IoT Assignment");
    Serial.println(" Phase 1 - I2C Sensor");
    Serial.println("=================================");

    // Initialize I2C
    Wire.begin(21, 22);

    Serial.println("[I2C] Initializing MPU6050...");

    if (!mpu.begin())
    {
        Serial.println("[ERROR] MPU6050 not detected!");
        return;
    }

    Serial.println("[I2C] MPU6050 detected successfully");

    // Configure sensor
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
}

void loop()
{
    sensors_event_t acceleration;
    sensors_event_t gyro;
    sensors_event_t temperature;

    mpu.getEvent(&acceleration, &gyro, &temperature);

    Serial.println();
    Serial.println("[I2C SENSOR] MPU6050");

    Serial.print("Acceleration X: ");
    Serial.print(acceleration.acceleration.x);
    Serial.println(" m/s^2");

    Serial.print("Acceleration Y: ");
    Serial.print(acceleration.acceleration.y);
    Serial.println(" m/s^2");

    Serial.print("Acceleration Z: ");
    Serial.print(acceleration.acceleration.z);
    Serial.println(" m/s^2");

    Serial.print("Temperature: ");
    Serial.print(temperature.temperature);
    Serial.println(" C");

    delay(10000);
}