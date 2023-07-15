#include "comun.h"
#include "dict.h"
#include "cola.h"

//definicion de funciones
int recibir_op(int sock);
void recibir_tema(int socket, char ** tema);
void recibir_mensajes(int socket);
void cerrar_conexiones(struct cola * cl, char * tema);
void notificar_usuarios(struct cola *cl, int op, char * tema);
//operaciones
uint32_t crear_tema(int socket);
uint32_t eliminar_tema(int socket);
uint32_t generar_evento();
void alta_subscripcion_tema();
void baja_subscripcion_tema();
void alta_recibir_tema();
void baja_recibir_tema();

//estructuras
struct evento {
   char * tema;
   char * msg;
};
struct client{
    int sock;
};