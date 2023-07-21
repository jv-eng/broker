#include "edsu.h"
#include "comun.h"

UUID_t uuid;

// se ejecuta antes que el main de la aplicación
__attribute__((constructor)) void inicio(void){
    if (begin_clnt()<0) {
        fprintf(stderr, "Error al iniciarse aplicación\n");
        // terminamos con error la aplicación antes de que se inicie
	// en el resto de la biblioteca solo usaremos return
        _exit(1);
    }
}

// se ejecuta después del exit de la aplicación
__attribute__((destructor)) void fin(void){
    if (end_clnt()<0) {
        fprintf(stderr, "Error al terminar la aplicación\n");
        // terminamos con error la aplicación
	// en el resto de la biblioteca solo usaremos return
        _exit(1);
    }
}

// operaciones que implementan la funcionalidad del proyecto
int begin_clnt(void){
    //variables locales
    struct iovec paq[2];
    uint32_t res = 0, op = 0; op = htonl(op);
    int sock = -1;

    //generar uid
    res = generate_UUID(uuid);
    if (res != 0) perror("error al generar el UUID");
    printf("UUID generado: %s\n",uuid);

    //abrir socket
    sock = conectar_broker();
    
    //preparar paquete
    paq[0].iov_base = &op;
    paq[0].iov_len = sizeof(uint32_t);
    paq[1].iov_base = &uuid;
    paq[1].iov_len = sizeof(UUID_t);

    //enviar paquete
    if ((writev(sock,paq,2)) < 0) {
        perror("error al enviar el paquete al broker");
        return -1;
    }

    //recibir respuesta
    if (recv(sock,&res,sizeof(uint32_t),MSG_WAITALL) < 0){
        perror("error al recibir respuesta\n");
        return -1;
    }

    return ntohl(res);
}
int end_clnt(void){
    //variables locales
    struct iovec paq[2];
    uint32_t res = 0, op = 1; op = htonl(op);
    int sock = -1;

    //abrir socket
    sock = conectar_broker();
    
    //preparar paquete
    paq[0].iov_base = &op;
    paq[0].iov_len = sizeof(uint32_t);
    paq[1].iov_base = &uuid;
    paq[1].iov_len = sizeof(UUID_t);

    //enviar paquete
    if ((writev(sock,paq,2)) < 0) {
        perror("error al enviar el paquete al broker");
        return -1;
    }

    //recibir respuesta
    if (recv(sock,&res,sizeof(uint32_t),MSG_WAITALL) < 0){
        perror("error al recibir respuesta\n");
        return -1;
    }

    return ntohl(res);
}
int subscribe(const char *tema){
    //variables locales
    struct iovec paq[4];
    uint32_t res = 0, op = 2, length = strlen(tema) + 1, tam_envio;
    int sock = conectar_broker();

    //transformar datos
    op = htonl(op);
    tam_envio = htonl(length);
    
    //preparar paquete
    paq[0].iov_base = &op;
    paq[0].iov_len = sizeof(uint32_t);
    paq[1].iov_base = &uuid;
    paq[1].iov_len = sizeof(UUID_t);
    paq[2].iov_base = &tam_envio;
    paq[2].iov_len = sizeof(uint32_t);
    paq[3].iov_base = (char *) tema;
    paq[3].iov_len = tam_envio;

    //enviar paquete
    if ((writev(sock,paq,4)) < 0) {
        perror("error al enviar el paquete al broker");
        return -1;
    }

    //recibir respuesta
    if (recv(sock,&res,sizeof(uint32_t),MSG_WAITALL) < 0){
        perror("error al recibir respuesta\n");
        return -1;
    }

    return ntohl(res);
}
int unsubscribe(const char *tema){
    //variables locales
    struct iovec paq[4];
    uint32_t res = 0, op = 3, length = strlen(tema) + 1, tam_envio;
    int sock = conectar_broker();

    //transformar datos
    op = htonl(op);
    tam_envio = htonl(length);
    
    //preparar paquete
    paq[0].iov_base = &op;
    paq[0].iov_len = sizeof(uint32_t);
    paq[1].iov_base = &uuid;
    paq[1].iov_len = sizeof(UUID_t);
    paq[2].iov_base = &tam_envio;
    paq[2].iov_len = sizeof(uint32_t);
    paq[3].iov_base = (char *) tema;
    paq[3].iov_len = tam_envio;

    //enviar paquete
    if ((writev(sock,paq,4)) < 0) {
        perror("error al enviar el paquete al broker");
        return -1;
    }

    //recibir respuesta
    if (recv(sock,&res,sizeof(uint32_t),MSG_WAITALL) < 0){
        perror("error al recibir respuesta\n");
        return -1;
    }

    return ntohl(res);
}
int publish(const char *tema, const void *evento, uint32_t tam_evento){
    //variables locales
    struct iovec paq[5];
    uint32_t res = 0, op = 4, length = strlen(tema) + 1, tam_envio, tam_ev;
    int sock = conectar_broker();
printf("res = %d\n",tam_evento);
    //transformar datos
    op = htonl(op);
    tam_envio = htonl(length);
    tam_ev = htonl(tam_evento);
    
    //preparar paquete
    paq[0].iov_base = &op;
    paq[0].iov_len = sizeof(uint32_t);
    paq[1].iov_base = &tam_envio;
    paq[1].iov_len = sizeof(uint32_t);
    paq[2].iov_base = (char *) tema;
    paq[2].iov_len = length;
    paq[3].iov_base = &tam_ev;
    paq[3].iov_len = sizeof(uint32_t);
    paq[4].iov_base = (void *) evento;
    paq[4].iov_len = tam_evento;

    //enviar paquete
    if ((writev(sock,paq,5)) < 0) {
        perror("error al enviar el paquete al broker");
        return -1;
    }

    //recibir respuesta
    if (recv(sock,&res,sizeof(uint32_t),MSG_WAITALL) < 0){
        perror("error al recibir respuesta\n");
        return -1;
    }

    return ntohl(res);
}
int get(char **tema, void **evento, uint32_t *tam_evento){
    return 0;
}

