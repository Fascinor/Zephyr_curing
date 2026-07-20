#ifndef CURING_TIME_H
#define CURING_TIME_H

#define CURING_TIME_MAX_VALUE			3000
#define CURING_TIME_DEFAULT_VALUE		60

/**
 * @brief Curing time getter
 * @return curing time value
 */
uint16_t ct_get_time();

/**
 * @brief Curing time setter
 * @param new_time value to change the time to.
 */
void ct_set_time(uint16_t new_time);

/**
 * @brief Increase the curing time variable by 1~5 depending on its value
 */
void ct_increase_time();

/**
 * @brief Decrease the curing time variable by 1~5 depending on its value
 */
void ct_decrease_time();

/**
 * @brief Initialize the mutex used to secure the access to curing time variable
 */
void ct_mutex_init();

#endif //   CURING_TIME_H