/* Esptouch example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "esp_smartconfig.h"
#include "smartconfig_ack.h"
#include "driver/uart.h"
#include "gb_encoding_definition.h"

/* FreeRTOS event group to signal when we are connected & ready to make a request */
/* static  */EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
/* static  */const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;
/*static */const int CONNECTED_MQTT_BIT = BIT2;
/*static */const int SIGN_BIT = BIT3;
static const char *TAG = "sc";
static int init_data_flag = 0;

static esp_err_t read_wifi_init_sta_info1(wifi_config_t *cfg);
static esp_err_t write_wifi_init_sta_info1(wifi_config_t *cfg);

static void sc_callback(smartconfig_status_t status, void *pdata);
static esp_err_t event_handler(void *ctx, system_event_t *event);
void smartconfig_example_task(void * parm);
static void initialise_wifi(void);

#define NVS_PRODUCT "aliyun-key"

static esp_err_t read_wifi_init_sta_info1(wifi_config_t *cfg)
{
    nvs_handle mHandleNvs;
    static const char *FILED_SELF_Struct = "struct_Self";
	
	size_t length = sizeof(wifi_config_t);

    esp_err_t err = nvs_open(NVS_PRODUCT, NVS_READONLY, &mHandleNvs);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Open NVS Table fail");
    } else {
        ESP_LOGI(TAG, "Open NVS Table ok.");
    }

    err = nvs_get_blob(mHandleNvs, FILED_SELF_Struct, cfg, &length);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_get_blob Struct Fail");
    } else {
        // ESP_LOGI(TAG, "connect to ap SSID:%s password:%s",
        //          cfg.sta.ssid, cfg.sta.password);
        // ESP_LOGI(TAG, "connect to ap SSID:%s password:%s",
        //          cfg->sta.ssid, cfg->sta.password);
    }

    nvs_close(mHandleNvs);

    return err;
}

static esp_err_t write_wifi_init_sta_info1(wifi_config_t *cfg)
{
    nvs_handle mHandleNvs;
    static const char *FILED_SELF_Struct = "struct_Self";

    esp_err_t err = nvs_open(NVS_PRODUCT, NVS_READWRITE, &mHandleNvs);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Open NVS Table fail");
    } else {
        ESP_LOGI(TAG, "Open NVS Table ok.");
    }

    err = nvs_set_blob(mHandleNvs, FILED_SELF_Struct, cfg, sizeof(wifi_config_t));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_set_blob Struct Fail");
    } else {
        ESP_LOGI(TAG, "nvs_set_blob Struct ok.");
    }

    nvs_commit(mHandleNvs);
    nvs_close(mHandleNvs);

    return err;
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    /* For accessing reason codes in case of disconnection */
    system_event_info_t *info = &event->event_info;
	wifi_ap_record_t ap_info;
    static size_t cnt = 0;
	char buf[64];

    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
		// if(!init_data_flag) {
			esp_wifi_connect();
		// } else {
		// 	xTaskCreate(smartconfig_example_task, "smartconfig_example_task", 4096, NULL, 3, NULL);
		// }
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        esp_wifi_sta_get_ap_info(&ap_info);
        ESP_LOGI(TAG, "SSID:%s", ap_info.ssid);
		// bzero(buf, sizeof(buf));
		// sprintf(buf, "\r\nGet Connected [SSID:%s]\r\n", ap_info.ssid);
        // uart_write_bytes(UART_NUM_0, buf, strlen(buf));
        // bzero(buf, sizeof(buf));
        // sprintf(buf, "\r\nIP:"IPSTR"\r\n", IP2STR(ip_2_ip4(&(info->got_ip.ip_info.ip))));
        // uart_write_bytes(UART_NUM_0, buf, strlen(buf));
        
        // bzero(buf, sizeof(buf));
        // strcpy(buf, "崔红强");
        // for (size_t i = 0; i < strlen("崔红强"); i++)
        // {
        //     printf("%02X, ", buf[i]);
        // }

        // uart_write_bytes(UART_NUM_0, buf, strlen(buf));
        // bzero(buf, sizeof(buf));
        // strcpy(buf, GB_);

        // for (size_t i = 0; i < strlen(GB_); i++)
        // {
        //     printf("%02X, ", buf[i]);
        // }
        // uart_write_bytes(UART_NUM_0, buf, strlen(buf));
        /* 获取连接到的信息 */
        /* uart_write_bytes(UART_NUM_0, "\r\nGet Connected to [SSID:]\r\n", strlen("\r\nWiFi Connected to [SSID:]\r\n")); */
        break;
	case SYSTEM_EVENT_STA_CONNECTED:
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_CONNECTED");
        /* 连接到AP */
        // uart_write_bytes(UART_NUM_0, "\r\nWiFi Connected to ap\r\n", strlen("\r\nWiFi Connected to ap\r\n"));
        uint8_t Cmd_Query_Buf[] = {0x1B, 0xF3};
        uart_write_bytes(UART_NUM_0, (const char *)Cmd_Query_Buf, sizeof(Cmd_Query_Buf));
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        ESP_LOGE(TAG, "Disconnect reason : %d", info->disconnected.reason);
        if (info->disconnected.reason == WIFI_REASON_BASIC_RATE_NOT_SUPPORT) {
            /*Switch to 802.11 bgn mode */
            esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCAL_11B | WIFI_PROTOCAL_11G | WIFI_PROTOCAL_11N);
        }
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
		cnt++;
        if(cnt >= 2) {
            xTaskCreate(smartconfig_example_task, "smartconfig_example_task", 4096, NULL, 3, NULL);
            cnt = 0;
        }
        break;
    default:
        break;
    }
    return ESP_OK;
}

