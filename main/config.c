#include <stdio.h>
#include "estr.h"
#include "config.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "timer.h"

extern const uint8_t certificate_pem_start[] asm("_binary_certificate_pem_start");
extern const uint8_t certificate_pem_end[]   asm("_binary_certificate_pem_end");

void config_init(Config *config) {
    int valid_pins[MAX_DEVICES] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    for (int i = 0; i < MAX_DEVICES; i++) {
        config->valid_pins[i] = valid_pins[i];
        config->devices[i].name[0] = '\0';
        config->devices[i].status = DEVICE_STATUS_NOT_BOUND;
        config->devices[i].pin = -1;
    }
}

char *parse_and_execute_commands(Config *config, char *command) {
    if (command[0] != '!') return NULL;

    char buffer[100];
    char *response = NULL;
    char pinString[10];

    if (estr_sw(command, "!bind ")) {
        get_next_word(command, 6, buffer, 100);

        DEVICE_ERR status = register_device(config, buffer);

        if (status == DEVICE_ERR_NO_VACANT_PINS) {
            response = estr_cat("All Pins Used. Kindly unbind a pin before trying again");
        } else if (status == DEVICE_ERR_NONE || status == DEVICE_ERR_ALREADY_BOUND) {
            itoa(config->devices[get_device_idx(config, buffer)].pin, pinString, 10);

            if (status == DEVICE_ERR_NONE) response = estr_cat("Bound `", buffer, "` to Pin ", pinString);
            else if (status == DEVICE_ERR_ALREADY_BOUND) response = estr_cat("`", buffer, "` already bound to Pin `", pinString);
        }
        
    } else if (estr_sw(command, "!unbind ")) {
        get_next_word(command, 8, buffer, 100);
        DEVICE_ERR status = delete_device(config, buffer);

        if (status == DEVICE_ERR_NOT_BOUND) response = estr_cat("`", buffer, "`is not bound to any pin!");
        else if (status == DEVICE_ERR_NONE)  {
            itoa(config->devices[get_device_idx(config, buffer)].pin, pinString, 10);
            response = estr_cat("Unbound `", buffer, "` from Pin ", pinString);
        }

    } else if (estr_sw(command, "!switch_on ")) {
        get_next_word(command, 11, buffer, 100);
        DEVICE_ERR status = switch_on(config, buffer);

        if (status == DEVICE_ERR_NONE) {
            response = estr_cat("`", buffer, "` switched on.");
        } else if (status == DEVICE_ERR_DISABLED) {
            response = estr_cat("`", buffer, "` is disabled! Kindly enable before trying again.");
        } else if (status == DEVICE_ERR_ALREADY_IN_WANTED_STATE) {
            response = estr_cat("`", buffer, "` is already on.");
        } else if (status == DEVICE_ERR_NOT_BOUND) {
            response = estr_cat("`", buffer, "` not bound");
        }

    } else if (estr_sw(command, "!switch_off ")) {
        get_next_word(command, 12, buffer, 100);
        DEVICE_ERR status = switch_off(config, buffer);

        if (status == DEVICE_ERR_NONE) {
            response = estr_cat("`", buffer, "` switched off.");
        } else if (status == DEVICE_ERR_DISABLED) {
            response = estr_cat("`", buffer, "` is disabled! Kindly enable before trying again.");
        } else if (status == DEVICE_ERR_ALREADY_IN_WANTED_STATE) {
            response = estr_cat("`", buffer, "` is already off.");
        } else if (status == DEVICE_ERR_NOT_BOUND) {
            response = estr_cat("`", buffer, "` not bound");
        }
        
    } else if (estr_sw(command, "!disable ")) {
        get_next_word(command, 9, buffer, 100);
        DEVICE_ERR status = disable_device(config, buffer);

        if (status == DEVICE_ERR_NONE) {
            response = estr_cat("`", buffer, "` disabled.");
        } else if (status == DEVICE_ERR_ALREADY_IN_WANTED_STATE) {
            response = estr_cat("`", buffer, "` is already disabled.");
        } else if (status == DEVICE_ERR_NOT_BOUND) {
            response = estr_cat("`", buffer, "` not bound");
        }

    } else if (estr_sw(command, "!list_devices")) {
        char *res = list_devices(config);

        if (res) return res;
        return estr_cat("Err");
    } else if (estr_sw(command, "!list_commands")) {
        return list_commands(config);
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

int get_device_idx(Config *config, char *name) {
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (estr_eq(config->devices[i].name, name)) return i;
    }
    // return err_code: -1 (not found)
    return -1; 
}

DEVICE_ERR register_device(Config *config, char *name) {
    int index = 0;
    // if no pins are vacant, return err_code: -1
    if (config->pins_used == MAX_DEVICES) {
        return DEVICE_ERR_NO_VACANT_PINS;
    }
    // If device Already Exists, return err_code: -2 indicating that device already exists
    if (get_device_idx(config, name) != -1) return DEVICE_ERR_ALREADY_BOUND;
    // If device Does not Exist find a vacant pin and create device
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (config->devices[i].name[0] == '\0') {
            while (name[index] != '\0' && index < DEVICE_NAME_LENGTH - 1)
            {
                config->devices[i].name[index] = name[index++];
                config->devices[i].status = DEVICE_STATUS_OFF;
                config->devices[i].pin = config->valid_pins[i];
            }
            
            config->pins_used++;
            return DEVICE_ERR_NONE;
        }
    }
    // if no pins are vacant, return err_code: -1
    return DEVICE_ERR_NO_VACANT_PINS;
}

