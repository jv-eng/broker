#include "editor.h"

int generar_evento(const char *tema, const char *valor) {
	return 0;
}

/* solo para la version avanzada */
int crear_tema(const char *tema) {
	//variables locales
    int sock = -1;
    struct iovec paq[3];
    uint32_t tam_tema = strlen(tema) + 1, tam_envio = 0, op = 0, res = -1;

    //no generar trafico si nombre mayor al especificado
    if (tam_tema-1 > MAX_TAM_NAME) {
        perror("error, nombre de la cola a crear mayor a 2¹⁶\n");
        return -1;
    } 

    //conectar con el broker
    sock = conectar_broker();
    if (sock < 0) {
        perror("error al conectar con el broker");
        return -1;
    }

    //crear paquete para enviar 
    op = htonl(op);
    tam_envio = htonl(tam_tema);

    paq[0].iov_base = &op;
    paq[0].iov_len = sizeof(uint32_t);
    paq[1].iov_base = &tam_envio;
    paq[1].iov_len = sizeof(uint32_t);
    paq[2].iov_base = (char *)tema;
    paq[2].iov_len = tam_tema;

    //enviar paquete
    if ((writev(sock,paq,3)) < 0) {
        perror("error al enviar el paquete al broker en createMQ");
        return -1;
    }

    //recibir respuesta
    if (recv(sock,&res,sizeof(uint32_t),MSG_WAITALL) < 0){
        perror("error al recibir respuesta en la biblioteca, metodo create\n");
        return -1;
    }
    
	//devolver resultado
    res = ntohl(res);
    close(sock);
    if (res == 1) return -1;
    else return 0;
}

/* solo para la version avanzada */
int eliminar_tema(const char *tema) {
	//variables locales
    int sock = -1;
    struct iovec paq[3];
    uint32_t tam_tema = strlen(tema) + 1, tam_envio = 0, op = 1, res = -1;

    //conectar con el broker
    sock = conectar_broker();
    if (sock < 0) {
        perror("error al conectar con el broker");
        return -1;
    }

    //crear paquete para enviar 
    op = htonl(op);
    tam_envio = htonl(tam_tema);

    paq[0].iov_base = &op;
    paq[0].iov_len = sizeof(uint32_t);
    paq[1].iov_base = &tam_envio;
    paq[1].iov_len = sizeof(uint32_t);
    paq[2].iov_base = (char *)tema;
    paq[2].iov_len = tam_tema;

    //enviar paquete
    if ((writev(sock,paq,3)) < 0) {
        perror("error al enviar el paquete al broker en destroyMQ");
        return -1;
    }

    //recibir respuesta
    if (recv(sock,&res,sizeof(uint32_t),MSG_WAITALL) < 0){
        perror("error al recibir respuesta en la biblioteca, metodo destroy\n");
        return -1;
    }

	//resultado
    res = ntohl(res);
    close(sock);
    if (res == 1) return -1;
    else return 0;
}

