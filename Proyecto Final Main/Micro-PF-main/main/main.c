#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include <stdio.h>
#include <stdlib.h>
#include "esp_system.h"
#include "spi_flash_mmap.h"
#include <esp_http_server.h>
#include "esp_spiffs.h"
#include "cJSON.h"
#include "driver/gpio.h"

#define LED_PIN GPIO_NUM_8
#define GPIO_OUTPUT_PIN_SEL (1ULL << LED_PIN)

#define BUTTON_PIN GPIO_NUM_3
#define GPIO_INPUTPUT_PIN_SEL (1ULL << BUTTON_PIN)

#include "lwip/err.h"
#include "lwip/sys.h"

#define EXAMPLE_ESP_WIFI_SSID      "CAMI_WIFI"
#define EXAMPLE_ESP_WIFI_PASS      "CAMI_WIFI47121"
#define EXAMPLE_ESP_WIFI_CHANNEL   5
#define EXAMPLE_MAX_STA_CONN       2

#define INDEX_HTML_PATH "/storage/index.html"
char index_html[10000];
char response_data[10000];

#define OFF         0
#define MONOESTABLE 1
#define ASTABLE     2
#define PWM         3

TimerHandle_t tim_10ms; 

struct data {
    float freq;             //Frecuencia de Oscilación
    float duty_cycle;       //Ciclo de Trabajo
    float period_mons;      //Período del Monoastable
    bool pulse;             //Pulso del Botón
    uint8_t mode;           //Modo de Operación
} data;

static const char *TAG = "PROYECTO FINAL";

/*--------Timer CallBack--------*/
void TimerCallBack(TimerHandle_t xTimer) 
{
    static uint16_t cont = 0;
    // if (data.mode == MONOESTABLE) 
    // {
    //     cont += 1;

    //     if (cont )
    // }
    if (data.mode == ASTABLE || data.mode == PWM)
    {
        cont += 10;
        static uint16_t prev_cont = 0;

        float tiempo_high = ((1/data.freq)*(data.duty_cycle/100))*1000;
        float tiempo_low = ((1/data.freq)*(1-(data.duty_cycle/100)))*1000;

        if (cont <= tiempo_high)
        {
            gpio_set_level(LED_PIN, 0);
            prev_cont = cont;
        }
        else
        {
            gpio_set_level(LED_PIN, 1);
            if (cont >= prev_cont + tiempo_low) cont = 0;
        }

    }  
}

/*--------WEBPAGE--------*/

static void initi_web_page_buffer(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/storage",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true};

    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));

    memset((void *)index_html, 0, sizeof(index_html));
    struct stat st;
    if (stat(INDEX_HTML_PATH, &st))
    {
        ESP_LOGE(TAG, "index.html not found");
        return;
    }

    FILE *fp = fopen(INDEX_HTML_PATH, "r");
    if (fp == NULL) 
    {
        ESP_LOGE(TAG, "Failed to open file: %s", INDEX_HTML_PATH);
    }

    if (fread(index_html, st.st_size, 1, fp) == 0)
    {
        ESP_LOGE(TAG, "st_size, Bytes: %d", (int)st.st_size);
        ESP_LOGE(TAG, "file read failed");
    }
    fclose(fp);
}

esp_err_t send_web_page(httpd_req_t *req)
{
    int response;
    sprintf(response_data, index_html, "YES");
    response = httpd_resp_send(req, response_data, HTTPD_RESP_USE_STRLEN);
    return response;
}

esp_err_t get_req_handler(httpd_req_t *req)
{
    return send_web_page(req);
}

