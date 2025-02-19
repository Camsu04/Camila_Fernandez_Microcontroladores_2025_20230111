#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "esp_log.h"
#include "mqtt_client.h"

//--------DEFINE DE LA PUERTA--------/
#define INIT 0
#define CLOSED 1
#define OPEN 2
#define CLOSING 3
#define OPENING 4
#define ERR 5

#define TRUE 1
#define FALSE 0

#define RT_MAX 180
#define OP_TIME 60

//--------VARIABLES DE LA PUERTA--------/
int prev_state = INIT;
int curr_state = INIT;
int next_state = INIT;

bool mqtt_connected = false;


//--------ESTRUCTURA DE LA PUERTA--------/
struct DATA
{
    unsigned int LSC:1;     //LS Cerrado
    unsigned int LSO:1;     //LS Abierto
    unsigned int SPP:1;     //Pulsador
    unsigned int MO:1;      //Motor para abrir
    unsigned int MC:1;      //Motor para cerrar
    unsigned int RT_CNT;    //Runtime Timer
    unsigned int TIM;       //Timer
    unsigned int ledG:1;    //LED Verde
    unsigned int ledR:1;    //LED Rojo
    unsigned int ledO:1;    //LED Naranja

} data;

static const char *TAG = "mqtt5_example";
const char topic_r[17]  = "micro/esp32s3/R";
const char topic_g[17]  = "micro/esp32s3/G";
const char topic_b[17]  = "micro/esp32s3/B";

//int x = sizeof("/micro/esp32s3/G");



static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt5_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id = 0;

    ESP_LOGD(TAG, "free heap size is %" PRIu32 ", minimum %" PRIu32, esp_get_free_heap_size(), esp_get_minimum_free_heap_size());

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        mqtt_connected = true;

        //Subscripcion 
        msg_id = esp_mqtt_client_subscribe(client, "/topic/windoufoln", 1);
        
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        mqtt_connected = false;    
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        msg_id = esp_mqtt_client_publish(client, "/topic/windoufoln", "entro eri", sizeof("entro eri"), 1, 1);
        //msg_id = esp_mqtt_client_publish(client, "/micro/esp32s3/G", "0", sizeof("0"), 1, 1);
        //msg_id = esp_mqtt_client_publish(client, "/micro/esp32s3/B", "0", sizeof("0"), 1, 1);
        ESP_LOGI(TAG, "Published @ micro.");
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");

        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        ESP_LOGI(TAG, "MQTT5 return code is %d", event->error_handle->connect_return_code);
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

esp_mqtt_client_handle_t client = NULL;
static void mqtt5_app_start(void)
{
    ESP_LOGI(TAG, "STARTING MQTT");
    esp_mqtt_client_config_t mqttConfig = {
        .broker.address.uri = "mqtt://test.mosquitto.org:1883" ,
        .session.protocol_ver = MQTT_PROTOCOL_V_5,
        .network.disable_auto_reconnect = false,
        .credentials.username = "",
        .credentials.authentication.password = ""};
    
    client = esp_mqtt_client_init(&mqttConfig);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt5_event_handler, client);
    esp_mqtt_client_start(client);

}

//--------FUNCIONES DE LA PUERTA--------//
int initFunc (void) {
    prev_state == INIT;
    curr_state == INIT;

    data.MC = 0;
    data.MO = 0;
    data.RT_CNT = 0;
    data.SPP = 0;
    data.LSC = 0;
    data.LSO = 0;

    while (1)
    {    
        if (data.LSC == TRUE && data.LSO == FALSE) {
            return CLOSED;
        }

        if ((data.LSC == FALSE && data.LSO == FALSE) || (data.LSC == FALSE && data.LSO == TRUE)) {
            return CLOSING;
        }

        if (data.LSC && data.LSO) {
            return ERR;
        }
    }
}

int closedFunc (void) {
    prev_state= curr_state;
    curr_state == CLOSED;

    data.MC = FALSE;
    data.SPP == FALSE;

    while (1)
    {
        if (data.SPP == TRUE)
        {
            data.SPP == FALSE;
            return OPENING;
        }
    }    
}

int openFunc (void) {
    
    prev_state = curr_state;
    curr_state = OPEN;

    data.MO = FALSE;
    data.SPP == FALSE;

    while (1)
    {
        if (data.SPP == TRUE || data.TIM >= OP_TIME)
        {
            data.SPP == FALSE;
            return CLOSING;
        }
    }
}

int closingFunc (void) {
    prev_state= curr_state;
    curr_state == CLOSING;

    data.MC = TRUE;
    data.RT_CNT = 0;

    while (1) {
        if (data.LSC == TRUE && data.LSO == FALSE) {
            return CLOSED;
        }
        if (data.RT_CNT > RT_MAX) {
            return ERR;
        }
    }
}

int openingFunc (void) {
    prev_state = curr_state;
    curr_state = OPENING;

    data.MO = TRUE;
    data.RT_CNT = 0;
    
    while (1) {
        if (data.LSO == TRUE && data.LSC == FALSE) {
            return OPEN;
        }
        if (data.RT_CNT > RT_MAX) {
            return ERR;
        }
    }
}

int errFunc (void) {
    if (data.RT_CNT >= RT_MAX) {
        //error rt excedido
        print("error\n");
    }     
}

void app_main(void)
{

    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("mqtt_example", ESP_LOG_VERBOSE);
    esp_log_level_set("transport_base", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("transport", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    mqtt5_app_start();

    //--------LED--------/
    data.ledG = TRUE;
    data.ledR = TRUE;
    data.ledO = TRUE;
    vTaskDelay(1000/portTICK_PERIOD_MS);
    data.ledG = FALSE;
    data.ledR = FALSE;
    data.ledO = FALSE;

    //--------MÁQUINA DE ESTADO--------/
    
    while(1) {
        if (next_state == INIT) {
            next_state = initFunc();
        }
        if (next_state == CLOSED) {
            next_state = closedFunc();
        }
        if (next_state == OPEN) {
            next_state = openFunc();
        }
        if (next_state == CLOSING) {
            next_state = closingFunc();
        }
        if (next_state == OPENING) {
            next_state = openingFunc();
        }
        if (next_state == ERR) {
            next_state = errFunc();
        }
                
    }
}
