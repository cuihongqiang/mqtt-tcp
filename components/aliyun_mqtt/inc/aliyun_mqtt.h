/* aliyun_mqtt.h -- VERSION 1.0
 *
 * Guerrilla line editing library against the idea that a line editing lib
 * needs to be 20,000 lines of C code.
 *
 * See aliyun_mqtt.c for more information.
 *
 * ------------------------------------------------------------------------
 *
 * Copyright (c) 2010-2014, Salvatore Sanfilippo <antirez at gmail dot com>
 * Copyright (c) 2010-2013, Pieter Noordhuis <pcnoordhuis at gmail dot com>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  *  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  *  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __ALIYUN_MQTT_H
#define __ALIYUN_MQTT_H

#ifdef __cplusplus
extern "C" {
#endif

/* 设备签到参数 */
#define SN      "B02202005071033000"
#define FIRMID  "10019"
#define MODEL   "DM30"
#define VERSION "1.0.5"

#define ALIYUN_CFG_LEN  128

typedef struct
{
    char product_key[ALIYUN_CFG_LEN];
    char device_name[ALIYUN_CFG_LEN];
    char device_secret[ALIYUN_CFG_LEN];
    // char topic[ALIYUN_CFG_LEN];
    char subscribe_topic[ALIYUN_CFG_LEN];
    char publish_topic[ALIYUN_CFG_LEN];
    char device_id[ALIYUN_CFG_LEN];
    char device_username[ALIYUN_CFG_LEN];
    char device_password[ALIYUN_CFG_LEN];
	char device_server[ALIYUN_CFG_LEN];
	char device_ota_url[ALIYUN_CFG_LEN * 4];
	char ota_device_inform[ALIYUN_CFG_LEN];  // /ota/device/inform/${YourProductKey}/${YourDeviceName}
	char ota_device_upgrade[ALIYUN_CFG_LEN]; // /ota/device/upgrade/${YourProductKey}/${YourDeviceName}
	char ota_device_progress[ALIYUN_CFG_LEN];// /ota/device/progress/${YourProductKey}/${YourDeviceName}
    char sn[ALIYUN_CFG_LEN / 4];    // 保存sn号或者待签名字符串
    uint16_t sn_len;
    char private_key[ALIYUN_CFG_LEN * 16];
    uint16_t private_key_len;
    char certificate[ALIYUN_CFG_LEN * 16];
    uint16_t certificate_len;
    char version[ALIYUN_CFG_LEN / 4];    // 保存sn号或者待签名字符串
    uint16_t version_len;
    char firmid[ALIYUN_CFG_LEN / 4];    // 保存sn号或者待签名字符串
    uint16_t firmid_len;
    char model[ALIYUN_CFG_LEN / 4];    // 保存sn号或者待签名字符串
    uint16_t model_len;
} aliyun_config_t;

typedef struct
{
    uint8_t printer_status;
    uint8_t offline_status;
    uint8_t error_status;
    uint8_t sensor_status;
    uint8_t up_status;
} query_status_result_t;

#define MQTT_MESSAGE_BUF_LEN (10 * 1024)

extern aliyun_config_t aliyun_config;
extern query_status_result_t query_status_result;
// extern TimerHandle_t timer_heartbeat;

#define UPDAV0
// #define UPDAV1

#if defined (UPDAV0)
    /* 升级之前版本 */
    #define PRE_UPGRADE_VERSION    "{\"id\":\"1\",\"params\":{\"version\":\"1.0.0\"}}"
    /* 升级之后版本 */
    #define POST_UPGRADE_VERSION   "{\"id\":\"1\",\"params\":{\"version\":\"1.0.1\"}}"
#elif defined (UPDAV1)
    /* 升级之前版本 */
    #define PRE_UPGRADE_VERSION    "{\"id\":\"1\",\"params\":{\"version\":\"1.0.1\"}}"
    /* 升级之后版本 */
    #define POST_UPGRADE_VERSION   "{\"id\":\"1\",\"params\":{\"version\":\"1.0.0\"}}"
#endif

/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "cloudspeaker.chinaums.com"
// #define WEB_SERVER "qr-test1.chinaums.com"
#define WEB_PORT "443"
// #define WEB_URL "https://cloudspeaker.chinaums.com/uisiotfront/box/signIn"

