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
#include "mbedtls/md5.h"
#include "mbedtls/platform.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/esp_debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"
#include "mbedtls/sha1.h"
#include "mbedtls/base64.h"

#include "mbedtls/pk.h"
#include "mbedtls/md_internal.h"
#include "aliyun_mqtt.h"
#include "driver/uart.h"
#include "cJSON.h"

static const char *TAG = "ALIYUN_MQTT_EXAMPLE";

// uint8_t arrbuf[7] = {0x31, 0x32, 0x33, 0x34, 0x35, 0x0d, 0x0a};

/**
  * Function Name  : HexArrayToHexStr
  * Description    : 十六进制数组转十六进制字符串.
  * Input          : pHex: 十六进制数组.
                     nLen: 转换的长度.
  * Output         : pstr: 十六进制字符串.
  * Return         : Status.
  **/
void HexArrayToHexStr(const uint8_t *pHex, char *pstr, uint16_t nLen)
{
    uint8_t c;
    uint16_t i;

    for (i = 0; i < nLen; i++) {
        c = (pHex[i] & 0xF0) >> 4;

        if (c > 9) {
            *pstr++ = c - 10 + 'a';
        } else {
            *pstr++ = c + '0';
        }

        c = pHex[i] & 0x0F;

        if (c > 9) {
            *pstr++ = c - 10 + 'a';
        } else {
            *pstr++ = c + '0';
        }
    }
}

/* 十六进制数组转十六进制字符串 */
// static void HexArrayToHexStr(unsigned char *pHex, char *pAscii, int nLen)
// {
// 	unsigned char c;
// 	unsigned int i;

// 	for (i = 0; i < nLen; i++)
// 	{
// 		c = (pHex[i] & 0xF0) >> 4;

// 		if (c > 9)
// 		{
// 			*pAscii++ = c - 10 + 'A';
// 		}
// 		else
// 		{
// 			*pAscii++ = c + '0';
// 		}

// 		c = pHex[i] & 0x0F;

// 		if (c > 9)
// 		{
// 			*pAscii++ = c - 10 + 'A';
// 		}
// 		else
// 		{
// 			*pAscii++ = c + '0';
// 		}
// 	}
// }

/* 十六进制字符串转十六进制数组 */
static int HexStrToHexArray(char HexArray[], char HexStr[])
{
	int i = 0, index = 0;

	for (i = 0; HexStr[i]; i += 2)
	{
		if (HexStr[i] >= 'A' && HexStr[i] <= 'F')
		{
			HexArray[index] = HexStr[i] - 'A' + 10;
		}
		else if (HexStr[i] >= 'a' && HexStr[i] <= 'f')
		{
			HexArray[index] = HexStr[i] - 'a' + 10;
		}
		else
		{
			HexArray[index] = HexStr[i] - '0';
		}

		if (HexStr[i + 1] >= 'A' && HexStr[i + 1] <= 'F')
		{
			HexArray[index] = (HexArray[index] << 4) | (HexStr[i + 1] - 'A' + 10);
		}
		else if (HexStr[i + 1] >= 'a' && HexStr[i + 1] <= 'f')
		{
			HexArray[index] = (HexArray[index] << 4) | (HexStr[i + 1] - 'a' + 10);
		}
		else
		{
			HexArray[index] = (HexArray[index] << 4) | (HexStr[i + 1] - '0');
		}

		++index;
	}

	return index;
}

/**
  * Function Name  : HMAC_MD5
  * Description    : 使用 MD5 哈希函数计算基于哈希值的消息验证代码 (HMAC),并转换成十六进制字符串.
  * Input          : pBuffer: 加密的字符串.
                     len:     加密字符串的长度.
                     pKey:    初始密钥.
  * Output         : pOutput: 输出十六进制字符串.
  * Return         : 十六进制字符串的长度.
  **/
static uint16_t HMAC_MD5(uint8_t *pBuffer, uint16_t len, uint8_t *pKey, uint8_t *pOutput)
{
	uint8_t i, j;
    uint8_t Buffer2[80]; //第二次HASH
    uint8_t key[64];
    uint8_t ipad[64], opad[64];
    uint8_t output[16];
    uint8_t *tempBuffer = (uint8_t *)malloc(len + 64); //第一次HASH的参数

    memset(key, 0, 64);

    if (strlen((char *)pKey) > 64) {
        mbedtls_md5_ret(pKey, strlen((char *)pKey), key);
    } else {
        strncpy((char *)key, (char *)pKey, 64);
    }

    for (i = 0; i < 64; i++) {
        ipad[i] = 0x36;
        opad[i] = 0x5c;
    }

    for (i = 0; i < 64; i++) {
        ipad[i] = key[i] ^ ipad[i];   //K ⊕ ipad
        opad[i] = key[i] ^ opad[i];   //K ⊕ opad
    }

    for (i = 0; i < 64; i++) {
        tempBuffer[i] = ipad[i];
    }

    for (i = 64; i < len + 64; i++) {
        tempBuffer[i] = pBuffer[i - 64];
    }

    mbedtls_md5_ret(tempBuffer, len + 64, output);

    for (j = 0; j < 64; j++) {
        Buffer2[j] = opad[j];
    }

    for (i = 64; i < 80; i++) {
        Buffer2[i] = output[i - 64];
    }

    mbedtls_md5_ret(Buffer2, 80, output);

    HexArrayToHexStr(output, (char *)pOutput, sizeof(output));

    free(tempBuffer);

    return strlen((char *)pOutput);
}

