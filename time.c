#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

// Función para calcular el tiempo en milisegundos
double getCurrentTime() {
    FILETIME fileTime;
    ULARGE_INTEGER time;
    GetSystemTimeAsFileTime(&fileTime);
    time.LowPart = fileTime.dwLowDateTime;
    time.HighPart = fileTime.dwHighDateTime;
    return (double)time.QuadPart / 10000.0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <comando>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Medir el tiempo de inicio
    double start = getCurrentTime();

    // Crear un proceso hijo para ejecutar el comando
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Construir el comando completo
    char command[1024] = "";
    for (int i = 1; i < argc; i++) {
        strcat(command, argv[i]);
        strcat(command, " ");
    }

    // Crear el proceso hijo
    if (!CreateProcess(NULL, command, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        fprintf(stderr, "Error al crear el proceso: %lu\n", GetLastError());
        return EXIT_FAILURE;
    }

    // Esperar a que el proceso hijo termine
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Medir el tiempo de fin
    double end = getCurrentTime();

    // Calcular el tiempo transcurrido
    double elapsed = (end - start) / 1000.0; // Convertir a segundos
    printf("Elapsed time: %.5f seconds\n", elapsed);

    // Cerrar los handles
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return EXIT_SUCCESS;
}
