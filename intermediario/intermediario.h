#include "comun.h"
#include "dict.h"
#include "cola.h"

/*definicion de funciones*/
int recibir_op(int sock); //recibir operador
void recibir_tema(int socket, char ** tema); //recibir el tema
void recibir_mensajes(int socket); //recibir el mensaje del evento
//cerrar conexiones y notificar del evento
void cerrar_conexiones(struct cola * cl, char * tema);
//notificar si hay una creacion o destruccion de un tema
void notificar_usuarios(struct cola *cl, int op, char * tema);
int check_user(int socket, struct cola *cl); //ver si el usuario existe

//operaciones
uint32_t crear_tema(int socket);
uint32_t eliminar_tema(int socket);
uint32_t generar_evento(int socket);
uint32_t alta_subscripcion_tema(int socket);
uint32_t baja_subscripcion_tema(int socket);
uint32_t alta_recibir_tema(int socket);
uint32_t baja_recibir_tema(int socket);

//estructuras
struct evento {
   char * tema;
   char * msg;
};
struct client{
    int sock_contenidos;
    int sock_eventos;
};