#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

/* State machine */
typedef enum {
    time_selection = 0,
    curing
} curing_state_t;

/**
 * @brief State machine getter
 * @return State machine state
 */
curing_state_t sm_get_state();

/**
 * @brief State machine setter
 * @param new_state New state.
 */
void sm_set_state(curing_state_t new_state);

/**
 * @brief Initialize the ios, mutexes and variables needed to operate properly
 */
void sm_init();

#endif //   STATE_MACHINE_H