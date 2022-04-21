#pragma once

struct Config {
    int pinsUsed;
    char* appliances[10];
};

typedef struct Config Config;

void config_init(struct Config*);
void config_delete(struct Config*);
void register_appliance(struct Config *config, char *name);
void delete_appliance(struct Config *config, char *name);
void parse_and_execute_commands(struct Config *config, char *command);