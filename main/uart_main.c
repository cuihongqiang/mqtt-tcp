// Copyright 2018-2025 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "driver/uart.h"
#include "esp_log.h"
#include "aliyun_mqtt.h"
static const char *TAG = "uart";
extern const int SIGN_BIT;
extern EventGroupHandle_t wifi_event_group;
/**
 * This is an example which echos any data it receives on UART0 back to the sender,
 * with hardware flow control turned off. It does not use UART driver event queue.
 *
 * - Port: UART0
 * - Receive (Rx) buffer: on
 * - Transmit (Tx) buffer: off
 * - Flow control: off
 * - Event queue: off
 */

#define BUF_SIZE (1024 * 2)

static void uart_init(void)
{
#if CONFIG_UART0_SWAP_IO
	uart_enable_swap();
    // Configure parameters of an UART driver,
    // communication pins and install the driver
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_CTS
    };
#else
    uart_config_t uart_config = {
        .baud_rate = 74880,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
#endif
    uart_param_config(UART_NUM_0, &uart_config);
    uart_driver_install(UART_NUM_0, BUF_SIZE * 2, 0, 0, NULL);    
}

static void uart_task()
{
    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

    while (1) {
        // Read data from the UART
        int len = uart_read_bytes(UART_NUM_0, data, BUF_SIZE, 20 / portTICK_RATE_MS);
        
		// if (!len)
        // {
        //     /* code */
        //     continue;
        // }

        if (len)
        {
            /* code */
            
            
            /* 1D 28 7D F0 00 01 00 00 */
            if (data[0] == 0x1D && data[1] == 0x28 && data[2] == 0x7D && data[3] == 0xF0)
            {
            //     ESP_LOGI(TAG, "len%d", len);

            // for (size_t i = 0; i < len; i++)
            // {
            //     ESP_LOGI(TAG, "0x%02x", data[i]);
            // }
                /* code */
                uint16_t rcv_len = data[6] + data[7] * 256;
                if (data[4] == 0x00 && len == (rcv_len + 8))
                {
                    /* code */
                    switch (data[5])
                    {
                    case 0x01:
                        /* code */
                        aliyun_config.sn_len = rcv_len;
                        for (size_t i = 0; i < rcv_len; i++)
                        {
                            /* code */
                            aliyun_config.sn[i] = data[i + 8];
                        }
                        ESP_LOGI(TAG, "aliyun_config.sn");
                        break;
                    case 0x02:
                        /* code */
                        aliyun_config.private_key_len = rcv_len;
                        for (size_t i = 0; i < rcv_len; i++)
                        {
                            /* code */
                            aliyun_config.private_key[i] = data[i + 8];
                        }
                        ESP_LOGI(TAG, "aliyun_config.private_key%u", aliyun_config.private_key_len);
                        break;
                    case 0x03:
                        /* code */
                        aliyun_config.certificate_len = rcv_len;
                        for (size_t i = 0; i < rcv_len; i++)
                        {
                            /* code */
                            aliyun_config.certificate[i] = data[i + 8];
                        }
                        ESP_LOGI(TAG, "aliyun_config.certificate%u", aliyun_config.certificate_len);
                        break;
                    case 0x04:
                        /* code */
                        aliyun_config.version_len = rcv_len;
                        for (size_t i = 0; i < rcv_len; i++)
                        {
                            /* code */
                            aliyun_config.version[i] = data[i + 8];
                        }
                        ESP_LOGI(TAG, "aliyun_config.version");
                        break;
                    case 0x05:
                        /* code */
                        aliyun_config.firmid_len = rcv_len;
                        for (size_t i = 0; i < rcv_len; i++)
                        {
                            /* code */
                            aliyun_config.firmid[i] = data[i + 8];
                        }
                        ESP_LOGI(TAG, "aliyun_config.firmid");
                        break;
                    case 0x06:
                        /* code */
                        aliyun_config.model_len = rcv_len;
                        for (size_t i = 0; i < rcv_len; i++)
                        {
                            /* code */
                            aliyun_config.model[i] = data[i + 8];
                        }
                        ESP_LOGI(TAG, "aliyun_config.model");
                        xEventGroupSetBits(wifi_event_group, SIGN_BIT);
                        break;

                    default:
                        break;
                    }
                }
                
            }
            
        }
        
		
		// Write data back to the UART
        // uart_write_bytes(UART_NUM_0, (const char *) data, len);
		// esp_mqtt_client_publish(client1, aliyun_config.topic, (const char *) data, len, 1, 0);
    }
}

void uart_task_create(void)
{
    uart_init();
    xTaskCreate(uart_task, "uart_task", 1024 * 4, NULL, 10, NULL);
}
