
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

char *parse_and_execute_commands(struct Config *config, char *command) {
    if (command[0] != '!') return NULL;

    char buffer[100];

    if (estr_sw(command, "!register ")) {
        getWord(command, 10, buffer, 100);
        register_appliance(config, buffer);
        char *response = estr_cat("Registered Appliance, `", buffer, "` on Pin 1");
        return response;

    } else if (estr_sw(command, "!drop ")) {
        getWord(command, 6, buffer, 100);
        delete_appliance(config, buffer);
        char *response = estr_cat("Dropped Appliance, `", buffer, "` from Pin 1");
        return response;
    }

    return NULL;
}

void getWord(const char *command, int startIndex, char *buffer, int bufferSize) {
    int index = 0;
    char letter = command[startIndex];

    while (letter != '\0' && letter != '\n' && letter != '\t' && letter != ' ' && index < bufferSize - 2) {
        buffer[index++] = letter;
        letter = command[index + startIndex];
    }
    
    buffer[index] = '\0';
    
    return buffer;
}