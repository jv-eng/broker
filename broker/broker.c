#include "broker.h"
/*pthread_mutex_lock(&mutex);
    existe_th = 1;
    pthread_mutex_unlock(&mutex);*/
//variables globales
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; //mutex para acceder a las estructuras de datos
struct map * mapa_cl; 
struct map * mapa_temas;

//funciones auxiliares
int recibir_op(int sock) {
	//variables locales
    uint32_t res = 0;

    //recibir
    if((recv(sock,&res,sizeof(uint32_t),MSG_WAITALL)) < 0){
        perror("error al recibir el operador en el broker");
        return -1;
    }

    //modificar dato
    return ntohl(res);
}
void recibir_tema(int socket, char ** tema) {
    //variables locales
    uint32_t res = 0;

    //recibir size nombre
    if((recv(socket,&res,sizeof(uint32_t),MSG_WAITALL)) < 0){
        perror("error al recibir el tamaño de la cola en el broker");
        return;
    }

    //modificar dato
    res = ntohl(res);
    *tema = malloc(res);
    
    //recibir nombre de la cola sobre la que operar
    if((recv(socket,*tema,res,MSG_WAITALL)) < 0){
        perror("error al recibir el nombre del tema en el broker");
        return;
    }
}
void cerrar_conexiones(void *c, void *v) {
    
}
void visitar_elem(void *c, void *v) {

}

//funcionalidades
int crear_cliente() {
    return 0;
}
int fin_cliente() {
    return 0;
}
int subscribir() {
    return 0;
}
int desubscribir() {
    return 0;
}
int publicar_evento() {
    return 0;
}
int get_evento() {
    return 0;
}
int temas() {
    return 0;
}
int n_clientes() {
    return 0;
}
int n_subscriptores() {
    return 0;
}
int n_eventos_pendientes() {
    return 0;
}
int crear_tema() {
    return 0;
}
int eliminar_tema() {
    return 0;
}


//recibe el socket mediante arg
void * recibir_mensajes(void * arg) {
    //variables locales
    int socket = *((int*)arg);
	uint32_t op = 0, res = 0;
	struct iovec res_op[1];

	//recibir operador
	op = recibir_op(socket);

    //tratar segun el codigo
    switch (op) {
        case 0: //dar de alta un cliente
            res = crear_cliente(socket);
            res = htonl(res);
            res_op[0].iov_base = &res;
            res_op[0].iov_len = sizeof(uint32_t);
            writev(socket, res_op, 1);
            break;
        case 1: //dar de baja un cliente
            res = fin_cliente(socket);
            res = htonl(res);
            res_op[0].iov_base = &res;
            res_op[0].iov_len = sizeof(uint32_t);
            writev(socket, res_op, 1);
            break;
        case 2: //subscribir a un tema
            res = subscribir(socket);
            res = htonl(res);
            res_op[0].iov_base = &res;
            res_op[0].iov_len = sizeof(uint32_t);
            writev(socket, res_op, 1);
            break;
        case 3: //desubscribir a un tema
            res = desubscribir(socket);
            res = htonl(res);
            res_op[0].iov_base = &res;
            res_op[0].iov_len = sizeof(uint32_t);
            writev(socket, res_op, 1);
            break;
        case 4: //generar un evento
            res = publicar_evento(socket);
            res = htonl(res);
            res_op[0].iov_base = &res;
            res_op[0].iov_len = sizeof(uint32_t);
            writev(socket, res_op, 1);
            break;
        case 5: //obtener el primer evento de la cola
            res = get_evento(socket);
            res = htonl(res);
            res_op[0].iov_base = &res;
            res_op[0].iov_len = sizeof(uint32_t);
            writev(socket, res_op, 1);
            break;
        case 6: //crear un tema
            res = crear_tema(socket);
            res = htonl(res);
            res_op[0].iov_base = &res;
            res_op[0].iov_len = sizeof(uint32_t);
            writev(socket, res_op, 1);
            break;
        case 7: //eliminar un tema
            res = eliminar_tema(socket);
            res = htonl(res);
            res_op[0].iov_base = &res;
            res_op[0].iov_len = sizeof(uint32_t);
            writev(socket, res_op, 1);
            break;
        case 8: //obtener los temas disponibles en el sistema
            res = temas(socket);
            res = htonl(res);
            res_op[0].iov_base = &res;
            res_op[0].iov_len = sizeof(uint32_t);
            writev(socket, res_op, 1);
            break;
        case 9: //obtener numero de clientes del sistema
            res = n_clientes(socket);
            res = htonl(res);
            res_op[0].iov_base = &res;
            res_op[0].iov_len = sizeof(uint32_t);
            writev(socket, res_op, 1);
            break;
        case 10: //obtener numero de subscriptores de un tema
            res = n_subscriptores(socket);
            res = htonl(res);
            res_op[0].iov_base = &res;
            res_op[0].iov_len = sizeof(uint32_t);
            writev(socket, res_op, 1);
            break;
        case 11: //obtener numero de eventos pendientes de un cliente
            res = n_eventos_pendientes(socket);
            res = htonl(res);
            res_op[0].iov_base = &res;
            res_op[0].iov_len = sizeof(uint32_t);
            writev(socket, res_op, 1);
            break;
        default: //codigo no valido
            res = -1;
            res = htonl(res);
            res_op[0].iov_base = &res;
            res_op[0].iov_len = sizeof(uint32_t);
            writev(socket, res_op, 1);
            break;
    }

    return NULL;
}

int main(int argc, char *argv[]) {
	//si no se recibe puerto, error
	if (argc!=2) {
		fprintf(stderr, "Uso: %s puerto\n", argv[0]);
		return 1;
	}

	//variables locales necesarias
    //res -> resultado
    //s -> puerto por el que damos servicio
    //atoi -> pasar argumentos a Integer
    int port = atoi(argv[1]), s;
    unsigned int tam_dir, opcion=1;
    struct sockaddr_in dir_cliente, dir;
    int s_connect = -1;

    //arrancar estructuras de datos
    mapa_cl = map_create(key_int, 0);
    mapa_temas = map_create(key_string, 0);

	//crear conexion
    if ((s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        perror("error al crear el socket en el broker");
        exit(1);
    }

    //reutilizar socket
    if ((setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&opcion,sizeof(opcion))) < 0) {
        perror("error al configurar socket");
        close(s);
        return -1;
    }

	//configurar struct sockaddr
    dir.sin_addr.s_addr = INADDR_ANY;
    dir.sin_port = htons(port);
    dir.sin_family = PF_INET;

	//activar puerto
    if (bind(s,(struct sockaddr *)&dir,sizeof(dir)) < 0) {
        perror("bind");
        close(s);
        return -1;
    }
    if (listen(s,5) < 0){
        perror("listen");
        close(s);
        return -1;
    }

	//bucle servidor
    while (1) {
        //aceptar peticiones
        tam_dir = sizeof(dir_cliente);
        if ((s_connect = accept(s,(struct sockaddr *)&dir_cliente,&tam_dir)) < 0){
            perror("error al aceptar una peticion en el broker");
            exit(4);
        }

        //arrancar thread para encargarse de atender la peticion
        pthread_t hilo;
        int valor = s_connect;

        // Crear un nuevo hilo
        if (pthread_create(&hilo, NULL, recibir_mensajes, &valor) != 0) {
            printf("Error al crear el hilo.\n");
            return 1;
        }
    }

    //cerrar conexion y finalizar
    close(s);

	return 0;
}
