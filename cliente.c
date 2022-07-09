#include <arpa/inet.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#include "split.h"
#include "files.h"

void handle_sigterm(int);

int finished = 0;
sem_t mutex;

int main(int argc, char const *argv[])
{
    if (argc < 3)
    {
        printf("faltan el host y el puerto\n");
        exit(EXIT_FAILURE);
    }

    sem_init(&mutex, 0, 1);

    // argv[1] = "127.0.0.1", argv[2] = "1234"
    int s;
    struct sockaddr_in addr;
    int conectado;
    char comando[BUFSIZ];
    request r;
    request *aux;
    struct sigaction act;
    struct sigaction oldact;

    memset(&act, 0, sizeof(struct sigaction));
    memset(&oldact, 0, sizeof(struct sigaction));

    act.sa_handler = handle_sigterm;

    sigaction(SIGTERM, &act, &oldact);
    sigaction(SIGINT, &act, &oldact);

    s = socket(AF_INET, SOCK_STREAM, 0);

    if (s == -1)
    {
        printf("Error al abrir el socket\n");
        return 0;
    }

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    inet_aton(argv[1], &addr.sin_addr);

    conectado = connect(s, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));

    if (conectado == -1)
    {
        printf("Error al crear la conexion\n");
        return 0;
    }

    while (!finished)
    {
        memset(&r, 0, sizeof(request));
        memset(comando, 0, BUFSIZ);
        printf("Ingrese el comando \n");
        fgets(comando, BUFSIZ, stdin);
        split_list *l;

        l = split(comando, " \n\r\t");

        if (l->count == 0)
        {
            continue;
        }

        if (strcmp(l->parts[0], "exit") == 0)
        {
            strcpy(r.operation, l->parts[0]);
            strcpy(r.filename, "");
            // write(s, (char *)&r, sizeof(request));
            validar_r_w(s, (char *)&r, aux, sizeof(request), 0);
            finished = 1;
        }
        else if (strcmp(l->parts[0], "get") == 0 && l->count == 2)
        {
            strcpy(r.operation, l->parts[0]);
            strcpy(r.filename, l->parts[1]);
            // write(s, (char *)&r, sizeof(request));
            validar_r_w(s, (char *)&r, aux, sizeof(request), 0);
            recibir_archivo(s, l->parts[1], &mutex);
        }
        else if (strcmp(l->parts[0], "put") == 0 && l->count == 2)
        {
            strcpy(r.operation, l->parts[0]);
            strcpy(r.filename, l->parts[1]);
            // write(s, (char *)&r, sizeof(request));
            validar_r_w(s, (char *)&r, aux, sizeof(request), 0);
            enviar_archivo(s, l->parts[1]);
        }
    }

    close(s);
    return 0;
}

void handle_sigterm(int signal)
{
    printf("closed conection success\n");
    finished = 1;
}