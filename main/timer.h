#pragma once

#include <stdio.h>
#include <driver/timer.h>
#include <driver/gpio.h>

typedef struct timers_info_t timers_info_t;

#include "config.h"

#define TIMER_INTR_SEL TIMER_INTR_LEVEL  /*!< Timer level interrupt */
#define TIMER_DIVIDER   (16)               /*!< Hardware timer clock divider, 80 to get 1MHz clock to timer */
#define TIMER_SCALE    (TIMER_BASE_CLK / TIMER_DIVIDER)  /*!< used to calculate counter value */

typedef struct timer_event_info_t {
    int pin_idx;
    int pin_level;
    int fire_time;
} timer_event_info_t;

struct timers_info_t {
    timer_event_info_t **timers_arr;
    int num_timers;
};

bool IRAM_ATTR timer_group_isr_callback(void *args);
void init_timer(Config *config, int group, int timer, bool auto_reload, int timer_interval_sec);
void init_clock(Config *config);
void add_timer(Config *, int device_index, int level, int fire_delay);
void remove_timer(Config *, int device_index, int level, int fire_delay);