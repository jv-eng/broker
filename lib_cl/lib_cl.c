#include "comun.h"
#include "lib_cl.h"

//crear cola en broker con nombre 'cola'
//devuelve 0 si la creacion ha sido correcta y -1 en otro caso
int createMQ(const char *cola) {
    //variables locales
    int sock = -1;
    struct iovec paq[3];
    uint32_t tam_cola = strlen(cola) + 1, tam_envio = 0, op = 0, res = -1;

    //no generar trafico si nombre mayor al especificado
    if (tam_cola-1 > MAX_TAM_NAME) {
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
    tam_envio = htonl(tam_cola);

    paq[0].iov_base = &op;
    paq[0].iov_len = sizeof(uint32_t);
    paq[1].iov_base = &tam_envio;
    paq[1].iov_len = sizeof(uint32_t);
    paq[2].iov_base = strdup(cola);
    paq[2].iov_len = tam_cola;

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
    
    res = ntohl(res);
    free(paq[2].iov_base);
    close(sock);
    if (res == 1) return -1;
    else return 0;
}

//destruir cola con nombre cola. devuelve 0 si la destruccion es exitosa o -1 en otro caso
int destroyMQ(const char *cola){
    //variables locales
    int sock = -1;
    struct iovec paq[3];
    uint32_t tam_cola = strlen(cola) + 1, tam_envio = 0, op = 1, res = -1;

    //conectar con el broker
    sock = conectar_broker();
    if (sock < 0) {
        perror("error al conectar con el broker");
        return -1;
    }

    //crear paquete para enviar 
    op = htonl(op);
    tam_envio = htonl(tam_cola);

    paq[0].iov_base = &op;
    paq[0].iov_len = sizeof(uint32_t);
    paq[1].iov_base = &tam_envio;
    paq[1].iov_len = sizeof(uint32_t);
    paq[2].iov_base = strdup(cola);
    paq[2].iov_len = tam_cola;

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
    res = ntohl(res);
    close(sock);
    free(paq[2].iov_base);
    if (res == 1) return -1;
    else return 0;
}

//operacion de escritura/envio
/*
cola -> nombre de la cola sobre la que operar
mensaje -> mensaje a incluir en la cola
tam -> size maximo del mensaje (si tam = 0 no se hace nada)
*/
int put(const char *cola, const void *mensaje, uint32_t tam) {
    //variables locales
    int sock = 0;
    struct iovec paq[5];
    uint32_t tam_cola = strlen(cola) + 1, tam_envio = 0, tam_msg, op = 2, res = -1;
    
    //no hacer nada si tam = 0, devolver op correcta, pero no generar trafico
    if (tam == 0) {return 0;}
    
    //no generar trafico si msg mayor al especificado
    if (tam > MAX_TAM_MSG) {
        printf("error, nombre del mensaje a enviar mayor a 2³²\n");
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
    tam_envio = htonl(tam_cola);
    tam_msg = htonl(tam);

    paq[0].iov_base = &op;
    paq[0].iov_len = sizeof(uint32_t);

    paq[1].iov_base = &tam_envio;
    paq[1].iov_len = sizeof(uint32_t);

    paq[2].iov_base = strdup(cola);
    paq[2].iov_len = tam_cola;

    paq[3].iov_base = &tam_msg;
    paq[3].iov_len = sizeof(uint32_t);

    paq[4].iov_base = strdup(mensaje);
    paq[4].iov_len = tam;
    
    //enviar paquete
    if ((writevn(sock,paq, 5, sizeof(uint32_t)+sizeof(uint32_t)+sizeof(uint32_t)+tam_cola+tam)) < 0) {
        perror("error al enviar el paquete al broker en put");
        return -1;
    }

    //recibir respuesta
    if (recv(sock,&res,sizeof(uint32_t),MSG_WAITALL) < 0){
        perror("error al recibir respuesta en la biblioteca, metodo put\n");
        return -1;
    }
    res = ntohl(res);
    close(sock);
    free(paq[2].iov_base);
    free(paq[4].iov_base);
    if (res == 0) return 0;
    else return -1;
}

//operacion de lectura/recepcion
/*
cola -> nombre de la cola sobre la que operar
mensaje -> var en la que se devuelve el mensaje
tam -> tam del mensaje recibido
blocking -> si es bloqueante o no (si es no blq y no hay mensaje en cola -> tam = 0)
*/
int get(const char *cola, void **mensaje, uint32_t *tam, bool blocking) {
    //variables locales
    int sock = 0;
    struct iovec paq[3];
    uint32_t tam_cola = strlen(cola) + 1, tam_envio = 0, res = -1, espera = 0;
    uint32_t op = (blocking)?4:3;

    //no hacer nada si tam = 0, devolver op correcta, pero no generar trafico
    if (tam == 0) {return 0;}

    //conectar con el broker
    sock = conectar_broker();
    if (sock < 0) {
        perror("error al conectar con el broker");
        return -1;
    }

    //crear paquete para enviar
    op = htonl(op);
    tam_envio = htonl(tam_cola);

    paq[0].iov_base = &op;
    paq[0].iov_len = sizeof(uint32_t);

    paq[1].iov_base = &tam_envio;
    paq[1].iov_len = sizeof(uint32_t);

    paq[2].iov_base = strdup(cola);
    paq[2].iov_len = tam_cola;

    
    //enviar paquete
    if ((writev(sock,paq,3)) < 0) {
        perror("error al enviar el paquete al broker en put");
        return -1;
    }

    //recibir respuesta
    if (recv(sock,&res,sizeof(uint32_t),MSG_WAITALL) < 0){
        perror("error al recibir respuesta en la biblioteca, metodo get\n");
        return -1;
    }
    res = ntohl(res);

    if (res == 0) {
        if (blocking) {
            printf("error en espera bloqueante\n");
        }
        perror("error al recibir mensaje en get\n");
        *mensaje = NULL;
        *tam = 0;
        return -1;
    }
    if (res == 2) {
        *mensaje = NULL;
        *tam = 0;
        return 0;
    }
    
    res = recv(sock,tam,sizeof(uint32_t),MSG_WAITALL); //recibir tam del mensaje
    *tam = ntohl(*tam);

    if (*tam == 0) {
        printf("error al recibir el mensaje en la biblioteca\n");
        *mensaje = NULL;
        *tam = 0;
    } else {
        *mensaje = malloc(*tam);
        printf("%"PRIu32"\n - CLI RECV", *tam);
        res = readn(sock,*mensaje,*tam); //recibir mensaje
        printf("READED(readn) %"PRIu32"\n - CLI RECV", *tam);
    }

    //ver si ha estado bloqueado
    if (blocking) {
        if(recv(sock,&espera,sizeof(uint32_t),MSG_WAITALL) < 0) {
            perror("error al recibir confirmacion de bloqueo\n");
            return -1;
        }
        espera = ntohl(espera);
        if (espera == 1) {
            //preparar respuesta
            espera = htonl(1);
            paq[0].iov_base = &espera;
            paq[0].iov_len = sizeof(uint32_t);
            writev(sock,paq,1);
        }
    }

    //cerrar conexion
    close(sock);
    free(paq[2].iov_base);

    return ((*tam > 0)?0:-1);
}


//realiza la conexion con el broker y devuelve el numero de socket o -1
int conectar_broker() {
    //variables locales
    int sock = -1, opcion = 1;
    char *dir_ip_server = NULL, *port_server = NULL;
    struct hostent *host_info;
    struct sockaddr_in dir;

    //obtener dir ip y puerto del broker
    dir_ip_server = getenv("BROKER_HOST");
    port_server = getenv("BROKER_PORT");
    if (dir_ip_server == NULL || port_server == NULL){
        perror("error al obtener el nombre y/o puerto del broker");
        return -1;
    }

    //configurar puerto
    if ((sock = socket(PF_INET,SOCK_STREAM, IPPROTO_TCP)) < 0){
        perror("error al crear el socket");
        return -1;
    }
    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&opcion,sizeof(opcion));

    //configurar puerto
    host_info = gethostbyname(dir_ip_server);
    dir.sin_addr = *(struct in_addr *)host_info->h_addr_list[0];
    dir.sin_port = htons(atoi(port_server));
    dir.sin_family = PF_INET;

    //conectar
    if ((connect(sock,(struct sockaddr *)&dir,sizeof(dir)) < 0)) {
        perror("error al conectar con el broker");
        return -1;
    }

    return sock;
}
