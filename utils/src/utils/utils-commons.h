#ifndef utils_commons_h
#define utils_commons_h

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
#include <commons/collections/queue.h>
#include <utils/utils-config.h>
#include<string.h>
#include <ctype.h>
#include <utils/hello.h>
#define EXIT_PROGRAM "EXIT"
typedef enum
{
	//Genérico
	MENSAJE,
	PAQUETE,
	FIN_PROGRAMA,
	//CPU - Memoria
	TAMANIO_PAGINA,
	PROXIMA_INSTRUCCION,
	ACCESO_TABLA_PAGINAS,
	RESPUESTA_NRO_FRAME,
	LECTURA_MEMORIA,
	ESCRITURA_MEMORIA,
	ACCESO_ESPACIO_USUARIO,
	VALOR_LECTURA_MEMORIA,
	VALOR_ESCRITURA_MEMORIA,
	RESIZE,
	OUT_OF_MEMORY,
	RESIZE_OK,
	MOV_OUT_OK,
	//Kernel - CPU
	EJECUTAR_PROCESO,
	INTERRUMPIR_CPU,
	EJECUTAR_CPU,
	FIN_QUANTUM,
	IO_HANDLER,
	REC_HANDLER,
	WAIT,
	SIGNAL,
	FINALIZAR_PROCESO_POR_CONSOLA,
	//CPU - Kernel
	CPU_INTERRUPT,
	CPU_EXIT,
	//Kernel - Memoria
	CREACION_PROCESO,
	ELIMINACION_PROCESO,
	CREACION_PROCESO_FALLIDO,
	ELIMINACION_PROCESO_FALLIDO,
	//IO
	IO_GEN_SLEEP,
	IO_STDIN_READ,
	IO_STDOUT_WRITE,
	IO_DIALFS,
	IO_FS_CREATE,
	IO_FS_DELETE,
	IO_FS_TRUNCATE,
	IO_FS_READ,
	IO_FS_WRITE,
	FILE_SYSTEM,
	FS_WRITE,
	FS_READ,
	//IO - Kernel
	NUEVA_IO,
	TERMINO_IO_GEN_SLEEP,
	TERMINO_STDIN,
	TERMINO_STDOUT,
	TERMINO_IO,
	// IO - Memoria
	PEDIDO_STDIN,
	PEDIDO_STDOUT,
	RESPUESTA_STDIN,
	RESPUESTA_STDOUT,
	// /// TIPOS DE INTERFAZ
	// GENERICA,
	// STDIN,
	// STDOUT,
	// DIALFS
	NO_CODE
}op_code;

// BRAND NEW
// IDEA: la CPU le envia a kernel un struct con la info que necesita 
// para gestionar las instancias de IO generica.
//
typedef struct{
	int process_id;
	op_code operacion;
	char* tipo_interfaz;
	char* nombre_interfaz;
	int unidades_de_trabajo;
}t_peticion_generica;
// BRAND NEW



typedef struct 
{
	uint32_t PC;
	uint8_t AX;
	uint8_t BX;
	uint8_t CX;
	uint8_t DX;
	uint32_t EAX;
	uint32_t EBX;
	uint32_t ECX;
	uint32_t EDX;
	uint32_t SI;
	uint32_t DI;
	
} t_registros_cpu;


typedef struct
{
	uint32_t PID;
	uint32_t program_counter;
	uint8_t prioridad;
	uint32_t quantum;
	t_registros_cpu* registros_cpu;
	t_list* archivos_abiertos;
	char* path;
} t_pcb;

typedef struct
{
	uint32_t id;
	uint32_t valor;
} t_id_valor;

typedef struct
{
	uint32_t id;
	char* valor;
} t_id_valor_string;
typedef struct
{
	uint32_t PID;
	uint32_t valor;
} t_pid_valor;

typedef struct t_validacion
{
	bool resultado;
	char* descripcion;
}t_validacion;

