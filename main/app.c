#include <stdio.h>
#include "estr.h"
#include "esp_log.h"
#include "string.h"
#include "nvs_flash.h"
#include "app.h"
#include "cJSON.h"

/**
 * @brief Instantiates a default config object
 * @param config Config * to load config into
 */
void config_init(Config *config) {
    int valid_pins[MAX_DEVICES] = {13,25,26,27,32}; // 4, 18, 19, 23, 33

    for (int i = 0; i < MAX_DEVICES; i++) {
        config->valid_pins[i] = valid_pins[i];
        config->devices[i].name[0] = '\0';
        config->devices[i].status = DEVICE_STATUS_NOT_BOUND;
        config->devices[i].pin = -1;
        gpio_pad_select_gpio(valid_pins[i]);
        gpio_set_direction(valid_pins[i], GPIO_MODE_OUTPUT);
    }

    config->pins_used = 0;
    config->timers = calloc(1, sizeof (timers_info_t));
    config->timers->num_timers = 0;

    for (int i = 0; i < MAX_TIMERS; i++) config->timers->timers_arr[i].status = TIMER_INACTIVE;
}

/**
 * @brief Instantiates a default config object
 * @param config Config* containing config
 * @param command char* containing message string from discord
 * @return char* response to be send back to discord
 */
