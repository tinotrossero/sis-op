#ifndef utils_interfaz_h
#define utils_interfaz_h

#include<commons/config.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/memory.h>
#include "utils/utils-logger.h"
#include<math.h>
#include <stdio.h>
#include<commons/collections/dictionary.h>
#include "utils/utils-server.h"
#include "utils/utils-client.h"
#include "utils/utils-config.h"
#include "utils/utils-commons.h"
#include <stdint.h>
#include "utils/utils-serializacion.h"

void escribir_memoria_completa(t_direcciones_proceso* direcciones_fisicas_registros, char* registro_dato,int conexion);
t_buffer* leer_memoria_completa(t_direcciones_proceso* direcciones_fisicas_registros,int conexion);
void escribir_memoria_completa_io(t_direcciones_proceso* direcciones_fisicas_registros, char* registro_dato,int conexion_a_memoria,op_code op_code);
t_buffer* leer_memoria_completa_io(t_direcciones_proceso* direcciones_fisicas_registros,int conexion, op_code op_code);
t_operacion_fs* obtener_op_fs(uint32_t pid,char* nombre,t_list* lst,uint32_t tamanio_registro, uint32_t ptr , uint32_t tam_trunc, op_code cod_op);
void enviar_operacion_fs(t_operacion_fs*,op_code ,int);
void* serializar_operacion_fs(t_operacion_fs*, int*);
t_operacion_fs* recibir_op_fs(t_paquete* paquete);
void operacion_fs_destroy(t_operacion_fs* operacion_fs);
void escribir_memoria_completa_FS(uint32_t tamanio_registro,t_list* direcciones_registros, uint32_t pid, char* registro_dato,int conexion_a_memoria, op_code op_code);
t_buffer* leer_memoria_completa_FS(uint32_t tamanio_registro,t_list* direcciones_registros, uint32_t pid,int conexion,op_code op_code);

#endif /* utils_interfaz_h */