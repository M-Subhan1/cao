#pragma once

typedef struct Config Config;

#include "timer.h"

#define MAX_DEVICES 1
#define DEVICE_NAME_LENGTH 20
#define NVS_VARIABLES_NAMESPACE "VARIABLES"
#define NVS_CONFIG_VARIABLE "CONFIG"

typedef enum {
    DEVICE_ERR_NOT_BOUND,
    DEVICE_ERR_DISABLED,
    DEVICE_ERR_ALREADY_IN_WANTED_STATE,
    DEVICE_ERR_ALREADY_BOUND,
    DEVICE_ERR_NO_VACANT_PINS,
    DEVICE_ERR_TIMER_MAX_LIMIT,
    DEVICE_ERR_TIMER_DOES_NOT_EXIST,
    DEVICE_OK
} device_err_t ;

typedef enum {
    DEVICE_STATUS_ON,
    DEVICE_STATUS_OFF,
    DEVICE_STATUS_DISABLED,
    DEVICE_STATUS_NOT_BOUND
} device_status_t;

typedef struct {
    char name[DEVICE_NAME_LENGTH];
    int pin;
    int status;
} device_info_t;

struct Config {
    device_info_t devices[MAX_DEVICES];
    timers_info_t *timers;
    int valid_pins[MAX_DEVICES];
    int pins_used;
};

void config_init(Config*);
// main
char *parse_and_execute_commands(Config *config, char *command);
// features
device_err_t register_device(Config *config, char *name);
device_err_t delete_device(Config *config, char *name);
device_err_t switch_on(Config*, char *device);
device_err_t switch_off(Config*, char *device);
device_err_t disable_device(Config*, char *device);
device_err_t enable_device(Config*, char *device);
device_err_t switch_on_after_interval(Config*, char *device, int interval);
device_err_t switch_off_after_interval(Config*, char *device, int interval); 
device_err_t add_timer(Config *, int device_index, int level, int fire_delay);
device_err_t clear_timers(Config *, int device_index, bool);
void reset_all_timers(Config *config);
int get_device_idx(Config*config, char *device);
char* list_devices(Config *config);
char* list_commands(Config* config);
// utils
int get_next_word(const char *, int,char*, int);
void save_config(Config *);
void load_config(Config *);