char *parse_and_execute_commands(Config *config, char *command) {
    if (command[0] != '!') return NULL;

    char buffer[100];
    char *response = NULL;
    char pinString[10];

    if (estr_sw(command, "!bind ")) {
        get_next_word(command, strlen("!bind "), buffer, 100);

        device_err_t status = register_device(config, buffer);

        if (status == DEVICE_ERR_NO_VACANT_PINS) {
            response = estr_cat("All Pins Used. Kindly unbind a pin before trying again");
        } else if (status == DEVICE_OK || status == DEVICE_ERR_ALREADY_BOUND) {
            itoa(config->devices[get_device_idx(config, buffer)].pin, pinString, 10);

            if (status == DEVICE_OK) response = estr_cat("Bound `", buffer, "` to Pin ", pinString);
            else if (status == DEVICE_ERR_ALREADY_BOUND) response = estr_cat("`", buffer, "` already bound to Pin `", pinString);
        }
        
    } else if (estr_sw(command, "!unbind ")) {
        get_next_word(command, strlen("!unbind "), buffer, 100);
        device_err_t status = delete_device(config, buffer);

        if (status == DEVICE_ERR_NOT_BOUND) response = estr_cat("`", buffer, "`is not bound to any pin!");
        else if (status == DEVICE_OK)  {
            response = estr_cat("Unbound `", buffer);
        }

    } else if (estr_sw(command, "!on ")) {
        get_next_word(command, strlen("!on "), buffer, 100);
        device_err_t status = switch_on(config, buffer);

        if (status == DEVICE_OK) {
            response = estr_cat("`", buffer, "` switched on.");
        } else if (status == DEVICE_ERR_DISABLED) {
            response = estr_cat("`", buffer, "` is disabled! Kindly enable before trying again.");
        } else if (status == DEVICE_ERR_ALREADY_IN_WANTED_STATE) {
            response = estr_cat("`", buffer, "` is already on.");
        } else if (status == DEVICE_ERR_NOT_BOUND) {
            response = estr_cat("`", buffer, "` not bound");
        }

    } else if (estr_sw(command, "!off ")) {
        get_next_word(command, strlen("!off "), buffer, 100);
        device_err_t status = switch_off(config, buffer);

        if (status == DEVICE_OK) {
            response = estr_cat("`", buffer, "` switched off.");
        } else if (status == DEVICE_ERR_DISABLED) {
            response = estr_cat("`", buffer, "` is disabled! Kindly enable before trying again.");
        } else if (status == DEVICE_ERR_ALREADY_IN_WANTED_STATE) {
            response = estr_cat("`", buffer, "` is already off.");
        } else if (status == DEVICE_ERR_NOT_BOUND) {
            response = estr_cat("`", buffer, "` not bound");
        }
        
    } else if (estr_sw(command, "!disable ")) {
        get_next_word(command, strlen("!disable "), buffer, 100);
        device_err_t status = disable_device(config, buffer);

        if (status == DEVICE_OK) {
            response = estr_cat("`", buffer, "` disabled.");
        } else if (status == DEVICE_ERR_ALREADY_IN_WANTED_STATE) {
            response = estr_cat("`", buffer, "` is already disabled.");
        } else if (status == DEVICE_ERR_NOT_BOUND) {
            response = estr_cat("`", buffer, "` not bound");
        }

    } else if (estr_sw(command, "!enable ")) {
        get_next_word(command, strlen("!enable "), buffer, 100);
        device_err_t status = enable_device(config, buffer);

        if (status == DEVICE_OK) {
            response = estr_cat("`", buffer, "` enabled.");
        } else if (status == DEVICE_ERR_ALREADY_IN_WANTED_STATE) {
            response = estr_cat("`", buffer, "` is already enabled.");
        } else if (status == DEVICE_ERR_NOT_BOUND) {
            response = estr_cat("`", buffer, "` not bound");
        }

    } else if (estr_sw(command, "!on_delayed ")) {
        char time_buffer[50];
        int next_index = get_next_word(command, strlen("!on_delayed "), buffer, 100);
        get_next_word(command, next_index, time_buffer, 100);

        if (atoi(time_buffer) == 0) {
            response = estr_cat("Invalid Time Value. Value should be a valid integer > 0");
        } else {
            device_err_t status = switch_on_after_interval(config, buffer, atoi(time_buffer));

            if (status == DEVICE_OK) {
                response = estr_cat("`", buffer, "` will be switched on in ", time_buffer, " seconds");
            } else if (status == DEVICE_ERR_NOT_BOUND) {
                response = estr_cat("`", buffer, "` not bound");
            } else if (status == DEVICE_ERR_TIMER_MAX_LIMIT) {
                response = estr_cat("Timer Limit. Cannot add more timers. Kindly wait for a timer to finish or reset all timers for a device.");
            } else if (status == DEVICE_ERR_DISABLED) {
                response = estr_cat("`", buffer, "` is disabled! Kindly enable before trying again.");
            }
        }
    } else if (estr_sw(command, "!off_delayed ")) {
        char time_buffer[50];
        int next_index = get_next_word(command, strlen("!off_delayed "), buffer, 100);
        get_next_word(command, next_index, time_buffer, 100);

        if (atoi(time_buffer) == 0) {
            response = estr_cat("Invalid Time Value. Value should be a valid integer > 0");
        } else {
            device_err_t status = switch_off_after_interval(config, buffer, atoi(time_buffer));

            if (status == DEVICE_OK) {
                response = estr_cat("`", buffer, "` will be switched off in ", time_buffer, " seconds");
            } else if (status == DEVICE_ERR_NOT_BOUND) {
                response = estr_cat("`", buffer, "` not bound");
            } else if (status == DEVICE_ERR_TIMER_MAX_LIMIT) {
                response = estr_cat("Timer Limit. Cannot add more timers. Kindly wait for a timer to finish or clear timers for a device.");
            } else if (status == DEVICE_ERR_DISABLED) {
                response = estr_cat("`", buffer, "` is disabled! Kindly enable before trying again.");
            }
        }
    } else if (estr_sw(command, "!clear_timers ")) {
        get_next_word(command, strlen("!clear_timers "), buffer, 100);
        device_err_t status = clear_timers(config, get_device_idx(config, buffer), false);

        if (status == DEVICE_OK) {
            response = estr_cat("All Timers for `", buffer ," Removed.");
        } else if (status == DEVICE_ERR_NOT_BOUND) {
            response = estr_cat("`", buffer, "` not bound");
        } else if (status == DEVICE_ERR_TIMER_DOES_NOT_EXIST) {
            response = estr_cat("No Timers for `", buffer ,"` exist");
        } 

    } else if (estr_sw(command, "!devices")) {
        char *res = list_devices(config);

        if (res) return res;
        return estr_cat("No Devices bound");
    } else if (estr_sw(command, "!help")) {
        return list_commands(config);
    } else if (estr_sw(command, "!timers")) {
        return list_timers(config);
    }

    save_config_as_json(config);
    return response;
}


/**
 * @brief gets next work and stores inside buffer
 * @param buffer char* to store word into
 * @param startIndex int Index to start from
 * @return int index of the character after word
 */
