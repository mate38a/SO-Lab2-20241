#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

#define BUFFER_SIZE 1024
#define MAX_ARGS 64

// Ruta de búsqueda inicial
char *path[] = {"/bin", NULL};

// Función para ejecutar comandos integrados
int execute_builtin(char **args);

// Función para ejecutar comandos externos
void execute_command(char **args, int background);

// Función para manejar la redirección de entrada y salida
int handle_redirection(char **args);

// Función para manejar pipes
int handle_pipes(char **args);

int main() {
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
                int background = 0;
                // Verificar si el último argumento es '&'
                if (i > 0 && strcmp(args[i - 1], "&") == 0) {
                    background = 1;
                    args[i - 1] = NULL;
                }
                // Manejar pipes si es necesario
                if (!handle_pipes(args)) {
                    // Ejecutar un comando externo
                    execute_command(args, background);
                }
            }
        }
    }

    return 0;
}

// Función para ejecutar comandos integrados
int execute_builtin(char **args) {
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            // Cambio al directorio HOME si no hay argumentos
            char *home = getenv("HOME");
            if (home == NULL) {
                fprintf(stderr, "An error has occurred\n");
                return 1;
            }
            if (chdir(home) != 0) {
                perror("chdir failed");
                return 1;
            }
        } else if (args[2] != NULL) {
            // Error si hay más de un argumento
            fprintf(stderr, "An error has occurred\n");
            return 1;
        } else if (chdir(args[1]) != 0) {
            // Error si el directorio no es accesible
            perror("chdir failed");
            return 1;
        }
        return 0;
    } else if (strcmp(args[0], "path") == 0) {
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
    return -1;
}

// Función para ejecutar comandos externos
void execute_command(char **args, int background) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
    } else if (pid == 0) {
        // Redirección de entrada/salida
        if (handle_redirection(args) == -1) {
            fprintf(stderr, "Redirection error\n");
            exit(1);
        }

        // Buscar el ejecutable en cada directorio del path
        for (int i = 0; path[i] != NULL; i++) {
            char command_path[BUFFER_SIZE];
            snprintf(command_path, sizeof(command_path), "%s/%s", path[i], args[0]);
            if (access(command_path, X_OK) == 0) {
                execv(command_path, args);
                perror("execv failed");
                exit(1);
            }
        }
        fprintf(stderr, "Command not found: %s\n", args[0]);
        exit(1);
    } else {
        if (!background) {
            waitpid(pid, NULL, 0); // Esperar si no es en segundo plano
        }
    }
}

// Función para manejar la redirección de entrada y salida
int handle_redirection(char **args) {
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], ">") == 0) {
            if (args[i + 1] == NULL) {
                fprintf(stderr, "An error has occurred\n");
                return -1;
            }
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("open failed");
                return -1;
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            args[i] = NULL;
        } else if (strcmp(args[i], "<") == 0) {
            if (args[i + 1] == NULL) {
                fprintf(stderr, "An error has occurred\n");
                return -1;
            }
            int fd = open(args[i + 1], O_RDONLY);
            if (fd < 0) {
                perror("open failed");
                return -1;
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
            args[i] = NULL;
        }
    }
    return 0;
}

// Función para manejar pipes
int handle_pipes(char **args) {
    int pipe_index = -1;
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "|") == 0) {
            pipe_index = i;
            break;
        }
    }

    if (pipe_index == -1) {
        return 0; // No hay pipes
    }

    args[pipe_index] = NULL;

    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("pipe failed");
        return -1;
    }

    pid_t pid1 = fork();
    if (pid1 < 0) {
        perror("fork failed");
        return -1;
    } else if (pid1 == 0) {
        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        execvp(args[0], args);
        perror("execvp failed");
        exit(1);
    }

    pid_t pid2 = fork();
    if (pid2 < 0) {
        perror("fork failed");
        return -1;
    } else if (pid2 == 0) {
        dup2(pipe_fd[0], STDIN_FILENO);
        close(pipe_fd[1]);
        close(pipe_fd[0]);
        execvp(args[pipe_index + 1], &args[pipe_index + 1]);
        perror("execvp failed");
        exit(1);
    }

    close(pipe_fd[0]);
    close(pipe_fd[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    return 1;
}