/**
  * Function Name  : gen_aliyun_mqtt_password
  * Description    : 生成阿里云的mqtt密码,因为密码需要根据DEVICE_ID、DEVICE_NAME、PRODUCT_KEY和DEVICE_SECRET动态生成.
  * Input          : pBuffer: 加密的字符串长度.
  * Input          : pKey:    初始密钥.
  * Output         : pOutput: 输出十六进制字符串.
  * Return         : Status.
  **/
/* static void gen_aliyun_mqtt_password(void)
{
	uint16_t len;
	
	len = HMAC_MD5(HASH_STR, strlen(HASH_STR), HASH_KEY, passbuf);
	
	mbedtls_printf("pass:%s\r\n", passbuf);
    mbedtls_printf("len:%d\r\n", len);
} */

size_t gen_aliyun_mqtt_password_hmac_md5(const uint8_t *pBuffer, const uint8_t *pKey, uint8_t *pOutput)
{
    int ret;
    size_t len = 0;
    uint8_t digest[16];
    mbedtls_md_context_t sha_ctx;

    mbedtls_md_init(&sha_ctx);
    memset(digest, 0x00, sizeof(digest));

    ret = mbedtls_md_setup(&sha_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_MD5), 1);
    // ret = mbedtls_md_setup(&sha_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA1), 1);
    if (ret != 0)
    {
        mbedtls_printf("  ! mbedtls_md_setup() returned -0x%04x\n", -ret);
        goto exit;
    }

    mbedtls_md_hmac_starts(&sha_ctx, (uint8_t *)pKey, strlen((char *)pKey));
    mbedtls_md_hmac_update(&sha_ctx, (uint8_t *)pBuffer, strlen((char *)pBuffer));
    mbedtls_md_hmac_finish(&sha_ctx, digest);

    // mbedtls_printf("\n");
    // mbedtls_printf("HMAC: ");
    // for (int i = 0; i < sizeof(digest); i++)
    // {
    //     mbedtls_printf("%02X", digest[i]);
    // }
    // mbedtls_printf("\n");

    HexArrayToHexStr(digest, (char *)pOutput, sizeof(digest));
    len = strlen((char *)pOutput);

    // mbedtls_base64_encode((uint8_t *)pOutput, strlen((char *)pOutput), &len, digest, sizeof(digest));
    // mbedtls_printf("base64 encode relust: %s\nbase64 encode lenght: %d\n", pOutput, len);

exit:
    mbedtls_md_free(&sha_ctx);

    return len;
}

size_t gen_aliyun_mqtt_password_hmac_sha1(const uint8_t *pBuffer, const uint8_t *pKey, uint8_t *pOutput)
{
    int ret;
    size_t len = 0;
    uint8_t digest[20];
    mbedtls_md_context_t sha_ctx;

    mbedtls_md_init(&sha_ctx);
    memset(digest, 0x00, sizeof(digest));

    // ret = mbedtls_md_setup(&sha_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_MD5), 1);
    ret = mbedtls_md_setup(&sha_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA1), 1);
    if (ret != 0)
    {
        mbedtls_printf("  ! mbedtls_md_setup() returned -0x%04x\n", -ret);
        goto exit;
    }

    mbedtls_md_hmac_starts(&sha_ctx, (uint8_t *)pKey, strlen((char *)pKey));
    mbedtls_md_hmac_update(&sha_ctx, (uint8_t *)pBuffer, strlen((char *)pBuffer));
    mbedtls_md_hmac_finish(&sha_ctx, digest);

    // mbedtls_printf("\n");
    // mbedtls_printf("HMAC: ");
    // for (int i = 0; i < sizeof(digest); i++)
    // {
    //     mbedtls_printf("%02X", digest[i]);
    // }
    // mbedtls_printf("\n");

    // HexArrayToHexStr(digest, (char *)pOutput, sizeof(digest));
    // len = strlen((char *)pOutput);

    mbedtls_base64_encode((uint8_t *)pOutput, strlen((char *)pOutput), &len, digest, sizeof(digest));
    mbedtls_printf("base64 encode relust: %s\nbase64 encode lenght: %d\n", pOutput, len);

exit:
    mbedtls_md_free(&sha_ctx);

    return len;
}

