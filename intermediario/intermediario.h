#include "comun.h"
#include "dict.h"
#include "cola.h"

//definicion de funciones
int recibir_op(int sock);
void recibir_mensajes(int socket);
//operaciones
void crear_tema();
void eliminar_tema();
void generar_evento();
void alta_subscripcion_tema();
void baja_subscripcion_tema();
void alta_recibir_tema();
void baja_recibir_tema();

//estructuras
struct evento {
   char * tema;
   char * msg;
};