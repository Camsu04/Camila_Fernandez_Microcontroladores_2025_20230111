#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

// Definición de pines
#define PIN_BOTON_ABRIR 2
#define PIN_BOTON_CERRAR 3
#define PIN_BOTON_PARO 4
#define PIN_BOTON_OBSTACULO 5
#define PIN_LED_ESTADO 12
#define PIN_LED_FALLA 13

// Definición de estados
typedef enum {
    ESPERA,
    ABRIENDO,
    ABIERTA,
    CERRANDO,
    CERRADA,
    PARADA,
    FALLA
} EstadoPuerta;

// Variables globales
EstadoPuerta estadoActual = ESPERA;
bool hayObstaculo = false; // Simulación de obstáculo
bool fallaDetectada = false; // Indica si hay una falla en el sistema
uint32_t startTime = 0;
const uint32_t duration = 30000; // 30 segundos

// Prototipos de funciones
void setupPines();
void leerBotones();
void manejarFalla();
void parpadearBombillo();

void app_main() {
    printf("Sistema de Puerta Automática Iniciado\n");
    setupPines();

    while (1) {
        leerBotones();

        switch (estadoActual) {
            case ESPERA:
                printf("Estado: ESPERA\n");
                gpio_set_level(PIN_LED_ESTADO, 0);
                gpio_set_level(PIN_LED_FALLA, 0);
                break;

            case ABRIENDO:
                printf("Estado: ABRIENDO\n");
                gpio_set_level(PIN_LED_ESTADO, 1);
                gpio_set_level(PIN_LED_FALLA, 0);
                if (esp_timer_get_time() - startTime >= duration * 1000) {
                    if (estadoActual == ABRIENDO) {
                        estadoActual = ABIERTA;
                    }
                } else {
                    printf("Abriendo... (%lu/%lu segundos)\n", (esp_timer_get_time() - startTime) / 1000000, duration / 1000);
                    if (hayObstaculo) {
                        printf("¡Obstáculo detectado! Pasando a estado PARADA...\n");
                        estadoActual = PARADA;
                    }
                }
                break;

            case ABIERTA:
                printf("Estado: ABIERTA\n");
                gpio_set_level(PIN_LED_ESTADO, 1);
                gpio_set_level(PIN_LED_FALLA, 0);
                vTaskDelay(2000 / portTICK_PERIOD_MS); // Espera 2 segundos antes de cerrar automáticamente
                estadoActual = CERRANDO;
                break;

            case CERRANDO:
                printf("Estado: CERRANDO\n");
                gpio_set_level(PIN_LED_ESTADO, 0);
                gpio_set_level(PIN_LED_FALLA, 0);
                if (esp_timer_get_time() - startTime >= duration * 1000) {
                    if (estadoActual == CERRANDO) {
                        estadoActual = CERRADA;
                    }
                } else {
                    printf("Cerrando... (%lu/%lu segundos)\n", (esp_timer_get_time() - startTime) / 1000000, duration / 1000);
                    if (hayObstaculo) {
                        printf("¡Obstáculo detectado! Pasando a estado PARADA...\n");
                        estadoActual = PARADA;
                    }
                }
                break;

            case CERRADA:
                printf("Estado: CERRADA\n");
                gpio_set_level(PIN_LED_ESTADO, 0);
                gpio_set_level(PIN_LED_FALLA, 0);
                estadoActual = ESPERA;
                break;

            case PARADA:
                printf("Estado: PARADA\n");
                gpio_set_level(PIN_LED_ESTADO, 0);
                gpio_set_level(PIN_LED_FALLA, 0);
                if (!hayObstaculo) {
                    printf("Obstáculo quitado. Volviendo al estado anterior...\n");
                    estadoActual = ESPERA; // Vuelve al estado de espera
                }
                break;

            case FALLA:
                printf("Estado: FALLA\n");
                manejarFalla();
                break;
        }

        // Simulación de tiempo entre iteraciones
        vTaskDelay(500 / portTICK_PERIOD_MS); // 0.5 segundos
    }
}

// Función para configurar los pines
void setupPines() {
    gpio_config_t io_conf;

    // Configuración de botones
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << PIN_BOTON_ABRIR) | (1ULL << PIN_BOTON_CERRAR) | (1ULL << PIN_BOTON_PARO) | (1ULL << PIN_BOTON_OBSTACULO);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    // Configuración de LEDs
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << PIN_LED_ESTADO) | (1ULL << PIN_LED_FALLA);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
}

// Función para leer los botones
void leerBotones() {
    static bool lastAbrirState = true;
    static bool lastCerrarState = true;
    static bool lastParoState = true;
    static bool lastObstaculoState = true;

    bool currentAbrirState = gpio_get_level(PIN_BOTON_ABRIR);
    bool currentCerrarState = gpio_get_level(PIN_BOTON_CERRAR);
    bool currentParoState = gpio_get_level(PIN_BOTON_PARO);
    bool currentObstaculoState = gpio_get_level(PIN_BOTON_OBSTACULO);

    if (currentAbrirState == false && lastAbrirState == true) {
        printf("Botón 'Abrir' presionado.\n");
        estadoActual = ABRIENDO;
        startTime = esp_timer_get_time();
    }

    if (currentCerrarState == false && lastCerrarState == true) {
        printf("Botón 'Cerrar' presionado.\n");
        estadoActual = CERRANDO;
        startTime = esp_timer_get_time();
    }

    if (currentParoState == false && lastParoState == true) {
        printf("Botón 'Paro' presionado. Reiniciando sistema...\n");
        estadoActual = ESPERA;
        hayObstaculo = false;
        fallaDetectada = false;
    }

    if (currentObstaculoState == false && lastObstaculoState == true) {
        printf("Simulando obstáculo...\n");
        hayObstaculo = true;
    }

    if (currentObstaculoState == true && lastObstaculoState == false) {
        printf("Quitando obstáculo...\n");
        hayObstaculo = false;
    }

    lastAbrirState = currentAbrirState;
    lastCerrarState = currentCerrarState;
    lastParoState = currentParoState;
    lastObstaculoState = currentObstaculoState;
}

// Función para manejar el estado de falla
void manejarFalla() {
    printf("¡FALLA DETECTADA! Bombillo parpadeando...\n");
    parpadearBombillo();
    fallaDetectada = false; // Reinicia la falla después de manejarla
    estadoActual = ESPERA;  // Vuelve al estado de espera
}

// Función para simular el parpadeo del bombillo
void parpadearBombillo() {
    for (int i = 0; i < 5; i++) {
        gpio_set_level(PIN_LED_FALLA, 1);
        vTaskDelay(500 / portTICK_PERIOD_MS); // 0.5 segundos encendido
        gpio_set_level(PIN_LED_FALLA, 0);
        vTaskDelay(500 / portTICK_PERIOD_MS); // 0.5 segundos apagado
    }
}