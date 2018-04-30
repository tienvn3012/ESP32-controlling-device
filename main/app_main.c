// control
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <pthread.h>

#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event_loop.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt.h"

#include "esp_system.h"
#include "driver/gpio.h"



const char *MQTT_TAG = "MQTT_SAMPLE";

static EventGroupHandle_t wifi_event_group;
const static int CONNECTED_BIT = BIT0;

#define LED_GPIO            19
#define PUMP_A_GPIO         13
#define PUMP_B_GPIO         15
#define PUMP_PH_UP_GPIO     18
#define PUMP_PH_DOWN_GPIO   27
#define FAN_GPIO            12
#define PUMP_UP_GPIO        4
#define PUMP_WATER          21
#define TOKEN               "1;nct_laboratory;control;nct"
#define ERROR_TOKEN         "1;nct_laboratory;control;nct"
#define KEEP_ALIVE_TOKEN    "1;nct_laboratory;nct"


int authentication_error = 0;

// #define CONFIG_MQTT_SECURITY_ON 1

void connected_cb(void *self, void *params)
{
    mqtt_client *client = (mqtt_client *)self;
    // authentication device
    mqtt_publish(client,"nct_authentication",TOKEN,sizeof(TOKEN),0,0);

    // subscribe for authentication result
    mqtt_subscribe(client, "nct_authentication_result_1", 0);
}
void disconnected_cb(void *self, void *params)
{

}
void reconnect_cb(void *self, void *params)
{

}
void subscribe_cb(void *self, void *params)
{
    ESP_LOGI(MQTT_TAG, "[APP] Subscribe ok, test publish msg");
    mqtt_client *client = (mqtt_client *)self;
    
}

void publish_cb(void *self, void *params)
{

}



void stattement_analysis(char* statement){
    printf("%s\n", statement);
    if(strcmp(statement,"L_1\0") == 0){
        gpio_set_level(LED_GPIO,0);
    }else if(strcmp(statement,"L_0\0") == 0){
        printf("%s\n",statement);
        gpio_set_level(LED_GPIO,1);
    }else if(strcmp(statement,"PA_0\0") == 0){
        printf("%s\n",statement);
        gpio_set_level(PUMP_A_GPIO,0);
    }else if(strcmp(statement,"PA_1\0") == 0){
        printf("%s\n",statement);
        gpio_set_level(PUMP_A_GPIO,1);
    }else if(strcmp(statement,"PB_0\0") == 0){
        printf("%s\n",statement);
        gpio_set_level(PUMP_B_GPIO,0);
    }else if(strcmp(statement,"PB_1\0") == 0){
        printf("%s\n",statement);
        gpio_set_level(PUMP_B_GPIO,1);
    }else if(strcmp(statement,"F_0\0") == 0){
        printf("%s\n",statement);
        gpio_set_level(FAN_GPIO,0);
    }else if(strcmp(statement,"F_1\0") == 0){
        printf("%s\n",statement);
        gpio_set_level(FAN_GPIO,1);
    }else if(strcmp(statement,"PU_1\0") == 0){
        printf("%s\n",statement);
        gpio_set_level(PUMP_PH_UP_GPIO,1);
    }else if(strcmp(statement,"PU_0\0") == 0){
        printf("%s\n",statement);
        gpio_set_level(PUMP_PH_UP_GPIO,0);
    }else if(strcmp(statement,"PD_1\0") == 0){
        printf("%s\n",statement);
        gpio_set_level(PUMP_PH_DOWN_GPIO,1);
    }else if(strcmp(statement,"PD_0\0") == 0){
        printf("%s\n",statement);
        gpio_set_level(PUMP_PH_DOWN_GPIO,0);
    }else if(strcmp(statement,"UP_1\0") == 0){
        printf("%s\n",statement);
        gpio_set_level(PUMP_UP_GPIO,0);
    }else if(strcmp(statement,"UP_0\0") == 0){
        printf("%s\n",statement);
        gpio_set_level(PUMP_UP_GPIO,1);
    }else if(strcmp(statement,"PW_1\0") == 0){
        printf("%s\n",statement);
        gpio_set_level(PUMP_WATER,0);
    }else if(strcmp(statement,"PW_0\0") == 0){
        printf("%s\n",statement);
        gpio_set_level(PUMP_WATER,1);
    }else{
        printf("statement not found !!!! \n");
    }
}


// keep alive thread
void *keep_alive(void *vargp){
    mqtt_client *client = (mqtt_client *)vargp;
    while(1){
        sleep(60);
        // keep alive
        mqtt_publish(client,"nct_keep_alive",KEEP_ALIVE_TOKEN,sizeof(KEEP_ALIVE_TOKEN),0,0);
        printf("Keep alive message publish !!!!\n");
    }
}