void gen_aliyun_mqtt_info(aliyun_config_t *p_aliyun_config)
{
	char buf[512];
	
	ESP_LOGI(TAG, "p_aliyun_config->product_key:[%s]", p_aliyun_config->product_key);
    ESP_LOGI(TAG, "p_aliyun_config->device_name:[%s]", p_aliyun_config->device_name);
    ESP_LOGI(TAG, "p_aliyun_config->device_secret:[%s]", p_aliyun_config->device_secret);
	ESP_LOGI(TAG, "p_aliyun_config->subscribe_topic:[%s]", p_aliyun_config->subscribe_topic);
	
	/* MQTT_SERVER */
	sprintf(p_aliyun_config->device_server, MQTT_SERVER, p_aliyun_config->product_key, REGION_ID);
	/* MQTT_ID */
    sprintf(p_aliyun_config->device_id, ALIYUN_CLIENT_ID, p_aliyun_config->product_key, p_aliyun_config->device_name, TIMESTAMP_STR);
	/* MQTT_USERNAME */
    sprintf(p_aliyun_config->device_username, ALIYUN_USERNAME, p_aliyun_config->device_name, p_aliyun_config->product_key);
	/* MQTT_PASSWORD */
    memset(buf, 0, sizeof(buf));
    sprintf(buf, HASH_STR, p_aliyun_config->product_key, p_aliyun_config->device_name, p_aliyun_config->device_name, p_aliyun_config->product_key, TIMESTAMP_STR);
    ESP_LOGI(TAG, "HASH_STR:[%s]", buf);
    HMAC_MD5((unsigned char *)buf, strlen(buf), (unsigned char *)p_aliyun_config->device_secret, (unsigned char *)p_aliyun_config->device_password);

	ESP_LOGI(TAG, "p_aliyun_config->device_server:[%s]", p_aliyun_config->device_server);
    ESP_LOGI(TAG, "p_aliyun_config->device_id:[%s]", p_aliyun_config->device_id);
    ESP_LOGI(TAG, "p_aliyun_config->device_username:[%s]", p_aliyun_config->device_username);
    ESP_LOGI(TAG, "p_aliyun_config->device_password:[%s]", p_aliyun_config->device_password);
}

void parse_json_aliyun_update_objects(const char *value, aliyun_config_t *p_aliyun_config)
{
    cJSON *pJsonRoot = cJSON_Parse(value);

    if (pJsonRoot != NULL)
    {
        ESP_LOGI(TAG, "cJSON_Version:[%s]", cJSON_Version());

        cJSON *pdata = cJSON_GetObjectItem(pJsonRoot, "data");
        if (pdata != NULL)
        {
            ESP_LOGI(TAG, "parsing the second layer of data");
            if (cJSON_IsObject(pdata))
            {
                cJSON *pUrl = cJSON_GetObjectItem(pdata, "url");
                // sprintf(p_aliyun_config->device_ota_url, "%s", pUrl->valuestring);
                pUrl->valuestring += 5;
                sprintf(p_aliyun_config->device_ota_url, "%s%s", "http", pUrl->valuestring);
                ESP_LOGI(TAG, "pUrl->valuestring:[%s]", pUrl->valuestring);
                ESP_LOGI(TAG, "p_aliyun_config->device_ota_url:[%s]", p_aliyun_config->device_ota_url);
            }
            else
            {
                ESP_LOGI(TAG, "get data failed");
            }
        }
    }
}

int parse_json_yinlian_objects(const char *value, unsigned char *pstr)
{
    cJSON *pJsonRoot = cJSON_Parse(value);

    if (pJsonRoot !=NULL) {
        
        ESP_LOGI(TAG, "cJSON_Version:[%s]", cJSON_Version());
        
		//解析status字段字符串内容
		cJSON *pStatus = cJSON_GetObjectItem(pJsonRoot, "status");
		//判断status字段是否json格式
		if (pStatus) {
			//判断status字段是否string类型
			if (cJSON_IsString(pStatus)) {
                // ESP_LOGI(TAG, "get status:[%s]", pStatus->valuestring);
				ESP_LOGI(TAG, "get status:[%d]", atoi(pStatus->valuestring));
            }
		} else {
            ESP_LOGE(TAG, "get status failed\r\n");
        }
		
		/* status == 2返回正确值 */
		if(atoi(pStatus->valuestring) == 2) {
			//解析result字段字符串内容
			cJSON *pResult = cJSON_GetObjectItem(pJsonRoot, "result");
			//判断result字段是否json格式
			if (pResult) {
				//判断result字段是否string类型
				if (cJSON_IsString(pResult)) {
					ESP_LOGI(TAG, "get result:[%s]", pResult->valuestring);
					/* stpcpy(pstr, pResult->valuestring); */
                }
            } else {
				ESP_LOGE(TAG, "get result failed\r\n");
			}
			
			/* return strlen(pResult->valuestring); */
			/*
			* 十六进制字符串转十六进制数组.
			*/
			return HexStrToHexArray((char *)pstr, pResult->valuestring);
		}
	}
	
	return 0;
}