// void init_wifi_config(void)
// {
//     uint64_t ssid_number = 0;

//     HAL_GetProductInt(&ssid_number, "ssidNumber");

//     if (ssid_number == 0)
//     {
//         wifi_config_t switch_wifi_config;

//         strncpy((char *) switch_wifi_config.sta.ssid, "sctestwifi", sizeof(switch_wifi_config.sta.ssid));
//         strlcpy((char *) switch_wifi_config.sta.password, "12345678", sizeof(switch_wifi_config.sta.password));
//         save_wifi_config(&switch_wifi_config);
//     }
// }

static void initialise_wifi(void)
{
    wifi_config_t wifi_config;
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

	memset(&wifi_config, 0x0, sizeof(wifi_config));
    read_wifi_init_sta_info1(&wifi_config);

    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
	
    ESP_LOGI(TAG, "wifi_config.sta.ssid:%d", strlen((char *)wifi_config.sta.ssid));
	
    if(strlen((char *)wifi_config.sta.ssid) == 0) {
        // init_data_flag = 1;
        // wifi_config_t switch_wifi_config;

        // // strncpy((char *) switch_wifi_config.sta.ssid, "sctestwifi", sizeof(switch_wifi_config.sta.ssid));
        // // strlcpy((char *) switch_wifi_config.sta.password, "12345678", sizeof(switch_wifi_config.sta.password));
        // // save_wifi_config(&switch_wifi_config);
        // write_wifi_init_sta_info1(&switch_wifi_config);

        // ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &switch_wifi_config) );

        // ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
        wifi_config_t wifi_default_config = {
            .sta = {
                .ssid = "sctestwifi",
                .password = "12345678",
            },
        };
        ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_default_config.sta.ssid);
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_default_config));

        write_wifi_init_sta_info1(&wifi_default_config);
    }
    else
    {
        /* code */
        ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    }
    
    ESP_ERROR_CHECK( esp_wifi_start() );
}

