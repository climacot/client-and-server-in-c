#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "files.h"

void *handle_connection(void *p_cliente_socket);
void handle_sigterm(int);

int finished = 0;
sem_t mutex;

int main(int argc, char const *argv[])
{

    if (argc < 2)
    {
        printf("missing port\n");
        return 1;
    }

    sem_init(&mutex, 0, 1);

    int c;
    int queued;
    int s;
    socklen_t client_addr_len;
    struct sigaction act;
    struct sigaction oldact;
    struct sockaddr_in addr;
    struct sockaddr_in client_addr;

    act.sa_handler = handle_sigterm;

    sigaction(SIGINT, &act, &oldact);
    sigaction(SIGTERM, &act, &oldact);

    s = socket(AF_INET, SOCK_STREAM, 0);

    if (s == -1)
    {
        printf("Error al abrir el socket\n");
        return 1;
    }

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1]));

    bind(s, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
    queued = listen(s, 10);

    if (queued != 0)
    {
        printf("Error al preparar para recibir conexiones por el socket\n");
        return 1;
    }

    printf("listening in port [%s]\n", argv[1]);

    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    client_addr_len = sizeof(struct sockaddr_in);

    while (!finished)
    {
        c = accept(s, (struct sockaddr *)&client_addr, &client_addr_len);

        if (c == -1)
        {
            printf("Error al aceptar la conexion\n");
            continue;
        }

        printf("client connected\n");

        pthread_t t;
        int *pclient = malloc(sizeof(int));
        *pclient = c;
        pthread_create(&t, NULL, handle_connection, pclient);
    }

    close(s);
    return 0;
}

void *handle_connection(void *p_cliente_socket)
{
    char destino[255];
    request r;
    request *aux;
    int c = *((int *)p_cliente_socket);
    struct stat s;

    while (1)
    {
        memset(&r, 0, sizeof(request));
        validar_r_w(c, &r, &aux, sizeof(request), 1);
        // read(c, &r, sizeof(request));

        if (strcmp(r.operation, "get") == 0)
        {
            printf("operation, filename %s %s\n", r.filename, r.operation);
            strcpy(destino, "serverFiles/");
            strcat(destino, r.filename);
            enviar_archivo(c, destino);
        }
        else if (strcmp(r.operation, "put") == 0)
        {
            printf("operation, filename %s %s\n", r.filename, r.operation);

            if (stat("serverFiles", &s) < 0)
            {
                mkdir("serverFiles", S_IRWXU);
            }

            strcpy(destino, "serverFiles/");
            strcat(destino, r.filename);
            recibir_archivo(c, destino, &mutex);
        }
        else if (strcmp(r.operation, "exit") == 0)
        {
            printf("operation, filename %s %s\n", r.filename, r.operation);
            printf("closed conection with socket number [%d]\n", c);
            break;
        }
    }

    printf("closed conection success\n");
    close(c);
    return 0;
}

void handle_sigterm(int signal)
{
    printf("closed conection success\n");
    finished = 1;
}
