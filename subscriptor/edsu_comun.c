/*
   Incluya en este fichero todas las implementaciones que pueden
   necesitar compartir los módulos editor y subscriptor,
   si es que las hubiera.
*/

#include "edsu_comun.h"

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
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
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