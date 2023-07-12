#include "intermediario.h"

//estructuras globales
struct diccionario *dict; //almacena los temas y sus subscriptores

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

void crear_tema();
void eliminar_tema();
void generar_evento();
void alta_subscripcion_tema();
void baja_subscripcion_tema();
void alta_recibir_tema();
void baja_recibir_tema();

//recibir y procesar mensajes
//recibe el socket y el struct en caso de 
void recibir_mensajes(int socket) {
	//variables locales
	int op = 0;
	struct iovec res_op;

	//recibir operador
	op = recibir_op(socket);

	//operacion segun el operador
	switch (op) {
		case 0: //crear tema
		case 1: //eliminar tema
		case 2: //generar evento
		case 3: //alta subscripcion a tema
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
