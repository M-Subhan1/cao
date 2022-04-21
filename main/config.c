
#include<stdio.h>
#include "estr.h"
#include "config.h"

#define NUM_APPLIANCES 10

void config_init(struct Config *config) {

    for (int i = 0; i < NUM_APPLIANCES; i++) config->appliances[i] = NULL;
    config->pinsUsed = 0;
}

void config_delete(struct Config *config) {
    for (int i = 0; i < NUM_APPLIANCES; i++) if (config->appliances[i]) free(config->appliances[i]);
}

void register_appliance(struct Config *config, char *name) {
    for (int i = 0; i < NUM_APPLIANCES; i++) {
        if (!config->appliances[i]) {
            config->appliances[i] = estr_cat(name);
            config->pinsUsed++;
            return;
        }
    }
}

void delete_appliance(struct Config *config, char *name) {
    for (int i = 0; i < NUM_APPLIANCES; i++) {
        if (config->appliances[i] && estr_eq(config->appliances[i], name)) {
            free(config->appliances[i]);
            config->appliances[i] = NULL;
            config->pinsUsed--;
        }
    }
}

void parse_and_execute_commands(struct Config *config, char *command) {

}

