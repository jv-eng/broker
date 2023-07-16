#include "subscriptor.h"
#include "comun.h"
#include "edsu_comun.h"

void (* handler_event)(const char *, const char *);
int sock = 0;

void* thread_handler(void * arg) {
	return NULL;
}

int alta_subscripcion_tema(const char *tema) {
	pthread_t hilo;
    int valor = 42;

    // Crear un nuevo hilo
    if (pthread_create(&hilo, NULL, thread_handler, &valor) != 0) {
        printf("Error al crear el hilo.\n");
        return 1;
    }
	
	return 0;
}

int baja_subscripcion_tema(const char *tema) {
	return 0;
}

int inicio_subscriptor(void (*notif_evento)(const char *, const char *),
                void (*alta_tema)(const char *),
                void (*baja_tema)(const char *)) {
	handler_event = notif_evento;
	return 0;
}

int fin_subscriptor() {
	return 0;
}

