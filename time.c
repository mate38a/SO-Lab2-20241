#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <comando>\n", argv[0]);
        return EXIT_FAILURE;
    }

    struct timeval start, end;
    pid_t pid;

    // Obtener tiempo inicial
    gettimeofday(&start, NULL);

    // Crear proceso hijo
    pid = fork();

    if (pid < 0) {
        // Error en fork
        fprintf(stderr, "Error en fork()\n");
        return EXIT_FAILURE;
    } 
    else if (pid == 0) {
        // Proceso hijo
        execvp(argv[1], &argv[1]);
        // Si llegamos aquí, hubo error en exec
        fprintf(stderr, "Error al ejecutar %s\n", argv[1]);
        exit(EXIT_FAILURE);
    } 
    else {
        // Proceso padre
        int status;
        wait(&status);
        
        // Obtener tiempo final
        gettimeofday(&end, NULL);

        // Calcular tiempo transcurrido en segundos
        double elapsed = (end.tv_sec - start.tv_sec) + 
                        (end.tv_usec - start.tv_usec) / 1000000.0;

        printf("Tiempo transcurrido: %.5f segundos\n", elapsed);
    }

    return EXIT_SUCCESS;
}