void data_cb(void *self, void *params)
{
    mqtt_client *client = (mqtt_client *)self;
    mqtt_event_data_t *event_data = (mqtt_event_data_t *)params;

    

char *topic = malloc(event_data->topic_length + 1);
        memcpy(topic, event_data->topic, event_data->topic_length);
        topic[event_data->topic_length] = 0;
    // char *data = malloc(event_data->data_length + 1);
    // memcpy(data, event_data->data, event_data->data_length);
    // data[event_data->data_length] = 0;
    ESP_LOGI(MQTT_TAG, "[APP] Publish data[%d/%d bytes]",
             event_data->data_length + event_data->data_offset,
             event_data->data_total_length);
    // data);

    // free(data);
    // printf("fuck %s\n",event_data->data);
    char* x = (char*)event_data->topic;
    if((strcmp(topic,"nct_authentication_result_1") == 0)){

        char* recv = (char*)event_data->data;
        // printf("xxxxx %s\n", event_data->data);
        if((strcmp(recv,"PASS")) == 0){
            printf("CHECK PASSWORD DONE !!!!\n");
            mqtt_subscribe(client, "nct_control_1", 0);
            printf("SUBSCRIBER DONE !!!!\n");

            pthread_t tid;
            pthread_create(&tid, NULL,keep_alive, (void*)client);

        }else{
            printf("CHECK PASSWORD FAILED !!!!\n");
            if(authentication_error >2){
                mqtt_publish(client,"nct_error",ERROR_TOKEN,sizeof(ERROR_TOKEN),0,0);
                printf("PUBLISH TO NCT_ERROR !!!!\n");
            }else{
                authentication_error+=1;
                vTaskDelay(60000 / portTICK_PERIOD_MS);
                printf("RE AUTHENTICATION !!!!\n");
                mqtt_publish(client,"nct_authentication",TOKEN,sizeof(TOKEN),0,0);
            }
        }

    }else if((strcmp(topic,"nct_control_1") == 0)){
        
        
        char bu[15];
        char *f = (char*)event_data->data;
        sprintf(bu,"nct;1;%s;nct",f);
        stattement_analysis(f);
        printf("RECEIVE STATEMENT : %s\n", bu);
        mqtt_publish(client, "nct_log",bu,sizeof(bu), 0, 0);
    }

}



mqtt_settings settings = {
    .host = "iot.eclipse.org",
    .port = 1883,
// #if defined(CONFIG_MQTT_SECURITY_ON)
//     .port = 8883, // encrypted
// #else
//     .port = 1883, // unencrypted
// #endif
    .client_id = "uhg",
    .username = "tienvn3012",
    .password = "tienchocorion",
    .clean_session = 0,
    .keepalive = 120,
    .lwt_topic = "/lwt",
    .lwt_msg = "offline",
    .lwt_qos = 0,
    .lwt_retain = 0,
    .connected_cb = connected_cb,
    .disconnected_cb = disconnected_cb,
    // .reconnect_cb = reconnect_cb,
    .subscribe_cb = subscribe_cb,
    .publish_cb = publish_cb,
    .data_cb = data_cb
};



static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
            mqtt_start(&settings);
            //init app here
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            /* This is a workaround as ESP32 WiFi libs don't currently
               auto-reassociate. */
            esp_wifi_connect();
            mqtt_stop();
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            break;
        default:
            break;
    }
    return ESP_OK;
}

static void wifi_conn_init(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "NCT",
            .password = "dccndccn",
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_LOGI(MQTT_TAG, "start the WIFI SSID:[%s] password:[%s]", CONFIG_WIFI_SSID, "******");
    ESP_ERROR_CHECK(esp_wifi_start());
}

void app_main()
{
    ESP_LOGI(MQTT_TAG, "[APP] Startup..");
    ESP_LOGI(MQTT_TAG, "[APP] Free memory: %d bytes", system_get_free_heap_size());
    ESP_LOGI(MQTT_TAG, "[APP] SDK version: %s, Build time: %s", system_get_sdk_version(), BUID_TIME);

    gpio_pad_select_gpio(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_INPUT_OUTPUT);
    gpio_pad_select_gpio(PUMP_A_GPIO);
    gpio_set_direction(PUMP_A_GPIO, GPIO_MODE_INPUT_OUTPUT);
    gpio_pad_select_gpio(PUMP_B_GPIO);
    gpio_set_direction(PUMP_B_GPIO, GPIO_MODE_INPUT_OUTPUT);
    gpio_pad_select_gpio(FAN_GPIO);
    gpio_set_direction(FAN_GPIO, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_level(FAN_GPIO,0);
    gpio_set_direction(PUMP_PH_UP_GPIO, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_level(PUMP_PH_UP_GPIO,0);
    gpio_set_direction(PUMP_PH_DOWN_GPIO, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_level(PUMP_PH_DOWN_GPIO,0);
    gpio_set_direction(PUMP_UP_GPIO, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_level(PUMP_UP_GPIO,0);
    gpio_set_direction(PUMP_WATER, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_level(PUMP_WATER,0);


    nvs_flash_init();
    wifi_conn_init();
}