#define POST_REQUEST_TEST "POST /uisiotfront/box/signIn HTTP/1.1\r\n\
Host: %s\r\n\
Content-Type: application/json;charset=utf-8\r\n\
Content-Length: %d\r\n\
Connection: close\r\n\
Cache-Control: no-cache\r\n\
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/75.0.3770.142 Safari/537.36\r\n\
Accept: */*\r\n\
Accept-Encoding: gzip, deflate, br\r\n\
Accept-Language: zh-CN,zh;q=0.9\r\n\r\n\
%s"

#define POST_REQUEST_VALUE "{\"sn\":\"%s\",\"firmId\":\"%s\",\"model\":\"%s\",\"version\":\"%s\",\"sign\":\"%s\"}"

#define SIGN_STRING "firmId=%s&model=%s&sn=%s&version=%s"

/* 采用随机数 */
#define TIMESTAMP_STR	     "789"

/* 银联 */
// #define ESP8266_CUP
#define ESP8266_HTTPS_POST_CUP
// #define ESP8266_LED
// #define ESP8266_BUTTON
// #define ESP8266_TEST
#ifndef ESP8266_HTTPS_POST_CUP
#if CONFIG_UART0_SWAP_IO
#define ESP8266_BUTTON
#else
#define ESP8266_BUTTON
#endif
#endif /* ESP8266_HTTPS_POST_CUP */

/* 阿里云三元组 */
/* 设备证书信息 */
/* #define PRODUCT_KEY          "替换为设备的ProductKey"
#define DEVICE_NAME          "替换为设备的DeviceName"
#define DEVICE_SECRET        "替换为设备的DeviceSecret"
#define REGION_ID            "替换为设备所在地域ID如cn-shanghai" */
#if defined (ESP8266_HTTPS_POST_CUP)
/* https post request */
#endif

/* 订阅 */
/* 自定义 */
#if defined (ESP8266_LED) || defined (ESP8266_BUTTON) || defined (ESP8266_TEST)
/* #define TOPIC                "/" PRODUCT_KEY "/" DEVICE_NAME "/user/get" */
#define PUBLISH_TOPIC                    "/%s/%s/user/sprt/status"
#define SUBSCRIBE_TOPIC                  "/%s/%s/user/sprt/data"
/* 银联测试 */
#elif defined (ESP8266_CUP)
#define PUBLISH_TOPIC                    "/%s/%s/user/update"
#define SUBSCRIBE_TOPIC                  "/%s/%s/user/pb/notify"
#else
#define PUBLISH_TOPIC                    "/%s/%s/user/update"
#define SUBSCRIBE_TOPIC                  "/%s/%s/user/get"
#endif

/* OTA */
// /ota/device/inform/${YourProductKey}/${YourDeviceName}
// /ota/device/upgrade/${YourProductKey}/${YourDeviceName}
// /ota/device/progress/${YourProductKey}/${YourDeviceName}
#define OTA_DEVICE_INFORM                "/ota/device/inform/%s/%s"
#define OTA_DEVICE_UPGRADE               "/ota/device/upgrade/%s/%s"
#define OTA_DEVICE_PROGRESS              "/ota/device/progress/%s/%s"
#define UPGRADE_PROGRESS                 "{\"id\":\"1\",\"params\":{\"step\":\"100\",\"desc\":\" xxxxxxxx \"}}"

/* 唯一选择 */
#define REGION_ID            "cn-shanghai"

/* 线上环境域名和端口号，不需要改 */
#define MQTT_SERVER          "%s.iot-as-mqtt.%s.aliyuncs.com"
/* #define MQTT_SERVER          PRODUCT_KEY ".iot-as-mqtt." REGION_ID ".aliyuncs.com" */
#define MQTT_PORT            1883
/* #define MQTT_USRNAME         DEVICE_NAME "&" PRODUCT_KEY */

/* 阿里云ID */
/* #define DEVICE_ID		     PRODUCT_KEY"."DEVICE_NAME */
/* #define ALIYUN_CLIENT_ID	 DEVICE_ID"|securemode=3,signmethod=hmacmd5,timestamp=%s|" */
#define ALIYUN_CLIENT_ID	 "%s.%s|securemode=3,signmethod=hmacmd5,timestamp=%s|"

/* 阿里云用户名 */
#define ALIYUN_USERNAME	     "%s&%s"
/* #define ALIYUN_USERNAME	     DEVICE_NAME "&" PRODUCT_KEY */

/* 阿里云密码 */
// #define HASH_STR             "clientId"DEVICE_ID"deviceName"DEVICE_NAME"productKey"PRODUCT_KEY"timestamp%s"
#define HASH_STR             "clientId%s.%sdeviceName%sproductKey%stimestamp%s"