int get_next_word(const char *command, int startIndex, char *buffer, int buffer_size) {
    int index = 0;
    char letter = command[startIndex];

    while (letter != '\0' && letter != '\n' && letter != '\t' && letter != ' ' && index < buffer_size - 2) {
        buffer[index++] = letter;
        letter = command[index + startIndex];
    }
    
    buffer[index] = '\0';
    return index + startIndex + 1;
}

/**
 * @brief gets device index from Config given name
 * @param config Config * to extract device index from
 * @param sname char * name of the device to extract index for
 * @return int index of the device (-1, if none)
 */
int get_device_idx(Config *config, char *name) {
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (estr_eq(config->devices[i].name, name)) return i;
    }
    // return err_code: -1 (not found)
    return -1; 
}

/**
 * @brief registers a device
 * @param config Config * to extract device index from
 * @param name char * name of the device to register
 * @return device_err_t enum
 */
device_err_t register_device(Config *config, char *name) {
    // if no pins are vacant, return err_code: -1
    if (config->pins_used == MAX_DEVICES) {
        return DEVICE_ERR_NO_VACANT_PINS;
    }
    // If device Already Exists, return err_code: -2 indicating that device already exists
    if (get_device_idx(config, name) != -1) return DEVICE_ERR_ALREADY_BOUND;
    // If device Does not Exist find a vacant pin and create device
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (config->devices[i].status == DEVICE_STATUS_NOT_BOUND) {
            strncpy(config->devices[i].name, name, DEVICE_NAME_LENGTH - 2);
            config->devices[i].name[DEVICE_NAME_LENGTH - 1] = '\0';
            config->devices[i].name[strlen(name)] = '\0';
            config->devices[i].pin = config->valid_pins[i];
            config->devices[i].status = DEVICE_STATUS_OFF;
            config->pins_used++;
            save_config_as_json(config);
            return DEVICE_OK;
        }
    }
    // if no pins are vacant, return err_code: -1
    return DEVICE_ERR_NO_VACANT_PINS;
}

/**
 * @brief deletes a device
 * @param config Config * to extract device index from
 * @param name char * name of the device to register
 * @return device_err_t enum
 */
device_err_t delete_device(Config *config, char *name) {
    if (config->pins_used == 0) return DEVICE_ERR_NOT_BOUND;

    // find the index of the device and remove device
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (estr_eq(config->devices[i].name, name)) {
            gpio_set_level(config->devices[i].pin, 0);
            config->devices[i].status = DEVICE_STATUS_NOT_BOUND;
            config->devices[i].pin = -1;
            config->devices[i].name[0] = '\0';
            config->pins_used--;

            clear_timers(config, i, true);

            save_config_as_json(config);
            return DEVICE_OK;
        }
    }

    // if device does not exist, return err_code: -1
    return DEVICE_ERR_NOT_BOUND;
}

/**
 * @brief turns on a device
 * @param config Config * to extract device index from
 * @param name char * name of the device to register
 * @return device_err_t enum
 */
device_err_t switch_on(Config* config, char *device) {
    int device_index = get_device_idx(config, device);

    // return err_code: -3 (no device bound)
    if (device_index == -1) return DEVICE_ERR_NOT_BOUND;
    // if device is bound and device status is disabled return err_code: -1 // (disabled on)
    if (config->devices[device_index].status == DEVICE_STATUS_DISABLED) return DEVICE_ERR_DISABLED;

    // if device is bound and device is on, return err_code: -2
    if (config->devices[device_index].status == DEVICE_STATUS_ON) return DEVICE_ERR_ALREADY_IN_WANTED_STATE; // (already on)
    
    // update status
    config->devices[device_index].status = DEVICE_STATUS_ON;
    gpio_set_level(config->devices[device_index].pin, 1);
    // turn device on

    save_config_as_json(config);
    return DEVICE_OK; // (turned on)
}

/**
 * @brief turns off a device
 * @param config Config * to extract device index from
 * @param name char * name of the device to register
 * @return device_err_t enum
 */
device_err_t switch_off(Config* config, char *device) {
    int device_index = get_device_idx(config, device);

    // return err_code: -3 (no device bound)
    if (device_index == -1) return DEVICE_ERR_NOT_BOUND;
    // if device is bound and device status is disabled return err_code: -1 (disabled)
    if (config->devices[device_index].status == DEVICE_STATUS_DISABLED) return DEVICE_ERR_DISABLED;

    // if device is bound and device is on, return err_code: -2 (already off)
    if (config->devices[device_index].status == DEVICE_STATUS_OFF) return DEVICE_ERR_ALREADY_IN_WANTED_STATE;
    
    // update status
    config->devices[device_index].status = DEVICE_STATUS_OFF;
    gpio_set_level(config->devices[device_index].pin, 0);
    // turn device off

    save_config_as_json(config);
    return DEVICE_OK;
} 

