#include "intermediario.h"

//estructuras globales
struct diccionario *dict; //almacena los temas y sus subscriptores
int s; //socket
struct sockaddr_in dir; //configuracion del socket
int cod_user = 0;

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
    int length = cola_length(cl) - 1, i, err;
    struct client * cli;
    for (i = 0; i < length; i++) {
        cli = cola_pop_front(cl, &err);
        close(cli->sock_eventos);
        free(cli);
    }
    cola_destroy(cl, NULL);
}
void notificar_usuarios(struct cola *cl, int oper, char * tema, void * msg) {
    //variables globales
    struct iovec paq[5];
    struct client * cli;
    int num_paq, tam_tema = strlen(tema)+1, tam_envio, tam_msg_envio, tam_msg, 
        length = cola_length(cl), op, i, err, tmp =-1;

    switch (oper) {
        case 0: //caso evento
            num_paq = 5;
            tam_msg = strlen(msg);
            op = 0;

            op = htonl(op);
            tam_envio = htonl(tam_tema);
            tam_msg_envio = htonl(tam_msg);

            paq[0].iov_base = &op;
            paq[0].iov_len = sizeof(uint32_t);

            paq[1].iov_base = &tam_envio;
            paq[1].iov_len = sizeof(uint32_t);

            paq[2].iov_base = (char *)tema;
            paq[2].iov_len = tam_tema;

            paq[3].iov_base = &tam_msg_envio;
            paq[3].iov_len = sizeof(uint32_t);

            paq[4].iov_base = (char *) msg;
            paq[4].iov_len = tam_msg;
            break;

        case 1: //tema creado
            num_paq = 3;
            op = 1;

            op = htonl(op);
            tam_envio = htonl(tam_tema);

            paq[0].iov_base = &op;
            paq[0].iov_len = sizeof(uint32_t);

            paq[1].iov_base = &tam_envio;
            paq[1].iov_len = sizeof(uint32_t);

            paq[2].iov_base = (char *)tema;
            paq[2].iov_len = tam_tema;

            break;

        case 2: //tema eliminado
            num_paq = 3;
            op = 1;

            op = htonl(op);
            tam_envio = htonl(tam_tema);

            paq[0].iov_base = &op;
            paq[0].iov_len = sizeof(uint32_t);

            paq[1].iov_base = &tam_envio;
            paq[1].iov_len = sizeof(uint32_t);

            paq[2].iov_base = (char *)tema;
            paq[2].iov_len = tam_tema;

            break;
    }

    for (i = 0; i < length; i++) {
        cli = cola_pop_front(cl, &err);
        if (tmp == cli->id) continue;
        tmp = cli->id;
        if ((writev(cli->sock_eventos,paq,num_paq)) < 0) {
            perror("error al generar el evento");
        }
        cola_push_back(cl, cli);
    }
}
struct client * check_user(int id, struct cola *cl) {
    //variables locales
    struct client * cli;
    int i, length = cola_length(cl), pos = -1, err;

    //revisar elementos
    for (i = 0; i < length && pos == -1; i++) {
        cli = cola_pop_front(cl, &err);
        if (cli->id == id) {
            pos = 0;
        }
        cola_push_back(cl, cli);
    }

    if (pos == -1) cli = NULL;

    return cli;
}


/*eventos editor*/
uint32_t crear_tema(int socket) {
    //variables locales
    char * tema;
    uint32_t res = 0;
    struct cola * cl = cola_create();
    int err;

    //obtener tema
    recibir_tema(socket, &tema); 

    //almacenar cola: comprobar que no existe
    if ((dic_put(dict, tema, cl)) < 0) {
        printf("error, el tema %s ya existe\n", tema);
        res = -1;
    } 
    else printf("creado tema \"%s\"\n",tema);

    //avisar a los usuarios que hay nuevo tema
    cl = dic_get(dict, "notif", &err);
    notificar_usuarios(cl, 1, tema, NULL);

    return res;
}
uint32_t eliminar_tema(int socket) {
    //variables locales
    int existe_entrada = 0, err;
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

    if (res == 0) {
        printf("eliminado tema \"%s\"\n",tema);
        cl = dic_get(dict, "notif", &err);
        notificar_usuarios(cl, 2, tema, NULL);
    }

    cerrar_conexiones(cl, tema);

    return res;
}
uint32_t generar_evento(int socket) {
    //variables locales
    uint32_t res = 0;
    int err;
    char * tema;
    void * msg = NULL;
    struct cola * cl;

    //recibir tema
    recibir_tema(socket, &tema); 
    cl = dic_get(dict, tema, &err);

    //recibir size msg
    if((recv(socket,&res,sizeof(uint32_t),MSG_WAITALL)) < 0){
        perror("error al recibir el tamaño de la cola en el broker");
        return -1;
    }

    //modificar dato
    res = ntohl(res);
    msg = malloc(res);

    //recibir msg
    readn(socket, msg, res);

    //notificar clientes
    printf("notificar usuarios cola %s con mensaje %p\n",tema, msg);
    notificar_usuarios(cl, 0, tema, msg);

    return 0;
}

