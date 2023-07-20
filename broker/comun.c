#include "comun.h"

// debe usarse para obtener un UUID para el cliente
#define STR_VALUE(x) STR(x)
#define STR(x) #x

int generate_UUID(UUID_t uuid) {
    FILE *d = popen("dbus-uuidgen", "r");
    if (d)
        fscanf(d, "%"STR_VALUE(UUID_SIZE)"s", uuid);
    pclose(d);
    return (d? 0:-1);
}

ssize_t  writevn(int fd, struct iovec *vector, int count, size_t n) {
   int i;
   size_t nleft;
   ssize_t nwritten;

   nleft = n;
   while (nleft > 0) {
      if ( (nwritten = writev(fd, vector, count)) <= 0) {
         if (errno == EINTR)
            nwritten = 0;           /* and call writev() again */
         else
            return(-1);                     /* error */
      }

      nleft -= nwritten;
      if (nwritten>0 && nleft>0) {
         i = 0;
         while (i<count && nwritten>0) {
            if (vector[i].iov_len<=nwritten) {
               nwritten -= vector[i].iov_len;
               vector[i].iov_len = 0;
            } else {
               vector[i].iov_len -= nwritten;
               vector[i].iov_base = (void *) ((char *) vector[i].iov_base + nwritten);
               nwritten = 0;
            }
            i++;
         }
      }
   }
   return(n);
}

ssize_t readn(int fd, void *vptr, size_t n) {
   size_t    nleft;
   ssize_t   nread;
   char      *ptr;

   ptr = vptr;
   nleft = n;
   while (nleft > 0) {
      if ( (nread = read(fd, ptr, nleft)) < 0) {
      if (errno == EINTR)
            nread = 0;          /* and call read() again */
      else
            return(-1);
      } else if (nread == 0)
      break;                                /* EOF */

      nleft -= nread;
      ptr   += nread;
   }

   return(n - nleft);                /* return >= 0 */
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