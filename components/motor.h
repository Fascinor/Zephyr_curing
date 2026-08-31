#ifndef MOTOR_H
#define MOTOR_H

/**
 * @brief motor power ratio getter
 * @return motor power ratio
 */
uint8_t motor_get_pwr();

/**
 * @brief Increase the motor power ratio variable by 5
 */
void motor_increase_pwr();

/**
 * @brief Decrease the motor power ratio variable by 5
 */
void motor_decrease_pwr();

/**
 * @brief create the motor control thread
 */
void motor_thread_create();

/**
 * @brief set the motor status flag to 0
 */
void motor_deactivate();

/**
 * @brief set the motor status flag to 1
 */
void motor_activate();

#endif //   MOTOR_H