void parse_json_aliyun_objects(const char *value, aliyun_config_t *p_aliyun_config)
{
    cJSON *pJsonRoot = cJSON_Parse(value);

    if (pJsonRoot !=NULL) {
        
        ESP_LOGI(TAG, "cJSON_Version:[%s]", cJSON_Version());
        
		//解析product_key字段字符串内容
		cJSON *pProductKey = cJSON_GetObjectItem(pJsonRoot, "product_key");
		//判断product_key字段是否json格式
		if (pProductKey) {
			//判断product_key字段是否string类型
			if (cJSON_IsString(pProductKey)) {
                sprintf(p_aliyun_config->product_key, "%s", pProductKey->valuestring);
                ESP_LOGI(TAG, "get product key:[%s]", pProductKey->valuestring);
            }
		} else {
            ESP_LOGE(TAG, "get product key failed");
        }
		
        //解析device_name字段字符串内容
		cJSON *pDeviceName = cJSON_GetObjectItem(pJsonRoot, "device_name");
		//判断device_name字段是否json格式
		if (pDeviceName) {
			//判断device_name字段是否string类型
			if (cJSON_IsString(pDeviceName)) {
                sprintf(p_aliyun_config->device_name, "%s", pDeviceName->valuestring);
                ESP_LOGI(TAG, "get device name:[%s]", pDeviceName->valuestring);
            }
		} else {
            ESP_LOGE(TAG, "get device name failed");
        }
		
        //解析device_secret字段字符串内容
		cJSON *pDeviceSecret = cJSON_GetObjectItem(pJsonRoot, "device_secret");
		//判断device_secret字段是否json格式
		if (pDeviceSecret) {
			//判断device_secret字段是否string类型
			if (cJSON_IsString(pDeviceSecret)) {
                sprintf(p_aliyun_config->device_secret, "%s", pDeviceSecret->valuestring);
                ESP_LOGI(TAG, "get device secret:[%s]", pDeviceSecret->valuestring);
            }
		} else {
            ESP_LOGE(TAG, "get device secret failed");
        }
		
		//解析topic字段字符串内容
		cJSON *pTopic = cJSON_GetObjectItem(pJsonRoot, "topic");
		//判断topic字段是否json格式
		if (pTopic) {
			//判断topic字段是否string类型
			if (cJSON_IsString(pTopic)) {
                sprintf(p_aliyun_config->subscribe_topic, "%s", pTopic->valuestring);
                ESP_LOGI(TAG, "get topic:[%s]", pTopic->valuestring);
            }
		} else {
            ESP_LOGE(TAG, "get topic failed");
        }

        ESP_LOGI(TAG, "p_aliyun_config->product_key:[%s]", p_aliyun_config->product_key);
        ESP_LOGI(TAG, "p_aliyun_config->device_name:[%s]", p_aliyun_config->device_name);
        ESP_LOGI(TAG, "p_aliyun_config->device_secret:[%s]", p_aliyun_config->device_secret);
        ESP_LOGI(TAG, "p_aliyun_config->subscribe_topic:[%s]", p_aliyun_config->subscribe_topic);
	}
}

// void parse_json_data_objects(const char *value, aliyun_config_t *p_aliyun_config)
// {
//     cJSON *pJsonRoot = cJSON_Parse(value);

//     if (pJsonRoot !=NULL) {
        
//         ESP_LOGI(TAG, "cJSON_Version:[%s]", cJSON_Version());
        
// 		//解析orderNumber字段字符串内容
// 		cJSON *pOrderNumber = cJSON_GetObjectItem(pJsonRoot, "OrderNumber");
// 		//判断orderNumber字段是否json格式
// 		if (pOrderNumber) {
// 			//判断orderNumber字段是否string类型
// 			if (cJSON_IsString(pOrderNumber)) {
//                 sprintf(p_aliyun_config->order_number, "%s", pOrderNumber->valuestring);
//                 ESP_LOGI(TAG, "get order number:[%s]", pOrderNumber->valuestring);
//             }
// 		} else {
//             ESP_LOGI(TAG, "get product key failed");
//         }

//         //解析orderData字段字符串内容
//         cJSON *pOrderData = cJSON_GetObjectItem(pJsonRoot, "OrderData");
//         //判断orderNumber字段是否json格式
//         if (pOrderData)
//         {
//             //判断orderData字段是否string类型
//             if (cJSON_IsString(pOrderData))
//             {
//                 // sprintf(p_aliyun_config->order_data, "%s", pOrderData->valuestring);
//                 ESP_LOGI(TAG, "get order data:[%s]", pOrderData->valuestring);
//                 // mbedtls_base64_decode(buffer, sizeof(buffer), &len, src, 88);
//                 size_t string_len = strlen(pOrderData->valuestring);
//                 if (mbedtls_base64_decode(p_aliyun_config->order_data, sizeof(p_aliyun_config->order_data), &(p_aliyun_config->order_data_len), (uint8_t *)pOrderData->valuestring, string_len) == 0)
//                 {
//                     ESP_LOGI(TAG, "get order data lenght:[%u]", p_aliyun_config->order_data_len);
//                 }
//             }
//             else if (cJSON_IsArray(pOrderData))
//             {
//                 // 获取数组长度
//                 uint32_t len = cJSON_GetArraySize(pOrderData);
//                 ESP_LOGI(TAG, "get order data len:[%d]", len);
//                 for (size_t i = 0; i < len; i++)
//                 {
//                     ESP_LOGI(TAG, "cJSON_GetArrayItem(pOrderData, %d)= %d\n", i, cJSON_GetArrayItem(pOrderData, i)->valueint);
//                 }
                
