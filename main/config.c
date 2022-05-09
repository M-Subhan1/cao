
#include<stdio.h>
#include "estr.h"
#include "config.h"

void config_init(struct Config *config) {
    int validPins[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    config->maxAppliances = 10;

    for (int i = 0; i < config->maxAppliances; i++) {
        config->validPins[i] = validPins[i];
        config->appliances[i] = NULL;
        config->appliance_status[i] = 0;
    }
}

void config_delete(struct Config *config) {
    for (int i = 0; i < config->maxAppliances; i++) if (config->appliances[i]) free(config->appliances[i]);
}

int register_appliance(struct Config *config, char *name) {
    // if no pins are vacant, return err_code: -1
    if (config->pinsUsed == config->maxAppliances) return -1;
    // If Appliance Already Exists, return err_code: -2 indicating that appliance already exists
    if (get_pin_idx(config, name) != -1) return -2;
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
        get_next_word(command, 6, buffer, 100);
        int pin = register_appliance(config, buffer);
        itoa(pin,pinString,10); 

        if (pin > 0) response = estr_cat("Bound `", buffer, "` to Pin ", pinString);
        else if (pin == -1) response = estr_cat("All Pins Used. Kindly unbind a pin before trying again");
        else if (pin == -2) response = estr_cat("`", buffer, "` already bound to Pin `", pinString);

    } else if (estr_sw(command, "!unbind ")) {
        get_next_word(command, 8, buffer, 100);
        int pin = delete_appliance(config, buffer);
        itoa(pin,pinString,10); 

        if (pin > 0)  response = estr_cat("Unbound `", buffer, "` from Pin ", pinString);
        else if (pin == -1) response = estr_cat("`", buffer, "`is not bound to any pin!");

    } else if (estr_sw(command, "!turn_on ")) {
        get_next_word(command, 9, buffer, 100);
        int res_code = switch_on(config, buffer);

        if (res_code == 1) {
            response = estr_cat("`", buffer, "` switched on.");
        } else if (res_code == -1) {
            response = estr_cat("`", buffer, "` is disabled! Kindly enable before trying again.");
        } else if (res_code == -2) {
            response = estr_cat("`", buffer, "` is already on.");
        } else if (res_code == -3) {
            response = estr_cat("`", buffer, "` not bound");
        }

    } else if (estr_sw(command, "!turn_off ")) {
        get_next_word(command, 10, buffer, 100);
        int res_code = switch_off(config, buffer);

        if (res_code == 1) {
            response = estr_cat("`", buffer, "` switched off.");
        } else if (res_code == -1) {
            response = estr_cat("`", buffer, "` is disabled! Kindly enable before trying again.");
        } else if (res_code == -2) {
            response = estr_cat("`", buffer, "` is already off.");
        } else if (res_code == -3) {
            response = estr_cat("`", buffer, "` not bound");
        }
        
    }

    return response;
}

int get_next_word(const char *command, int startIndex, char *buffer, int buffer_size) {
    int index = 0;
    char letter = command[startIndex];

    while (letter != '\0' && letter != '\n' && letter != '\t' && letter != ' ' && index < buffer_size - 2) {
        buffer[index++] = letter;
        letter = command[index + startIndex];
    }
    
    buffer[index] = '\0';
    return index;
}

int switch_on(struct Config* config, char *appliance) {
    int pin_index = get_pin_idx(config, appliance);

    // return err_code: -3 (no appliance bound)
    if (pin_index == -1) return -3;
    // if appliance is bound and appliance status is disabled return err_code: -1 // (disabled on)
    if (pin_index != -1 && config->appliance_status[pin_index] == -1) return -1;

    // if appliance is bound and appliance is on, return err_code: -2
    if (pin_index != -1 && config->appliance_status[pin_index] == 1) return -2; // (already on)
    
    // update status
    config->appliance_status[pin_index] = 1;
    // turn appliance on

    return 1; // (turned on)
}

int switch_on(struct Config* config, char *appliance) {
    int pin_index = get_pin_idx(config, appliance);

    // return err_code: -3 (no appliance bound)
    if (pin_index == -1) return -3;
    // if appliance is bound and appliance status is disabled return err_code: -1 (disabled)
    if (pin_index != -1 && config->appliance_status[pin_index] == -1) return -1;

    // if appliance is bound and appliance is on, return err_code: -2 (already ff)
    if (pin_index != -1 && config->appliance_status[pin_index] == 0) return -2;
    
    // update status
    config->appliance_status[pin_index] = 0;
    // turn appliance ff

    return 1; // (turned off)
} 

int get_pin_idx(struct Config *config, char *name) {
    for (int i = 0; i < config->maxAppliances; i++) {
        if (estr_eq(config->appliances[i], name)) return i;
    }
    // return err_code: -1 (not found)
    return -1; 
}