/**
 * @brief disables a device
 * @param config Config * to extract device index from
 * @param name char * name of the device to register
 * @return device_err_t enum
 */
device_err_t disable_device(Config *config, char *name) {
    int device_index = get_device_idx(config, name);

    // return err_code: -3 (no device bound)
    if (device_index == -1) return DEVICE_ERR_NOT_BOUND;
    // if device is bound and device status is disabled return err_code: -2 (already disabled)
    if (config->devices[device_index].status == DEVICE_STATUS_DISABLED) return DEVICE_ERR_ALREADY_IN_WANTED_STATE;

    // update status
    config->devices[device_index].status = DEVICE_STATUS_DISABLED;
    gpio_set_level(config->devices[device_index].pin, 0);

    save_config_as_json(config);
    return DEVICE_OK;
}

/**
 * @brief enables a device
 * @param config Config * to extract device index from
 * @param name char * name of the device to register
 * @return device_err_t enum
 */
device_err_t enable_device(Config *config, char *name) {
    int device_index = get_device_idx(config, name);

    // return err_code: -3 (no device bound)
    if (device_index == -1) return DEVICE_ERR_NOT_BOUND;
    // if device is bound and device status is disabled return err_code: -2 (already disabled)
    if (config->devices[device_index].status != DEVICE_STATUS_DISABLED) return DEVICE_ERR_ALREADY_IN_WANTED_STATE;

    // update status
    config->devices[device_index].status = DEVICE_STATUS_OFF;

    save_config_as_json(config);
    return DEVICE_OK;
}

/**
 * @brief Add timer to turn off a device
 * @param config Config * to extract device index from
 * @param device char * name of the device to register
 * @param interval_sec int - trigger delay in seconds
 * @return device_err_t enum
 */
device_err_t switch_off_after_interval(Config *config, char *device, int interval_sec) { 
    if (get_device_idx(config, device) == -1) return DEVICE_ERR_NOT_BOUND;

    if (config->devices[get_device_idx(config, device)].status == DEVICE_STATUS_DISABLED) return DEVICE_ERR_DISABLED;

    return add_timer(config, get_device_idx(config, device), 0, interval_sec);
}

/**
 * @brief Add timer to turn on a device
 * @param config Config * to extract device index from
 * @param device char * name of the device to register
 * @param interval_sec int - trigger delay in seconds
 * @return device_err_t enum
 */
device_err_t switch_on_after_interval(Config *config, char *device, int interval_sec) {
    if (get_device_idx(config, device) == -1) return DEVICE_ERR_NOT_BOUND;

    if (config->devices[get_device_idx(config, device)].status == DEVICE_STATUS_DISABLED) return DEVICE_ERR_DISABLED;

    return add_timer(config, get_device_idx(config, device), 1, interval_sec);
}

/**
 * @brief Generic function to add a timer
 * @param config Config * to extract device index from
 * @param device_index int - index of the device in Config
 * @param pin_level int - device status after trigger
 * @param fire_delay int - trigger delay in seconds
 * @return device_err_t enum
 */
device_err_t add_timer(Config *config, int device_index, int pin_level, int fire_delay) {
    if (config->timers->num_timers == MAX_TIMERS) return DEVICE_ERR_TIMER_MAX_LIMIT;

    for (int i = 0; i < MAX_TIMERS; i++) {
        if (config->timers->timers_arr[i].status == TIMER_ACTIVE) continue;

        config->timers->timers_arr[i].status = TIMER_ACTIVE;
        config->timers->timers_arr[i].fire_time = fire_delay;
        config->timers->timers_arr[i].pin_level = pin_level;
        config->timers->timers_arr[i].pin_idx = config->devices[device_index].pin;
        config->timers->num_timers++;

        ESP_LOGI("TIMER ADDED", "Num timers: %d", config->timers->num_timers);

        return DEVICE_OK;
    }

    ESP_LOGE("TIMER ERROR", "Num timers: %d", config->timers->num_timers);

    return DEVICE_ERR_TIMER_MAX_LIMIT;
}