typedef struct
{
	uint32_t PID;
	uint32_t direccion_fisica;
	
	uint32_t size_registro;
	void* registro_dato;
	
}t_acceso_espacio_usuario;

typedef struct t_direcciones_proceso
{
	t_pid_valor pid_size_total; 
	t_list* direcciones; 
}t_direcciones_proceso;


typedef struct t_direcciones_fs{
	char* nombre_archivo;
	uint32_t puntero_archivo;
	t_direcciones_proceso* direcciones_proceso;
}t_direcciones_fs;

typedef struct t_direcciones_registros
{
	t_pid_valor dir_proceso_id;
	t_queue* direcciones;
}t_direcciones_registros;


typedef struct t_direccion_registro
{
 uint32_t direccion_fisica;
 uint32_t size_registro_pagina;
}t_direccion_registro;


t_direccion_registro* direccion_registro_new(uint32_t direccion,uint32_t size);

typedef struct t_operacion_fs{
	op_code cod_op;
	uint32_t pid;
	uint32_t tamanio_registro;
	uint32_t registro_puntero;//FSEEK
	uint32_t tamanio_truncate;
	char* nombre_archivo;
	t_list* direcciones;
}t_operacion_fs;

typedef struct t_direccion_tamanio{
	uint32_t direccion_fisica;
	uint32_t tamanio_bytes;
}t_direccion_tamanio;


t_validacion* validacion_new();
t_pcb* pcb_create(char*);
t_pcb* pcb_create_copy(char*);
t_pcb* pcb_create_quantum(char* path_programa,int quantum);
void pcb_destroy(t_pcb*);
bool is_numeric(const char*);
void loguear_registros(t_registros_cpu* registros);
void loguear_pcb(t_pcb*);
char* path_resolve(char*, char*);
char * uint_a_string(uint);
t_list* get_instrucciones(char* path_inicial,char *nombre_archivo);
char* get_linea_archivo(char* directorio,char* nombre_archivo,int posicion);
bool es_exit(void *comando);
t_registros_cpu* inicializar_registros();
void reemplazar_pcb_con(t_pcb* destino,t_pcb* origen);
void loguear_pid_value(t_pid_valor* tamanio_proceso);
t_pid_valor* pid_value_create(uint32_t pid, uint32_t valor);
t_acceso_espacio_usuario* acceso_espacio_usuario_create(uint32_t PID, uint32_t direccion, uint32_t size_registro,void* valor);
t_direccion_tamanio* direccion_tamanio_new(uint32_t direccion,uint32_t size);
t_direccion_registro* direccion_registro_new(uint32_t direccion,uint32_t size);
void direcciones_proceso_destroy(t_direcciones_proceso* direcciones_proceso);
void loguear_direccion_proceso(t_direcciones_proceso* dir_proceso);
t_direcciones_proceso* direcciones_proceso_create(uint32_t pid,uint32_t tamanio);
t_id_valor* id_valor_new(uint32_t id,uint32_t valor);
void validacion_destroy(t_validacion* validacion);
void acceso_espacio_usuario_destroy(t_acceso_espacio_usuario* acceso);
bool es_codigo_valido(int code);

/**
 * @fn    list_find_index
 * @brief Retorna el índice del primer valor encontrado que haga que condition devuelva != 0
 * @param self La lista
 * @param condition Función que recibe un elemento y devuelve true si se cumple la condición
 * @return El índice del primer elemento que cumple la condición o -1 si ninguno lo cumple
 */
int list_find_index(t_list* self, bool(*condition)(void*));


/**
 * @fn    is_true
 * @brief Retorna el valor booleano del valor al que apunta el elemento del parámetro. Si es NULL retorna false.
 * @param element Un puntero a un valor que debería ser booleano.
 * @return El valor booleano al que apunta el elemento del parámetro
 */
bool is_true(void* element);
bool is_false(void* element);

#endif /* utils_commons_h */