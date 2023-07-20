#include "comun.h"
#include "map.h"
#include "queue.h"
#include "set.h"
#include <pthread.h>

//declarar funciones auxiliares
void * recibir_mensajes(void * arg);
int recibir_op(int sock);
void recibir_tema(int socket, char ** tema);

//funcionalidades
int crear_cliente();
int fin_cliente();
int subscribir();
int desubscribir();
int publicar_evento();
int get_evento();
int temas();
int n_clientes();
int n_subscriptores();
int n_eventos_pendientes();
int crear_tema();
int eliminar_tema();

//manejadores
void cerrar_conexiones(void *c, void *v); //al destruir el mapa
void visitar_elem(void *c, void *v); //revisar elementos

//declarar estructuras
struct client {
    int id; //UUID unico
    struct queue * cola_eventos; //eventos pendientes
    struct queue * cola_temas; //temas subscritos
};
struct evento {
    char * tema;
    int tam_msg;
    void * msg;
};