//             }
//         }
//         else
//         {
//             ESP_LOGI(TAG, "get order data failed");
//         }

//         // //解析device_secret字段字符串内容
// 		// cJSON *pDeviceSecret = cJSON_GetObjectItem(pJsonRoot, "device_secret");
// 		// //判断device_secret字段是否json格式
// 		// if (pDeviceSecret) {
// 		// 	//判断device_secret字段是否string类型
// 		// 	if (cJSON_IsString(pDeviceSecret)) {
//         //         sprintf(p_aliyun_config->device_secret, "%s", pDeviceSecret->valuestring);
//         //         ESP_LOGI(TAG, "get device secret:[%s]", pDeviceSecret->valuestring);
//         //     }
// 		// } else {
//         //     ESP_LOGI(TAG, "get device secret failed");
//         // }
		
// 		// //解析topic字段字符串内容
// 		// cJSON *pTopic = cJSON_GetObjectItem(pJsonRoot, "topic");
// 		// //判断topic字段是否json格式
// 		// if (pTopic) {
// 		// 	//判断topic字段是否string类型
// 		// 	if (cJSON_IsString(pTopic)) {
//         //         sprintf(p_aliyun_config->subscribe_topic, "%s", pTopic->valuestring);
//         //         ESP_LOGI(TAG, "get topic:[%s]", pTopic->valuestring);
//         //     }
// 		// } else {
//         //     ESP_LOGI(TAG, "get topic failed");
//         // }

//         ESP_LOGI(TAG, "p_aliyun_config->order_number:[%s]", p_aliyun_config->order_number);
//         ESP_LOGI(TAG, "p_aliyun_config->order_data:[%s]", p_aliyun_config->order_data);
//         // ESP_LOGI(TAG, "p_aliyun_config->device_secret:[%s]", p_aliyun_config->device_secret);
//         // ESP_LOGI(TAG, "p_aliyun_config->subscribe_topic:[%s]", p_aliyun_config->subscribe_topic);
// 	}
// }

//Music:选择背景音乐。0:无背景音乐，1~15：选择背景音乐
void SYN_FrameInfo(uint8_t Music, char *HZdata)
{
    /****************需要发送的文本**********************************/
    unsigned char Frame_Info[50];
    unsigned char HZ_Length;
    unsigned char ecc = 0; //定义校验字节
    unsigned int i = 0;
    HZ_Length = strlen(HZdata); //需要发送文本的长度

    /*****************帧固定配置信息**************************************/
    Frame_Info[0] = 0xFD;              //构造帧头FD
    Frame_Info[1] = 0x00;              //构造数据区长度的高字节
    Frame_Info[2] = HZ_Length + 3;     //构造数据区长度的低字节
    Frame_Info[3] = 0x01;              //构造命令字：合成播放命令
    Frame_Info[4] = 0x01 | Music << 4; //构造命令参数：背景音乐设定

    /*******************校验码计算***************************************/
    for (i = 0; i < 5; i++) //依次发送构造好的5个帧头字节
    {
        ecc = ecc ^ (Frame_Info[i]); //对发送的字节进行异或校验
    }

    for (i = 0; i < HZ_Length; i++) //依次发送待合成的文本数据
    {
        ecc = ecc ^ (HZdata[i]); //对发送的字节进行异或校验
    }
    /*******************发送帧信息***************************************/
    memcpy(&Frame_Info[5], HZdata, HZ_Length);
    Frame_Info[5 + HZ_Length] = ecc;

    for (size_t i = 0; i < 5 + HZ_Length + 1; i++)
    {
        ESP_LOGI(TAG, "%02X, ", Frame_Info[i]);
    }

    ESP_LOGI(TAG, "cJSON_Version:[%d]", 5 + HZ_Length + 1);

    // UART_Write(UART2, Frame_Info, 5 + HZ_Length + 1);
}

