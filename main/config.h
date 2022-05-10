#pragma once

struct Config {
    char* appliances[10];
    int appliance_status[10];
    int validPins[10];
    int maxAppliances;
    int pinsUsed;
};

typedef struct Config Config;

void config_init(struct Config*);
void config_delete(struct Config*);
// main
char *parse_and_execute_commands(struct Config *config, char *command);
// features
int register_appliance(struct Config *config, char *name);
int delete_appliance(struct Config *config, char *name);
int switch_on(struct Config*, char *appliance);
int switch_off(struct Config*, char *appliance);
int get_pin_idx(struct Config*config, char *appliance);
char *list_devices(struct Config *config);
char* list_commands(struct Config* config);
// utils
int get_next_word(const char *, int,char*, int);