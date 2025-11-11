#ifndef utils_logger_h
#define utils_logger_h
#include <pthread.h>
#include <ctype.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdbool.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include<string.h>

extern t_log* logger;
char* construir_path(char*,char*);
t_log* iniciar_logger_(char*, int);
t_log* iniciar_logger(char*);
t_log* iniciar_logger_(char* nombre_archivo, int mostrar_cola);
void loguear(const char*, ...)__attribute__((format(printf, 1, 2)));
void loguear_warning(const char*, ...)__attribute__((format(printf, 1, 2)));
void loguear_error(const char*, ...)__attribute__((format(printf, 1, 2)));

#endif /* utils_logger_h */