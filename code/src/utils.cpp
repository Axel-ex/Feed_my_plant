#include "../include/utils.hpp"
#include "PubSubClient.h"
#include "WiFi.h"
#include "WiFiType.h"
#include "esp32-hal-adc.h"
#include "esp32-hal.h"
#include "Zanshin_BME680.h"
#include <cstdio>

char* ssid = "MEO-BD8310";
const char* pswd = "9f24731014";

WiFiClient espclient;
PubSubClient client(espclient);

const int stableValue = 3155;
const int tolerance = 50;
const unsigned long requiredStableTime = 10000;
unsigned long lastStableTime = 0;
bool isStable = false;
bool is_raining = false;
bool last_is_raining = false;
extern BME680_Class BME680;

void print_debug(int32_t temp, int32_t humidity, int32_t pressure)
{
    static char buf[16];

    Serial.println("==========================");
    sprintf(buf, "Temperature: %3d.%02d°C", (int8_t)(temp / 100),
            (uint8_t)(temp % 100)); // Temp in degree celsius
    Serial.println(buf);
    sprintf(buf, "humidity: %3d.%03d\%", (int8_t)(humidity / 1000),
            (uint16_t)(humidity % 1000)); // Humidity milli-pct
    Serial.println(buf);
    sprintf(buf, "pressure: %7d.%02dhPa", (int16_t)(pressure / 100),
            (uint8_t)(pressure % 100)); // Pressure Pascals
    Serial.println(buf);
    Serial.println("==========================\n");
}

void init_wifi(void)
{
    Serial.println("Connecting to wifi");
    WiFi.begin(ssid, pswd);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
}

void callback(char* topic, byte* payload, unsigned int length)
{
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++)
        Serial.print((char)payload[i]);
    Serial.println();

    if (!strcmp(topic, "feed_my_plant/relay"))
    {
        if ((char)payload[0] == '1')
            digitalWrite(RELAY_PIN, LOW);
        else
            digitalWrite(RELAY_PIN, HIGH);
    }
    else if (!strcmp(topic, "feed_my_plant/new_measure"))
    {
        static int32_t temp, humidity, pressure, gas;

        BME680.getSensorData(temp, humidity, pressure, gas);
        client.publish("feed_my_plant/bme_readings/temp", String(temp / 100).c_str());
        client.publish("feed_my_plant/bme_readings/hum",
                       String(humidity / 1000).c_str());
        client.publish("feed_my_plant/bme_readings/press",
                       String(pressure / 100).c_str());
    }
}

void check_rain_sensor(void)
{
    int reading = analogReadMilliVolts(RAIN_SENSOR_PIN);

    if (abs(reading - stableValue) <= tolerance)
    {
        if (!isStable)
        {
            lastStableTime = millis(); // Reset the stable timer
            isStable = true;
        }
        else if (millis() - lastStableTime >= requiredStableTime)
            is_raining = true;
    }
    else
    {
        is_raining = false;
        isStable = false;
    }

    if (is_raining != last_is_raining)
    {
        if (is_raining)
            client.publish("feed_my_plant/rain", "1");
        else
            client.publish("feed_my_plant/rain", "0");
        last_is_raining = is_raining;
    }
}

void reconnect()
{
    static int connecting_tries;

	if (WiFi.status() != WL_CONNECTED)
		init_wifi();

    while (!client.connected())
    {
        Serial.print("Attempting MQTT connection...");
        String clientId = "ESP8266Client-";
        clientId += String(random(0xffff), HEX);
        if (client.connect(clientId.c_str()))
        {
            Serial.println("connected");
            client.publish("feed_my_plant/mqtt_status", "Connected");
            client.subscribe("feed_my_plant/relay");
            client.subscribe("feed_my_plant/new_measure");
        }
        else
        {
            connecting_tries++;
            if (connecting_tries >= 5)
                client.publish("feed_my_plant/mqtt_status", "Not connected");
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}
