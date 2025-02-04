#include <stdio.h>
#include <stdbool.h>
#include <unistd.h> // Para usleep()

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

// Prototipos de funciones
void mostrarMenu();
void manejarFalla();
void parpadearBombillo();

int main() {
    printf("Sistema de Puerta Automática Iniciado\n");

    while (1) {
        mostrarMenu();

        switch (estadoActual) {
            case ESPERA:
                printf("Estado: ESPERA\n");
                break;

            case ABRIENDO:
                printf("Estado: ABRIENDO\n");
                for (int i = 0; i < 30; i++) { // Simula 30 segundos
                    if (hayObstaculo) {
                        printf("¡Obstáculo detectado! Pasando a estado PARADA...\n");
                        estadoActual = PARADA;
                        break;
                    }
                    printf("Abriendo... (%d/30 segundos)\n", i + 1);
                    usleep(1000000); // 1 segundo
                }
                if (estadoActual == ABRIENDO) {
                    estadoActual = ABIERTA;
                }
                break;

            case ABIERTA:
                printf("Estado: ABIERTA\n");
                usleep(2000000); // Espera 2 segundos antes de cerrar automáticamente
                estadoActual = CERRANDO;
                break;

            case CERRANDO:
                printf("Estado: CERRANDO\n");
                for (int i = 0; i < 30; i++) { // Simula 30 segundos
                    if (hayObstaculo) {
                        printf("¡Obstáculo detectado! Pasando a estado PARADA...\n");
                        estadoActual = PARADA;
                        break;
                    }
                    printf("Cerrando... (%d/30 segundos)\n", i + 1);
                    usleep(1000000); // 1 segundo
                }
                if (estadoActual == CERRANDO) {
                    estadoActual = CERRADA;
                }
                break;

            case CERRADA:
                printf("Estado: CERRADA\n");
                estadoActual = ESPERA;
                break;

            case PARADA:
                printf("Estado: PARADA\n");
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
        usleep(500000); // 0.5 segundos
    }

    return 0;
}

// Función para mostrar el menú interactivo
void mostrarMenu() {
    int opcion;

    printf("\n--- MENÚ DE BOTONERA ---\n");
    printf("1. Abrir puerta\n");
    printf("2. Cerrar puerta\n");
    printf("3. Paro de emergencia\n");
    printf("4. Simular obstáculo\n");
    printf("5. Quitar obstáculo\n");
    printf("6. Salir\n");
    printf("Seleccione una opción: ");
    scanf("%d", &opcion);

    switch (opcion) {
        case 1:
            printf("Botón 'Abrir' presionado.\n");
            estadoActual = ABRIENDO;
            break;

        case 2:
            printf("Botón 'Cerrar' presionado.\n");
            estadoActual = CERRANDO;
            break;

        case 3:
            printf("Botón 'Paro' presionado. Reiniciando sistema...\n");
            estadoActual = ESPERA;
            hayObstaculo = false;
            fallaDetectada = false;
            break;

        case 4:
            printf("Simulando obstáculo...\n");
            hayObstaculo = true;
            break;

        case 5:
            printf("Quitando obstáculo...\n");
            hayObstaculo = false;
            break;

        case 6:
            printf("Saliendo del sistema...\n");
            exit(0);

        default:
            printf("Opción inválida. Intente nuevamente.\n");
            break;
    }
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
        printf("Bombillo ENCENDIDO\n");
        usleep(500000); // 0.5 segundos encendido
        printf("Bombillo APAGADO\n");
        usleep(500000); // 0.5 segundos apagado
    }
}