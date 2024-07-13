#include "../include/utils.hpp"
#include "Arduino.h"
#include "PubSubClient.h"
#include "WiFi.h"
#include "Zanshin_BME680.h"
#include "esp32-hal-gpio.h"
#include <cstring>
#include <stdlib.h>

#define MEASUREMENT_DELAY 30000

const char* mqtt_url = "192.168.1.71";
unsigned long last_messag = 0;

BME680_Class BME680;
extern WiFiClient espclient;
extern PubSubClient client;
extern int rain_sensor_status;

void setup()
{
    Serial.begin(115200);

    // Init relay
    pinMode(RELAY_PIN, OUTPUT);
	digitalWrite(RELAY_PIN, HIGH);

	//INit rain sensor 
	pinMode(RAIN_SENSOR_PIN, INPUT_PULLDOWN);

    // // Initialize BME680 sensor
    while (!BME680.begin(I2C_STANDARD_MODE))
    {
        Serial.println("Unable to find BME680");
        delay(2000);
    }
    BME680.setOversampling(TemperatureSensor, Oversample16);
    BME680.setOversampling(HumiditySensor, Oversample16);
    BME680.setOversampling(PressureSensor, Oversample16);

    // Init wifi
    init_wifi();

    // init mqtt
    client.setServer(mqtt_url, MQTT_PORT);
    client.setCallback(callback);
}

void loop()
{
    static int32_t temp, humidity, pressure, gas;

    BME680.getSensorData(temp, humidity, pressure, gas);
    reconnect();
    client.loop();
	check_rain_sensor();


    unsigned long now = millis();
    if (now - last_messag > MEASUREMENT_DELAY)
    {
        last_messag = now;
        client.publish("feed_my_plant/bme_readings/temp", String(temp / 100).c_str());
        client.publish("feed_my_plant/bme_readings/hum",
                       String(humidity / 1000).c_str());
        client.publish("feed_my_plant/bme_readings/press",
                       String(pressure / 100).c_str());
    }
}
