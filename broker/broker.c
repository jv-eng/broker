#include "broker.h"

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
//recibir mensaje, devuelve el size o -1
uint32_t recibir_msg(int sock, void **msg) {
    //variables locales
    uint32_t res = 0;

    //recibir size msg
    if((recv(sock,&res,sizeof(uint32_t),MSG_WAITALL)) < 0){
        perror("error al recibir el tamaño de la cola en el broker");
        return -1;
    }

    //modificar dato
    res = ntohl(res);
    *msg = malloc(res);
    printf("res = %d\n",res);
    printf("%"PRIu32" CLI-PUT \n", res);
    //recibir msg
    readn(sock, *msg, res);

    return res;
}
int check_set(struct set * s, struct uid_cl * uid) {
    int res = 1;
    struct set_iter * iter = set_iter_init(s);
    struct uid_cl * tmp;
    for (; set_iter_has_next(iter) && res != 0; set_iter_next(iter)) {
        tmp = (struct uid_cl *)set_iter_value(iter);
        res = strcmp(tmp->id, uid->id);
        //printf("comp %s - %s = %d\n",tmp->id, uid->id,res);
    }
    return res;
}
void cerrar_conexiones(void *c, void *v) {
    
}
void visitar_elem(void *c, void *v) {

}

//funcionalidades
int crear_cliente(int sock) {
    //variables locales
    uint32_t res = 0;
    struct client * cl;

    //crear cliente
    cl = malloc(sizeof(struct client));
    cl->cola_eventos = queue_create(0);
    cl->cola_temas = queue_create(0);

    //obtener uid
    if (recv(sock,&(cl->id),sizeof(UUID_t),MSG_WAITALL) < 0){
        perror("error al recibir respuesta en la biblioteca\n");
        return -1;
    }
    printf("UUID recibido: %s\n",cl->id);

    //comprobar si el cliente existe
    pthread_mutex_lock(&mutex);
    if (map_put(mapa_cl, &(cl->id), cl) < 0) {
        fprintf(stderr, "cliente duplicado\n");
        res = -1;
    }
    pthread_mutex_unlock(&mutex);

    return res;
}
int fin_cliente(int sock) {
    //variables locales
    int res, err, length, i;
    char * tema;
    UUID_t uid;
    struct client * cl;
    struct set * s;
    struct event * evento;

    //obtener cliente
    if (recv(sock,&uid,sizeof(UUID_t),MSG_WAITALL) < 0){
        perror("error al recibir uid\n");
        return -1;
    }

    //obtener cliente
    pthread_mutex_lock(&mutex);
    cl = map_get(mapa_cl, uid, &err);
    if (cl) {
        length = queue_length(cl->cola_temas);
        //eliminar de las listas de temas
        for (i = 0; i < length; i++) {
            tema = queue_pop_front(cl->cola_temas, &err);
            s = map_get(mapa_temas, tema, &err);
            set_remove(s,uid,NULL);
            free(tema);
        }
        //eliminar lista de eventos
        length = queue_length(cl->cola_eventos);
        for (i = 0; i < length; i++) {
            evento = queue_pop_front(cl->cola_eventos,&err);
            evento->cont--;
            if (evento->cont == 0) {
                free(evento);
            }
        }
        map_remove_entry(mapa_cl, uid, NULL);
        printf("fin cliente %s\n", uid);
    } else res = -1;
    pthread_mutex_unlock(&mutex);

    return res;
}
int subscribir(int sock) {
    //variables locales
    int res, err;
    char * tema;
    struct set * s;
    struct client * cl;
    struct uid_cl * id_cl = malloc(sizeof(struct uid_cl));

    //obtener uid
    if (recv(sock,id_cl->id,sizeof(UUID_t),MSG_WAITALL) < 0){
        perror("error al recibir uid\n");
        return -1;
    }

    //obtener tema
    recibir_tema(sock, &tema);

    //almacenar en set
    pthread_mutex_lock(&mutex);
    s = map_get(mapa_temas, tema, &err);
    if (check_set(s, id_cl) == 0) {
        printf("error, cliente existente\n");
        res = -1;
    } else {
        //almacenar en lista de temas subscritos
        set_add(s, id_cl);
        cl = map_get(mapa_cl,id_cl->id,&err);
        queue_push_back(cl->cola_temas, tema);
        printf("cliente %s subscrito a tema %s\n", id_cl->id, tema);
    }
    pthread_mutex_unlock(&mutex);    

    return res;
}
int desubscribir(int sock) {
    //variables locales
    int res = 0, err, length, i, flag = 1;
    char * tema, * tema_aux;
    UUID_t uid;
    struct set * s;
    struct client * cl;

    //obtener uid
    if (recv(sock,&uid,sizeof(UUID_t),MSG_WAITALL) < 0){
        perror("error al recibir uid\n");
        return -1;
    }

    //obtener tema
    recibir_tema(sock, &tema);

    //eliminar cliente de la subscripcion
    pthread_mutex_lock(&mutex);
    s = map_get(mapa_temas, tema, &err);
    if (set_contains(s,uid) == 0) {
        set_remove(s, uid, NULL);
        cl = map_get(mapa_cl, uid, &err);
        length = queue_length(cl->cola_temas);
        for (i = 0; i < length && flag; i++) { 
            tema_aux = queue_pop_front(cl->cola_temas,&err);
            if (strcmp(tema, tema_aux) == 0) {
                flag = 0;
                printf("tema %s eliminado de cliente %s\n",tema,uid);
            } else {
                queue_push_back(cl->cola_temas, tema_aux);
            }
        }
    } else printf("error, cliente ya subscrito\n");
    pthread_mutex_unlock(&mutex);  

    return res;
}
int publicar_evento(int sock) {
    //variables locales
    int res = 0, err;
    uint32_t tam = 0;
    char * tema;
    void * msg;
    struct event * ev;
    struct set * s;
    struct set_iter * iter;
    struct client * cl; 
    struct uid_cl * uid;

    //recibir tema
    recibir_tema(sock, &tema);

    //recibir msg
    tam = recibir_msg(sock, &msg);

    //almacenar
    ev = malloc(sizeof(struct event));
    ev->cont = 0;
    ev->msg = msg;
    ev->tam_msg = tam;
    ev->tema = tema;

    //obtener set de clientes y almacenar en las listas
    pthread_mutex_lock(&mutex);
    s = map_get(mapa_temas,tema,&err);
    iter = set_iter_init(s);
    for (; set_iter_has_next(iter); set_iter_next(iter)) {
        uid = (struct uid_cl *)set_iter_value(iter);
        cl = map_get(mapa_cl,uid->id,&err);
        queue_push_back(cl->cola_eventos,ev);
        ev->cont++;
    }
    set_iter_exit(iter);    
    pthread_mutex_unlock(&mutex);

    return res;
}
int get_evento(int sock) {
    return 0;
}
int temas(int sock) {
    pthread_mutex_lock(&mutex);
    uint32_t res = map_size(mapa_temas);
    pthread_mutex_unlock(&mutex);
    return res;
}
int n_clientes(int sock) {
    pthread_mutex_lock(&mutex);
    uint32_t res = map_size(mapa_cl);
    pthread_mutex_unlock(&mutex);
    return res;
}
int n_subscriptores(int sock) {
    //variables locales
    uint32_t res;
    int err;
    char * tema;
    set * s;

    //obtener tema
    recibir_tema(sock, &tema);

    //obtener cola
    pthread_mutex_lock(&mutex);
    s = map_get(mapa_temas, tema, &err);
    if (s) {
        res = set_size(s);
    } else res = -1;
    pthread_mutex_unlock(&mutex);

    return res;
}
int n_eventos_pendientes(int sock) {
    //variables locales
    uint32_t res;
    int err;
    struct client * cl;
    UUID_t uid;

    //obtener cliente
    if (recv(sock,&uid,sizeof(UUID_t),MSG_WAITALL) < 0){
        perror("error al recibir uid\n");
        return -1;
    }

    //obtener cola
    pthread_mutex_lock(&mutex);
    cl = map_get(mapa_cl, uid, &err);
    if (cl) {
        res = queue_length(cl->cola_eventos);
    } else res = -1;
    pthread_mutex_unlock(&mutex);

    return res;
}
int crear_tema(int sock) {
    //variables locales
    uint32_t res;
    char * tema;
    set * q;

    //obtener tema
    recibir_tema(sock, &tema);

    //crear cola
    q = set_create(0);

    //insertar en mapa
    pthread_mutex_lock(&mutex);
    if (map_put(mapa_temas, tema, q) < 0) {
        fprintf(stderr, "tema duplicado\n");
        res = -1;
    } else printf("creado tema %s\n",tema);
    pthread_mutex_unlock(&mutex);

    return res;
}
int eliminar_tema(int sock) {
    //variables locales
    uint32_t res;
    char * tema;

    //obtener tema
    recibir_tema(sock, &tema);

    //eliminar del mapa
    pthread_mutex_lock(&mutex);
    if (map_remove_entry(mapa_cl, tema, cerrar_conexiones) < 0) {
        fprintf(stderr, "tema no existe\n");
        res = -1;
    } else printf("eliminado tema %s\n",tema);
    pthread_mutex_unlock(&mutex);

    return res;
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
    mapa_cl = map_create(key_string, 0);
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
