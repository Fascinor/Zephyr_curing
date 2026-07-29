#ifndef THREAD_H
#define THREAD_H

#define STACK_SIZE 1024
#define THREAD_PRIORITY 5

/**
 * @brief ssd1306 thread function getter
 */
struct k_thread *thread_get_ssd1306();

/**
 * @brief curing thread function getter
 */
struct k_thread *thread_get_curing();

/**
 * @brief time increase thread function getter
 */
struct k_thread *thread_get_time_increase();

/**
 * @brief time decrease thread function getter
 */
struct k_thread *thread_get_time_decrease();

/**
 * @brief motor thread function getter
 */
struct k_thread *thread_get_motor_ctrl();

#endif //   THREAD_H