#pragma once

#include "cJSON.h"
#include "esp_http_client.h"

typedef enum DEVICE_ERR {
    DEVICE_ERR_NOT_BOUND,
    DEVICE_ERR_DISABLED,
    DEVICE_ERR_ALREADY_IN_WANTED_STATE,
    DEVICE_ERR_ALREADY_BOUND,
    DEVICE_ERR_NO_VACANT_PINS,
    DEVICE_ERR_NONE
} DEVICE_ERR;

typedef enum DEVICE_STATUS {
    DEVICE_STATUS_ON,
    DEVICE_STATUS_OFF,
    DEVICE_STATUS_DISABLED,
} DEVICE_STATUS;


typedef struct Config {
    char* devices[10];
    DEVICE_STATUS device_status[10];
    int valid_pins[10];
    int max_devices;
    int pins_used;
} Config;

void config_init(Config*);
void config_delete(Config*);
// main
char *parse_and_execute_commands(Config *config, char *command);
// features
DEVICE_ERR register_device(Config *config, char *name);
DEVICE_ERR delete_device(Config *config, char *name);
DEVICE_ERR switch_on(Config*, char *device);
DEVICE_ERR switch_off(Config*, char *device);
DEVICE_ERR disable_device(Config*, char *device);
int get_pin_idx(Config*config, char *device);
char *list_devices(Config *config);
char* list_commands(Config* config);
// utils
int get_next_word(const char *, int,char*, int);