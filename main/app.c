#include <stdio.h>
#include "estr.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "string.h"
#include "nvs_flash.h"
#include "app.h"

extern const uint8_t certificate_pem_start[] asm("_binary_certificate_pem_start");
extern const uint8_t certificate_pem_end[]   asm("_binary_certificate_pem_end");

void config_init(Config *config) {
    int valid_pins[MAX_DEVICES] = {2};

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
            itoa(config->valid_pins[get_device_idx(config, buffer)], pinString, 10);
            response = estr_cat("Unbound `", buffer, "` from Pin ", pinString);
        }

    } else if (estr_sw(command, "!switch_on ")) {
        get_next_word(command, strlen("!switch_on "), buffer, 100);
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

    } else if (estr_sw(command, "!switch_off ")) {
        get_next_word(command, strlen("!switch_off "), buffer, 100);
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

    } else if (estr_sw(command, "!switch_on_delayed ")) {
        char time_buffer[50];
        int next_index = get_next_word(command, strlen("!switch_on_delayed "), buffer, 100);
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
    } else if (estr_sw(command, "!switch_off_delayed ")) {
        char time_buffer[50];
        int next_index = get_next_word(command, strlen("!switch_off_delayed "), buffer, 100);
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

    } else if (estr_sw(command, "!reset_all_timers")) {
        reset_all_timers(config);

        response = estr_cat("All Timers Reset");
    }  else if (estr_sw(command, "!list_devices")) {
        char *res = list_devices(config);

        if (res) return res;
        return estr_cat("Err");
    } else if (estr_sw(command, "!list_commands")) {
        return list_commands(config);
    }

    save_config(config);
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
    return index + startIndex + 1;
}

int get_device_idx(Config *config, char *name) {
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (estr_eq(config->devices[i].name, name)) return i;
    }
    // return err_code: -1 (not found)
    return -1; 
}

device_err_t register_device(Config *config, char *name) {
    int index = 0;
    // if no pins are vacant, return err_code: -1
    if (config->pins_used == MAX_DEVICES) {
        return DEVICE_ERR_NO_VACANT_PINS;
    }
    // If device Already Exists, return err_code: -2 indicating that device already exists
    if (get_device_idx(config, name) != -1) return DEVICE_ERR_ALREADY_BOUND;
    // If device Does not Exist find a vacant pin and create device
    for (int i = 0; i < MAX_DEVICES; i++) {
        index = 0;

        if (config->devices[i].name[0] == '\0') {
            while (name[index] != '\0' && index < DEVICE_NAME_LENGTH - 1)
            {
                config->devices[i].name[index] = name[index];
                config->devices[i].status = DEVICE_STATUS_OFF;
                config->devices[i].pin = config->valid_pins[i];
                index++;
            }
            
            config->pins_used++;
            return DEVICE_OK;
        }
    }
    // if no pins are vacant, return err_code: -1
    return DEVICE_ERR_NO_VACANT_PINS;
}

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

            return DEVICE_OK;
        }
    }

    // if device does not exist, return err_code: -1
    return DEVICE_ERR_NOT_BOUND;
}

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

    return DEVICE_OK; // (turned on)
}

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

    return DEVICE_OK;
} 

device_err_t disable_device(Config *config, char *name) {
    int device_index = get_device_idx(config, name);

    // return err_code: -3 (no device bound)
    if (device_index == -1) return DEVICE_ERR_NOT_BOUND;
    // if device is bound and device status is disabled return err_code: -2 (already disabled)
    if (config->devices[device_index].status == DEVICE_STATUS_DISABLED) return DEVICE_ERR_ALREADY_IN_WANTED_STATE;

    // update status
    config->devices[device_index].status = DEVICE_STATUS_DISABLED;
    gpio_set_level(config->devices[device_index].pin, 0);

    return DEVICE_OK;
}

device_err_t enable_device(Config *config, char *name) {
    int device_index = get_device_idx(config, name);

    // return err_code: -3 (no device bound)
    if (device_index == -1) return DEVICE_ERR_NOT_BOUND;
    // if device is bound and device status is disabled return err_code: -2 (already disabled)
    if (config->devices[device_index].status != DEVICE_STATUS_DISABLED) return DEVICE_ERR_ALREADY_IN_WANTED_STATE;

    // update status
    config->devices[device_index].status = DEVICE_STATUS_OFF;

    return DEVICE_OK;
}

device_err_t switch_off_after_interval(Config *config, char *device, int interval_sec) { 
    if (get_device_idx(config, device) == -1) return DEVICE_ERR_NOT_BOUND;

    if (config->devices[get_device_idx(config, device)].status == DEVICE_STATUS_DISABLED) return DEVICE_ERR_DISABLED;

    return add_timer(config, get_device_idx(config, device), 0, interval_sec);
}

