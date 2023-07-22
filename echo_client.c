#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    SSL_CTX *ctx;
    SSL *ssl;
    int client_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    // Inicializar OpenSSL
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    // Crear contexto SSL
    ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) {
        perror("Error al crear contexto SSL");
        return 1;
    }

    // Crear socket del cliente
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("Error al crear el socket");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &(server_addr.sin_addr));

    // Conectar al servidor
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error en connect");
        return 1;
    }

    // Establecer la conexión segura
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, client_fd);
    if (SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        return 1;
    }

    // Enviar y recibir datos del servidor
    while (1) {
        printf("Ingrese un mensaje (o 'salir' para terminar): ");
        fgets(buffer, BUFFER_SIZE, stdin);
        if (strcmp(buffer, "salir\n") == 0) {
            break;
        }
        SSL_write(ssl, buffer, strlen(buffer));
        memset(buffer, 0, BUFFER_SIZE);
        SSL_read(ssl, buffer, BUFFER_SIZE);
        printf("Respuesta del servidor: %s", buffer);
    }

    // Cerrar conexión y liberar recursos
    SSL_shutdown(ssl);
    close(client_fd);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    return 0;
}
