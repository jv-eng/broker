#include "subscriptor.h"

//manejadores de eventos
void (* handler_event)(const char *, const char *) = NULL;
void (* alta_tema_)(const char *) = NULL;
void (* baja_tema_)(const char *) = NULL;

//variables globales
int sock = -1; //socket alta/baja contenidos
int sock_2 = -1; //segundo socket, notificar eventos
int cod = -1; //numero de secuencia del cliente
int existe_th = 0; //ver si se ha lanzado el thread
int subscrito = 0; //subscrito a creacion de temas
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int isSocketOpen(int socketDescriptor) {
    int optval;
    socklen_t optlen = sizeof(optval);

    // Obtener la opcion SO_ERROR del socket
    if (getsockopt(socketDescriptor, SOL_SOCKET, SO_ERROR, &optval, &optlen) == -1) {
        return 0; // No está abierto
    }

    // Verificar si hay algun error en el socket
    if (optval == 0) {
        return 1; // Esta abierto
    } else {
        return 0; // No esta abierto
    }
}

void* thread_handler(void * arg) {
    //variables locales
    struct iovec paq[1];
    uint32_t op = 9, res = -1;
    sock = conectar_broker();
    char * buff, * buff2;

    //crear paquete para enviar 
    op = htonl(op);

    paq[0].iov_base = &op;
    paq[0].iov_len = sizeof(uint32_t);

    //enviar paquete
    if ((writev(sock,paq,1)) < 0) {
        perror("error al enviar el paquete al broker");
        return NULL;
    }

    //recibir codigo
    if (recv(sock,&cod,sizeof(uint32_t),MSG_WAITALL) < 0){
        perror("error al recibir respuesta en la biblioteca\n");
        return NULL;
    }
    cod = ntohl(cod);
    printf("cliente codigo %d\n",cod);

    //crear segundo socket
    sock_2 = conectar_broker();
    close(sock);

    pthread_mutex_lock(&mutex);
    existe_th = 1;
    pthread_mutex_unlock(&mutex);

    for (;;) {
        //recibir respuesta
        if (recv(sock_2,&res,sizeof(uint32_t),MSG_WAITALL) < 0){
            perror("error al recibir respuesta en la biblioteca\n");
            return NULL;
        }
        res = ntohl(res);
        
        //hay que procesar el tipo de evento
        switch (res) {
            case 0: //evento
                //recibimos tam tema
                if (recv(sock_2,&res,sizeof(uint32_t),MSG_WAITALL) < 0){
                    perror("error al recibir respuesta en la biblioteca\n");
                    return NULL;
                } res = ntohl(res);
                //recibimos tema
                buff = malloc(sizeof(char) * res);
                if (recv(sock_2,buff,res,MSG_WAITALL) < 0){
                    perror("error al recibir respuesta en la biblioteca\n");
                    return NULL;
                }
                //recibimos tam msg
                if (recv(sock_2,&res,sizeof(uint32_t),MSG_WAITALL) < 0){
                    perror("error al recibir respuesta en la biblioteca\n");
                    return NULL;
                } res = ntohl(res);
                //recibimos msg
                buff2 = malloc(sizeof(char) * res);
                if (recv(sock_2,buff2,res,MSG_WAITALL) < 0){
                    perror("error al recibir respuesta en la biblioteca\n");
                    return NULL;
                }
                //imprimimos
                handler_event(buff, buff2);
                break;
            case 1: //tema creado
                //recibimos tam tema
                if (recv(sock_2,&res,sizeof(uint32_t),MSG_WAITALL) < 0){
                    perror("error al recibir respuesta en la biblioteca\n");
                    return NULL;
                } res = ntohl(res);
                //recibimos tema
                buff = malloc(sizeof(char) * res);
                if (recv(sock_2,buff,res,MSG_WAITALL) < 0){
                    perror("error al recibir respuesta en la biblioteca\n");
                    return NULL;
                }
                if (alta_tema_ != NULL) {
                    alta_tema_(buff);
                } else perror("Error, no existe manejador");
                break;
            case 2: //tema eliminado
                //recibimos tam tema
                if (recv(sock_2,&res,sizeof(uint32_t),MSG_WAITALL) < 0){
                    perror("error al recibir respuesta en la biblioteca\n");
                    return NULL;
                } res = ntohl(res)-1;
                //recibimos tema
                buff = malloc(sizeof(char) * res);
                if (recv(sock_2,buff,res,MSG_WAITALL) < 0){
                    perror("error al recibir respuesta en la biblioteca\n");
                    return NULL;
                }
                if (baja_tema_ != NULL) {
                    baja_tema_(buff);
                } else perror("Error, no existe manejador");
                break;
        }
        
    }    
    if (buff) free(buff);
    if (buff2) free(buff2);
	return NULL;
}