device_err_t switch_on_after_interval(Config *config, char *device, int interval_sec) {
    if (get_device_idx(config, device) == -1) return DEVICE_ERR_NOT_BOUND;

    if (config->devices[get_device_idx(config, device)].status == DEVICE_STATUS_DISABLED) return DEVICE_ERR_DISABLED;

    return add_timer(config, get_device_idx(config, device), 1, interval_sec);
}

device_err_t add_timer(Config *config, int device_index, int pin_level, int fire_delay) {
    int num_timers = config->timers->num_timers;
    uint64_t alarm_value = 0;

    timer_get_alarm_value(TIMER_GROUP_1, TIMER_1, &alarm_value);

    if (config->timers->num_timers == MAX_TIMERS) return DEVICE_ERR_TIMER_MAX_LIMIT;

    for (int i = 0; i < MAX_TIMERS; i++) {
        if (config->timers->timers_arr[num_timers].status == TIMER_ACTIVE) continue;

        config->timers->timers_arr[num_timers].status = TIMER_ACTIVE;
        config->timers->timers_arr[num_timers].fire_time = alarm_value + fire_delay;
        config->timers->timers_arr[num_timers].pin_level = pin_level;
        config->timers->timers_arr[num_timers].pin_idx = config->devices[device_index].pin;
        config->timers->num_timers++;

        ESP_LOGI("TIMER ADDED", "Num timers: %d", config->timers->num_timers);

        return DEVICE_OK;
    }

    ESP_LOGE("TIMER ERROR", "Num timers: %d", config->timers->num_timers);

    return DEVICE_ERR_TIMER_MAX_LIMIT;
}

device_err_t clear_timers(Config *config, int device_index, bool remove) {
    int num_timers = config->timers->num_timers;

    if (num_timers == 0) return DEVICE_ERR_TIMER_DOES_NOT_EXIST;

    for (int i = 0; i < MAX_TIMERS; i++) {
        if (config->timers->timers_arr[num_timers].status != TIMER_ACTIVE) continue;
        if (config->timers->timers_arr[num_timers].pin_idx != config->devices[device_index].pin) continue;

        if (remove == true) config->timers->timers_arr[num_timers].pin_idx = -1;
        config->timers->timers_arr[num_timers].status = TIMER_INACTIVE;
        config->timers->num_timers--;

        ESP_LOGI("TIMER REMOVED", "Num timers: %d", config->timers->num_timers);

        return DEVICE_OK;
    }

    ESP_LOGE("TIMER ERROR", "Num timers: %d", config->timers->num_timers);

    return DEVICE_ERR_TIMER_DOES_NOT_EXIST;
}

void reset_all_timers(Config *config) {
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (config->timers->timers_arr[i].status != TIMER_ACTIVE) continue;
        if (config->timers->timers_arr[i].pin_idx == -1) continue;

        config->timers->timers_arr[i].status = TIMER_ACTIVE;
        config->timers->num_timers++;
    }
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

void save_config(Config *config) {
    nvs_handle_t nvs_handle;
    size_t length = sizeof(Config);

    ESP_ERROR_CHECK(nvs_open(NVS_VARIABLES_NAMESPACE, NVS_READWRITE, &nvs_handle));
    ESP_ERROR_CHECK(nvs_set_blob(nvs_handle, NVS_CONFIG_VARIABLE, &config, length));
    ESP_ERROR_CHECK(nvs_commit(nvs_handle));
    nvs_close(nvs_handle);
}

void load_config(Config *config) {
    config_init(config);
    init_clock(config);

    return;

    nvs_handle_t nvs_handle;
    size_t length = sizeof(Config);
    nvs_open(NVS_VARIABLES_NAMESPACE, NVS_READWRITE, &nvs_handle);

    esp_err_t status = nvs_get_blob(nvs_handle, NVS_CONFIG_VARIABLE, &config, &length);
    if (status == ESP_ERR_NVS_NOT_FOUND) {
        config_init(config);
        length = sizeof(Config);
        ESP_ERROR_CHECK(nvs_set_blob(nvs_handle, NVS_CONFIG_VARIABLE, &config, length));
        ESP_ERROR_CHECK(nvs_commit(nvs_handle));
    } else {
        ESP_ERROR_CHECK(status);
    }

    // Initialise timers 
    // config->timers = calloc(1, sizeof (timers_info_t));
    // config->timers->timers_arr = NULL;
    // config->timers->num_timers = 0;
    
    nvs_close(nvs_handle);
}