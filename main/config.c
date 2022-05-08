
#include<stdio.h>
#include "estr.h"
#include "config.h"

void config_init(struct Config *config) {
    int validPins[10] = {1, 2, 3, 4, 5, 6};
    config->maxAppliances = 10; 

    for (int i = 0; i < config->maxAppliances; i++) {
        config->validPins[i] = validPins[i];
    }

    for (int i = 0; i < config->maxAppliances; i++) {
        config->appliances[i] = NULL;
    }
}

void config_delete(struct Config *config) {
    for (int i = 0; i < config->maxAppliances; i++) if (config->appliances[i]) free(config->appliances[i]);
}

int register_appliance(struct Config *config, char *name) {
    // if no pins are vacant, return err_code: -1
    if (config->pinsUsed == config->maxAppliances) return -1;
    // If Appliance Already Exists, return err_code: -2 indicating that appliance already exists
    for (int i = 0; i < config->maxAppliances; i++) {
        if (estr_eq(config->appliances[i], name)) return -2;
    }

    // If Appliance Already Does not Exist find a vacant pin and create appliance
    for (int i = 0; i < config->maxAppliances; i++) {
        int pin = config->validPins[i];

        if (!config->appliances[i]) {
            config->appliances[i] = estr_cat(name);
            config->pinsUsed++;
            return pin;
        }
    }

    // if no pins are vacant, return err_code: -1
    return -1;
}

int delete_appliance(struct Config *config, char *name) {
    if (config->pinsUsed == 0) return -1;

    // find the index of the appliance and remove appliance
    for (int i = 0; i < config->maxAppliances; i++) {
        if (config->appliances[i] && estr_eq(config->appliances[i], name)) {
            free(config->appliances[i]);
            config->appliances[i] = NULL;
            config->pinsUsed--;

            return config->validPins[i];
        }
    }

    // if appliance does not exist, return err_code: -1
    return -1;
}

char *parse_and_execute_commands(struct Config *config, char *command) {
    if (command[0] != '!') return NULL;

    char buffer[100];
    char *response = NULL;
    char pinString[10];

    if (estr_sw(command, "!bind ")) {
        getNextWord(command, 6, buffer, 100);
        int pin = register_appliance(config, buffer);
        itoa(pin,pinString,10); 

        if (pin > 0) response = estr_cat("Bound `", buffer, "` to Pin ", pinString);
        else if (pin == -1) response = estr_cat("All Pins Used. Kindly unbind a pin before trying again");
        else if (pin == -2) response = estr_cat("`", buffer, "` already bound to Pin `", pinString);
    } else if (estr_sw(command, "!unbind ")) {
        getNextWord(command, 8, buffer, 100);
        int pin = delete_appliance(config, buffer);
        itoa(pin,pinString,10); 

        if (pin > 0)  response = estr_cat("Unbound `", buffer, "` from Pin ", pinString);
        else if (pin == -1) response = estr_cat("`", buffer, "`is not bound to any pin!");
    }

    return response;
}

int getNextWord(const char *command, int startIndex, char *buffer, int bufferSize) {
    int index = 0;
    char letter = command[startIndex];

    while (letter != '\0' && letter != '\n' && letter != '\t' && letter != ' ' && index < bufferSize - 2) {
        buffer[index++] = letter;
        letter = command[index + startIndex];
    }
    
    buffer[index] = '\0';
    return index;
}