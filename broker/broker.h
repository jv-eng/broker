#include "comun.h"
#include "map.h"
#include "queue.h"
#include "set.h"
#include <pthread.h>

//declarar funciones auxiliares
void * recibir_mensajes(void * arg);
int recibir_op(int sock);
void recibir_tema(int socket, char ** tema);
uint32_t recibir_msg(int sock, void **msg);

//funcionalidades
int crear_cliente(int sock);
int fin_cliente(int sock);
int subscribir(int sock);
int desubscribir(int sock);
int publicar_evento(int sock);
int get_evento(int sock);
int temas(int sock);
int n_clientes(int sock);
int n_subscriptores(int sock);
int n_eventos_pendientes(int sock);
int crear_tema(int sock);
int eliminar_tema(int sock);

//manejadores
void cerrar_conexiones(void *c, void *v); //al destruir el mapa
void visitar_elem(void *c, void *v); //revisar elementos

//declarar estructuras
struct client {
    UUID_t id; //UUID unico
    struct queue * cola_eventos; //eventos pendientes
    struct queue * cola_temas; //temas subscritos
};
struct event {
    char * tema; //nombre del tema
    int tam_msg; //tamaño del mensaje
    void * msg; //mensaje
    int cont; //contador de copias del evento
};
struct uid_cl {
    UUID_t id; //identificador de cliente
};