static void sc_callback(smartconfig_status_t status, void *pdata)
{
    switch (status) {
        case SC_STATUS_WAIT:
            ESP_LOGI(TAG, "SC_STATUS_WAIT");
            break;
        case SC_STATUS_FIND_CHANNEL:
            ESP_LOGI(TAG, "SC_STATUS_FINDING_CHANNEL");
            /* 开始配网，用微信扫描二维码 */
            // uart_write_bytes(UART_NUM_0, "\r\nSC_STATUS_FINDING_CHANNEL\r\n", strlen("\r\nSC_STATUS_FINDING_CHANNEL\r\n"));
			// uart_write_bytes(UART_NUM_0, "\r\nScan QR code with WeChat\r\n", strlen("\r\nScan QR code with WeChat\r\n"));
            uint8_t Cmd_Query_Buf[] = {0x1B, 0xF2};
            uart_write_bytes(UART_NUM_0, (const char *)Cmd_Query_Buf, sizeof(Cmd_Query_Buf));
            break;
        case SC_STATUS_GETTING_SSID_PSWD:
            ESP_LOGI(TAG, "SC_STATUS_GETTING_SSID_PSWD");
            /* 获取SSID_PSWD */
            /* uart_write_bytes(UART_NUM_0, "\r\nSC_STATUS_GETTING_SSID_PSWD\r\n", strlen("\r\nSC_STATUS_GETTING_SSID_PSWD\r\n")); */
			// uart_write_bytes(UART_NUM_0, "\r\nGet SSID and PSWD\r\n", strlen("\r\nGet SSID and PSWD\r\n"));
            // uint8_t Cmd_Query_Buf[] = {0x1B, 0xF1};
            // uart_write_bytes(UART_NUM_0, (const char *)Cmd_Query_Buf, sizeof(Cmd_Query_Buf));
            break;
        case SC_STATUS_LINK:
            ESP_LOGI(TAG, "SC_STATUS_LINK");
			/* 开始连接AP */
            /* uart_write_bytes(UART_NUM_0, "\r\nSC_STATUS_LINK\r\n", strlen("\r\nSC_STATUS_LINK\r\n")); */
            wifi_config_t *wifi_config = pdata;
            ESP_LOGI(TAG, "SSID:%s", wifi_config->sta.ssid);
            ESP_LOGI(TAG, "PASSWORD:%s", wifi_config->sta.password);
            write_wifi_init_sta_info1(wifi_config);
            ESP_ERROR_CHECK(esp_wifi_disconnect());
            ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_config) );
            ESP_ERROR_CHECK( esp_wifi_connect() );
            break;
        case SC_STATUS_LINK_OVER:
            ESP_LOGI(TAG, "SC_STATUS_LINK_OVER");
            if (pdata != NULL) {
                sc_callback_data_t *sc_callback_data = (sc_callback_data_t *)pdata;
                switch (sc_callback_data->type) {
                    case SC_ACK_TYPE_ESPTOUCH:
                        ESP_LOGI(TAG, "Phone ip: %d.%d.%d.%d", sc_callback_data->ip[0], sc_callback_data->ip[1], sc_callback_data->ip[2], sc_callback_data->ip[3]);
                        ESP_LOGI(TAG, "TYPE: ESPTOUCH");
                        break;
                    case SC_ACK_TYPE_AIRKISS:
                        ESP_LOGI(TAG, "TYPE: AIRKISS");
                        break;
                    default:
                        ESP_LOGE(TAG, "TYPE: ERROR");
                        break;
                }
            }
            xEventGroupSetBits(wifi_event_group, ESPTOUCH_DONE_BIT);
            break;
        default:
            break;
    }
}

void smartconfig_example_task(void * parm)
{
    EventBits_t uxBits;
    ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_AIRKISS) );
    ESP_ERROR_CHECK( esp_smartconfig_start(sc_callback) );
    while (1) {
        uxBits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY); 
        if(uxBits & CONNECTED_BIT) {
            ESP_LOGI(TAG, "WiFi Connected to ap");
        }
        if(uxBits & ESPTOUCH_DONE_BIT) {
            ESP_LOGI(TAG, "smartconfig over");
			/* 配网结束 */
            // uart_write_bytes(UART_NUM_0, "\r\nsmartconfig over\r\n", strlen("\r\nsmartconfig over\r\n"));
            uint8_t Cmd_Query_Buf[] = {0x1B, 0xF1};
            uart_write_bytes(UART_NUM_0, (const char *)Cmd_Query_Buf, sizeof(Cmd_Query_Buf));
			vTaskDelay(1000 / portTICK_PERIOD_MS);
			xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
            esp_smartconfig_stop();
            vTaskDelete(NULL);
        }
    }
}

void smartconfig_task_create(void)
{
    initialise_wifi();
}
