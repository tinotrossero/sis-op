#ifndef utils_client_h
#define utils_client_h

#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<commons/log.h>
#include<utils/utils-serializacion.h>
#include<utils/utils-commons.h>

int crear_conexion(char* ip, int puerto);
void enviar_mensaje(char* mensaje, int socket_cliente);
t_paquete* crear_paquete(op_code codigo_operacion);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void liberar_conexion(int socket_cliente);
void eliminar_paquete(t_paquete*);
void enviar_pcb(t_pcb* pcb,op_code operacion,int socket);
void enviar_texto(char*texto,op_code codigo_operacion,int socket);
void enviar_id_value(t_id_valor* id_value,op_code operacion,int socket);
void enviar_id_value_string(t_id_valor_string* id_value,op_code operacion,int socket);
void enviar_pid_value(t_pid_valor* pid_value,op_code operacion,int socket);
void enviar_acceso_espacio_usuario(t_acceso_espacio_usuario* acceso_espacio_usuario,op_code operacion,int socket);
void _enviar_acceso_espacio_usuario(t_acceso_espacio_usuario* acceso_espacio_usuario,op_code operacion,int socket);
void* _serializar_acceso_espacio_usuario(t_acceso_espacio_usuario* acceso_espacio_usuario,int* size);
void* serializar_acceso_espacio_usuario(t_acceso_espacio_usuario* acceso_espacio_usuario,int* size);
void _enviar_texto(char* texto,op_code operacion,int socket);
void _enviar_stream_(void* texto,int size,int socket,op_code operacion);
void enviar_stream(void*stream,int size,int socket,op_code codigo_operacion);
t_buffer* crear_buffer(size_t size);
void enviar_direcciones_proceso(t_direcciones_proceso* direcciones_proceso,op_code operacion,int socket);
void* serializar_direcciones_proceso(t_direcciones_proceso* direcciones_proceso,int* size);
void agregar_a_buffer(t_buffer* buffer,const void* lugar_origen,size_t tam);
#endif /* utils_client_h */
