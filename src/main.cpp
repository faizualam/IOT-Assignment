#include <Arduino.h>

void setup()
{
    Serial.begin(115200);

    delay(1000);

    Serial.println("================================");
    Serial.println("IoT Assignment");
    Serial.println("ESP32 starting...");
    Serial.println("================================");
}

void loop()
{
    Serial.println("System running...");
    delay(2000);
}