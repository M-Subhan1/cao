#include "timer.h"

bool IRAM_ATTR timer_group_isr_callback(void *args) {
    Config *config = (Config *) args;
    
    gpio_set_level(2, !gpio_get_level(2));
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

void add_timer(Config *config, int device_index, int pin_level, int fire_delay) {
    int num_timers = config->timers->num_timers;
    uint64_t alarm_value = 0;

    timer_get_alarm_value(TIMER_GROUP_1, TIMER_1, &alarm_value);

    config->timers->timers_arr = realloc(config->timers->timers_arr, (config->timers->num_timers + 1) * sizeof(timer_event_info_t));
    config->timers->timers_arr[num_timers]->fire_time = alarm_value + fire_delay;
    config->timers->timers_arr[num_timers]->pin_level = pin_level;
    config->timers->timers_arr[num_timers]->pin_idx = config->devices[device_index].pin;
    config->timers->num_timers++;
}

void reset_timers(Config *config, int device_index) {
    int num_timers = config->timers->num_timers;
    int pin_index = config->timers->timers_arr[device_index]->pin_idx;

    for (int i = 0; i < config->timers->num_timers; i++) {
        if (config->timers->timers_arr[i]->pin_idx == pin_index) {
            free(config->timers->timers_arr[i]);
            config->timers->timers_arr[i] = NULL;
            num_timers--;
        }
    }

    timer_event_info_t **timers_arr = calloc(num_timers, sizeof (timer_event_info_t));

    for (int i = 0; i < config->timers->num_timers; i++) {
        if (config->timers->timers_arr[i]) {
            timers_arr[num_timers - 1]->fire_time = config->timers->timers_arr[i]->fire_time;
            timers_arr[num_timers - 1]->pin_idx = config->timers->timers_arr[i]->pin_idx;
            timers_arr[num_timers - 1]->pin_level = config->timers->timers_arr[i]->pin_level;

            free(config->timers->timers_arr[i]);
            config->timers->timers_arr[i] = NULL;
            num_timers--;
        }
    }

    config->timers->timers_arr = timers_arr;
}