#include <stdio.h>
#include <stdbool.h>
#include <unistd.h> 
#include "driver/gpio.h" 

// Definición de pines
#define PIN_BOTON_ABRIR     GPIO_NUM_2
#define PIN_BOTON_CERRAR    GPIO_NUM_3
#define PIN_BOTON_PARO      GPIO_NUM_4
#define PIN_FOTO_CELDA      GPIO_NUM_5
#define PIN_LED_ESTADO      GPIO_NUM_12
#define PIN_LED_FALLA       GPIO_NUM_13

// Definición de estados
typedef enum {
    ESPERA,
    ABRIENDO,
    ABIERTA,
    CERRANDO,
    CERRADA,
    CERRANDO_EMERGENCIA,
    REVERSA,
    PARADA,
    FALLA
} EstadoPuerta;

// Variables globales
EstadoPuerta estadoActual = ESPERA;
bool hayObstaculo = false;
bool fallaDetectada = false;
static unsigned int cnt_led_falla = 0;
#define PARPADEO_LENTO 10 // Cada 500ms * 10 = 5 segundos

// Prototipos de funciones
void setupPines();
void manejarFalla();
void mostrarEstado();
void delaySegundos(int segundos);
void manejarEventos();
void Timer50ms();
void leerBotones();

int main() {
    printf("Sistema de Portón Automático Iniciado\n");
    setupPines();
    while (1) {
        leerBotones();
        Timer50ms();
        manejarEventos();
        mostrarEstado();
        usleep(500000); // 0.5 segundos
    }
    return 0;
}

// Muestra el estado actual del sistema
void mostrarEstado() {
    const char *nombresEstados[] = {"ESPERA", "ABRIENDO", "ABIERTA", "CERRANDO", "CERRADA", "CERRANDO_EMERGENCIA", "REVERSA", "PARADA", "FALLA"};
    printf("Estado actual: %s\n", nombresEstados[estadoActual]);
}

// Función para hacer un delay en segundos
void delaySegundos(int segundos) {
    for (int i = 0; i < segundos; i++) {
        if (hayObstaculo && estadoActual == CERRANDO) {
            printf("Obstáculo detectado. Revirtiendo...\n");
            estadoActual = REVERSA;
            return;
        }
        printf("Procesando... (%d/%d segundos)\n", i + 1, segundos);
        usleep(1000000); // 1 segundo
    }
}

// Manejo de eventos según la entrada del usuario
void manejarEventos() {
    switch (estadoActual) {
        case ESPERA:
            if (gpio_get_level(PIN_BOTON_ABRIR) == 0) {
                printf("Iniciando apertura...\n");
                estadoActual = ABRIENDO;
                delaySegundos(5);
                estadoActual = ABIERTA;
            }
            gpio_set_level(PIN_LED_ESTADO, 0); // Apagar LED ESTADO
            gpio_set_level(PIN_LED_FALLA, 0);  // Apagar LED FALLA
            break;
        case ABRIENDO:
            gpio_set_level(PIN_LED_ESTADO, 1); // Encender LED ESTADO
            gpio_set_level(PIN_LED_FALLA, 0);  // Apagar LED FALLA
            break;
        case ABIERTA:
            gpio_set_level(PIN_LED_ESTADO, 1); // Encender LED ESTADO
            gpio_set_level(PIN_LED_FALLA, 0);  // Apagar LED FALLA
            delaySegundos(2);
            estadoActual = CERRANDO;
            break;
        case CERRANDO:
            gpio_set_level(PIN_LED_ESTADO, 0); // Apagar LED ESTADO
            gpio_set_level(PIN_LED_FALLA, 0);  // Apagar LED FALLA
            break;
        case CERRADA:
            gpio_set_level(PIN_LED_ESTADO, 0); // Apagar LED ESTADO
            gpio_set_level(PIN_LED_FALLA, 0);  // Apagar LED FALLA
            break;
        case CERRANDO_EMERGENCIA:
            gpio_set_level(PIN_LED_ESTADO, 0); // Apagar LED ESTADO
            gpio_set_level(PIN_LED_FALLA, 0);  // Apagar LED FALLA
            break;
        case REVERSA:
            gpio_set_level(PIN_LED_ESTADO, 1); // Encender LED ESTADO
            gpio_set_level(PIN_LED_FALLA, 0);  // Apagar LED FALLA
            delaySegundos(3);
            estadoActual = ABIERTA;
            break;
        case PARADA:
            gpio_set_level(PIN_LED_ESTADO, 0); // Apagar LED ESTADO
            gpio_set_level(PIN_LED_FALLA, 0);  // Apagar LED FALLA
            if (gpio_get_level(PIN_BOTON_PARO) == 0) {
                printf("Paro de emergencia desactivado!\n");
                estadoActual = ESPERA;
            }
            break;
        case FALLA:
            gpio_set_level(PIN_LED_ESTADO, 0); // Apagar LED ESTADO
            gpio_set_level(PIN_LED_FALLA, 0);  // Apagar LED FALLA
            manejarFalla();
            break;
        default:
            printf("Estado desconocido.\n");
            break;
    }
}

