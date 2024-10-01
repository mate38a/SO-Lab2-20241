#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Función personalizada para reemplazar getline() en Windows
ssize_t custom_getline(char **lineptr, size_t *n, FILE *stream) {
    char *buf = NULL;
    char ch;
    size_t len = 0;

    if (*lineptr == NULL || *n == 0) {
        *n = 128; // Tamaño inicial del buffer
        buf = malloc(*n);
        if (buf == NULL) {
            return -1;
        }
        *lineptr = buf;
    } else {
        buf = *lineptr;
    }

    while (fread(&ch, 1, 1, stream) > 0) {
        if (len + 1 >= *n) {
            *n *= 2;
            char *new_buf = realloc(buf, *n);
            if (new_buf == NULL) {
                return -1;
            }
            buf = new_buf;
            *lineptr = buf;
        }

        buf[len++] = ch;
        if (ch == '\n') {
            break;
        }
    }

    if (len == 0) {
        return -1; // End of file or error
    }

    buf[len] = '\0';
    return len;
}

int main(int argc, char *argv[]) {
    char *line = NULL;  // Línea de entrada del usuario
    size_t len = 0;     // Tamaño del buffer para getline()
    ssize_t nread;      // Número de caracteres leídos

    // Verifica si el programa se ejecuta en modo batch (con un archivo de entrada)
    FILE *input = stdin;  // El archivo de entrada por defecto es la entrada estándar (stdin)
    if (argc == 2) {
        input = fopen(argv[1], "r");  // Abre el archivo batch
        if (input == NULL) {
            perror("Error abriendo archivo batch");
            exit(EXIT_FAILURE);
        }
    } else if (argc > 2) {
        fprintf(stderr, "Usage: %s [batch file]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Bucle principal del shell
    while (1) {
        if (argc == 1) {  // Si estamos en modo interactivo, imprime el prompt
            printf("wish> ");
        }

        nread = custom_getline(&line, &len, input);  // Usar custom_getline en lugar de getline
        if (nread == -1) {  // Si se alcanza el final de la entrada, salir del bucle
            break;
        }

        // Quitar el salto de línea al final de la entrada
        line[strcspn(line, "\n")] = '\0';

        // Aquí es donde procesaremos la línea de entrada en los siguientes pasos

    }

    // Liberar recursos y cerrar archivos abiertos
    free(line);
    if (input != stdin) {
        fclose(input);
    }

    return 0;
}
