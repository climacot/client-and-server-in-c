#include <sys/types.h>
#include <semaphore.h>

#ifndef FILE_H_
#define FILE_H_

typedef struct
{
  char filename[PATH_MAX];
  char operation[16];
} request;

typedef struct
{
  char filename[PATH_MAX];
  int size;
  mode_t mode;
} file_info;

void enviar_archivo(int socket, char *nombre);
void recibir_archivo(int socket, char *nombre, void *mutex);
void validar_r_w(int c, void *buff, void *aux, int size, int mode);

#endif