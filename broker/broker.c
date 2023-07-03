#include "broker.h"

//inicio de la aplicación
int main(int argc, char *argv[]){

    //si no pasamos un puerto, error
    if(argc!=2) {
        fprintf(stderr, "Uso: %s puerto\n", argv[0]);
        return 1;
    }

    //variables locales necesarias
    //res -> resultado
    //s -> puerto por el que damos servicio
    //atoi -> pasar argumentos a Integer
    int port = atoi(argv[1]), s;
    unsigned int tam_dir, opcion=1;
    struct iovec res_op[4];
    struct sockaddr_in dir, dir_cliente;
    int s_connect = -1; //almacenar direccion del socket a utilizar
    
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

    //arrancar estructuras necesarias para gestionar las colas
    dic = dic_create();
    dic_wait = dic_create();

    //bucle servidor
    while (1) {
        //aceptar peticiones
        tam_dir = sizeof(dir_cliente);
        if ((s_connect = accept(s,(struct sockaddr *)&dir_cliente,&tam_dir)) < 0){
            perror("error al aceptar una peticion en el broker");
            exit(4);
        }
        
        //tratar mensajes
        recibir_mensajes(s_connect,res_op);

        //cerrar conexion con el cliente
        if (!flag_cl_espera) close(s_connect);
        else flag_cl_espera = 0;
    }

    //cerrar conexion y finalizar
    close(s);
    return 0;
}


//recibir mensajes y ejecutar accion necesaria
void recibir_mensajes(int sock, struct iovec res_op[4]){
    //variables locales
    int aux = 0;
    uint32_t res = 0, op = 0, tam_res = 0, tam_msg = 0, blq = 0;
    void * msg = NULL;
    char * name = NULL;
    struct msg_cola * cl = NULL;
    
    //obtener mensaje
    op = recibir_op(sock);
    if (op == -1) {perror("error al recibir operador en broker"); return;}
    recibir_nombre(sock,&name);
    
    if (name == NULL) {perror("error al recibir nombre de la cola en broker"); return;}

    //realizar operacion
    /*
    1 -> operacion
    2 -> convertir resultado a formato de red
    3 -> configurar paquete respuesta
    */
    switch (op) {
    case 0: //crear cola
        res = create_cola(name);
        res = htonl(res);

        res_op[0].iov_base = &res;
        res_op[0].iov_len = sizeof(uint32_t);
        
        writev(sock,res_op,1);

        break;
    case 1: //destruir cola
        res = destroy_cola(name);
        res = htonl(res);

        res_op[0].iov_base = &res;
        res_op[0].iov_len = sizeof(uint32_t);
        
        writev(sock,res_op,1);

        break;
    case 2: //meter datos
        //recibir mensaje
        tam_msg = recibir_msg(sock,&msg);

        res = put_cola(name,msg,tam_msg,sock);
        res = htonl(res);

        res_op[0].iov_base = &res;
        res_op[0].iov_len = sizeof(uint32_t);
        
        writev(sock,res_op,1);

        break;
    case 3: //get no blq
        cl = get_cola(name,0);
        if (cl == NULL){ //si no existe cola, enviamos un 2, que el cliente interpreta como "no existe cola"
            if (flag_cola_vacia){
                res = htonl(2);
                tam_res = 0;
                res_op[0].iov_base = &res;
                res_op[0].iov_len = sizeof(uint32_t);
            } else {
                res = htonl(0);
                tam_res = 0;
                res_op[0].iov_base = &res;
                res_op[0].iov_len = sizeof(uint32_t);
            }    
            writev(sock,res_op,1);

        } else { //devolvemos el primer mensaje de la cola
            res = htonl(1);
            tam_res = htonl(cl->length);
            
            res_op[0].iov_base = &res;
            res_op[0].iov_len = sizeof(uint32_t);

            res_op[1].iov_base = &tam_res;
            res_op[1].iov_len = sizeof(uint32_t);

            res_op[2].iov_base = cl->msg;
            res_op[2].iov_len = cl->length;
            printf("%"PRIu32"\n", cl->length);
            writevn(sock,res_op,3, cl->length+ sizeof(uint32_t)+ sizeof(uint32_t));
	        free(cl->msg);

            free(cl);
        }
        break;
    case 4: //get blq
        cl = get_cola_blq(name,&aux,sock);
        if (aux == 2){ //error
            res = htonl(0);
            tam_res = 0;
            res_op[0].iov_base = &res;
            res_op[0].iov_len = sizeof(uint32_t);
            writev(sock,res_op,1);
        } else if (aux == 1){ //el cliente se queda esperando
            flag_cl_espera = 1;
        } else { //devolvemos mensaje
            //res = htonl(cl->length);
            res = htonl(1);
            tam_res = htonl(cl->length);
            blq = htonl(0);
            
            res_op[0].iov_base = &res;
            res_op[0].iov_len = sizeof(uint32_t);

            res_op[1].iov_base = &tam_res;
            res_op[1].iov_len = sizeof(uint32_t);

            res_op[2].iov_base = cl->msg;
            res_op[2].iov_len = cl->length;

            res_op[3].iov_base = &blq;
            res_op[3].iov_len = sizeof(uint32_t);

            writevn(sock,res_op,4,sizeof(uint32_t)+sizeof(uint32_t)+cl->length+sizeof(uint32_t));
            
	        free(cl->msg);

            free(cl);
        }
        break;
    default:
        perror("error codigo de operacion en el broker\n");
        break;
    }

    return;
}


