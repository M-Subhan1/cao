#pragma once

#include <stdio.h>
#include "driver/timer.h"
#include <driver/gpio.h>

#define TIMER_INTR_SEL TIMER_INTR_LEVEL  /*!< Timer level interrupt */
#define TIMER_DIVIDER   (16)               /*!< Hardware timer clock divider, 80 to get 1MHz clock to timer */
#define TIMER_SCALE    (TIMER_BASE_CLK / TIMER_DIVIDER)  /*!< used to calculate counter value */

typedef struct {
    int pin_idx;
    int pin_level;
} timer_event_info_t;

bool IRAM_ATTR timer_group_isr_callback(void *args);
void init_timer(int group, int timer, bool auto_reload, int timer_interval_sec, int pin_number, int pin_level);