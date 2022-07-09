#include <arpa/inet.h>
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
#include <unistd.h>

#include "files.h"
#include "split.h"

typedef struct files
{
  char filename[PATH_MAX];
} head;

head lista[100];
int count = 0;

void enviar_archivo(int socket, char *nombre)
{
  char aux[255];
  char buf_aux[BUFSIZ];
  char buf[BUFSIZ];
  char error[255];
  file_info *info_aux;
  file_info info;
  int a_leer;
  int faltantes;
  int fd;
  int leidos;
  int out_fd;
  struct stat s;

  if (stat(nombre, &s) < 0 || !S_ISREG(s.st_mode))
  {
    perror("stat");
    memset(error, 0, sizeof(error));
    memset(aux, 0, sizeof(aux));
    strcpy(error, "error");
    // write(socket, &error, sizeof(error));
    validar_r_w(socket, error, aux, sizeof(error), 0);
    return;
  }

  memset(error, 0, sizeof(error));
  memset(aux, 0, sizeof(aux));
  strcpy(error, "");
  // write(socket, &error, sizeof(error));
  validar_r_w(socket, error, aux, sizeof(error), 0);

  memset(&info, 0, sizeof(file_info));
  info.size = s.st_size;
  info.mode = s.st_mode;
  strcpy(info.filename, basename(nombre));

  fd = open(nombre, O_RDONLY);

  if (fd < 0)
  {
    printf("No se pudo leer el archivo\n");
    // return;
  }

  faltantes = info.size;

  // write(socket, &info, sizeof(file_info));
  validar_r_w(socket, &info, &info_aux, sizeof(file_info), 0);

  memset(error, 0, sizeof(error));
  validar_r_w(socket, error, aux, sizeof(error), 1);

  if (strcmp(error, "error") == 0)
  {
    printf("El archivo esta siendo procesado por otro usuario\n");
    return;
  }

  while (faltantes > 0)
  {
    a_leer = BUFSIZ;
    if (faltantes < a_leer)
    {
      a_leer = faltantes;
    }

    leidos = read(fd, buf, a_leer);

    // write(socket, buf, leidos);
    printf("leidos, info.size %d %d\n", leidos, info.size);
    validar_r_w(socket, &buf, &buf_aux, leidos, 0);
    faltantes = faltantes - leidos;
  }

  printf("send file success\n");
  close(fd);
}

void recibir_archivo(int socket, char *nombre, void *mutex)
{
  char *aux[255];
  char buf[BUFSIZ];
  char error[255];
  file_info *info_aux;
  file_info info;
  int a_leer;
  int faltantes;
  int fd;
  int leidos;

  // read(socket, &error, sizeof(error));
  memset(error, 0, sizeof(error));
  validar_r_w(socket, error, aux, sizeof(error), 1);

  if (strcmp(error, "error") == 0)
  {
    printf("No such file or directory\n");
    return;
  }

  memset(&info, 0, sizeof(file_info));
  // read(socket, &info, sizeof(info));
  validar_r_w(socket, &info, &info_aux, sizeof(info), 1);

  for (int i = 0; i < 100; i++)
  {
    if (strcmp(lista[i].filename, info.filename) == 0)
    {
      printf("El archivo ya se esta cargando por otro usuario\n");
      memset(error, 0, sizeof(error));
      memset(aux, 0, sizeof(aux));
      strcpy(error, "error");
      // write(socket, &error, sizeof(error));
      validar_r_w(socket, error, aux, sizeof(error), 0);
      return;
    }
  }

  memset(error, 0, sizeof(error));
  memset(aux, 0, sizeof(aux));
  strcpy(error, "");
  // write(socket, &error, sizeof(error));
  validar_r_w(socket, error, aux, sizeof(error), 0);

  strcpy(lista[count].filename, info.filename);

  sem_wait(mutex);

  fd = open(nombre, O_CREAT | O_WRONLY, info.mode);

  if (fd < 0)
  {
    printf("No se pudo leer el archivo\n");
    // return;
  }

  printf("recibiendo un archivo\n");

  faltantes = info.size;

  while (faltantes > 0)
  {
    a_leer = BUFSIZ;
    if (faltantes < a_leer)
    {
      a_leer = faltantes;
    }

    // TODO: validar
    leidos = read(socket, buf, a_leer);

    write(fd, buf, leidos);
    faltantes = faltantes - leidos;
  }

  sleep(5);
  sem_post(mutex);
  printf("get file success\n");
  strcpy(lista[count].filename, "");
  count++;
  close(fd);
}

void validar_r_w(int c, void *buff, void *aux, int size, int mode)
{
  ssize_t assize_t;
  size_t asize_t;
  asize_t = size;
  aux = buff;

  do
  {
    if (mode == 1)
    {
      assize_t = read(c, aux, asize_t);
    }
    else
    {
      assize_t = write(c, (char *)aux, asize_t);
    }

    if (assize_t <= 0)
    {
      // printf("salida de validacion con szzize_t: %ld\n", assize_t);
      break;
    }

    if (assize_t <= size)
    {
      aux = aux + assize_t;
      asize_t = asize_t - assize_t;
    }

  } while (asize_t != 0 && assize_t != 0);
}