/* This is taken from tests/data_files/test-ca-sha256.crt. */
/* BEGIN FILE string macro TEST_CA_CRT_RSA_SHA256_PEM tests/data_files/test-ca-sha256.crt */

#define _TEST_KEY_RSA_SHA256_PEM                                         \
    "-----BEGIN PRIVATE KEY-----\n" \
    "MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQCDZvy7ISj+vb41\n" \
    "1UoDoZ2RHNcs0/Ia9q5apcg88KXjIgrNcwX0UJfHkntazPfF4qsCa5yaEPtHuoca\n" \
    "7NnwulfRjMYpkS6KDQ28fCLM3xj5IqzjDBS6yV6l2bqjwM4OeOJu8j97OZPQhzo2\n" \
    "HW8qElLdw5h35/UDOYTaF7hty5iqnV6xxmt3RRxUwLx4j4x1tMncLOE+J8oo/TMf\n" \
    "aIAsFlcjFZca2mL0FCp/kjI3hX3gs/mJBRCbBRHfMG60iHDny3aJ+FA3Bh9GriZj\n" \
    "Ta3Gq5/GLEjnFaUtpj3l4Va4CkyLm2YS+errAHykA3E1jD5wbiEtUf9v46TbsOV+\n" \
    "OVwBjYBnAgMBAAECggEADX5j8auEFDTT9Z9DjH/qOF6n0hipwg8W9IQVvfxQbLSw\n" \
    "O0zWjbJBA+OM+1QVFaMjQIpgeInJ5CHJggBUmR3pqsE0EEyyZMbWZEazJCq828it\n" \
    "zFsPPecz6wAi5HIGNwI+7nm9/x94iG96kNgQ9FcRX2iYnaeaR4x+kg7hh5TixpJg\n" \
    "1WdWEu5asErJ1CFzT/xUFh61LC7yK246dqH92wCBGFhi8AgpjahK+x/wywWfBgZb\n" \
    "0OBShC6A1pMRm3W4CZWjK95LbBeHocLkd3Lk75UmGm9AUQCtGbjr5+4Flg1cOVcd\n" \
    "mymjd/7nmuKRvnessk552zMESwC4czH3hnijTTiVIQKBgQC/MO7O8fxI3ts7rIaJ\n" \
    "unfYztKXm5pRwSaPwWQ/F5UAxapL4XPlUVRbIL64wesZE3lv2OdxqpNvBqP6zewz\n" \
    "zaPPdYbV1HNOUI4sMfaaZyz143mCg7VwvMdQ+UMsYZfTnvqoAycqm7nO7zTY4HVZ\n" \
    "Bn6a0UFgwOeuRByNRmx/DzILWQKBgQCv8bwD5kRGne7bJE/vlNUrYujOqQPBzkyj\n" \
    "VaF8w1EtQDmkheRqFzq6A813rElyIO/pf5K6FGPAUqf8XQMdQJYWjUY3kxMX0Pqw\n" \
    "111zl7ZwsPfmjyiWpDjc6moSJG2rQsXBjP/D0rkwpKwIM92kStn9IOtv+Kof+zmO\n" \
    "mLdq/QMxvwKBgGEhrAFpAOHIjpqXgNZR7HcyQ90QCWuFdGDOQG6pPWLiCS15wZZ8\n" \
    "Jh4R2bMlmZnoweYnzRV7MmHSftZ5bYm37IQGUlUqFNZxqHkdfQpeZoIZae77mN9I\n" \
    "mP4zVIQkpKy3dOKMj4ZfDRvrfO69wgBTg1iF/O/5sPpg0hyX7aDziFopAoGAR1AA\n" \
    "d8gmFkU/Id1m9OFrQWmWmOTSXARE9dLoYEw+I8wrUO7f9Mwzhl+yMPZI0pRdCVQm\n" \
    "4XsQL0yRP+1nLL0X2E4sjqvzCi0u3ux7uTVdwfFImU+fEOfBEHGGypGvTcIDq359\n" \
    "0B4h54BnJe/3vvaDwmBMbRbpxYqq+owVk6y3VbsCgYEAp3HSs2UC31u+rqmPsw0Z\n" \
    "9+t1RMed8zAGJfQU7r2+RBDIHQn53yaCEU6IaPj5YcR7sa7tuveMBlJGgn9KPiuy\n" \
    "lIa4xDq7iU5UkjLUhWauOMlYUQ4AL7yfgWpVSY51YDC75pu4KO9U+iet7trlkAxX\n" \
    "9DAfSAfDapII2VUTfGZncsw=\n" \
    "-----END PRIVATE KEY-----\n"
