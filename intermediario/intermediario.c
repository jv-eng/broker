#include "intermediario.h"

//estructuras globales
struct diccionario *dict; //almacena los temas y sus subscriptores

/*funciones auxiliares*/
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
void cerrar_conexiones(struct cola * cl, char * tema) {
    //variables locales
    int i, length = cola_length(cl), err = 0;
    struct client * cli;
    struct cola * cl_all = dic_get(dict, "all", &err);
    
    //notificar a todos los usuarios que se ha eliminado un tema
    notificar_usuarios(cl_all,7,tema);   

    //revisar conexiones
    for (i = 0; i < length; i++) {
        cli = cola_pop_front(cl, &err);
        close(cli->sock);
        free(cli);
    }    
}
void notificar_usuarios(struct cola *cl, int oper, char * tema) {
    //variables
    int length = cola_length(cl), i, op = htonl(oper), err;
    uint32_t tam_envio = htonl(strlen(tema));
    struct iovec paq[3];
    struct client * cli;
    
    //enviar datos
    for (i = 0; i < length; i++) {
        cli = cola_pop_front(cl,&err);
        paq[0].iov_base = &op;
        paq[0].iov_len = sizeof(uint32_t);
        paq[1].iov_base = &tam_envio;
        paq[1].iov_len = sizeof(uint32_t);
        paq[2].iov_base = tema;
        paq[2].iov_len = tam_envio;
        if ((writev(cli->sock,paq,3)) < 0) {
            perror("error al enviar el paquete");
            return;
        }
        cola_push_back(cl,cli);
    }
}
int check_user(int socket, struct cola *cl) {
    //variables locales
    struct client * cli;
    int i, length = cola_length(cl), pos = -1, err;

    //revisar elementos
    for (i = 0; i < length && pos == -1; i++) {
        cli = cola_pop_front(cl, &err);
        if (cli->sock == socket) {
            pos = 0;
        }
        cola_push_back(cl, cli);
    }

    return pos;
}


/*eventos editor*/
uint32_t crear_tema(int socket) {
    //variables locales
    char * tema;
    uint32_t res = 0;
    struct cola * cl = cola_create();

    //obtener tema
    recibir_tema(socket, &tema); 

    //almacenar cola: comprobar que no existe
    if ((dic_put(dict, tema, cl)) < 0) {
        printf("error, el tema %s ya existe\n", tema);
        res = -1;
    }
    printf("creado tema \"%s\"\n",tema);

    //avisar a los usuarios que hay nuevo tema
    notificar_usuarios(cl, 8, tema);

    return res;
}
uint32_t eliminar_tema(int socket) {
    //variables locales
    int existe_entrada = 0;
    uint32_t res = 0;
    char * tema;
    struct cola * cl;

    //obtener tema
    recibir_tema(socket, &tema); 

    //comprobar si existe cola
    dic_get(dict,tema,&existe_entrada);

    //operar
    if (existe_entrada < 0) {
        printf("el tema %s no existe\n",tema);
        res = -1;
    } else {
        cl = dic_get(dict,tema,&existe_entrada);
        cerrar_conexiones(cl, tema);
        if ((dic_remove_entry(dict,tema,NULL)) < 0){
            perror("error al eliminar la entrada del diccionario\n");
            res = -1;
        }
        if ((cola_destroy(cl,NULL)) < 0) {
            perror("error al destruir la cola\n");
            res = -1;
        }
    }

    if (res == 0) printf("eliminado tema \"%s\"\n",tema);

    return res;
}
uint32_t generar_evento(int socket);

//eventos subscriptor
uint32_t alta_subscripcion_tema(int socket) {
    //variables locales
    uint32_t res = 0;
    int err;
    char * tema;
    struct cola *cl;
    struct client * cli;

    //obtener tema
    recibir_tema(socket, &tema);
    cl = dic_get(dict, tema, &err);

    //comprobar
    if (check_user(socket, cl) < 0) {
        perror("Error, el cliente esta ya subscrito");
        res = -1;
    } else {
        cli = calloc(sizeof(struct client *), 1);
        cli->sock = socket;
        cola_push_back(cl, cli); //cola del tema
        cl = dic_get(dict, "all", &err); //cola general
        if (check_user(socket, cl) == 0) {
            cola_push_back(cl, cli);
        }
        printf("cliente %d dado de alta en tema %s\n",socket,tema);
    }

    return res;
}
uint32_t baja_subscripcion_tema(int socket);
uint32_t alta_recibir_tema(int socket);
uint32_t baja_recibir_tema(int socket);

//recibir y procesar mensajes
//recibe el socket y el struct en caso de 
void recibir_mensajes(int socket) {
	//variables locales
	uint32_t op = 0, res = 0;
	struct iovec res_op[4];

	//recibir operador
	op = recibir_op(socket);

	//operacion segun el operador
    //primero tratamos la operacion, luego notificamos resultado
	switch (op) {
		case 0: //crear tema
            res = crear_tema(socket);
            res = htonl(res);
            res_op[0].iov_base = &res;
            res_op[0].iov_len = sizeof(uint32_t);
            writev(socket, res_op, 1);
            break;
		case 1: //eliminar tema
            res = eliminar_tema(socket);
            res = htonl(res);
            res_op[0].iov_base = &res;
            res_op[0].iov_len = sizeof(uint32_t);
            writev(socket, res_op, 1);
            break;
		case 2: //generar evento
		case 3: //alta subscripcion a tema
            res = alta_subscripcion_tema(socket);
            res = htonl(res);
            res_op[0].iov_base = &res;
            res_op[0].iov_len = sizeof(uint32_t);
            writev(socket, res_op, 1);
            break;
		case 4: //baja subscripcion a tema
		case 5: //alta a recibir nuevos temas
		case 6: //baja a recibir nuevos temas
	}
}

//main
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
    struct sockaddr_in dir, dir_cliente;
    int s_connect = -1;
    struct cola * cl = cola_create();

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

	//arrancar estructuras
	dict = dic_create();
    dic_put(dict, "all", cl); //cola para todos los usuarios

	//bucle servidor
    while (1) {
        //aceptar peticiones
        tam_dir = sizeof(dir_cliente);
        if ((s_connect = accept(s,(struct sockaddr *)&dir_cliente,&tam_dir)) < 0){
            perror("error al aceptar una peticion en el broker");
            exit(4);
        }
        
        //tratar mensajes
        recibir_mensajes(s_connect);
    }

    //cerrar conexion y finalizar
    close(s);

	return 0;
}
