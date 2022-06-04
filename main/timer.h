#pragma once

#include <stdio.h>
#include <driver/timer.h>
#include <driver/gpio.h>

typedef struct timers_info_t timers_info_t;

#include "app.h"

#define MAX_TIMERS 20 // MAX number of timers
#define TIMER_DIVIDER   (80)               /*timer clock divider, 80 to get 1MHz clock to timer */
#define TIMER_SCALE    (TIMER_BASE_CLK / TIMER_DIVIDER)  /*!< used to calculate counter value */

typedef enum {
    TIMER_ACTIVE,
    TIMER_INACTIVE
} timer_status;

typedef struct timer_event_info_t {
    int pin_idx;
    int pin_level;
    int fire_time;
    timer_status status;
} timer_event_info_t;

struct timers_info_t {
    timer_event_info_t timers_arr[MAX_TIMERS];
    int num_timers;
};

bool IRAM_ATTR timer_group_isr_callback(void *args); // isr for timer
void init_timer(Config *config, int group, int timer, bool auto_reload, int timer_interval_sec); // function to configure and start timer
void init_clock(Config *config); // init clock
