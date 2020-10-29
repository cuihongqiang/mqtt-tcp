/* HTTPS GET Example using plain mbedTLS sockets
 *
 * Contacts the howsmyssl.com API via TLS v1.2 and reads a JSON
 * response.
 *
 * Adapted from the ssl_client1 example in mbedtls.
 *
 * Original Copyright (C) 2006-2016, ARM Limited, All Rights Reserved, Apache 2.0 License.
 * Additions Copyright (C) Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD, Apache 2.0 License.
 *
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"

#include "nvs_flash.h"

#include <netdb.h>
#include <sys/socket.h>

#define _BMP "Qk0uEAAAAAAAAD4AAAAoAAAAgAEAAFUAAAABAAEAAAAAAPAPAADEDgAAxA4AAAAAAAAAAAAAAAAAAP///wD////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////wAAOAeAfAMAAD//////////////////////////////////////h/H+HDh/j4Ah/gAAGAOAPAEAAAf/////////////////////////////8AAA///8AHH+HDh/hwAh/gAAGAOAPAGAAAf///////////////////////gAAAD/AAAAH//wADD+HDh/hwAR/AAACAOAPAGAAAH///////////////////////AAAAB+AAAAD//gABD/Dhw/hh/w/ABgCAPAHAGAAAH////////////////8AAP//+AAAAB8AAAAD//A/5j/Dhw/hh/w/ADgDAHAHgDAAAD////////////////wAAf//+AAAAB4AAAAD//B//j/jxw/wx/w/gDgDAHgHgD//gD///////////////+AAA///8AAAAB4AAAAD//B//h/jxw/wwf4fgDgDgHgDgD//gD///////////////wAAHwH/8AAAABwAAAAD/+D//h/hx4fw4D4fgDwBgDgDgD//AD///////////////AAAPAA/4AAAABwAAAAD/+D//w/hw4fw8AAfgBwBgDgDwB+AAD//////////////8AAA+AA/4AAAABwAAAAH/+D//w/x48f4fgAfgBwBgDwDwB8AAH//////////////4AAB4AAf4AAAADgAAAAH/+D//w/w4cf4f8APwBwBwDwBwBwAAH//////////////gAADwAAPwAAAADgAAAAH/+D//w/w4cf8f/gPwB4AwBwBwBwAAP/////////////+AAAHAAAHwAAAADAAAAAP/+D//4fw4cP8f/8PwB4AwBwB4AwAA//////////////+AAAOAAAHwAAAAHAAAAAf//D//4Pw4cH8f/8PwA4AwB4B4AwB///////////////4AAAcAAADgAAAAHAAAAAf//B//4Bg8OAQfP8P4A4AwB4A4AwD///////////////wAAA8AAADgAAAAGAAAAAf//B//4AA8OAAfAAP4A8AYA4A4AQD///////////////gAABwAAAHAAAAAMAAAAA///g//8IB8OEA/AAP4A8AYAwAQAQAAD/////////////AAADwAAAPAAAAAcAAAAA///gf/8P//////8B/4A+AYAAAAAQAAD////////////+AAADgAAAOAAAAAcAAAAA///wP/8P/////////4AeAMAAAAAwAAD////////////8AAAHAAAAOAAAAAYAAAAB///4D/uP/+H//////8AeAGAAAAB8AAB////////////4AAAOAAAAcAAAAA4AAAAB///8AAOH/+H//////+AOADAAAAD+AAB////////////4AAAcAAAAcAAAAA4AAAAB///+AAOH//H/////////////////wAB////////////wAAAcAAAAcAAAAA4AAAAD////gAOH//D////////////////////////////////AAAA4AAAA4AAAABwAAAAD////+Af////////////////////////////////////AAAA/gAAA4AAAABwAAAAH//////////////////////////////////////////+AAAB/wAAA4AAAABwAAAAH//////////////////////////////////////////+AAAB/8AABwAAAADgAAAAH///8AMABAP/8AAP4D/////g/AHwAAAP///////////8AAAB/8AADwAAAAHAAAAAP///4AIACAcf8AAHwDgEAAAgPgB4AAAH///////////4AAAD/+AADgAAAAHAAAAAP///4AIACAcD8AADwHgEAAAgHwB4AAAD///////////wAAAH//AADgAAAAHAAAAAf///wAIAEA8A8BwDwHgEAAAgH4A8AAAD///////////wAAAH//AADgAAAAOAAAAAf///wH4D8A8AEB4BgHgEAAAgH4A///gD///////////gAAAP//gAHAAAAAOAAAAAf///wH4D4B8AAB4BAPgED/ggH8Af//gD///////////gAAAP//gAHAAAAAMAAAAA////wH4DwA8AAB4BAPgED/ggH8AP//gD///////////AAAAP//wAPAAAAAcAAAAB////wH4DgA/AAB8BAfgED/ggH/AH//gD///////////AAAAf//wAOAAAAAcAAAAB////wH4DgA/AAB8AAfgED/ggH/AD//gD//////////+AAAAf//wAOAAAAAYAAAAB////wH4DAA/AYB8AAfgED/ggHgAAAAAD//////////8AAAAf//wAcAAAAA4AAAAD////wH4CAAfAcB8AA/gEAAAgHgAAAAAD//////////8AAAA///4A8AAAABwAAAAD////wH4CAgfAcB8AB/gEAAAgHgAAAAAD//////////4AAAA///4A4AAAABwAAAAD////wH4AAwfAcB+AB/gEAAAgHgAAAAAD//////////4AAAB///4A4AAAABwAAAAH///+AAYABwPAcB+Af/gEAAAgH/4Af/////////////wAAAB///4BwAAAADgAAAAH///+AAYD///AAAAAAHgH///gHAAD/+AAf/////////wAAAB///4BwAAAADgAAAAH///+AAYD///AAAAAAHgAD/AAHAAAHwAAf/////////wAAAB///4BwAAAADgAAAAP///+AAYAAA/AAAAAAHgAAcAAHAAAAAAAf/////////gAAAD///8BgAAAAHAAAAAP////wH4AAAfAAAAAAHgAAYAAHAAAAAAAf/////////AAAAD///8DgAAAAPAAAAAf///wwH4AAAfAcB/Af/gAAYAAHgAAAAAA//////////AAAAD///8DgAAAAPAAAAAf///wQH4AAAfAcB/Af/gHAYDgH//wAA////////////AAAAD///8DAAAAAOAAAAAf///wAAYD/gPAcB/Af/gGAcBgH//4AB///////////+AAAAH///+HAAAAAcAAAAA////8AAYD/gPAcB/Af/gGA8AgH//8AB///////////+AAAAH///+OAAAAAcAAAAA////+AAYAAAPAABAAAPgEA+AAHA/4AAf//////////+AAAAH///+OAAAAAcAAAAB////+B/4AAAPAABAAAPgEB+AAHAfwAAf//////////8AAAAH///+cAAAAA4AAAAB////+A/4AAAPAABAAAPgAD/AAHAHgAAH//////////4AAAAH////cAAAAA4AAAAB/////Af4AAAfAABAAAPgAD/gAH4AAAAD//////////4AAAAP////4AAAADgAAAAH/////AAYAAAfAcB4ED//wH/gH/8AAAAB//////////4AAAAP////4AAAAHgAAAAP/////AAYD/gfAcBwGB//gH/wD/+AAOAA//////////wAAAAf////wAAAAPAAAAAf/////gAYD/gfAcBwOA+AAAAAAA/AAPAAf/////////wAAAAf/////////////////////gAYAAAfAcBgOA+AAAAAAA/AAAAAf/////////wAAAAf/////////////////////gH4AAAcAABAOAeAAAAAAA/gAAAAf/////////wAAAAf/////////////////////gH4AAAcAABAfAeAAAAAAA/wAAAAf/////////gAAAAf/////////////////////gD4AAAcAABAfAeAAAAAAA/wAAAAP/////////gAAAAf/////////////////////wD4AAAcAABAfAP//4Af///wAf////////////AAAAAf/////////////////////////////////////4Af///4Af////////////AAAAA//////////////////////////////////////////////////////////+AAAAA//////////////////////////////////////////////////////////+AAAAA//////////////////////////////////////////////////////////+AAAAA//////////////////////////////////////////////////////////+AAAAA//////////////////////////////////////////////////////////8AAAAA//////////////////////////////////////////////////////////8AAAAA//////////////////////////////////////////////////////////8AAAAP//////////////////////////////////////////////////////////8AAAA///////////////////////////////////////////////////////////4AAAP///////////////////////////////////////////////////////////4AAD////////////////////////////////////////////////////////////4AB/////////////////////////////////////////////////////////////4AH/////////////////////////////////////////////////////////////wB//////////////////////////////////////////////////////////////wf///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////8="


extern void https_task_create(void);
extern void uart_task_create(void);
extern void smartconfig_task_create(void);
extern void mqtt_app_start(void);

esp_err_t parse_bmp(const unsigned char *src, size_t slen);

static const char *TAG = "MAIN";
// void uart_event_task(void *pvParameters);

void app_main(void)
{
	ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);
	
    ESP_ERROR_CHECK( nvs_flash_init() );
	/* 以上是初始化 */
	/* 以下是创建三个任务 */
    uart_task_create();
	smartconfig_task_create();
	https_task_create();
    mqtt_app_start();
    // xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, 12, NULL);
    // parse_bmp((uint8_t *)_BMP, strlen(_BMP));
}
