#include <stdio.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "discord.h"
#include "discord/session.h"
#include "discord/message.h"
#include "app.h"
#include <driver/gpio.h>
#include "timer.h"

#define ESP_WIFI_SSID      "..."
#define ESP_WIFI_PASS      "rblock123"
#define ESP_MAXIMUM_RETRY  10

static discord_handle_t bot;
static bool is_running = false; 

static const char *TAG = "wifi station";
static const char* DISCORD_TAG = "discord_bot";
static int s_retry_num = 0;
struct Config config; 

static void event_handler(void*, esp_event_base_t, int32_t, void*);
void wifi_init_sta(void);
static void bot_event_handler(void*, esp_event_base_t, int32_t, void*);
static discord_config_t bot_cfg;

void app_main(void)
{   
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);
    // Restore device State
    load_config(&config);
    // initialize_discord_bot
    bot_cfg.intents = DISCORD_INTENT_GUILD_MESSAGES;
    wifi_init_sta();
    init_clock(&config);
}

void wifi_init_sta(void)
{
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = ESP_WIFI_SSID,
            .password = ESP_WIFI_PASS,
	        .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");
}

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, ".");
            return;
        } 

        ESP_LOGI(TAG,"connect to the AP fail");
        
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));

        if (is_running) discord_destroy(bot); 

        bot = discord_create(&bot_cfg);
        ESP_ERROR_CHECK(discord_register_events(bot, DISCORD_EVENT_ANY, bot_event_handler, NULL));
        discord_login(bot);
        is_running = true;

        s_retry_num = 0;
    } 
}

static void bot_event_handler(void* handler_arg, esp_event_base_t base, int32_t event_id, void* event_data) {
    discord_event_data_t* data = (discord_event_data_t*) event_data;

    switch(event_id) {
        case DISCORD_EVENT_CONNECTED: {
                discord_session_t* session = (discord_session_t*) data->ptr;

                ESP_LOGI(TAG, "Bot %s#%s connected",
                    session->user->username,
                    session->user->discriminator
                );
            }
            break;
        
        case DISCORD_EVENT_MESSAGE_RECEIVED: {
                discord_message_t* msg = (discord_message_t*) data->ptr;

                ESP_LOGI(DISCORD_TAG, "New message (dm=%s, autor=%s#%s, bot=%s, channel=%s, guild=%s, content=%s)",
                    !msg->guild_id ? "true" : "false",
                    msg->author->username,
                    msg->author->discriminator,
                    msg->author->bot ? "true" : "false",
                    msg->channel_id,
                    msg->guild_id ? msg->guild_id : "NULL",
                    msg->content
                );

                char *response = parse_and_execute_commands(&config, msg->content);
                
                if (response) {                    
                    discord_message_t echo = {
                        .content = response,
                        .channel_id = msg->channel_id
                    };

                    discord_message_t* sent_msg = NULL;
                    discord_message_send(bot, &echo, &sent_msg);
                    free(response);
                }
            }

            break;
    }
}