//manejar temas
int crear_tema(char * tema) {
    //variables locales
    struct iovec paq[3];
    uint32_t res = 0, op = 6, length = strlen(tema) + 1, tam_envio;
    int sock = conectar_broker();

    //transformar datos
    op = htonl(op);
    tam_envio = htonl(length);
    
    //preparar paquete
    paq[0].iov_base = &op;
    paq[0].iov_len = sizeof(uint32_t);
    paq[1].iov_base = &tam_envio;
    paq[1].iov_len = sizeof(uint32_t);
    paq[2].iov_base = (char *) tema;
    paq[2].iov_len = tam_envio;

    //enviar paquete
    if ((writev(sock,paq,3)) < 0) {
        perror("error al enviar el paquete al broker");
        return -1;
    }

    //recibir respuesta
    if (recv(sock,&res,sizeof(uint32_t),MSG_WAITALL) < 0){
        perror("error al recibir respuesta\n");
        return -1;
    }

    return ntohl(res);
}
int eliminar_tema(char * tema) {
    //variables locales
    struct iovec paq[3];
    uint32_t res = 0, op = 7, length = strlen(tema) + 1, tam_envio;
    int sock = conectar_broker();

    //transformar datos
    op = htonl(op);
    tam_envio = htonl(length);
    
    //preparar paquete
    paq[0].iov_base = &op;
    paq[0].iov_len = sizeof(uint32_t);
    paq[1].iov_base = &tam_envio;
    paq[1].iov_len = sizeof(uint32_t);
    paq[2].iov_base = (char *) tema;
    paq[2].iov_len = tam_envio;

    //enviar paquete
    if ((writev(sock,paq,3)) < 0) {
        perror("error al enviar el paquete al broker");
        return -1;
    }

    //recibir respuesta
    if (recv(sock,&res,sizeof(uint32_t),MSG_WAITALL) < 0){
        perror("error al recibir respuesta\n");
        return -1;
    }

    return ntohl(res);
}

// operaciones que facilitan la depuración y la evaluación
int topics(){ // cuántos temas existen en el sistema
    //variables locales
    struct iovec paq[1];
    uint32_t res = 0, op = 8;
    int sock = conectar_broker();

    op = htonl(op);
    
    //preparar paquete
    paq[0].iov_base = &op;
    paq[0].iov_len = sizeof(uint32_t);

    //enviar paquete
    if ((writev(sock,paq,1)) < 0) {
        perror("error al enviar el paquete al broker");
        return -1;
    }

    //recibir respuesta
    if (recv(sock,&res,sizeof(uint32_t),MSG_WAITALL) < 0){
        perror("error al recibir respuesta\n");
        return -1;
    }

    return ntohl(res);
}
int clients(){ // cuántos clientes existen en el sistema
    //variables locales
    struct iovec paq[1];
    uint32_t res = 0, op = 9;
    int sock = conectar_broker();

    op = htonl(op);
    
    //preparar paquete
    paq[0].iov_base = &op;
    paq[0].iov_len = sizeof(uint32_t);

    //enviar paquete
    if ((writev(sock,paq,1)) < 0) {
        perror("error al enviar el paquete al broker");
        return -1;
    }

    //recibir respuesta
    if (recv(sock,&res,sizeof(uint32_t),MSG_WAITALL) < 0){
        perror("error al recibir respuesta\n");
        return -1;
    }

    return ntohl(res);
}
int subscribers(const char *tema){ // cuántos subscriptores tiene este tema
    //variables locales
    struct iovec paq[3];
    uint32_t res = 0, op = 10, length = strlen(tema) + 1, tam_envio;
    int sock = conectar_broker();

    //transformar datos
    op = htonl(op);
    tam_envio = htonl(length);
    
    //preparar paquete
    paq[0].iov_base = &op;
    paq[0].iov_len = sizeof(uint32_t);
    paq[1].iov_base = &tam_envio;
    paq[1].iov_len = sizeof(uint32_t);
    paq[2].iov_base = (char *) tema;
    paq[2].iov_len = tam_envio;

    //enviar paquete
    if ((writev(sock,paq,3)) < 0) {
        perror("error al enviar el paquete al broker");
        return -1;
    }

    //recibir respuesta
    if (recv(sock,&res,sizeof(uint32_t),MSG_WAITALL) < 0){
        perror("error al recibir respuesta\n");
        return -1;
    }

    return ntohl(res);
}
int events() { // nº eventos pendientes de recoger por este cliente
    //variables locales
    struct iovec paq[3];
    uint32_t res = 0, op = 11;
    int sock = conectar_broker();

    //transformar datos
    op = htonl(op);
    
    //preparar paquete
    paq[0].iov_base = &op;
    paq[0].iov_len = sizeof(uint32_t);
    paq[1].iov_base = uuid;
    paq[1].iov_len = sizeof(UUID_t);

    //enviar paquete
    if ((writev(sock,paq,2)) < 0) {
        perror("error al enviar el paquete al broker");
        return -1;
    }

    //recibir respuesta
    if (recv(sock,&res,sizeof(uint32_t),MSG_WAITALL) < 0){
        perror("error al recibir respuesta\n");
        return -1;
    }

    return ntohl(res);
}
