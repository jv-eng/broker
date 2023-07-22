#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    SSL_CTX *ctx;
    SSL *ssl;
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    // Inicializar OpenSSL
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    // Crear contexto SSL
    ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) {
        perror("Error al crear contexto SSL");
        return 1;
    }

    // Cargar certificado y clave privada
    if (SSL_CTX_use_certificate_file(ctx, "/home/jv/broker/claves/certificate.pem", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        return 1;
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, "/home/jv/broker/claves/private_key.pem", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        return 1;
    }

    // Crear socket del servidor
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Error al crear el socket");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Asociar el socket a la dirección y al puerto
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error en bind");
        return 1;
    }

    // Escuchar por conexiones
    if (listen(server_fd, 1) < 0) {
        perror("Error en listen");
        return 1;
    }

    printf("Servidor listo para recibir conexiones seguras...\n");

    // Aceptar conexiones entrantes
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
    if (client_fd < 0) {
        perror("Error en accept");
        return 1;
    }

    // Establecer la conexión segura
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, client_fd);
    if (SSL_accept(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
    }

    // Leer y escribir los datos recibidos
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_read = SSL_read(ssl, buffer, BUFFER_SIZE);
        if (bytes_read <= 0) {
            break;
        } else {
            printf("Mensaje: %s\n",buffer);
        }
        SSL_write(ssl, buffer, bytes_read);
    }

    // Cerrar conexión y liberar recursos
    SSL_shutdown(ssl);
    close(client_fd);
    close(server_fd);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    return 0;
}
