#include "editor.h"
#include "comun.h"
#include "edsu_comun.h"

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
    paq[2].iov_base = tema;
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
    paq[2].iov_base = tema;
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