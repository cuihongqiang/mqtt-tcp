#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
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
#include "mqtt_client.h"

#include "driver/uart.h"
#include "aliyun_mqtt.h"

extern EventGroupHandle_t wifi_event_group;
extern const int CONNECTED_MQTT_BIT;

aliyun_config_t aliyun_config;

static const char *TAG = "MQTT_EXAMPLE";

esp_mqtt_client_handle_t client1;

uint8_t mqtt_message_buf[MQTT_MESSAGE_BUF_LEN];

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    client1 = event->client;
    int msg_id;
    static uint8_t flag = 0;
    
    // test_uart_event_t uart_event;

    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
			/* 连接到MQTT */
            // uart_write_bytes(UART_NUM_0, "\r\nMQTT_EVENT_CONNECTED\r\n", strlen("\r\nMQTT_EVENT_CONNECTED\r\n"));
            // msg_id = esp_mqtt_client_publish(client, "/a1dYbQxiTv2/ESP8266_LED/user/update", "data_3", 0, 1, 0);
            // ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

            /* OTA */
            ESP_LOGI(TAG, "VERSION_INFO:[%s]", PRE_UPGRADE_VERSION);
            msg_id = esp_mqtt_client_publish(client, aliyun_config.ota_device_inform, PRE_UPGRADE_VERSION, strlen(PRE_UPGRADE_VERSION), 1, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

            /* OTA */
            msg_id = esp_mqtt_client_subscribe(client, aliyun_config.ota_device_upgrade, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            // msg_id = esp_mqtt_client_subscribe(client, "/a1dYbQxiTv2/ESP8266_LED/user/get", 0);
            msg_id = esp_mqtt_client_subscribe(client, aliyun_config.subscribe_topic, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%s", aliyun_config.subscribe_topic);

            // msg_id = esp_mqtt_client_subscribe(client, "/a1dYbQxiTv2/ESP8266_LED/user/update", 1);
            // ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            // msg_id = esp_mqtt_client_unsubscribe(client, "/a1dYbQxiTv2/ESP8266_LED/user/update");
            // ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
            ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());

            uint8_t Cmd_Query_Buf[] = {0x1B, 0xF0};
            uart_write_bytes(UART_NUM_0, (const char *)Cmd_Query_Buf, sizeof(Cmd_Query_Buf));
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            /* 连接到MQTT失败 */
            // uart_write_bytes(UART_NUM_0, "\r\nMQTT_EVENT_DISCONNECTED\r\n", strlen("\r\nMQTT_EVENT_DISCONNECTED\r\n"));
            uint8_t Cmd_Query_Buf1[] = {0x1B, 0xF6};
            uart_write_bytes(UART_NUM_0, (const char *)Cmd_Query_Buf1, sizeof(Cmd_Query_Buf1));
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            // msg_id = esp_mqtt_client_publish(client, "/a1dYbQxiTv2/ESP8266_LED/user/get", "data", 0, 0, 0);
            // ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            
            ESP_LOGW(TAG, "event->total_data_len:%u", event->total_data_len);
            ESP_LOGW(TAG, "event->current_data_offset:%u", event->current_data_offset);
            ESP_LOGW(TAG, "event->data_len:%u", event->data_len);

            if (flag == 0)
            {
                uart_write_bytes(UART_NUM_0, event->topic, event->topic_len);
                flag = 1;
            }

            if ((event->topic_len > 0) && (strncmp(event->topic, aliyun_config.ota_device_upgrade, event->topic_len) == 0) && (event->total_data_len == event->current_data_offset + event->data_len))
			{
                ESP_LOGI(TAG, "ota_device_upgrade");
                // uart_write_bytes(UART_NUM_0, event->data, event->data_len);
				// parse_json_aliyun_update_objects(event->data, &aliyun_config);
				// ota_task_create();
				// esp_mqtt_client_publish(client, aliyun_config.ota_device_progress, UPGRADE_PROGRESS, strlen(UPGRADE_PROGRESS), 1, 0);
				// esp_mqtt_client_publish(client, aliyun_config.ota_device_inform, POST_UPGRADE_VERSION, strlen(POST_UPGRADE_VERSION), 1, 0);
			}
            else //if (strncmp(event->topic, aliyun_config.subscribe_topic, event->topic_len) == 0)
            {
                // if (strncmp(event->topic, aliyun_config.subscribe_topic, event->topic_len) == 0)
                // {
                //     
                //     strncpy((char *)&mqtt_message_buf[event->current_data_offset], event->data, event->data_len);
                // }
                // else
                // {
                //     strncpy((char *)&mqtt_message_buf[event->current_data_offset], event->data, event->data_len);
                // }

                if (event->current_data_offset == 0)
                {
                    bzero(mqtt_message_buf, MQTT_MESSAGE_BUF_LEN);
                }
                

                strncpy((char *)&mqtt_message_buf[event->current_data_offset], event->data, event->data_len);

                if (event->total_data_len == event->current_data_offset + event->data_len)
                {
                    /* code */
                    // ESP_LOGI(TAG, "ESP_:%s", mqtt_message_buf);
                    // uart_write_bytes(UART_NUM_0, (char *)mqtt_message_buf, strlen((char *)mqtt_message_buf));
                    // if (parse_print_json_objects((char *)&mqtt_message_buf) != ESP_OK)
                    // {
                    //     // uart_write_bytes(UART_NUM_0, event->data, event->data_len);
                    //     ESP_LOGI(TAG, "ESP_FAIL");
                    // }
                    // else
                    // {
                    //     ESP_LOGI(TAG, "ESP_OK");
                    // }
                    // \r\n+QMTRECV: 0,0,/a1dYbQxiTv2/ESP8266_BUTTON/user/get,
                    
                    // uart_write_bytes(UART_NUM_0, buf, strlen(buf));
                    uart_write_bytes(UART_NUM_0, ",", strlen(","));
                    uart_write_bytes(UART_NUM_0, (char *)mqtt_message_buf, strlen((char *)mqtt_message_buf));
                    uart_write_bytes(UART_NUM_0, "\r\n", strlen("\r\n"));
                    flag = 0;
                }

                

                ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());

                // parse_json_data_objects(event->data, &aliyun_config);

                // uart_write_bytes(UART_NUM_0, (const char *)aliyun_config.order_data, aliyun_config.order_data_len);
                // /* 查询打印完成 */
                // /* 查询打印是否完成 */
                // uint8_t Cmd_Query_Buf[] = {0x1D, 0x28, 0x48, 0x06, 0x00, 0x30, 0x30, 0x20, 0x21, 0x22, 0x23};
                // uart_write_bytes(UART_NUM_0, (const char *)Cmd_Query_Buf, sizeof(Cmd_Query_Buf));
                // /* 打印完成后，上报打印完成 */
            }
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
    }
    return ESP_OK;
}

