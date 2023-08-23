#include <stdlib.h>
#include <stdio.h>

/* DISCLAIMER:
 * the values set in this file are completely arbitrary and 
 * based on São Paulo's average data on the day it was made
 * and it's intended only for having a value sent to mqqt
 */

// random number generator
// https://learn.microsoft.com/pt-br/cpp/c-runtime-library/reference/rand?view=msvc-170#example
int rand_range(int min, int max) {
	return ((float)rand() / RAND_MAX) * (max - min) + min;
}

// temperature in celcius between 25C and 30C
int get_temperature(void) {
	return rand_range(25, 30);
}

// pressure in hPa between 1000hPa and 1020hPa
int get_pressure(void) {
	return rand_range(1000, 1020);
}

// humidity in % between 70% and 75%
int get_humidity(void) {
	return rand_range(70, 75);
}

// precipitation in mm between 8mm and 12mm
int get_precipitation(void) {
	return rand_range(8, 12);
}

// wind speed in km/h between 4km/h and 8km/h
int get_wind_speed(void) {
	return rand_range(4, 8);
}

// luminosity value between 80 to 100
int get_luminosity(void) {
	return rand_range(80, 100);
}

// ground humidity value in % between 80% and 90%
int get_ground_humidity(void) {
	return rand_range(80, 90);
}

char* create_sensor_data_string(void) {
	char* msg;

	int str_result = asprintf(&msg,
		"DATA_TO_MQTT;t|%d|ah|%d|pc|%d|w|%d|p|%d|l|%d|gh|%d",
		get_temperature(), get_humidity(), get_precipitation(), get_wind_speed(), get_pressure(),
		get_luminosity(), get_ground_humidity());
	
	if (str_result > 0) return msg;
	return "";
}