// Función para configurar los pines
void setupPines() {
    gpio_config_t io_conf;
    // Configuración de botones
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << PIN_BOTON_ABRIR) | (1ULL << PIN_BOTON_CERRAR) | (1ULL << PIN_BOTON_PARO) | (1ULL << PIN_FOTO_CELDA);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);
    // Configuración de LEDs
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << PIN_LED_ESTADO) | (1ULL << PIN_LED_FALLA);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
}

// Función para leer los botones y detectar obstáculos
void leerBotones() {
    static bool lastAbrirState = true;
    static bool lastCerrarState = true;
    static bool lastParoState = true;
    static bool lastFotoCeldaState = true;

    bool currentAbrirState = gpio_get_level(PIN_BOTON_ABRIR);
    bool currentCerrarState = gpio_get_level(PIN_BOTON_CERRAR);
    bool currentParoState = gpio_get_level(PIN_BOTON_PARO);
    bool currentFotoCeldaState = gpio_get_level(PIN_FOTO_CELDA);

    if (currentAbrirState == false && lastAbrirState == true) {
        printf("Botón 'Abrir' presionado.\n");
        estadoActual = ABRIENDO;
    }
    if (currentCerrarState == false && lastCerrarState == true) {
        printf("Botón 'Cerrar' presionado.\n");
        estadoActual = CERRANDO;
    }
    if (currentParoState == false && lastParoState == true) {
        printf("Botón 'Paro' presionado. Activando paro de emergencia...\n");
        estadoActual = PARADA;
    }
    if (currentFotoCeldaState == false && lastFotoCeldaState == true) {
        printf("Obstáculo detectado por foto celda...\n");
        hayObstaculo = true;
    }
    if (currentFotoCeldaState == true && lastFotoCeldaState == false) {
        printf("Obstáculo quitado por foto celda...\n");
        hayObstaculo = false;
    }

    lastAbrirState = currentAbrirState;
    lastCerrarState = currentCerrarState;
    lastParoState = currentParoState;
    lastFotoCeldaState = currentFotoCeldaState;
}

// Función para manejar el estado de falla
void manejarFalla() {
    printf("¡FALLA DETECTADA! Bombillo parpadeando...\n");
    cnt_led_falla = 0; // Reiniciar el contador de parpadeo
    fallaDetectada = false; // Reinicia la falla después de manejarla
}

// Función el parpadeo del bombillo
void Timer50ms() {
    static unsigned int cnt_led_falla = 0; // Declaración estática para conservar el valor entre llamadas
    cnt_led_falla++;
    if (cnt_led_falla >= PARPADEO_LENTO) {
        cnt_led_falla = 0; // Reiniciar el contador
        gpio_set_level(PIN_LED_FALLA, !gpio_get_level(PIN_LED_FALLA)); // Cambiar el estado del LED
    }
}
