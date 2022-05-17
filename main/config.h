#pragma once

#define MAX_DEVICES 10
#define DEVICE_NAME_LENGTH 20

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
    DEVICE_STATUS_NOT_BOUND
} DEVICE_STATUS;

typedef struct Device {
    char name[DEVICE_NAME_LENGTH];
    int pin;
    DEVICE_STATUS status;
} Device;

typedef struct Config {
    Device devices[MAX_DEVICES];
    int pins_used;
    int valid_pins[MAX_DEVICES];
} Config;

void config_init(Config*);
// main
char *parse_and_execute_commands(Config *config, char *command);
// features
DEVICE_ERR register_device(Config *config, char *name);
DEVICE_ERR delete_device(Config *config, char *name);
DEVICE_ERR switch_on(Config*, char *device);
DEVICE_ERR switch_off(Config*, char *device);
DEVICE_ERR disable_device(Config*, char *device);
DEVICE_ERR switch_on_after_interval(Config*, char *device, int interval); // TODO Add functionality
DEVICE_ERR switch_off_after_interval(Config*, char *device, int interval); // TODO Add functionality
int get_device_idx(Config*config, char *device);
char* list_devices(Config *config);
char* list_commands(Config* config);
// utils
int get_next_word(const char *, int,char*, int);

// TODO Configure Database