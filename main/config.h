#pragma once

#include "discord/message.h"
#include "discord/session.h"
#include "discord.h"

struct Config {
    int pinsUsed;
    char* appliances[10];
    int maxAllowed;
};

typedef struct Config Config;

void config_init(struct Config*);
void config_delete(struct Config*);
int register_appliance(struct Config *config, char *name);
int delete_appliance(struct Config *config, char *name);
void parse_and_execute_commands(discord_handle_t bot, struct Config *config, discord_message_t* message);
int enable_appliance(struct Config *config, char *name);
int disable_appliance(struct Config *config, char *name);
int switch_on(struct Config *config, char *name);
int switch_off(struct Config *config, char *name);
char *getApplianceName(char *message);
    