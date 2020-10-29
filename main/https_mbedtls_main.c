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

#include "mbedtls/platform.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/esp_debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"

#include "mbedtls/pk.h"
#include "mbedtls/md_internal.h"

#include "driver/uart.h"
#include "cJSON.h"
#include "aliyun_mqtt.h"
#include "spi_flash.h"

/* FreeRTOS event group to signal when we are connected & ready to make a request */
extern EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
extern const int CONNECTED_BIT;
extern const int CONNECTED_MQTT_BIT;
extern const int SIGN_BIT;
static const char *TAG = "example";

void mqtt_app_start(void);
void mqtt_app_start_ota(void);

#if 0
static const char *REQUEST = "POST /uisiotfront/box/signIn HTTP/1.1\r\n\
Host: cloudspeaker.chinaums.com\r\n\
Content-Type: application/json;charset=utf-8\r\n\
Content-Length: 601\r\n\
Connection: close\r\n\
Cache-Control: no-cache\r\n\
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/75.0.3770.142 Safari/537.36\r\n\
Accept: */*\r\n\
Accept-Encoding: gzip, deflate, br\r\n\
Accept-Language: zh-CN,zh;q=0.9\r\n\r\n\r\n\
{\"sn\":\"B02201907232366025\",\"firmId\":\"10003\",\"model\":\"DM30\",\"version\":\"1.0.5\",\"sign\":\"91A8C5B8AA99E39BED9D5BED6C9A5AA089F7BCD578F289BAA1D9980A41E9E095E0A2DE9CF129D31333FA1225FA24ABFEA27557BEDED8002906999A7B9CFF01D04FF20984E0A541EA668EE98937766AF850B23AC853913271750A948EA2B387830EE2BE8DDB5B54D11C1BC16B5C8C480107E2311FAE4E647C865A19F8F6C14B89CAE69FCF4B85506A5E719883BCA887964C62D68EA2D859DD8454C4AA2784E881D830FEDFBA24F70EC81758750CBAB62C35B01DDDAB5B765EEDFB57267C27DA87F93C281B07270DB2524A2158D5529C94C69725A8E09550FD76FA7F0682A42547E9D077BDE4A21A16EA91CC60DA83E73770F1A459C87D51CAB875866D0535161C\"}";
#endif
/* Root cert for howsmyssl.com, taken from server_root_cert.pem

   The PEM file was extracted from the output of this command:
   openssl s_client -showcerts -connect www.howsmyssl.com:443 </dev/null

   The CA root cert is the last cert given in the chain of certs.

   To embed it in the app binary, the PEM file is named
   in the component.mk COMPONENT_EMBED_TXTFILES variable.
*/
extern const uint8_t server_root_cert_pem_start[] asm("_binary_server_root_cert_pem_start");
extern const uint8_t server_root_cert_pem_end[]   asm("_binary_server_root_cert_pem_end");

/* Concatenation of all available Private key in PEM format */
const unsigned char mbedtls_test_private_key_pem[] =
_TEST_KEY_RSA_SHA256_PEM
"";
const size_t mbedtls_test_private_key_pem_len = sizeof(mbedtls_test_private_key_pem);

static int mbedtls_md_string(const mbedtls_md_info_t *md_info, const char *pstr, size_t len, unsigned char *output)
{
	int ret;
	mbedtls_md_context_t ctx;

	if (md_info == NULL)
		return(MBEDTLS_ERR_MD_BAD_INPUT_DATA);

	mbedtls_md_init(&ctx);

	if ((ret = mbedtls_md_setup(&ctx, md_info, 0)) != 0)
		goto cleanup;

	if ((ret = md_info->starts_func(ctx.md_ctx)) != 0)
		goto cleanup;

	if ((ret = md_info->update_func(ctx.md_ctx, (unsigned char *)pstr, len)) != 0)
		goto cleanup;

	ret = md_info->finish_func(ctx.md_ctx, output);

cleanup:
	mbedtls_md_free(&ctx);

	return(ret);
}