/**
 * @brief Function to clear timers for a device
 * @param config Config * to extract device index from
 * @param device_index int - index of the device in Config
 * @param remove - remove the timer from the list or not
 * @return device_err_t enum
 */
device_err_t clear_timers(Config *config, int device_index, bool remove) {
    int num_timers = config->timers->num_timers;

    if (num_timers == 0) return DEVICE_ERR_TIMER_DOES_NOT_EXIST;

    for (int i = 0; i < MAX_TIMERS; i++) {
        if (config->timers->timers_arr[i].status != TIMER_ACTIVE) continue;
        if (config->timers->timers_arr[i].pin_idx != config->devices[device_index].pin) continue;

        if (remove == true) config->timers->timers_arr[i].pin_idx = -1;
        config->timers->timers_arr[i].status = TIMER_INACTIVE;
        config->timers->num_timers--;

        ESP_LOGI("TIMERS REMOVED", "Num timers: %d", config->timers->num_timers);

        return DEVICE_OK;
    }

    ESP_LOGE("TIMER ERROR", "Num timers: %d", config->timers->num_timers);

    return DEVICE_ERR_TIMER_DOES_NOT_EXIST;
}


/**
 * @brief List all timers
 * @param config Config * to extract device index from
 * @return summary of all active timers
 */
char* list_timers(Config *config) {
    char *response = NULL;
    char buffer[150];

    if (config->pins_used == 0) return estr_cat("No Devices bound");

    // loop over each device and append to string
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (config->timers->timers_arr[i].status == TIMER_ACTIVE) {
            char *status = NULL;
            const device_info_t *device = NULL;

            for (int j = 0; j < MAX_DEVICES; j++) {
                if (config->devices[j].pin == config->timers->timers_arr[i].pin_idx) device = &config->devices[j];
            }

            if (!device) continue;

            if (config->timers->timers_arr[i].pin_level == 0) status = estr_cat("OFF");
            else status = estr_cat("ON");

            sprintf(buffer, "TIMER ID: %d,ACTION: %s will turn %s in %d seconds\n", i, device->name, status, config->timers->timers_arr[i].fire_time);
            // concatenate line
            char *temp = response; // get pointer to response string
            if (response) response = estr_cat(response, buffer); // update response
            else response = estr_cat(buffer);
            // Free memory
            if (temp) free(temp);
            free(status);
        }
    }   

    if (!response) return estr_cat("No Active timers");
    
    return response;
}

/**
 * @brief provides summary of all devices
 * @param config Config * to extract device index from
 * @return message to send to discord (summary of all devices)
*/

char* list_devices(Config* config) {
    char *response = NULL;
    char *status = NULL;
    char buffer[150];

    if (config->pins_used == 0) return estr_cat("No Devices bound");

    // loop over each device and append to string
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (config->devices[i].status != DEVICE_STATUS_NOT_BOUND) {
            if (config->devices[i].status == DEVICE_STATUS_ON) status = estr_cat("ON      ");
            else if (config->devices[i].status == DEVICE_STATUS_OFF) status = estr_cat("OFF     ");
            else status = estr_cat("DISABLED");

            sprintf(buffer, "DEVICE:\t%.20s\t\tSTATUS:\t%.8s\t\tPIN:\t%d\n", config->devices[i].name, status, config->devices[i].pin);
            // concatenate line
            char *temp = response; // get pointer to response string
            if (response) response = estr_cat(response, buffer); // update response
            else response = estr_cat(buffer);
            // Free memory
            if (temp) free(temp);
            free(status);
        }
    }

    return response;
}


/**
 * @brief Prints all available commands
 * @param config Config * to extract device index from
 * @return message to send to discord
 */
char* list_commands(Config* config) {
    return estr_cat(
        "!bind <DEVICE_NAME> - Binds the device to a free pin\n"
        "!unbind <DEVICE_NAME> - Frees the pin bound to the device\n"
        "!on <DEVICE_NAME> - Turn on the device\n"
        "!off <DEVICE_NAME> - Turns off the device\n"
        "!disable <DEVICE_NAME> - Disables a device, stopping all features from working\n"
        "!enable <DEVICE_NAME> - Enables a device, allowing all features to work\n"
        "!on_delayed <DEVICE_NAME> <DELAY_IN_SEC> - Turns on the device after Time delay(in seconds)\n"
        "!off_delayed <DEVICE_NAME> <DELAY_IN_SEC> - Turns off the device after Time delay(in seconds)\n"
        "!on_delayed <DEVICE_NAME> <DELAY_IN_SEC> - Turns off the device after Time delay(in seconds)\n"
        "!clear_timers <DEVICE_NAME> - Clears all timers for the specified device\n",
        "!timers <DEVICE_NAME> - List all active timers\n"
        "!devices - Prints details about all registered devices\n"
        "!help - Returns the list of available commands\n"
    );
}

