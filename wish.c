#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#define BUFFER_SIZE 1024
#define MAX_ARGS 64

// Ruta de búsqueda inicial
char *path[] = {"/bin", NULL};

// Función para ejecutar comandos integrados
int execute_builtin(char **args);

// Función para ejecutar comandos externos
void execute_command(char **args);

// Función para manejar la redirección de salida
void handle_redirection(char **args);

int main(int argc, char *argv[]) {
    char input[BUFFER_SIZE];
    char *args[MAX_ARGS];
    char *token;
    int is_running = 1;

    while (is_running) {
        printf("wish> ");
        if (fgets(input, BUFFER_SIZE, stdin) == NULL) {
            break;  // Salir en caso de error o EOF
        }

        // Eliminar el salto de línea al final de input
        input[strcspn(input, "\n")] = 0;

        // Dividir el comando en argumentos
        int i = 0;
        token = strtok(input, " ");
        while (token != NULL && i < MAX_ARGS - 1) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        // Verificar si el comando es un comando integrado
        if (args[0] != NULL) {
            if (strcmp(args[0], "exit") == 0) {
                is_running = 0;
            } else if (execute_builtin(args) == 0) {
                continue;
            } else {
                // Ejecutar un comando externo
                execute_command(args);
            }
        }
    }

    return 0;
}

// Función para ejecutar comandos integrados
int execute_builtin(char **args) {
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL || args[2] != NULL) {
            fprintf(stderr, "An error has occurred\n");
        } else if (chdir(args[1]) != 0) {
            perror("chdir failed");
        }
        return 0;
    } else if (strcmp(args[0], "path") == 0) {
        // Limpiar la ruta y agregar los nuevos directorios especificados
        for (int i = 0; path[i] != NULL; i++) {
            free(path[i]);
        }
        int j = 0;
        while (args[++j] != NULL && j < MAX_ARGS) {
            path[j - 1] = strdup(args[j]);
        }
        path[j - 1] = NULL;
        return 0;
    }
    return -1;  // No es un comando integrado
}

// Función para ejecutar comandos externos
void execute_command(char **args) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
    } else if (pid == 0) {
        // Redirección de salida si es necesario
        handle_redirection(args);

        // Buscar el ejecutable en cada directorio del path
        for (int i = 0; path[i] != NULL; i++) {
            char command_path[BUFFER_SIZE];
            snprintf(command_path, sizeof(command_path), "%s/%s", path[i], args[0]);
            if (access(command_path, X_OK) == 0) {
                execv(command_path, args);
                perror("execv failed"); // Si execv falla
                exit(1);
            }
        }
        fprintf(stderr, "An error has occurred\n");  // No se encontró el ejecutable
        exit(1);
    } else {
        wait(NULL);  // Esperar a que el hijo termine
    }
}

// Función para manejar la redirección de salida
void handle_redirection(char **args) {
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], ">") == 0) {
            if (args[i + 1] == NULL || args[i + 2] != NULL) {
                fprintf(stderr, "An error has occurred\n");
                exit(1);
            }
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
            if (fd < 0) {
                perror("open failed");
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            close(fd);
            args[i] = NULL;  // Termina los argumentos antes del símbolo de redirección
            break;
        }
    }
}