esp_err_t get_data(httpd_req_t *req)
{
    char buffer[250];
    size_t recv_size = MIN(req->content_len, sizeof(buffer));

    int ret = httpd_req_recv(req, buffer, recv_size);

    cJSON *root = cJSON_Parse(buffer);

    cJSON *modo_item =  cJSON_GetObjectItemCaseSensitive(root, "modo");
    uint8_t modo = atoi(cJSON_GetStringValue(modo_item));
    data.mode = modo;

    if (modo == 0)
    {
        ESP_LOGD(TAG, "Apagado");
    }
    else if (modo == 1) 
    {
        ESP_LOGD(TAG, "Monoestable");

        cJSON *period_item = cJSON_GetObjectItemCaseSensitive(root, "pulso");
        char *period_str = cJSON_GetStringValue(period_item);
        data.period_mons = (float)atof(period_str);

        ESP_LOGE(TAG, "Tiempo de Pulso %.2f", data.period_mons);
    }
    else if (modo == 2) 
    {
        ESP_LOGD(TAG, "Astable");

        cJSON *frecuencia_item = cJSON_GetObjectItemCaseSensitive(root, "frecuencia");
        char *frecuencia_str = cJSON_GetStringValue(frecuencia_item);
        data.freq = (float)atof(frecuencia_str);

        cJSON *duty_item = cJSON_GetObjectItemCaseSensitive(root, "duty");
        char *duty_str = cJSON_GetStringValue(duty_item);
        data.duty_cycle = (float)atof(duty_str);

        ESP_LOGE(TAG, "Frecuencia %.2f", data.freq);
        ESP_LOGE(TAG, "Duty %.2f", data.duty_cycle);
    }
    else if (modo == 3) 
    {
        ESP_LOGD(TAG, "PWM");

        cJSON *frecuencia_item = cJSON_GetObjectItemCaseSensitive(root, "frecuencia");
        char *frecuencia_str = cJSON_GetStringValue(frecuencia_item);
        data.freq = (float)atof(frecuencia_str);

        cJSON *duty_item = cJSON_GetObjectItemCaseSensitive(root, "duty");
        char *duty_str = cJSON_GetStringValue(duty_item);
        data.duty_cycle = (float)atof(duty_str);

        ESP_LOGE(TAG, "Frecuencia %.2f", data.freq);
        ESP_LOGE(TAG, "Duty %.2f", data.duty_cycle);
    }
    else 
    {
        ESP_LOGE(TAG, "Y qsy bro");
    }

    buffer[0] = "\0";

    const char resp[] = "URI POST Response";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

httpd_uri_t uri_get = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = get_req_handler,
    .user_ctx = NULL};

httpd_uri_t uri_data = {
    .uri = "/valore",
    .method = HTTP_POST,
    .handler = get_data,
    .user_ctx = NULL};

httpd_handle_t setup_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &uri_data);
    }

    return server;
}

/*--------WIFI--------*/

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d, reason=%d",
                 MAC2STR(event->mac), event->aid, event->reason);
    }
}

void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
#ifdef CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
            .authmode = WIFI_AUTH_WPA3_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
#else /* CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT */
            .authmode = WIFI_AUTH_WPA2_PSK,
#endif
            .pmf_cfg = {
                    .required = true,
            },
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
}

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    wifi_init_softap();

    initi_web_page_buffer();
    setup_server();

    tim_10ms = xTimerCreate("Timer1", 10 / portTICK_PERIOD_MS, pdTRUE, (void *)0, &TimerCallBack);

    if (tim_10ms == NAME_MAX)
    {
        ESP_LOGE(TAG, "ERROR TIMER");
    }
    else
    {
        xTimerStart(tim_10ms, 0);
    }

    gpio_config_t pin_conf = {};
    pin_conf.intr_type = GPIO_INTR_DISABLE;      // disable interrupt
    pin_conf.mode = GPIO_MODE_OUTPUT;            // set as output mode
    pin_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL; //  Bit Mask
    pin_conf.pull_down_en = 0;                   // disable pull-down mode
    pin_conf.pull_up_en = 0;                     // disable pull-up mode
    gpio_config(&pin_conf);                      // configure GPIO with the given settings

    gpio_set_level(LED_PIN, 0);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    gpio_set_level(LED_PIN, 1);

    pin_conf.intr_type = GPIO_INTR_POSEDGE;      // disable interrupt
    pin_conf.mode = GPIO_MODE_INPUT;            // set as output mode
    pin_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL; //  Bit Mask
    pin_conf.pull_down_en = 0;                   // disable pull-down mode
    pin_conf.pull_up_en = 0;                     // disable pull-up mode
    gpio_config(&pin_conf);  
}
