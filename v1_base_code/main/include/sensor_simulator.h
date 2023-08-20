#ifndef SENSOR_SIMULATOR_HEADER_FILE
#define SENSOR_SIMULATOR_HEADER_FILE

/**
 *  @brief Generate a random value between min and max, inclusive
*/
int rand_range(int min, int max);

/**
 *  @brief Generate a random temperature value, in celcius, between 25C and 30C
*/
int get_temperature(void);

/**
 *  @brief Generate a random pressure value, in hPa, between 1000hPa and 1020hPa
*/
int get_pressure(void);

/**
 *  @brief Generate a random humidity value, in %, between 70% and 75%
*/
int get_humidity(void);

/**
 *  @brief Generate a random precipitation value, in mm, between 8mm and 12mm
*/
int get_precipitation(void);

/**
 *  @brief Generate a random wind speed value, in km/h, between 4km/h and 8km/h
*/
int get_wind_speed(void);

/**
 *  @brief Generate a random luminosity value, between 80 and 100
*/
int get_luminosity(void);

/**
 *  @brief Generate a random ground humidity value, in %, between 80% and 90%
*/
int get_ground_humidity(void);

char* create_sensor_data_string(void);

#endif