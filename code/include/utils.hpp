#ifndef INCLUDE_INCLUDE_FUNCTIONS_HPP_
#define INCLUDE_INCLUDE_FUNCTIONS_HPP_

#include "Arduino.h"

#define RELAY_PIN 26
#define MQTT_PORT 1883
#define RAIN_SENSOR_PIN 33

void callback(char* topic, byte* payload, unsigned int length);
void print_debug(int32_t temp, int32_t humidity, int32_t pressure);
void init_wifi(void);
void reconnect();
void check_rain_sensor(void);


#endif  // INCLUDE_INCLUDE_FUNCTIONS_HPP_
