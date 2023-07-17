#include "subscriptor.h"


void (* handler_event)(const char *, const char *);
int sock = 0; //socket alta/baja contenidos
int sock_2 = 0; //segundo socket, notificar eventos

void* thread_handler(void * arg) {
    //variables locales
    int sock = -1;
    struct iovec paq[3];
    uint32_t op = 5, res = -1;

    //conectar con el broker
    sock = conectar_broker();
    if (sock < 0) {
        perror("error al conectar con el broker");
        return NULL;
    }

    //crear paquete para enviar 
    op = htonl(op);

    paq[0].iov_base = &op;
    paq[0].iov_len = sizeof(uint32_t);

    //enviar paquete
    if ((writev(sock,paq,1)) < 0) {
        perror("error al enviar el paquete al broker en destroyMQ");
        return NULL;
    }

    //crear segundo socket
    sock_2 = conectar_broker();

    for (;;) {
        //recibir respuesta
        if (recv(sock_2,&res,sizeof(uint32_t),MSG_WAITALL) < 0){
            perror("error al recibir respuesta en la biblioteca, metodo destroy\n");
            return NULL;
        }
        printf("hola, paquete recibido\n");
    }

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

