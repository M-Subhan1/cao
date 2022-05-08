#pragma once

struct Config {
    char* appliances[10];
    int validPins[10];
    int maxAppliances;
    int pinsUsed;
};

typedef struct Config Config;

void config_init(struct Config*);
void config_delete(struct Config*);
int register_appliance(struct Config *config, char *name);
int delete_appliance(struct Config *config, char *name);
char *parse_and_execute_commands(struct Config *config, char *command);
int getNextWord(const char *command, int startIndex, char *buffer, int bufferSize);
