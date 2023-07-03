/*
 * Incluya en este fichero todas las definiciones que pueden
 * necesitar compartir el broker y la biblioteca, si es que las hubiera.
 */
//librerias necesarias
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>

#define MAX_TAM_NAME 65536
#define MAX_TAM_MSG 4294967296

//estructura para almacenar mensajes en la cola
struct msg_cola{
    void * msg;
    uint32_t length;
};

struct msg_sock{
    int sock;
};

ssize_t readn(int fd, void *vptr, size_t n);
ssize_t  writevn(int fd, struct iovec *vector, int count, size_t n);