#ifndef _COMUN_H
#define _COMUN_H	1
#define UUID_SIZE 36

#include <unistd.h>
#include <stdio.h>
#include <stdio.h>
#include <inttypes.h>
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

//funciones compartidas
ssize_t readn(int fd, void *vptr, size_t n);
ssize_t  writevn(int fd, struct iovec *vector, int count, size_t n);

typedef char UUID_t[UUID_SIZE+1]; // +1 para el nulo del string

int generate_UUID(UUID_t uuid);

#endif // _COMUN_H