/* END FILE */


// AccessKeySecret
#define MY_KEY "z5xeZz1TqGE30WaCA0rFu9g95kOAr2FI"
#define HAM "clientIda1RoUO76vcW.ESP32_OTAdeviceNameESP32_OTAproductKeya1RoUO76vcWtimestamp789"

// #define TEST_VALUE "{\"context\":{\"pc\":[{\"cols\":[{\"cv\":\"一了双定时\"}],\"tp\":\"0\"},{\"tp\":\"3\"},{\"cols\":[{\"cv\":\"̻ƣ\"},{\"cv\":\"̻\"}],\"tp\":\"1\"},{\"cols\":[{\"cv\":\"̻ţ\"},{\"cv\":\"898310148164007\"}],\"tp\":\"1\"},{\"cols\":[{\"cv\":\"ն˱ţ\"},{\"cv\":\"00000001\"}],\"tp\":\"1\"},{\"cols\":[{\"cv\":\" RMB\"},{\"cv\":\"0.01Ԫ\"}],\"tp\":\"1\"},{\"cols\":[{\"cv\":\"Żݽ\"},{\"cv\":\"0.00Ԫ\"}],\"tp\":\"1\"},{\"cols\":[{\"cv\":\"ʱ䣺\"},{\"cv\":\"2020-03-07 12:46:57\"}],\"tp\":\"1\"},{\"cols\":[{\"cv\":\"֧\"},{\"cv\":\"΢\"}],\"tp\":\"1\"},{\"cols\":[{\"cv\":\"̻ţ\"},{\"cv\":\"100020030763846411240012870\"}],\"tp\":\"1\"},{\"cols\":[{\"cv\":\"ײοţ\"},{\"cv\":\"00689400313N\"}],\"tp\":\"1\"},{\"cols\":[{\"cv\":\"ԣ\"},{\"cv\":\"һβտ\"}],\"tp\":\"1\"},{\"al\":\"1\",\"cols\":[{\"cv\":\"https://service.chinaums.com/appresjump1.html\"}],\"tp\":\"2\"},{\"tp\":\"3\"},{\"tp\":\"4\"},{\"cols\":[{\"cv\":\": 95534\"},{\"al\":\"2\",\"cv\":\"ôӡ\"}],\"tp\":\"5\"}],\"sc\":\"收款totalAmount\"},\"msg_type\":\"2\",\"index\":\"20200307100066127\",\"version\":\"1.0.0\"}"

#define TEST_VALUE "{\"context\":{\"pc\":[{\"cols\":[{\"cv\":\"\"}],\"tp\":\"0\"},{\"tp\":\"3\"},{\"cols\":[{\"cv\":\"商户名称：\"},{\"cv\":\"广告黑名单测试商户\"}],\"tp\":\"1\"},{\"cols\":[{\"cv\":\"商户编号：\"},{\"cv\":\"898310148164007\"}],\"tp\":\"1\"},{\"cols\":[{\"cv\":\"终端编号：\"},{\"cv\":\"00000001\"}],\"tp\":\"1\"},{\"cols\":[{\"cv\":\"金额 RMB：\"},{\"cv\":\"0.01元\"}],\"tp\":\"1\"},{\"cols\":[{\"cv\":\"优惠金额：\"},{\"cv\":\"0.00元\"}],\"tp\":\"1\"},{\"cols\":[{\"cv\":\"交易时间：\"},{\"cv\":\"2020-03-05 17:35:29\"}],\"tp\":\"1\"},{\"cols\":[{\"cv\":\"支付渠道：\"},{\"cv\":\"微信\"}],\"tp\":\"1\"},{\"cols\":[{\"cv\":\"商户订单号：\"},{\"cv\":\"100020030595035181735470130\"}],\"tp\":\"1\"},{\"cols\":[{\"cv\":\"交易参考号：\"},{\"cv\":\"00688900048N\"}],\"tp\":\"1\"},{\"al\":\"1\",\"cols\":[{\"cv\":\"https://service.chinaums.com/appresjump1.html\"}],\"tp\":\"2\"},{\"tp\":\"3\"},{\"tp\":\"4\"},{\"cols\":[{\"cv\":\"服务热线: 95534\"},{\"al\":\"2\",\"cv\":\"银联商务悦打印\"}],\"tp\":\"5\"}],\"sc\":\"收款0.01元\"},\"msg_type\":\"2\",\"index\":\"20200305100065918\",\"version\":\"1.0.0\"}"