DEVICE_ERR delete_device(Config *config, char *name) {
    if (config->pins_used == 0) return DEVICE_ERR_NOT_BOUND;

    // find the index of the device and remove device
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (estr_eq(config->devices[i].name, name)) {
            config->devices[i].status = DEVICE_STATUS_NOT_BOUND;
            config->devices[i].pin = -1;
            config->devices[i].name[0] = '\0';
            config->pins_used--;

            return DEVICE_ERR_NONE;
        }
    }

    // if device does not exist, return err_code: -1
    return DEVICE_ERR_NOT_BOUND;
}

DEVICE_ERR switch_on(Config* config, char *device) {
    int device_index = get_device_idx(config, device);

    // return err_code: -3 (no device bound)
    if (device_index == -1) return DEVICE_ERR_NOT_BOUND;
    // if device is bound and device status is disabled return err_code: -1 // (disabled on)
    if (config->devices[device_index].status == DEVICE_STATUS_DISABLED) return DEVICE_ERR_DISABLED;

    // if device is bound and device is on, return err_code: -2
    if (config->devices[device_index].status == DEVICE_STATUS_ON) return DEVICE_ERR_ALREADY_IN_WANTED_STATE; // (already on)
    
    // update status
    config->devices[device_index].status = DEVICE_STATUS_ON;
    // turn device on

    return DEVICE_ERR_NONE; // (turned on)
}

DEVICE_ERR switch_off(Config* config, char *device) {
    int device_index = get_device_idx(config, device);

    // return err_code: -3 (no device bound)
    if (device_index == -1) return DEVICE_ERR_NOT_BOUND;
    // if device is bound and device status is disabled return err_code: -1 (disabled)
    if (config->devices[device_index].status == DEVICE_STATUS_DISABLED) return DEVICE_ERR_DISABLED;

    // if device is bound and device is on, return err_code: -2 (already off)
    if (config->devices[device_index].status == DEVICE_STATUS_OFF) return DEVICE_ERR_ALREADY_IN_WANTED_STATE;
    
    // update status
    config->devices[device_index].status = DEVICE_STATUS_OFF;
    // turn device off

    return DEVICE_ERR_NONE;
} 

DEVICE_ERR disable_device(Config *config, char *name) {
    int device_index = get_device_idx(config, name);

    // return err_code: -3 (no device bound)
    if (device_index == -1) return DEVICE_ERR_NOT_BOUND;
    // if device is bound and device status is disabled return err_code: -2 (already disabled)
    if (config->devices[device_index].status == DEVICE_STATUS_DISABLED) return DEVICE_ERR_ALREADY_IN_WANTED_STATE;

    // update status
    config->devices[device_index].status = DEVICE_STATUS_DISABLED;

    return DEVICE_ERR_NONE;
}

DEVICE_ERR switch_off_after_interval(Config *config, char *device, int interval_sec) { // TODO Add functionality
    return DEVICE_ERR_ALREADY_BOUND;
}

DEVICE_ERR switch_off_after_interval(Config *config, char *device, int interval) { // TODO Add functionality
    return DEVICE_ERR_ALREADY_BOUND;
}

char* list_devices(Config* config) {
    char pinString[10];
    char *response = NULL;

    if (config->pins_used == 0) return estr_cat("No Devices bound");

    // loop over each device and append to string
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (config->devices[i].status != DEVICE_STATUS_NOT_BOUND) {
            itoa(config->valid_pins[i], pinString, 10);
            char *line = estr_cat("`" ,config->devices[i].name, "` bound to Pin ", pinString, "\n");
            char *temp = response; // get pointer to response string
            if (response) response = estr_cat(response, line); // update response
            else response = line;
            if (temp) free(temp); // if temp exists, free temp
        }
    }

    return response;
}

char* list_commands(Config* config) {
    return NULL;
}
