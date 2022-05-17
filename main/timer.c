#include "timer.h"

bool IRAM_ATTR timer_group_isr_callback(void *args) {
    timer_event_info_t *info = (timer_event_info_t *) args;

    gpio_set_level(info->pin_idx, info->pin_level);
    return true;
}

void init_timer(int group, int timer, bool auto_reload, int timer_interval_sec, int pin_number, int pin_level)
{
    timer_config_t config = {
        .divider = TIMER_DIVIDER,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = auto_reload
    };

    timer_event_info_t *timer_info = calloc(1, sizeof(timer_event_info_t));
    timer_info->pin_idx = pin_number;
    timer_info->pin_level = pin_level;

    /*Configure timer*/
    ESP_ERROR_CHECK(timer_init(group, timer, &config));
        /*Load counter value */
    ESP_ERROR_CHECK(timer_set_counter_value(group, timer, 0x00000000ULL));
    /*Set alarm value*/
    ESP_ERROR_CHECK(timer_set_alarm_value(group, timer, (timer_interval_sec * TIMER_SCALE)));
    /*Enable timer interrupt*/
    ESP_ERROR_CHECK(timer_enable_intr(group, timer));
    /*Set ISR handler*/
    ESP_ERROR_CHECK(timer_isr_callback_add(group, timer, timer_group_isr_callback, timer_info, 0));
    /*Start timer counter*/
    ESP_ERROR_CHECK(timer_start(group, timer));
}