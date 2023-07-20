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