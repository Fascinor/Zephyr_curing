#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

/* State machine */
typedef enum {
    time_selection = 0,
    curing
} curing_state_t;

curing_state_t sm_get_state();

void sm_set_state(curing_state_t new_state);

void sm_init();

#endif //   STATE_MACHINE_H