// #define _BMP "Qk2AGgAAAAAAAD4AAAAoAAAAgAEAAIwAAAABAAEAAAAAAEIaAADDDgAAww4AAAAAAAAAAAAA////AAAAAAD//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////wAAOAf//AGAAB///////////////////////////////////////////////wD//AAAOAOAPAGAAAP////////////////////////////+AACf///gBw/w4eP+PgDD+AAAGAPAHADAAAH//////////////////////gAAAH/gAAAD//+AAw/w8eP+PABD+AAAGAPAHgDAAAD/////////////////////+AAAAD+AAAAB//4AAY/48OH+GD/D+ACAGAPAHgDAAAB///////////////+AH///+AAAAB8AAAAB//wH8Yf4cOH+GH/h+AHAHAHAHgDgAAB///////////////AAA///8AAAAB8AAAAA//wf/4f4cPH/HH/h+APADAHgDwBgAgB//////////////4AAD///4AAAAB4AAAAB//gf/8f8ePD/DD/h+AHgDAHgDwB//4A//////////////AAAPw//4AAAAB4AAAAB//g//8P8OHD/Dg/h/AHgDgHgDwB//4A/////////////8AAA+AP/4AAAABwAAAAB//g//8P8OHD/DwDw/AHgDgDwDwB/wAB/////////////wAAD4AD/wAAAABwAAAAB//g//8P8OHj/j8AQ/ADgBgDwB4A/AAB/////////////gAAHwAB/wAAAADwAAAAD//h//+P+PHh/h/gA/ADwBgDwB4A8AAB////////////+AAAPAAA/gAAAADgAAAAD//h//+H+HDh/h/8A/gDwBwDwB4A8AAD////////////8AAAeAAAfgAAAAHgAAAAH//g//+H+HDh/h//gfgDwBwB4A4A4AAH////////////wAAA8AAAfAAAAAHAAAAAH//g//+D+Hjw/h//4fgB4AwB4A8AYAAf////////////gAAB4AAAPAAAAAHAAAAAH//g///B+HhwPh9/4fwB4AwB4A8AYA//////////////AAADwAAAPAAAAAPAAAAAP//wf//AAHhwAB8f4fwB4A4A4A8AYB/////////////+AAAHgAAAOAAAAAOAAAAAP//wf//AAPh4AB8AAfwB4A4A4AcAYB/////////////8AAAPAAAAeAAAAAeAAAAAf//4P//jAfx44D+AA/wA8AYAwAYAYAAA///////////4AAAOAAAAeAAAAAcAAAAAf//4H//j///////4D/4A8AcAAAAAcAAA///////////wAAAcAAAAcAAAAAcAAAAAf//8D//h//////////4A8AcAAAAAcAAA///////////gAAA8AAAA8AAAAA8AAAAA///+A/5h//w///////4A8AOAAAAA+AAA///////////AAAB4AAAA4AAAAA4AAAAA////AABx//wf//////4AeAHgAAAB/AAAf/////////+AAABwAAAB4AAAAB4AAAAB////wABw//4f//////////z//////4AAf/////////8AAADwAAAB4AAAABwAAAAB////8ABw//4f//////////////////////////////4AAADgAAABwAAAABwAAAAB/////wH///////////////////////////////////4AAAHMAAADwAAAADwAAAAD//////////////////////////////////////////wAAAP/AAADgAAAADgAAAAD//////////////////////////////////////////gAAAP/gAAHgAAAADgAAAAH////ADgAcB//4MA/wH/////w/gD4AAAH//////////gAAAf/wAAHgAAAAHAAAAAH///+ADAAYB3/wEAfgHAMAABwPwB8AAAD//////////AAAAf/4AAHAAAAAHAAAAAH///8ACAAwDg/wEAPgPAMAAAwHwA8AAAB/////////+AAAAf/4AAPAAAAAPAAAAAP///8ACAAgHgPwEAPAPAMAAAwH4A+AAAB/////////+AAAA//8AAOAAAAAOAAAAAP///8A2ARgPgBwHgHAfAMAAAwD8Af//wA/////////8AAAA//8AAeAAAAAOAAAAAf///8B+A/APgAAHgGAfAMD/AwD8AP//4B/////////4AAAB//+AAeAAAAAcAAAAAf///8B+A+APgAAHwGAfAMD/AwD+AP//wB/////////4AAAB//+AAcAAAAAcAAAAAf///8B+A+APwAAHwGA/AMD/AwD/AH//wB/////////wAAAD//+AA8AAAAA8AAAAA////8B+A8AH8AAHwAA/AMD/AwD/AD//wA/////////wAAAD///AA4AAAAA4AAAAA////8B+A4AH4AAHwAB/AMH/AwDwAAAAAA/////////gAAAD///AB4AAAAA4AAAAB////8B+A4AD4DwH4AB/AMD/AwDwAAAAAB/////////gAAAH///AB4AAAABwAAAAB////8B+AwED4DwH4AD/AMAAAwDwAAAAAB/////////AAAAH///gBwAAAABwAAAAB////8B+AgED4DwH4AD/AMAAAwDwAAAAAA/////////AAAAH///gDwAAAADwAAAAD////8A+AAOB4DwH4AH/AMAAAwD4AAAAAB////////+AAAAP///gDgAAAADgAAAAD////AACAAfB4DwH4B//AMAAAwD/8Af///////////+AAAAP///gHgAAAADgAAAAH////AACA///4AAEAAAfAP///wD///////////////8AAAAP///gHgAAAAHAAAAAH////AACAf//4AAEAAAPAAB+AADAAABwAAH///////8AAAAP///wHAAAAAHAAAAAH////AAGAAAH4AAAAAAPAAA8AADgAAAAAAH///////4AAAAf///wPAAAAAPAAAAAP////8B+AAAD4AAEAAAfAAAYAADgAAAAAAP///////4AAAAf///wOAAAAAOAAAAAP///8cB+AAAD4DwH8B//AAAYAADwAAAAAAP///////wAAAAf///wOAAAAAOAAAAAf///8EA+AAAB4DwH8B//AOAYDwD//4AAf/////////wAAAA////weAAAAAeAAAAAf///+AACA/8B4DwH8B//AOA8BwD//8AA//////////gAAAA////wcAAAAAcAAAAAf////AACA/8B4DwH8B//AMA8AwD//+AAf/////////gAAAA////w8AAAAA8AAAAA/////AACA/8B4AAEAAA/AMB+AwDh/+AAP/////////gAAAA////44AAAAA4AAAAA/////gAGAAAB4AAMAAAfAIB+AQDgP4AAH/////////AAAAB////54AAAAB4AAAAB/////gP+AAAB4AAEAAAfAAD/AADgHwAAD/////////AAAAB////5wAAAABwAAAAB/////wP+AAAB4AAEAAAfAAH/AAD4BgAAB////////+AAAAB/////wAAAADwAAAAD/////wAGAAAB4DwH/////gH/gH/8AAAAA////////+AAAAB/////gAAAAHgAAAAH/////wACA/8B4DwHgYH//gP/wH/+AADAAf///////+AAAAB/////AAAAAfAAAAAP/////4ACA/8B4DwHAYD//AP/wD//AAHwAP///////8AAAAD//////////////////////4ACA/8B8DwGA8B8AAAAAAA/gAAAAH///////8AAAAD//////////////////////4B+AAAB4BwGA8B8AAAAAAA/gAAAAH///////8AAAAD//////////////////////4B+AAABwAAGA8B8AAAAAAA/wAAAAH///////4AAAAD//////////////////////4A+AAABwAAEB+B8AAAAAAA/4AAAAD///////4AAAAD//////////////////////8A+AAABwAAEB+A///4Af///4Af//////////4AAAAH//////////////////////8A+AAAD///8D/A///wAf///8AP//////////wAAAAH///////////////////////////////////////4A////+AP//////////wAAAAH//////////////////////////////////////////////////////////wAAAAH//////////////////////////////////////////////////////////gAAAAH//////////////////////////////////////////////////////////gAAAAH//////////////////////////////////////////////////////////gAAAAH//////////////////////////////////////////////////////////AAAAAH//////////////////////////////////////////////////////////AAAAAP//////////////////////////////////////////////////////////AAAAB//////////////////////////////////////////////////////////+AAAAf//////////////////////////////////////////////////////////+AAAH///////////////////////////////////////////////////////////+AAA////////////////////////////////////////////////////////////+AAP////////////////////////////////////////////////////////////8AD/////////////////////////////////////////////////////////////8Af/////////////////////////////////////////////////////////////8H//////////////////////////////////////////////////////////////5////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////8AAA=="
#define _BMP "Qk0uEAAAAAAAAD4AAAAoAAAAgAEAAFUAAAABAAEAAAAAAPAPAADEDgAAxA4AAAAAAAAAAAAAAAAAAP///wD////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////wAAOAeAfAMAAD//////////////////////////////////////h/H+HDh/j4Ah/gAAGAOAPAEAAAf/////////////////////////////8AAA///8AHH+HDh/hwAh/gAAGAOAPAGAAAf///////////////////////gAAAD/AAAAH//wADD+HDh/hwAR/AAACAOAPAGAAAH///////////////////////AAAAB+AAAAD//gABD/Dhw/hh/w/ABgCAPAHAGAAAH////////////////8AAP//+AAAAB8AAAAD//A/5j/Dhw/hh/w/ADgDAHAHgDAAAD////////////////wAAf//+AAAAB4AAAAD//B//j/jxw/wx/w/gDgDAHgHgD//gD///////////////+AAA///8AAAAB4AAAAD//B//h/jxw/wwf4fgDgDgHgDgD//gD///////////////wAAHwH/8AAAABwAAAAD/+D//h/hx4fw4D4fgDwBgDgDgD//AD///////////////AAAPAA/4AAAABwAAAAD/+D//w/hw4fw8AAfgBwBgDgDwB+AAD//////////////8AAA+AA/4AAAABwAAAAH/+D//w/x48f4fgAfgBwBgDwDwB8AAH//////////////4AAB4AAf4AAAADgAAAAH/+D//w/w4cf4f8APwBwBwDwBwBwAAH//////////////gAADwAAPwAAAADgAAAAH/+D//w/w4cf8f/gPwB4AwBwBwBwAAP/////////////+AAAHAAAHwAAAADAAAAAP/+D//4fw4cP8f/8PwB4AwBwB4AwAA//////////////+AAAOAAAHwAAAAHAAAAAf//D//4Pw4cH8f/8PwA4AwB4B4AwB///////////////4AAAcAAADgAAAAHAAAAAf//B//4Bg8OAQfP8P4A4AwB4A4AwD///////////////wAAA8AAADgAAAAGAAAAAf//B//4AA8OAAfAAP4A8AYA4A4AQD///////////////gAABwAAAHAAAAAMAAAAA///g//8IB8OEA/AAP4A8AYAwAQAQAAD/////////////AAADwAAAPAAAAAcAAAAA///gf/8P//////8B/4A+AYAAAAAQAAD////////////+AAADgAAAOAAAAAcAAAAA///wP/8P/////////4AeAMAAAAAwAAD////////////8AAAHAAAAOAAAAAYAAAAB///4D/uP/+H//////8AeAGAAAAB8AAB////////////4AAAOAAAAcAAAAA4AAAAB///8AAOH/+H//////+AOADAAAAD+AAB////////////4AAAcAAAAcAAAAA4AAAAB///+AAOH//H/////////////////wAB////////////wAAAcAAAAcAAAAA4AAAAD////gAOH//D////////////////////////////////AAAA4AAAA4AAAABwAAAAD////+Af////////////////////////////////////AAAA/gAAA4AAAABwAAAAH//////////////////////////////////////////+AAAB/wAAA4AAAABwAAAAH//////////////////////////////////////////+AAAB/8AABwAAAADgAAAAH///8AMABAP/8AAP4D/////g/AHwAAAP///////////8AAAB/8AADwAAAAHAAAAAP///4AIACAcf8AAHwDgEAAAgPgB4AAAH///////////4AAAD/+AADgAAAAHAAAAAP///4AIACAcD8AADwHgEAAAgHwB4AAAD///////////wAAAH//AADgAAAAHAAAAAf///wAIAEA8A8BwDwHgEAAAgH4A8AAAD///////////wAAAH//AADgAAAAOAAAAAf///wH4D8A8AEB4BgHgEAAAgH4A///gD///////////gAAAP//gAHAAAAAOAAAAAf///wH4D4B8AAB4BAPgED/ggH8Af//gD///////////gAAAP//gAHAAAAAMAAAAA////wH4DwA8AAB4BAPgED/ggH8AP//gD///////////AAAAP//wAPAAAAAcAAAAB////wH4DgA/AAB8BAfgED/ggH/AH//gD///////////AAAAf//wAOAAAAAcAAAAB////wH4DgA/AAB8AAfgED/ggH/AD//gD//////////+AAAAf//wAOAAAAAYAAAAB////wH4DAA/AYB8AAfgED/ggHgAAAAAD//////////8AAAAf//wAcAAAAA4AAAAD////wH4CAAfAcB8AA/gEAAAgHgAAAAAD//////////8AAAA///4A8AAAABwAAAAD////wH4CAgfAcB8AB/gEAAAgHgAAAAAD//////////4AAAA///4A4AAAABwAAAAD////wH4AAwfAcB+AB/gEAAAgHgAAAAAD//////////4AAAB///4A4AAAABwAAAAH///+AAYABwPAcB+Af/gEAAAgH/4Af/////////////wAAAB///4BwAAAADgAAAAH///+AAYD///AAAAAAHgH///gHAAD/+AAf/////////wAAAB///4BwAAAADgAAAAH///+AAYD///AAAAAAHgAD/AAHAAAHwAAf/////////wAAAB///4BwAAAADgAAAAP///+AAYAAA/AAAAAAHgAAcAAHAAAAAAAf/////////gAAAD///8BgAAAAHAAAAAP////wH4AAAfAAAAAAHgAAYAAHAAAAAAAf/////////AAAAD///8DgAAAAPAAAAAf///wwH4AAAfAcB/Af/gAAYAAHgAAAAAA//////////AAAAD///8DgAAAAPAAAAAf///wQH4AAAfAcB/Af/gHAYDgH//wAA////////////AAAAD///8DAAAAAOAAAAAf///wAAYD/gPAcB/Af/gGAcBgH//4AB///////////+AAAAH///+HAAAAAcAAAAA////8AAYD/gPAcB/Af/gGA8AgH//8AB///////////+AAAAH///+OAAAAAcAAAAA////+AAYAAAPAABAAAPgEA+AAHA/4AAf//////////+AAAAH///+OAAAAAcAAAAB////+B/4AAAPAABAAAPgEB+AAHAfwAAf//////////8AAAAH///+cAAAAA4AAAAB////+A/4AAAPAABAAAPgAD/AAHAHgAAH//////////4AAAAH////cAAAAA4AAAAB/////Af4AAAfAABAAAPgAD/gAH4AAAAD//////////4AAAAP////4AAAADgAAAAH/////AAYAAAfAcB4ED//wH/gH/8AAAAB//////////4AAAAP////4AAAAHgAAAAP/////AAYD/gfAcBwGB//gH/wD/+AAOAA//////////wAAAAf////wAAAAPAAAAAf/////gAYD/gfAcBwOA+AAAAAAA/AAPAAf/////////wAAAAf/////////////////////gAYAAAfAcBgOA+AAAAAAA/AAAAAf/////////wAAAAf/////////////////////gH4AAAcAABAOAeAAAAAAA/gAAAAf/////////wAAAAf/////////////////////gH4AAAcAABAfAeAAAAAAA/wAAAAf/////////gAAAAf/////////////////////gD4AAAcAABAfAeAAAAAAA/wAAAAP/////////gAAAAf/////////////////////wD4AAAcAABAfAP//4Af///wAf////////////AAAAAf/////////////////////////////////////4Af///4Af////////////AAAAA//////////////////////////////////////////////////////////+AAAAA//////////////////////////////////////////////////////////+AAAAA//////////////////////////////////////////////////////////+AAAAA//////////////////////////////////////////////////////////+AAAAA//////////////////////////////////////////////////////////8AAAAA//////////////////////////////////////////////////////////8AAAAA//////////////////////////////////////////////////////////8AAAAP//////////////////////////////////////////////////////////8AAAA///////////////////////////////////////////////////////////4AAAP///////////////////////////////////////////////////////////4AAD////////////////////////////////////////////////////////////4AB/////////////////////////////////////////////////////////////4AH/////////////////////////////////////////////////////////////wB//////////////////////////////////////////////////////////////wf///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////8="


size_t gen_aliyun_mqtt_password_hmac_md5(const uint8_t *pBuffer, const uint8_t *pKey, uint8_t *pOutput);
size_t gen_aliyun_mqtt_password_hmac_sha1(const uint8_t *pBuffer, const uint8_t *pKey, uint8_t *pOutput);
void gen_aliyun_mqtt_info(aliyun_config_t *p_aliyun_config);
void HexArrayToHexStr(const uint8_t *pHex, char *pstr, uint16_t nLen);
void parse_json_aliyun_update_objects(const char *value, aliyun_config_t *p_aliyun_config);
void parse_json_aliyun_objects(const char *value, aliyun_config_t *p_aliyun_config);
int parse_json_yinlian_objects(const char *value, unsigned char *pstr);
void parse_json_data_objects(const char *value, aliyun_config_t *p_aliyun_config);

esp_err_t parse_print_json_objects(const char *value);

void ota_task_create(void);
void mqtt_app_start(void);

void publish_state(uint8_t val, bool flag);

esp_err_t parse_bmp(const unsigned char *src, size_t slen);

#ifdef __cplusplus
}
#endif

#endif /* __ALIYUN_MQTT_H */