//recibir operacion seleccionada
uint32_t recibir_op(int sock){
    //variables locales
    uint32_t res = 0;

    //recibir
    if((recv(sock,&res,sizeof(uint32_t),MSG_WAITALL)) < 0){
        perror("error al recibir el operador en el broker");
        return -1;
    }

    //modificar dato
    res = ntohl(res);

    return res;
}

//recibir el nombre de la cola sobre la que operar
void recibir_nombre(int sock, char ** name){
    //variables locales
    uint32_t res = 0;

    //recibir size nombre
    if((recv(sock,&res,sizeof(uint32_t),MSG_WAITALL)) < 0){
        perror("error al recibir el tamaño de la cola en el broker");
        return;
    }

    //modificar dato
    res = ntohl(res);
    *name = malloc(res);
    
    //recibir nombre de la cola sobre la que operar
    if((recv(sock,*name,res,MSG_WAITALL)) < 0){
        perror("error al recibir el nombre de la cola en el broker");
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
    printf("%"PRIu32" CLI-PUT \n", res);
    //recibir msg
    readn(sock, *msg, res);

    return res;
}



//crear la cola con nombre 'name'
uint32_t create_cola(char * name){
    //variables locales
    uint32_t res = 0;
    struct cola * cl = cola_create();
    struct cola * cl_cp = cola_create();
 
    //operar
    if ((dic_put(dic,name,cl)) < 0) {
        //ya existe la cola
        printf("la cola %s ya existe\n",name);
        cola_destroy(cl,0);
        res = 1;
    } else res = 0;
    //crear cola de espera
    if (res == 0){
        dic_put(dic_wait,name,cl_cp);
    }
    return res;
}

//destruir cola con nombre 'name'
uint32_t destroy_cola(char * name){
    //variables locales
    int existe_entrada = 0;
    uint32_t res = 0;
    struct cola * cl;

    //comprobar si existe cola
    dic_get(dic,name,&existe_entrada);
    
    //operar
    if (existe_entrada < 0) {
        //no existe cola, error
        printf("la cola %s a destruir no existe\n",name);
        res = 1;
    } else {
        //existe cola -> destruir
        cl = dic_get(dic,name,&existe_entrada);
        
        cerrar_conexiones(name);
	    dic_remove_entry(dic_wait,name,NULL);
        if ((dic_remove_entry(dic,name,NULL)) < 0){
            perror("error al eliminar la entrada del diccionario\n");
            return 1;
        } else res = 0;
        if ((cola_destroy(cl,NULL)) < 0) {
            perror("error al destruir la cola\n");
            return 1;
        }
    }
    
    return res;
}

//meter un mensaje msg con tam len en la cola name
uint32_t put_cola(char * name, void * msg, uint32_t len, int sock) {
    //variables locales
    int existe_entrada = 0;
    uint32_t res = 0;
    struct cola *cl = NULL;
    struct msg_cola *st_msg = malloc(sizeof(struct msg_cola)); //estructura para almacenar mensaje con su size
  
    //comprobar si existe cola
    cl = dic_get(dic,name,&existe_entrada);

    //asumimos que si no existe cola error
    if (existe_entrada < 0){
        printf("error en put, no existe cola %s\n",name);
        free(st_msg);
        res = 1;
    } else {
        //existe cola -> metemos mensaje con length
        if ((revisar_esperas(name,msg,sock,len)) == 0){ //no hay esperas
            st_msg->msg = msg;
            st_msg->length = len;
            if ((cola_push_back(cl,st_msg)) < 0) {
                printf("error al incluir mensaje en cola\n");
                res = 1;
            } else res = 0;
        } else res = 0;
    }
    
    return res;
}

//sacar mensaje de la cola name (null si hay algun problema)
//flag blq:
//si blq = 0 -> get no bloqueante
//si blq = 1 -> get bloqueante
struct msg_cola* get_cola(char * name, int blq){
    //variables locales
    int existe_entrada = 0, tam = -1;
    struct cola * cl;
    struct msg_cola * st_msg;
    
    //comprobar si existe cola
    cl = dic_get(dic,name,&existe_entrada);
 
    //asumimos que si no existe cola error
    if (existe_entrada < 0){
        printf("error en get, no existe cola\n");
        st_msg = NULL;
    } else {
        //existe cola -> ver si hay mensajes
        tam = cola_length(cl);
        if (tam == 0){ 
	        printf("error al obtener mensaje: cola %s vacia\n",name);
            flag_cola_vacia = 1;
            return NULL;
        } else {
            st_msg = cola_pop_front(cl,&existe_entrada);
            if (existe_entrada < 0) {
                printf("error al obtener mensaje get no blq\n");
                return NULL;
            }
        }
    }
    
    return st_msg;
}

//get bloqueante 
//dejar bloqueado un cliente si no hay mensaje (meter en dic auxiliar el sock)
struct msg_cola* get_cola_blq(char * name, int *blq, int sock) {
    int existe_entrada = 0, tam = -1;
    struct cola * cl;
    struct msg_cola * st_msg;
    struct msg_sock *st;
    st = malloc(sizeof(struct msg_sock));
    
    //comprobar si existe cola
    cl = dic_get(dic,name,&existe_entrada);

    //asumimos que si no existe cola error
    if (existe_entrada < 0){
        printf("error en get, no existe cola\n");
        st_msg = NULL;
	    *blq = 2;
    } else {
        //existe cola -> ver si hay mensajes
        tam = cola_length(cl);
        if (tam == 0){ //no hay mensaje -> esperar
            cl = dic_get(dic_wait,name,&existe_entrada);
            st->sock = sock;
            cola_push_back(cl,st);
            *blq = 1;
	        st_msg = NULL;
        } else { //hay mensaje -> devolver mensaje
            st_msg = cola_pop_front(cl,&existe_entrada);
            if (existe_entrada < 0) {
                printf("Error al obtener mensaje get blq\n");
                return NULL;                
            }
        }
    }
    
    return st_msg;   
}

//enviar mensaje de error a los clientes bloqueados si se destruye la cola
void cerrar_conexiones(char * name) {
    //variables locales
    int err = 0, length = 0, i = 0, res = 0, sock = 0;
    struct cola * cl = dic_get(dic_wait,name,&err);
    struct iovec st[1];
    struct msg_sock *st_sock;

    if (cl == NULL) {
        return; //no hay cola asociada a ese nombre
    } else{
        //hay cola -> enviar error
        length = cola_length(cl);
        for (i = 0; i < length; i++) {
            st_sock = cola_pop_front(cl,&err);
            res = htonl(0);
            st[0].iov_base = &res;
            st[0].iov_len = sizeof(uint32_t);
            writev(st_sock->sock,st,1);
            close(sock);
            free(st_sock);
        }
    }
}

//ver si hay algun cliente esperando por un mensaje en esa cola
int revisar_esperas(char * name, void * msg, int sock, uint32_t msg_len){
    //variables locales
    int res = 0, len = 0, tam_res = 0, flag = 1;
    uint32_t blq = htonl(1), espera = 0;
    struct cola * cl = dic_get(dic_wait,name,&res);
    struct iovec res_op [4];
    struct msg_sock *st;
    
    len = cola_length(cl);

    while (len > 0 && flag) {
        len--; //incrementar contador
        st = cola_pop_front(cl,&res);
        res = 1; //vamos a enviar a un cliente
        res = htonl(msg_len);
        tam_res = htonl(msg_len);
        
        res_op[0].iov_base = &res;
        res_op[0].iov_len = sizeof(uint32_t);

        res_op[1].iov_base = &tam_res;
        res_op[1].iov_len = sizeof(uint32_t);

        res_op[2].iov_base = msg;
        res_op[2].iov_len = msg_len;

        res_op[3].iov_base = &blq;
        res_op[3].iov_len = sizeof(uint32_t);

        writevn(sock,res_op,4,sizeof(uint32_t)+sizeof(uint32_t)+msg_len+sizeof(uint32_t));

        //si no recibo nada -> conexion muerta
        recv(st->sock,&espera,sizeof(uint32_t),MSG_WAITALL);
        espera = ntohl(espera);
        if (espera != 1) {
            free(st);
        } else { //si recibo algo -> trabajo terminado
            printf("espera=%d\n",espera);
            free(st);
            free(msg);
            flag = 0;
        }
    }

    //si flag a 0 -> ningun cliente en espera
    if (flag == 1) res = 0;

    return res;
}