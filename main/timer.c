#include "timer.h"
#include <stdio.h>
#include "estr.h"
#include "esp_log.h"
#include "string.h"

bool IRAM_ATTR timer_group_isr_callback(void *args) {
    Config *config = (Config *) args;
    uint64_t alarm_value = 0;

    timer_get_alarm_value(TIMER_GROUP_1, TIMER_1, &alarm_value);

    for (int i = 0; i < config->timers->num_timers; i++) {
        if (config->timers->timers_arr[i].status == TIMER_INACTIVE) continue;

        config->timers->timers_arr[i].fire_time--;

        if (config->timers->timers_arr[i].fire_time - alarm_value != 0) continue;;

        gpio_set_level(config->timers->timers_arr[i].pin_idx, config->timers->timers_arr[i].pin_level);
    }

    return true;
}

void init_timer(Config *config, int group, int timer, bool auto_reload, int timer_interval_sec)
{
    timer_config_t timer_config = {
        .divider = TIMER_DIVIDER,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = auto_reload
    };

    /*Configure timer*/
    ESP_ERROR_CHECK(timer_init(group, timer, &timer_config));
        /*Load counter value */
    ESP_ERROR_CHECK(timer_set_counter_value(group, timer, 0x00000000ULL));
    /*Set alarm value*/
    ESP_ERROR_CHECK(timer_set_alarm_value(group, timer, (timer_interval_sec * TIMER_SCALE)));
    /*Enable timer interrupt*/
    ESP_ERROR_CHECK(timer_enable_intr(group, timer));
    /*Set ISR handler*/
    ESP_ERROR_CHECK(timer_isr_callback_add(group, timer, timer_group_isr_callback, config, 0));
    /*Start timer counter*/
    ESP_ERROR_CHECK(timer_start(group, timer));
}

void init_clock(Config *config) {
    init_timer(config, TIMER_GROUP_1, TIMER_1, true, 1);
};