esp_err_t parse_print_json_objects(const char *value)
{
    cJSON *pJsonRoot = cJSON_Parse(value);

    if (pJsonRoot != NULL)
    {
        ESP_LOGI(TAG, "cJSON_Version:[%s]", cJSON_Version());

        // 解析index字段字符串内容
        cJSON *pIndex = cJSON_GetObjectItem(pJsonRoot, "index");
        // 判断index字段是否json格式
        if (pIndex)
        {
            // 判断index字段是否string类型
            if (cJSON_IsString(pIndex))
            {
                ESP_LOGI(TAG, "get index:[%s]", pIndex->valuestring);
            }
        }
        else
        {
            ESP_LOGE(TAG, "get index failed");
        }

        // 解析msg_type字段字符串内容
        cJSON *pMsg_Type = cJSON_GetObjectItem(pJsonRoot, "msg_type");
        // 判断msg_type字段是否json格式
        if (pMsg_Type)
        {
            // 判断msg_type字段是否string类型
            if (cJSON_IsString(pMsg_Type))
            {
                ESP_LOGI(TAG, "get msg type:[%s]", pMsg_Type->valuestring);
            }
        }
        else
        {
            ESP_LOGE(TAG, "get msg type failed");
        }

        // 解析version字段字符串内容
        cJSON *pVersion = cJSON_GetObjectItem(pJsonRoot, "version");
        // 判断version字段是否json格式
        if (pVersion)
        {
            // 判断version字段是否string类型
            if (cJSON_IsString(pVersion))
            {
                ESP_LOGI(TAG, "get version:[%s]", pVersion->valuestring);
            }
        }
        else
        {
            ESP_LOGE(TAG, "get version failed");
        }
#if 1
        // 解析context字段内容，判断是否为json
        cJSON *pContext = cJSON_GetObjectItem(pJsonRoot, "context");
        if (pContext)
        {
            // 音响播报内容
            // 解析sc字段字符串内容
            cJSON *pSc = cJSON_GetObjectItem(pContext, "sc");
            // 判断sc字段是否json格式
            if (pSc)
            {
                // 判断sc字段是否string类型
                if (cJSON_IsString(pSc))
                {
                    ESP_LOGI(TAG, "get sc:[%s]", pSc->valuestring);
                    char buf[128];
                    bzero(buf, 128);
                    sprintf(buf, "[v%d][m%d][t%d]%s", 15, 7, 5, pSc->valuestring);
                    // SYN_FrameInfo(2, "[v15][m7][t5]欢迎使用绿深旗舰店SYN6288语音合成模块");
                    SYN_FrameInfo(0, buf);
                }
            }
            else
            {
                // 不可空
            }
#if 1
            // 打印主体
            // 解析pc字段字符串内容
            cJSON *pPc = cJSON_GetObjectItem(pContext, "pc");
            // 判断pc字段是否json格式
            if (pPc)
            {
                // 判断pc字段是否string类型
                if (cJSON_IsArray(pPc))
                {
                    uint16_t pc_count = cJSON_GetArraySize(pPc);
                    ESP_LOGI(TAG, "get pc count:[%u]", pc_count);

                    
                    uint8_t tp = 0;
                    uint8_t Cmd_Al_Buf[] = {0x1B, 0x61, 0x00};
                    bool right = false;

                    for (size_t i = 0; i < pc_count; i++)
                    {
                        ESP_LOGI(TAG, "get pc i:[%u]", i);
                        cJSON *pPcArray = cJSON_GetArrayItem(pPc, i);

                        // char *s = cJSON_Print(pPcArray);
                        // ESP_LOGW(TAG, "%s", s);
                        // cJSON_free(s);
#if 1
                        right = false;

                        // 行数据类型
                        // 0-base64编码的logo数据
                        // 1-交易或通知数据
                        // 2-二维码链接
                        // 3-空行
                        // 4-分割横线
                        // 5-广告数据
                        // 解析tp字段字符串内容
                        cJSON *pTp = cJSON_GetObjectItem(pPcArray, "tp");
                        // 判断tp字段是否json格式
                        if (pTp)
                        {
                            // 判断tp字段是否string类型
                            if (cJSON_IsString(pTp))
                            {
                                ESP_LOGI(TAG, "get tp:[%s]", pTp->valuestring);
                                // tp为0时，打印机按顺序打印cols中的logo数据，如果logo数据为空，则打印默认logo
                                tp = (uint8_t)atoi(pTp->valuestring);
                                ESP_LOGI(TAG, "get tp:[%d]", tp);
                                if (tp == 3)
                                {
                                    ESP_LOGI(TAG, "0A");
                                    uint8_t Cmd_0A_Buf[] = {0x0A};
                                    uart_write_bytes(UART_NUM_0, (char *)Cmd_0A_Buf, 1);
                                }
                                else if (tp == 4)
                                {
                                    ESP_LOGI(TAG, "--------------");

                                    #define _H "--------------------------------"
                                    uart_write_bytes(UART_NUM_0, (char *)_H, strlen(_H));
                                }
                                
                            }
                        }
                        else
                        {
                            // 不可空
                        }

                        // 当前行的对齐方式，为空则默认左对齐
                        // 0-左对齐
                        // 1-居中
                        // 2-右对齐
                        // 解析al字段字符串内容
                        cJSON *pAl = cJSON_GetObjectItem(pPcArray, "al");
                        // 判断al字段是否json格式
                        if (pAl)
                        {
                            // 判断al字段是否string类型
                            if (cJSON_IsString(pAl))
                            {
                                ESP_LOGI(TAG, "get al:[%s]", pAl->valuestring);
                                Cmd_Al_Buf[2] = (uint8_t)atoi(pAl->valuestring);
                                ESP_LOGI(TAG, "get al:[%d]", Cmd_Al_Buf[2]);
                                uart_write_bytes(UART_NUM_0, (char *)Cmd_Al_Buf, sizeof(Cmd_Al_Buf));
                            }
                        }
                        else
                        {
                            ESP_LOGE(TAG, "get al failed");
                            Cmd_Al_Buf[2] = 0x00;
                            uart_write_bytes(UART_NUM_0, (char *)Cmd_Al_Buf, sizeof(Cmd_Al_Buf));
                        }

                        // 当前行的行距
                        // 解析ls字段字符串内容
                        cJSON *pLs = cJSON_GetObjectItem(pPcArray, "ls");
                        // 判断ls字段是否json格式
                        if (pLs)
                        {
                            // 判断ls字段是否string类型
                            if (cJSON_IsString(pLs))
                            {
                                ESP_LOGI(TAG, "get ls:[%s]", pLs->valuestring);
                            }
                        }
                        else
                        {
                            ESP_LOGE(TAG, "get ls failed");
                        }

                        // 当前行的字间距
                        // 解析ws字段字符串内容
                        cJSON *pWs = cJSON_GetObjectItem(pPcArray, "ws");
                        // 判断ws字段是否json格式
                        if (pWs)
                        {
                            // 判断ws字段是否string类型
                            if (cJSON_IsString(pWs))
                            {
                                ESP_LOGI(TAG, "get ws:[%s]", pWs->valuestring);
                            }
                        }
                        else
                        {
                            ESP_LOGE(TAG, "get ws failed");
                        }
                        
                        // 当前行各个列对应数据
                        // 解析cols字段内容，判断是否为数组json
                        cJSON *pCols = cJSON_GetObjectItem(pPcArray, "cols");
                        // 判断cols字段是否json格式
                        if (pCols)
                        {
                            // 判断cols字段是否Array类型
                            if (cJSON_IsArray(pCols))
                            {
                                uint16_t cols_count = cJSON_GetArraySize(pCols);
                                ESP_LOGI(TAG, "get cols count:[%u]", cols_count);

                                cJSON *pColsArray;

                                for (size_t i = 0; i < cols_count; i++)
                                {
                                    ESP_LOGI(TAG, "get cols i:[%u]", i);
                                    pColsArray = cJSON_GetArrayItem(pCols, i);

                                    // 当前列对齐方式，为空则默认左对齐
                                    // 0-左对齐
                                    // 1-居中
                                    // 2-右对齐
                                    // 解析al字段字符串内容
                                    cJSON *pAl = cJSON_GetObjectItem(pColsArray, "al");
                                    // 判断al字段是否json格式
                                    if (pAl)
                                    {
                                        // 判断al字段是否string类型
                                        if (cJSON_IsString(pAl))
                                        {
                                            ESP_LOGI(TAG, "get al:[%s]", pAl->valuestring);
                                            Cmd_Al_Buf[2] = (uint8_t)atoi(pAl->valuestring);
                                            ESP_LOGI(TAG, "get al:[%d]", Cmd_Al_Buf[2]);
                                            uart_write_bytes(UART_NUM_0, (char *)Cmd_Al_Buf, sizeof(Cmd_Al_Buf));
                                        }
                                    }
                                    else
                                    {
                                        // 当前列对齐方式，为空则默认取行的对齐方式
                                        ESP_LOGE(TAG, "get al failed");
                                        // Cmd_Al_Buf[2] = 0x00;
                                        // uart_write_bytes(UART_NUM_0, (char *)Cmd_Al_Buf, sizeof(Cmd_Al_Buf));
                                    }

                                    // 打印内容颜色
                                    // 解析fc字段字符串内容
                                    cJSON *pFc = cJSON_GetObjectItem(pColsArray, "fc");
                                    // 判断fc字段是否json格式
                                    if (pFc)
                                    {
                                        // 判断fc字段是否string类型
                                        if (cJSON_IsString(pFc))
                                        {
                                            ESP_LOGI(TAG, "get fc:[%s]", pFc->valuestring);
                                        }
                                    }
                                    else
                                    {
                                        ESP_LOGE(TAG, "get fc failed");
                                    }

                                    // 打印内容字体大小
                                    // 解析fs字段字符串内容
                                    cJSON *pFs = cJSON_GetObjectItem(pColsArray, "fs");
                                    // 判断fs字段是否json格式
                                    if (pFs)
                                    {
                                        // 判断fs字段是否string类型
                                        if (cJSON_IsString(pFs))
                                        {
                                            ESP_LOGI(TAG, "get fs:[%s]", pFs->valuestring);
                                        }
                                    }
                                    else
                                    {
                                        ESP_LOGE(TAG, "get fs failed");
                                    }

                                    // 打印字体样式
                                    // 解析tf字段字符串内容
                                    cJSON *pTf = cJSON_GetObjectItem(pColsArray, "tf");
                                    // 判断tf字段是否json格式
                                    if (pTf)
                                    {
                                        // 判断tf字段是否string类型
                                        if (cJSON_IsString(pTf))
                                        {
                                            ESP_LOGI(TAG, "get tf:[%s]", pTf->valuestring);
                                        }
                                    }
                                    else
                                    {
                                        ESP_LOGE(TAG, "get tf failed");
                                    }

                                    // 打印内容是否增加下划线，0-否 1-是。空则为0-否
                                    // 解析tu字段字符串内容
                                    cJSON *pTu = cJSON_GetObjectItem(pColsArray, "tu");
                                    // 判断tu字段是否json格式
                                    if (pTu)
                                    {
                                        // 判断tu字段是否string类型
                                        if (cJSON_IsString(pTu))
                                        {
                                            ESP_LOGI(TAG, "get tu:[%s]", pTu->valuestring);
                                        }
                                    }
                                    else
                                    {
                                        ESP_LOGE(TAG, "get tu failed");
                                    }
                                    
                                    // 打印内容是否加粗，0-否 1-是。空则为0-否
                                    // 解析tb字段字符串内容
                                    cJSON *pTb = cJSON_GetObjectItem(pColsArray, "tb");
                                    // 判断tb字段是否json格式
                                    if (pTb)
                                    {
                                        // 判断tb字段是否string类型
                                        if (cJSON_IsString(pTb))
                                        {
                                            ESP_LOGI(TAG, "get tb:[%s]", pTb->valuestring);
                                        }
                                    }
                                    else
                                    {
                                        ESP_LOGE(TAG, "get tb failed");
                                    }

                                    // 打印对应行列的值如：测试商户
                                    // 解析cv字段字符串内容
                                    cJSON *pCv = cJSON_GetObjectItem(pColsArray, "cv");
                                    // 判断cv字段是否json格式
                                    if (pCv)
                                    {
                                        // 判断cv字段是否string类型
                                        if (cJSON_IsString(pCv))
                                        {
                                            ESP_LOGI(TAG, "get cv:[%s]", pCv->valuestring);
                                            if (tp == 0)
                                            {
                                                if (strlen(pCv->valuestring) == 0)
                                                {
                                                    uint8_t Cmd_Logo_Buf[] = {0x1C, 0x50, 0x00, 0x0A};
                                                    ESP_LOGI(TAG, "1C 50 00");
                                                    uart_write_bytes(UART_NUM_0, (char *)Cmd_Logo_Buf, sizeof(Cmd_Logo_Buf));
                                                }
                                                else
                                                {
                                                    // if (mbedtls_base64_decode(p_aliyun_config->order_data, sizeof(p_aliyun_config->order_data), &(p_aliyun_config->order_data_len), (uint8_t *)pOrderData->valuestring, string_len) == 0)
                                                    {
                                                        ESP_LOGI(TAG, "logo mbedtls_base64_decode");
                                                        ESP_LOGI(TAG, "logo mbedtls_base64_decode%u", strlen(pCv->valuestring));
                                                    }
                                                }
                                            }
                                            else if (tp == 2)
                                            {
                                                /* 二维码 */
                                                uint8_t Cmd_Qrc_Buf[] = {0x1D, 0x5A, 0x02, 0x1B, 0x5A, 0x00, 0x4C, 0x08, 0x00, 0x00};
                                                uint16_t len = strlen(pCv->valuestring);
                                                uint8_t buf_len = sizeof(Cmd_Qrc_Buf);
                                                ESP_LOGW(TAG, "len:%d", len);
                                                Cmd_Qrc_Buf[buf_len - 2] = len % 256;
                                                Cmd_Qrc_Buf[buf_len - 1] = len / 256;
                                                uart_write_bytes(UART_NUM_0, (char *)Cmd_Qrc_Buf, buf_len);
                                                uart_write_bytes(UART_NUM_0, (char *)pCv->valuestring, len);
                                                // RINGBUF_Put(p_ringbuf, (uint8_t *)Cmd_Qrc_Buf, buf_len);
                                                // RINGBUF_Put(p_ringbuf, (uint8_t *)pstr, len);
                                            }
                                            else if (tp == 1)
                                            {
                                                uart_write_bytes(UART_NUM_0, (char *)pCv->valuestring, strlen(pCv->valuestring));
                                                ESP_LOGI(TAG, "%s", pCv->valuestring);
                                            }
                                            else if (tp == 5)
                                            {
                                                uart_write_bytes(UART_NUM_0, (char *)pCv->valuestring, strlen(pCv->valuestring));
                                                ESP_LOGI(TAG, "%s", pCv->valuestring);

                                                if (right == false)
                                                {
                                                    right = true;
                                                    uint8_t Cmd_20_Buf[] = {0x20, 0x20, 0x20};
                                                    uart_write_bytes(UART_NUM_0, (char *)Cmd_20_Buf, 3);
                                                }
                                                
                                            }
                                        }
                                    }
                                    else
                                    {
                                        // 不可空
                                    }
                                }

                                if (tp == 1 || tp == 4 || tp == 5)
                                {
                                    uint8_t Cmd_0A_Buf[] = {0x0A, 0x0A, 0x0A, 0x0A, 0x0A};
                                    if (tp != 5)
                                    {
                                        uart_write_bytes(UART_NUM_0, (char *)Cmd_0A_Buf, 1);
                                    }
                                    else
                                    {
                                        uart_write_bytes(UART_NUM_0, (char *)Cmd_0A_Buf, 5);
                                    }
                                }
                            }
                        }
                        else
                        {
                            // tp为3或4时可空，其余不可空
                            ESP_LOGE(TAG, "get cols failed");
                        }
                        #endif
                    }
                }
            }
            else
            {
                // 不可空
            }
#endif
        }
#endif
        cJSON_Delete(pJsonRoot);
    }
    else
    {
        return ESP_FAIL;
    }

    return ESP_OK;
}