int alta_subscripcion_tema(const char *tema) {
    //variables locales
    int sock = 0;
    struct iovec paq[4];
    uint32_t tam_tema = strlen(tema) + 1, tam_envio = 0, res = -1, op = 3;

    //conectar con el broker
    sock = conectar_broker();
    if (sock < 0) {
        perror("error al conectar con el broker");
        return -1;
    }

    //crear paquete para enviar
    op = htonl(op);
    tam_envio = htonl(tam_tema);
    res = htonl(cod);
    paq[0].iov_base = &op;
    paq[0].iov_len = sizeof(uint32_t);
    paq[1].iov_base = &res;
    paq[1].iov_len = sizeof(uint32_t);
    paq[2].iov_base = &tam_envio;
    paq[2].iov_len = sizeof(uint32_t);
    paq[3].iov_base = (char *)tema;
    paq[3].iov_len = tam_tema;

    //enviar paquete
    if ((writev(sock,paq,4)) < 0) {
        perror("error al enviar el paquete al broker al subscribir un tema");
        return -1;
    }

    //recibir respuesta
    if (recv(sock,&res,sizeof(uint32_t),MSG_WAITALL) < 0){
        perror("error al recibir respuesta en la biblioteca\n");
        return -1;
    }

    //terminamos
	close(sock);
    if (res == 0) return 0;
    else return -1;
}

int baja_subscripcion_tema(const char *tema) {
    //variables locales
    int sock = 0;
    struct iovec paq[4];
    uint32_t tam_tema = strlen(tema) + 1, tam_envio = 0, res = -1, op = 4;

    //conectar con el broker
    sock = conectar_broker();
    if (sock < 0) {
        perror("error al conectar con el broker");
        return -1;
    }

    //crear paquete para enviar
    op = htonl(op);
    tam_envio = htonl(tam_tema);
    res = htonl(cod);
    paq[0].iov_base = &op;
    paq[0].iov_len = sizeof(uint32_t);
    paq[1].iov_base = &res;
    paq[1].iov_len = sizeof(uint32_t);
    paq[2].iov_base = &tam_envio;
    paq[2].iov_len = sizeof(uint32_t);
    paq[3].iov_base = (char *)tema;
    paq[3].iov_len = tam_tema;

    //enviar paquete
    if ((writev(sock,paq,4)) < 0) {
        perror("error al enviar el paquete al broker al subscribir un tema");
        return -1;
    }

    //recibir respuesta
    if (recv(sock,&res,sizeof(uint32_t),MSG_WAITALL) < 0){
        perror("error al recibir respuesta en la biblioteca\n");
        return -1;
    }

    //terminamos
	close(sock);
    if (res == 0) return 0;
    else return -1;
}

int inicio_subscriptor(void (*notif_evento)(const char *, const char *),
                void (*alta_tema)(const char *),
                void (*baja_tema)(const char *)) {
	//almacenar ptros a funcion
    handler_event = notif_evento;
    if (alta_tema != NULL) {
        alta_tema_ = alta_tema;
    }
    if (baja_tema != NULL) {
        baja_tema_ = baja_tema;
    }

    //configurar el thread solo si no existe
    if (existe_th == 0) {
        pthread_t hilo;
        int valor = 42;

        // Crear un nuevo hilo
        if (pthread_create(&hilo, NULL, thread_handler, &valor) != 0) {
            printf("Error al crear el hilo.\n");
            return 1;
        }
    }

    if (alta_tema != NULL) {
        //esperar a que el thread cree la conexion
        pthread_mutex_lock(&mutex);
        while (existe_th == 0) {
            pthread_mutex_unlock(&mutex);
            sleep(0.5);
            pthread_mutex_lock(&mutex);
        }
        pthread_mutex_unlock(&mutex);

        //dar de alta
        struct iovec paq[2];
        uint32_t op = 5, res = -1;
        sock = conectar_broker();

        //crear paquete para enviar 
        op = htonl(op);
        res = htonl(cod);
        paq[0].iov_base = &op;
        paq[0].iov_len = sizeof(uint32_t);
        paq[1].iov_base = &res;
        paq[1].iov_len = sizeof(uint32_t);

        //enviar paquete
        if ((writev(sock,paq,2)) < 0) {
            perror("error al enviar el paquete al broker");
            return -1;
        }

        //recibir codigo
        if (recv(sock,&res,sizeof(uint32_t),MSG_WAITALL) < 0){
            perror("error al recibir respuesta en la biblioteca\n");
            return -1;
        }
    }
    
    return 0;
}

int fin_subscriptor() {
	//dar de alta
    struct iovec paq[2];
    uint32_t op = 6, res = -1;
    sock = conectar_broker();

    //crear paquete para enviar 
    op = htonl(op);
    res = htonl(cod);
    paq[0].iov_base = &op;
    paq[0].iov_len = sizeof(uint32_t);
    paq[1].iov_base = &res;
    paq[1].iov_len = sizeof(uint32_t);

    //enviar paquete
    if ((writev(sock,paq,2)) < 0) {
        perror("error al enviar el paquete al broker");
        return -1;
    }

    //recibir codigo
    if (recv(sock,&res,sizeof(uint32_t),MSG_WAITALL) < 0){
        perror("error al recibir respuesta en la biblioteca\n");
        return -1;
    }

    return 0;
}

