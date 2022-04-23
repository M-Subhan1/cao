
#include "estr.h"
#include "config.h"
#include "discord/message.h"
#include "discord.h"
#include "discord/session.h"

#define NUM_APPLIANCES 10

void config_init(struct Config *config) {
    for (int i = 0; i < NUM_APPLIANCES; i++) config->appliances[i] = NULL;
    config->pinsUsed = 0;
    config->maxAllowed = 10;
}

void config_delete(struct Config *config) {
    for (int i = 0; i < NUM_APPLIANCES; i++) if (config->appliances[i]) free(config->appliances[i]);
}

int register_appliance(struct Config *config, char *name) {
    for (int i = 0; i < NUM_APPLIANCES; i++) {
        if (!config->appliances[i]) {
            config->appliances[i] = estr_cat(name);
            config->pinsUsed++;
            return 1;
        }
    }

    return 0;
}

int delete_appliance(struct Config *config, char *name) {
    for (int i = 0; i < NUM_APPLIANCES; i++) {
        if (config->appliances[i] && estr_eq(config->appliances[i], name)) {
            free(config->appliances[i]);
            config->appliances[i] = NULL;
            config->pinsUsed--;
            return 1;
        }
    }

    return 0;
}

void parse_and_execute_commands(discord_handle_t bot, struct Config *config, discord_message_t *message) {
    char* echo_content = NULL;
    
    if (estr_sw(message->content, "!add")) {
        if (config->pinsUsed == config->maxAllowed) {
            echo_content = estr_cat("All Pins Used, Kindly Drop an appliance before trying a add a new one.");
        } else {
            char *applianceName = getApplianceName(&message->content[5]);
            int assignedPin = register_appliance(config, applianceName);

            if (assignedPin > -1) {
                echo_content = estr_cat("Appliance bound to Pin2.");
            } else {
                echo_content = estr_cat("Invalid Command");
            }
        }
    } else if (estr_sw(message->content, "!drop")) {
        char *applianceName = getApplianceName(&message->content[6]);

        if (delete_appliance(config, applianceName)) {
            echo_content = estr_cat("Appliance`", applianceName ,"` dropped from Pin2.");
        } else {
            echo_content = estr_cat("No such appliance exists. Type `!list` to view a list of appliances and their pin bindings");
        }

        free(applianceName);
    } else if (estr_sw(message->content, "!on")) {

    } else if (estr_sw(message->content, "!off")) {

    }
    
    if (!echo_content) echo_content = estr_cat("IDK");

    if (echo_content) {

        discord_message_t echo = {
            .content = echo_content,
            .channel_id = message->channel_id
        };

        discord_message_t* sent_msg = NULL;
        discord_message_send(bot, &echo, &sent_msg);
        free(echo_content);
    }
}

int enable_appliance(struct Config *config, char *name) {
    return 0;
}

int disable_appliance(struct Config *config, char *name) {
    return 0;
}

int switch_on(struct Config *config, char *name) {
    return 0;
}

int switch_off(struct Config *config, char *name) {
    return 0;
}

char *getApplianceName(char *message) {
    char *name = NULL;
    char character = message[0];

    while (character != '\n' && character != ' ' && character != '\t') {
        name = estr_cat(name, character);
    }

    if (name) name = estr_cat(name, '\0');

    return name;
}