//eventos subscriptor
uint32_t alta_subscripcion_tema(int socket) {
    //variables locales
    uint32_t res = 0;
    int err, cod;
    char * tema;
    struct cola *cl, *cl2;
    struct client * cli;

    //obtener cod
    cod = recibir_op(socket);

    //obtener tema
    recibir_tema(socket, &tema);
    cl = dic_get(dict, tema, &err);

    //comprobar usuario
    cli = check_user(cod, cl);
    if (cli == NULL) { //no esta subscrito
        cl2 = dic_get(dict, "all", &err);
        cli = check_user(cod, cl2);
        cola_push_back(cl, cli);
        printf("cliente %d subscrito a cola %s\n", cod, tema);
    } else {
        printf("cliente ya subscrito\n");
        res = -1;
    }

    return res;
}
uint32_t baja_subscripcion_tema(int socket) {
    //variables locales
    uint32_t res = 0;
    int err, cod, i, length, flag = 1;
    char * tema;
    struct cola *cl;
    struct client * cli, * cli2;

    //obtener cod
    cod = recibir_op(socket);

    //obtener tema
    recibir_tema(socket, &tema);
    cl = dic_get(dict, tema, &err);
    length = cola_length(cl);

    //comprobar usuario
    cli = check_user(cod, cl);
    if (cli != NULL) { //esta subscrito
        for (i = 0; i < length && flag; i++) {
            cli2 = cola_pop_front(cl,&err);
            if (cli2->id == cli->id) {
                flag = 0;
            } else cola_push_back(cl,cli2);
        }
        printf("cliente %d desubscrito a cola %s\n", cod, tema);
    } else {
        printf("cliente no subscrito\n");
        res = -1;
    }

    return res;
}
uint32_t alta_recibir_tema(int socket) {
    //variables locales
    uint32_t res = 0;
    int err, cod;
    struct cola *cl;
    struct client * cli;

    //obtener cod
    cod = recibir_op(socket);

    //obtener cola
    cl = dic_get(dict, "all", &err);

    //comprobar usuario
    cli = check_user(cod, cl);
    if (cli != NULL) { 
        cl = dic_get(dict, "notif", &err);
        cola_push_back(cl, cli);
        printf("cliente %d subscrito a temas\n", cod);
    } else {
        printf("cliente ya subscrito\n");
        res = -1;
    }

    return res;
}
uint32_t baja_recibir_tema(int socket) {
    //variables locales
    uint32_t res = 0;
    int err, cod, length, flag = 1, i;
    struct cola *cl;
    struct client * cli, * cli2;

    //obtener cod
    cod = recibir_op(socket);

    //obtener cola
    cl = dic_get(dict, "notif", &err);
    length = cola_length(cl);

    //comprobar usuario
    cli = check_user(cod, cl);
    if (cli != NULL) { 
        for (i = 0; i < length && flag; i++) {
            cli2 = cola_pop_front(cl,&err);
            if (cli2->id == cli->id) {
                flag = 0;
            } else cola_push_back(cl,cli2);
        }
        printf("cliente %d desubscrito a temas\n", cod);
    } else {
        printf("cliente no subscrito a temas\n");
        res = -1;
    }

    return res; 
}  
uint32_t alta_cliente(int socket) {
    //variables locales
    int err, sock_2, tmp;
    struct cola *cl;
    struct client * cli;
    struct sockaddr_in dir_cliente;
    unsigned int tam_dir = sizeof(dir_cliente);
    struct iovec paq[1];

    //crear cliente
    cli = malloc(sizeof(struct client));
    cli->id = cod_user;
    cod_user++;

    //enviar codigo
    tmp = htonl(cli->id);
    paq[0].iov_base = &tmp;
    paq[0].iov_len = sizeof(uint32_t);
    if ((writev(socket,paq,1)) < 0) {
        perror("error al enviar el paquete al broker");
        return -1;
    }

    //crear segundo socket
    if ((sock_2 = accept(s,(struct sockaddr *)&dir_cliente,&tam_dir)) < 0){
        perror("error al aceptar una peticion en el broker");
        exit(4);
    }

    cli->sock_eventos = sock_2;

    //almacenar cliente
    cl = dic_get(dict, "all", &err);
    cola_push_back(cl, cli);

    return 0;
}

//recibir y procesar mensajes
//recibe el socket y el struct en caso de 
void recibir_mensajes(int socket) {
	//variables locales
	uint32_t op = 0, res = 0;
	struct iovec res_op[1];

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
            res = generar_evento(socket); 
            res = htonl(res);
            res_op[0].iov_base = &res;
            res_op[0].iov_len = sizeof(uint32_t);
            writev(socket, res_op, 1);
            break;
		case 3: //alta subscripcion a tema
            res = alta_subscripcion_tema(socket);
            res = htonl(res);
            res_op[0].iov_base = &res;
            res_op[0].iov_len = sizeof(uint32_t);
            writev(socket, res_op, 1);
            break;
		case 4: //baja subscripcion a tema
            res = baja_subscripcion_tema(socket);
            res = htonl(res);
            res_op[0].iov_base = &res;
            res_op[0].iov_len = sizeof(uint32_t);
            writev(socket, res_op, 1);
            break;
		case 5: //alta a recibir nuevos temas
            res = alta_recibir_tema(socket);
            res = htonl(res);
            res_op[0].iov_base = &res;
            res_op[0].iov_len = sizeof(uint32_t);
            writev(socket, res_op, 1);
            break;
		case 6: //baja a recibir nuevos temas
            res = baja_recibir_tema(socket);
            res = htonl(res);
            res_op[0].iov_base = &res;
            res_op[0].iov_len = sizeof(uint32_t);
            writev(socket, res_op, 1);
            break;
        case 9:
            res = alta_cliente(socket);
            res = htonl(res);
            res_op[0].iov_base = &res;
            res_op[0].iov_len = sizeof(uint32_t);
            writev(socket, res_op, 1);
            break;
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
    int port = atoi(argv[1]);
    unsigned int tam_dir, opcion=1;
    struct sockaddr_in dir_cliente;
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
    dic_put(dict, "notif", cl); //cola para creacion/destruccion temas

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