void mqtt_app_start(void)
{
    xEventGroupWaitBits(wifi_event_group, CONNECTED_MQTT_BIT,
                            false, true, portMAX_DELAY);
        ESP_LOGI(TAG, "Connected to AP");
#if !defined (ESP8266_HTTPS_POST_CUP)
    sprintf(aliyun_config.product_key, "%s", PRODUCT_KEY);
    sprintf(aliyun_config.device_name, "%s", DEVICE_NAME);
    sprintf(aliyun_config.device_secret, "%s", DEVICE_SECRET);
    sprintf(aliyun_config.subscribe_topic, SUBSCRIBE_TOPIC, aliyun_config.product_key, aliyun_config.device_name);
    sprintf(aliyun_config.publish_topic, PUBLISH_TOPIC, aliyun_config.product_key, aliyun_config.device_name);
#endif
    
    /* OTA */
    sprintf(aliyun_config.ota_device_inform, OTA_DEVICE_INFORM, aliyun_config.product_key, aliyun_config.device_name);
    sprintf(aliyun_config.ota_device_progress, OTA_DEVICE_PROGRESS, aliyun_config.product_key, aliyun_config.device_name);
    sprintf(aliyun_config.ota_device_upgrade, OTA_DEVICE_UPGRADE, aliyun_config.product_key, aliyun_config.device_name);

	gen_aliyun_mqtt_info(&aliyun_config);
	
    esp_mqtt_client_config_t mqtt_cfg = {
        .event_handle = mqtt_event_handler,
        // .host = "192.168.1.103",
        .host = aliyun_config.device_server,
        .port = MQTT_PORT,
        .client_id = aliyun_config.device_id,
        .username = aliyun_config.device_username,
        .password = aliyun_config.device_password,
        .keepalive = 60,
        // .user_context = (void *)your_context
    };

#if CONFIG_BROKER_URL_FROM_STDIN
    char line[128];

    if (strcmp(mqtt_cfg.uri, "FROM_STDIN") == 0) {
        int count = 0;
        printf("Please enter url of mqtt broker\n");
        while (count < 128) {
            int c = fgetc(stdin);
            if (c == '\n') {
                line[count] = '\0';
                break;
            } else if (c > 0 && c < 127) {
                line[count] = c;
                ++count;
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        mqtt_cfg.uri = line;
        printf("Broker url: %s\n", line);
    } else {
        ESP_LOGE(TAG, "Configuration mismatch: wrong broker url");
        abort();
    }
#endif /* CONFIG_BROKER_URL_FROM_STDIN */

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    uint16_t i = 0;
    esp_err_t res;

    // do
    // {
    //     /* code */
    //     i++;
        vTaskDelay(3000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());

        res = esp_mqtt_client_start(client);
    //     if (res == ESP_OK)
    //     {
    //         i = 10;
    //     }

    // } while (i < 10);
}
