#include "comun.h"
#include "diccionario.h"
#include "cola.h"

//declaracion de variables globales
struct diccionario *dic; //diccionario donde guardar colas
struct diccionario *dic_wait; //diccionario de espera
int flag_cola_vacia = 0;
int flag_cl_espera = 0;

//declaracion de cabeceras de metodos auxiliares
void recibir_mensajes(int sock,struct iovec res_op[4]);

//operaciones
uint32_t create_cola(char * name);
uint32_t destroy_cola(char * name);
struct msg_cola* get_cola(char * name, int blq);
struct msg_cola* get_cola_blq(char * name, int *blq, int sock);
uint32_t put_cola(char * name, void * msg, uint32_t len, int sock);

//obtener datos
uint32_t recibir_op(int sock);
void recibir_nombre(int sock, char **name);
uint32_t recibir_msg(int sock, void **msg);

//cerrar las conexiones abiertas en el caso de que tengamos clientes esperando
//  y se destruya la cola
void cerrar_conexiones(char * name);

//comprobar los clientes en espera
int revisar_esperas(char * name, void * msg, int sock, uint32_t msg_len);