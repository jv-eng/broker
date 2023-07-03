#include <stdbool.h>
#include <stddef.h>

//cabeceras funciones auxiliares
int conectar_broker();

int createMQ(const char *cola);
int destroyMQ(const char *cola);

int put(const char *cola, const void *mensaje, uint32_t tam);
int get(const char *cola, void **mensaje, uint32_t *tam, bool blocking);