static void https_get_task(void *pvParameters)
{
    char buf[512];
    char strbuf[4096];
    char result[1024];
    int ret, flags, len;
    size_t olen = 0;
	/* 存放签名字符串 */
    char sign[513];
	/* 先存放hash数，后存放密文转换过后的十六进制数 */
    unsigned char encrypt_value[256];
	
    // char *pbuffer = NULL;
    char *pstr = NULL;
	char *pstr_start = NULL;
    char *pstr_end = NULL;
    
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context ssl;
    mbedtls_x509_crt cacert;
    mbedtls_ssl_config conf;
    mbedtls_net_context server_fd;
	mbedtls_pk_context pk;

    // memset(aliyun_config.version, 0, sizeof(aliyun_config.version));
    // memset(aliyun_config.sn, 0, sizeof(aliyun_config.sn));
    // memset(aliyun_config.model, 0, sizeof(aliyun_config.model));
    // memset(aliyun_config.firmid, 0, sizeof(aliyun_config.firmid));

    // sprintf(aliyun_config.version, "%s", VERSION);
    // aliyun_config.version_len = strlen(VERSION);

    // sprintf(aliyun_config.sn, "%s", SN);
    // aliyun_config.sn_len = strlen(SN);

    // sprintf(aliyun_config.model, "%s", MODEL);
    // aliyun_config.model_len = strlen(MODEL);

    // sprintf(aliyun_config.firmid, "%s", FIRMID);
    // aliyun_config.firmid_len = strlen(FIRMID);
    

    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                            false, true, portMAX_DELAY);
    ESP_LOGI(TAG, "Connected to AP");
    xEventGroupWaitBits(wifi_event_group, SIGN_BIT,
                            false, true, portMAX_DELAY);
    /* memset(buf, 0, sizeof(buf)); */
	bzero(buf, sizeof(buf));
	// sprintf(buf, SIGN_STRING, FIRMID, MODEL, SN, VERSION);
    sprintf(buf, SIGN_STRING, aliyun_config.firmid, aliyun_config.model, aliyun_config.sn, aliyun_config.version);
	ESP_LOGE(TAG, "\r\n  . sign_src...%s\r\n", buf);
	
	mbedtls_pk_init(&pk);
    mbedtls_ssl_init(&ssl);
    mbedtls_x509_crt_init(&cacert);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    ESP_LOGI(TAG, "Seeding the random number generator");

    mbedtls_ssl_config_init(&conf);

    mbedtls_entropy_init(&entropy);
    if((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                    NULL, 0)) != 0)
    {
        ESP_LOGE(TAG, "mbedtls_ctr_drbg_seed returned %d", ret);
        abort();
    }
	
	bzero(strbuf, sizeof(strbuf));
	/* 2 * 1024 * 1024 */
    // spi_flash_read(0x200000, strbuf, sizeof(strbuf));
    
    // if ((pstr_start = (char *)strstr(strbuf, "-----BEGIN RSA PRIVATE KEY-----\r\n")) != NULL)
    // {
    //     if ((pstr_end = (char *)strstr(strbuf, "-----END RSA PRIVATE KEY-----\r\n")) != NULL)
    //     {
    //         pstr_end += 32;
    //         olen = pstr_end - pstr_start;
    //         strbuf[olen - 1] = '\0';
    //         ESP_LOGI(TAG, "spi_flash_read RSA PRIVATE KEY:%d", olen);
    //         ESP_LOGI(TAG, "spi_flash_read RSA PRIVATE KEY:%s", strbuf);
    //     }
    // }

    printf("aliyun_config.private_key:%s,%u,%u", aliyun_config.private_key, aliyun_config.private_key_len, mbedtls_test_private_key_pem_len);

    if ((ret = mbedtls_pk_parse_key(&pk, (uint8_t*)&aliyun_config.private_key, aliyun_config.private_key_len + 1, NULL, 0)) != 0)
    // if ((ret = mbedtls_pk_parse_key(&pk, mbedtls_test_private_key_pem, mbedtls_test_private_key_pem_len, NULL, 0)) != 0)
    // if ((ret = mbedtls_pk_parse_key(&pk, (unsigned char *)strbuf, olen, NULL, 0)) != 0)
	{
		ESP_LOGE(TAG, " failed\n  ! mbedtls_pk_parse_keyfile returned -0x%04x\n", -ret);
		abort();
	}
	
	/*
	* Compute the SHA-256 hash of the input file,
	* then calculate the signature of the hash.
	*/
	ESP_LOGI(TAG, "Generating the SHA-256 signature");

    /* memset(encrypt_value, 0, sizeof(encrypt_value)); */
	bzero(encrypt_value, sizeof(encrypt_value));
	if ((ret = mbedtls_md_string(
		mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
		buf, strlen(buf), encrypt_value)) != 0)
	{
		ESP_LOGE(TAG, " failed\n  ! Could not open or read %s\n\n", "string.txt");
		abort();
	}
	
	/* memset(result, 0, sizeof(result)); */
	bzero(result, sizeof(result));
	if ((ret = mbedtls_pk_sign(&pk, MBEDTLS_MD_SHA256, encrypt_value, 0, (unsigned char *)result, &olen,
		mbedtls_ctr_drbg_random, &ctr_drbg)) != 0)
	{
		ESP_LOGE(TAG, " failed\n  ! mbedtls_pk_sign returned -0x%04x\n", -ret);
		abort();
	}

    /* 十六进制数组转十六进制字符串 */
    HexArrayToHexStr((unsigned char *)result, sign, 256);
	sign[512] = '\0';
    ESP_LOGI(TAG, "\r\n    sign value\r\n    %s\r\n", sign);
	

    ESP_LOGI(TAG, "Loading the CA root certificate...");

    ret = mbedtls_x509_crt_parse(&cacert, server_root_cert_pem_start,
                                 server_root_cert_pem_end-server_root_cert_pem_start);

    if(ret < 0)
    {
        ESP_LOGE(TAG, "mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
        abort();
    }

    ESP_LOGI(TAG, "Setting hostname for TLS session...");

     /* Hostname set here should match CN in server certificate */
    if((ret = mbedtls_ssl_set_hostname(&ssl, WEB_SERVER)) != 0)
    {
        ESP_LOGE(TAG, "mbedtls_ssl_set_hostname returned -0x%x", -ret);
        abort();
    }

    ESP_LOGI(TAG, "Setting up the SSL/TLS structure...");

    if((ret = mbedtls_ssl_config_defaults(&conf,
                                          MBEDTLS_SSL_IS_CLIENT,
                                          MBEDTLS_SSL_TRANSPORT_STREAM,
                                          MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
    {
        ESP_LOGE(TAG, "mbedtls_ssl_config_defaults returned %d", ret);
        goto exit;
    }

    /* MBEDTLS_SSL_VERIFY_OPTIONAL is bad for security, in this example it will print
       a warning if CA verification fails but it will continue to connect.

       You should consider using MBEDTLS_SSL_VERIFY_REQUIRED in your own code.
    */
    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
    mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
#ifdef CONFIG_MBEDTLS_DEBUG
    mbedtls_esp_enable_debug_log(&conf, 4);
#endif

    if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0)
    {
        ESP_LOGE(TAG, "mbedtls_ssl_setup returned -0x%x\n\n", -ret);
        goto exit;
    }

    bzero(result, sizeof(result));
    // sprintf(result, POST_REQUEST_VALUE, SN, FIRMID, MODEL, VERSION, sign);
    sprintf(result, POST_REQUEST_VALUE, aliyun_config.sn, aliyun_config.firmid, aliyun_config.model, aliyun_config.version, sign);
    ESP_LOGI(TAG, "\r\n    POST REQUEST VALUE:\r\n%s %u\r\n", result, strlen(result));

    while(1) {
        /* Wait for the callback to set the CONNECTED_BIT in the
           event group.
        */
        /* memset(strbuf, 0, sizeof(strbuf)); */
		bzero(strbuf, sizeof(strbuf));
        sprintf(strbuf, POST_REQUEST_TEST, WEB_SERVER, strlen(result), result);
        ESP_LOGI(TAG, "\r\n    POST REQUEST:\r\n%s\r\n", strbuf);

        xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                            false, true, portMAX_DELAY);
        ESP_LOGI(TAG, "Connected to AP");

        mbedtls_net_init(&server_fd);

        ESP_LOGI(TAG, "Connecting to %s:%s...", WEB_SERVER, WEB_PORT);

        if ((ret = mbedtls_net_connect(&server_fd, WEB_SERVER,
                                      WEB_PORT, MBEDTLS_NET_PROTO_TCP)) != 0)
        {
            ESP_LOGE(TAG, "mbedtls_net_connect returned -%x", -ret);
            goto exit;
        }

        ESP_LOGI(TAG, "Connected.");

        mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

        ESP_LOGI(TAG, "Performing the SSL/TLS handshake...");

        while ((ret = mbedtls_ssl_handshake(&ssl)) != 0)
        {
            if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
            {
                ESP_LOGE(TAG, "mbedtls_ssl_handshake returned -0x%x", -ret);
                goto exit;
            }
        }

        ESP_LOGI(TAG, "Verifying peer X.509 certificate...");

        if ((flags = mbedtls_ssl_get_verify_result(&ssl)) != 0)
        {
            /* In real life, we probably want to close connection if ret != 0 */
            ESP_LOGW(TAG, "Failed to verify peer certificate!");
            bzero(buf, sizeof(buf));
            mbedtls_x509_crt_verify_info(buf, sizeof(buf), "  ! ", flags);
            ESP_LOGW(TAG, "verification info: %s", buf);
        }
        else {
            ESP_LOGI(TAG, "Certificate verified.");
        }

        ESP_LOGI(TAG, "Cipher suite is %s", mbedtls_ssl_get_ciphersuite(&ssl));

        ESP_LOGI(TAG, "Writing HTTP request...");

        size_t written_bytes = 0;
        do {
            ret = mbedtls_ssl_write(&ssl,
                                    (const unsigned char *)strbuf + written_bytes,
                                    strlen(strbuf) - written_bytes);
            if (ret >= 0) {
                ESP_LOGI(TAG, "%d bytes written", ret);
                written_bytes += ret;
            } else if (ret != MBEDTLS_ERR_SSL_WANT_WRITE && ret != MBEDTLS_ERR_SSL_WANT_READ) {
                ESP_LOGE(TAG, "mbedtls_ssl_write returned -0x%x", -ret);
                goto exit;
            }
        } while(written_bytes < strlen(strbuf));

        ESP_LOGI(TAG, "Reading HTTP response...");
        olen = 0;
		bzero(strbuf, sizeof(strbuf));
        do
        {
            len = sizeof(buf) - 1;
            bzero(buf, sizeof(buf));
            ret = mbedtls_ssl_read(&ssl, (unsigned char *)buf, len);

            if(ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
                continue;

            if(ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
                ret = 0;
                break;
            }

            if(ret < 0)
            {
                ESP_LOGE(TAG, "mbedtls_ssl_read returned -0x%x", -ret);
                break;
            }

            if(ret == 0)
            {
                ESP_LOGI(TAG, "connection closed");
                strbuf[olen] = '\0';
                break;
            }

            len = ret;
            ESP_LOGD(TAG, "%d bytes read", len);
            /* Print response directly to stdout as it is read */
            /* for(int i = 0; i < len; i++) {
                putchar(buf[i]);
                strbuf[i + olen] = buf[i];
            } */
            
            strcpy(&strbuf[olen], buf);
            olen += len;
        } while(1);

        mbedtls_ssl_close_notify(&ssl);
        
        ESP_LOGI(TAG, "-------------------------------------------------------");

        ESP_LOGI(TAG, "olen:%d", olen);
        ESP_LOGI(TAG, "strbuf:%s", (char *)strbuf);

        if ((pstr = (char *)strstr(strbuf, "Content-Length: ")) != NULL)
        {
            pstr_start = pstr + 16;
            ESP_LOGI(TAG, "pstr_start:%s", (char *)pstr_start);
            if ((pstr_end = (char *)strstr(pstr_start, "\r\n")) != NULL)
            {
                ESP_LOGI(TAG, "byte length:%d", strlen(pstr_start) - strlen(pstr_end));
                memcpy(pstr, pstr_start, strlen(pstr_start) - strlen(pstr_end));
                len = atoi(pstr);
                ESP_LOGI(TAG, "Json length:%d", len);
                ESP_LOGI(TAG, " pstr%s", (char *)pstr);
            }
            else
            {
                goto exit;
            }

            /* post response value */
            if ((pstr = (char *)strstr(pstr, "\r\n\r\n")) != NULL)
            {
                /* 获取JSON密文字符串 */
                pstr_start = pstr + 4;
                ESP_LOGI(TAG, "Json length:%d", strlen(pstr_start));
                ESP_LOGI(TAG, "Json string:\r\n%s", (char *)pstr_start);
            }
            else
            {
                goto exit;
            }

            /* 提取密文，准备开始解密 */
            // if ((pstr = (char *)strstr(pstr_start, "result")) != NULL)
            // {
            //     pstr_start = pstr + 9;
            //     /*ESP_LOGI(TAG, "Json string:%s\r\n", (char *)pstr);*/
            //     if ((pstr = (char *)strstr(pstr_start, "\",")) != NULL)
            //     {
            //         /*ESP_LOGI(TAG, "Json string:%s\r\n", (char *)pstr);
            //         ESP_LOGI(TAG, "Json string:%s\r\n", (char *)pstr1);*/
            //         len = strlen(pstr_start) - strlen(pstr);
            //         ESP_LOGI(TAG, "string length:%d", len);
            //         memcpy(pstr, pstr_start, len);
            //         pstr[len] = '\0';
            //         ESP_LOGI(TAG, "string:\r\n%s", (char *)pstr);
            //     }
            // }
            // else
            // {
            //     goto exit;
            // }
        }
		
		/* memset(encrypt_value, 0, sizeof(encrypt_value)); */
		bzero(encrypt_value, sizeof(encrypt_value));
		len = parse_json_yinlian_objects((const char *)pstr_start, encrypt_value);

        if (len == -1)
        {
            uint8_t Cmd_Query_Buf[] = {0x1B, 0xF5};
            uart_write_bytes(UART_NUM_0, (const char *)Cmd_Query_Buf, sizeof(Cmd_Query_Buf));
            goto exit;
        }
        

        ESP_LOGI(TAG, "encrypt_length:%d", len);
        ESP_LOGI(TAG, "encrypt_value:%s", (char *)encrypt_value);
		
		if(len) {
			/*
			* Extract the RSA encrypted value from the text file
			*/
        // dec:
			/*
			* Decrypt the encrypted RSA data and print the result.
			*/
			ESP_LOGI(TAG, "Decrypting the encrypted data");
			/* memset(result, 0, sizeof(result)); */
			bzero(result, sizeof(result));

			if ((ret = mbedtls_pk_decrypt(&pk, (const unsigned char *)encrypt_value, len, (unsigned char *)result, &olen, sizeof(result),
				mbedtls_ctr_drbg_random, &ctr_drbg)) != 0)
			{
				ESP_LOGE(TAG, " failed\n  ! mbedtls_pk_decrypt returned -0x%04x\n",
					-ret);
				goto exit;
                // goto dec;
			}

            uint8_t Cmd_Query_Buf[] = {0x1B, 0xF4};
            uart_write_bytes(UART_NUM_0, (const char *)Cmd_Query_Buf, sizeof(Cmd_Query_Buf));

			ESP_LOGI(TAG, "OK");

			ESP_LOGI(TAG, "The decrypted result is:\r\n%s\r\n", result);

			parse_json_aliyun_objects((const char *)result, &aliyun_config);
        }

    exit:
        mbedtls_ssl_session_reset(&ssl);
        mbedtls_net_free(&server_fd);

        if(ret != 0)
        {
            mbedtls_strerror(ret, buf, 100);
            ESP_LOGE(TAG, "Last error was: -0x%x - %s", -ret, buf);
            vTaskDelete(NULL);
        }
        else
        {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            ESP_LOGI(TAG, "Starting again!");
            xEventGroupSetBits(wifi_event_group, CONNECTED_MQTT_BIT);
            
            vTaskDelete(NULL);
        }
        
	#if 0
        putchar('\n'); // JSON output doesn't have a newline at end

        static int request_count;
        ESP_LOGI(TAG, "Completed %d requests", ++request_count);

        for(int countdown = 10; countdown >= 0; countdown--) {
            ESP_LOGI(TAG, "%d...", countdown);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
	#endif
		// vTaskDelay(1000 / portTICK_PERIOD_MS);
        // mqtt_app_start();
        // mqtt_app_start_ota();
        // ESP_LOGI(TAG, "Starting again!");
        // vTaskDelete(NULL);
    }
}

void https_task_create(void)
{
    xTaskCreate(&https_get_task, "https_get_task", 12 * 1024, NULL, 4, NULL);
}
