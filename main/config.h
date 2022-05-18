#pragma once

#define MAX_DEVICES 10
#define DEVICE_NAME_LENGTH 20
#define NVS_VARIABLES_NAMESPACE "VARIABLES"
#define NVS_CONFIG_VARIABLE "CONFIG"

typedef enum device_err_t {
    DEVICE_ERR_NOT_BOUND,
    DEVICE_ERR_DISABLED,
    DEVICE_ERR_ALREADY_IN_WANTED_STATE,
    DEVICE_ERR_ALREADY_BOUND,
    DEVICE_ERR_NO_VACANT_PINS,
    DEVICE_OK
} device_err_t ;

typedef enum device_status_t {
    DEVICE_STATUS_ON,
    DEVICE_STATUS_OFF,
    DEVICE_STATUS_DISABLED,
    DEVICE_STATUS_NOT_BOUND
} device_status_t;

typedef struct Device {
    char name[DEVICE_NAME_LENGTH];
    int pin;
    int status;
} Device;

typedef struct Config {
    Device devices[MAX_DEVICES];
    int valid_pins[MAX_DEVICES];
    int pins_used;
} Config;

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
device_err_t switch_on_after_interval(Config*, char *device, int interval); // TODO Add functionality
device_err_t switch_off_after_interval(Config*, char *device, int interval); // TODO Add functionality
int get_device_idx(Config*config, char *device);
char* list_devices(Config *config);
char* list_commands(Config* config);
// utils
int get_next_word(const char *, int,char*, int);
void save_config(Config *);
void load_config(Config *);