/**
 * @brief Loads json string from nvs and populate Config obj
 * @param config Config * to extract device index from
 */
void load_config(Config *config) {
    nvs_handle_t nvs_handle;
    size_t length = 0;
    char *json_string = NULL;

    config_init(config);

    ESP_ERROR_CHECK(nvs_open(NVS_VARIABLES_NAMESPACE, NVS_READWRITE, &nvs_handle));
    nvs_get_str(nvs_handle, NVS_CONFIG_VARIABLE, NULL, &length);

    if (length > 0) json_string = calloc(length, sizeof(char));

    esp_err_t status = nvs_get_str(nvs_handle, NVS_CONFIG_VARIABLE, json_string, &length);

    if (status == ESP_OK) {
        // Parse json
        cJSON *json = cJSON_Parse(json_string);
        cJSON *pins_used = cJSON_GetObjectItemCaseSensitive(json, "pins_used");
        cJSON *devices = cJSON_GetObjectItemCaseSensitive(json, "devices");

        config->pins_used = pins_used->valueint;

        for (int i = 0; i < MAX_DEVICES; i++) {
            cJSON *device = cJSON_GetArrayItem(devices, i);

            if (device) {
                cJSON *name = cJSON_GetObjectItemCaseSensitive(device, "name");
                cJSON *status = cJSON_GetObjectItemCaseSensitive(device, "status");
                cJSON *pin = cJSON_GetObjectItemCaseSensitive(device, "pin");

                strcpy(config->devices[i].name, name->valuestring);
                config->devices[i].pin = pin->valueint;
                config->devices[i].status = status->valueint;
            }
        }

        cJSON_Delete(json);
        // Restore Device State
        for (int i = 0; i < MAX_DEVICES; i++) {
            gpio_pad_select_gpio(config->valid_pins[i]);
            ESP_ERROR_CHECK(gpio_set_direction(config->valid_pins[i], GPIO_MODE_OUTPUT));

            if (config->devices[i].status == DEVICE_STATUS_ON) {
                ESP_ERROR_CHECK(gpio_set_level(config->devices[i].pin, 1));
            }
        };

    } else if (status == ESP_ERR_NVS_NOT_FOUND) {
        config_init(config);
        save_config_as_json(config);
    } else {
        nvs_close(nvs_handle);
        ESP_ERROR_CHECK(status);
        return;
    }

    nvs_close(nvs_handle);
    if (json_string) free(json_string);
}


/**
 * @brief converts config to json and stores in nvs flash
 * @param config Config * to extract device index from
 */
void save_config_as_json(Config *config) {
    nvs_handle_t nvs_handle;

    cJSON *json = cJSON_CreateObject();
    cJSON *devices = cJSON_CreateArray();

    for (int i = 0; i < MAX_DEVICES; i++) {
        cJSON *device = cJSON_CreateObject();
        cJSON_AddNumberToObject(device, "pin", config->devices[i].pin);
        cJSON_AddStringToObject(device, "name", config->devices[i].name);
        cJSON_AddNumberToObject(device, "status", config->devices[i].status);
        cJSON_AddItemToArray(devices, device);
    }

    cJSON_AddNumberToObject(json, "pins_used", config->pins_used);
    cJSON_AddItemToObject(json, "devices", devices);

    char *json_string = cJSON_Print(json);
    cJSON_Delete(json);

    // save string
    ESP_ERROR_CHECK(nvs_open(NVS_VARIABLES_NAMESPACE, NVS_READWRITE, &nvs_handle));
    ESP_ERROR_CHECK(nvs_set_str(nvs_handle, NVS_CONFIG_VARIABLE, json_string));
    ESP_ERROR_CHECK(nvs_commit(nvs_handle));
    nvs_close(nvs_handle);
    free(json_string);
}
