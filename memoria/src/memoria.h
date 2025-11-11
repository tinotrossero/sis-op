#ifndef memoria_h
#define memoria_h
#include <math.h>
#include <utils/hello.h>
#include<commons/config.h>
#include<commons/string.h>
#include<commons/memory.h>
#include<commons/log.h>
#include<commons/collections/queue.h>
#include <utils/utils-client.h>
#include <utils/utils-server.h>
#include <utils/utils-commons.h>
#include <utils/utils-config.h>
#include <readline/readline.h>
#define MODULO "memoria"

typedef struct
{
	int PUERTO_ESCUCHA;
	int TAM_MEMORIA;
	int TAM_PAGINA;
	char* PATH_INSTRUCCIONES;
	int RETARDO_RESPUESTA;
	
	t_config* config;
} t_config_memoria;


typedef struct
{
	t_list* instrucciones;
	t_pcb* pcb;
	t_list* tabla_paginas;
} t_proceso;


typedef enum
{
	INICIAR_PROCESO,
	FINALIZAR_PROCESO,
	//ACCESO_TABLA_PAGINAS,
	AJUSTAR_TAMANIO_PROCESO
	
}op_code_memoria;

t_config_memoria* iniciar_config_memoria(char*);
void config_memoria_destroy();
void loguear_config_memoria();
bool iniciar_memoria(char*);
void finalizar_memoria();
extern t_log* logger;
extern t_config_memoria* config_memoria;
extern int memoria_escucha,conexion_cpu, conexion_kernel;
extern t_dictionary* procesos;
char* proxima_instruccion_de(t_pcb*);
void cargar_programa_de(t_pcb*,char*);
void enviar_proxima_instruccion (t_pcb* pcb);
int buscar_instrucciones();
int recibir_procesos();
void crear_frames_memoria_principal(int);
int asignar_frame();
void remover_proceso_del_frame(int);
void imprimir_uso_frames();
void imprimir_tabla_paginas_proceso(int PID);
int convertir_bytes_a_paginas(int);
bool validar_ampliacion_proceso(int);
void ampliar_proceso(t_list* ,int );
void reducir_proceso(t_list* ,int);
t_pid_valor* recibir_pid_value(t_paquete*);
void liberar_frame(int nro_frame);
int obtener_frame(t_list* tabla_de_paginas,int nro_pagina);
uint32_t ejecutar_resize(t_pid_valor* tamanio_proceso);
void acceder_tabla_de_paginas(t_pid_valor* pid_pagina);
void acceder_a_espacio_usuario(op_code tipo_acceso,t_acceso_espacio_usuario* acceso_espacio_usuario,int conexion);
void liberar_proceso_de_memoria(uint32_t pid);
int diferencia_tamaño_nuevo_y_actual(t_list* tabla_paginas,int tamanio_proceso);
void efectuar_retardo();
void escribir_memoria(void* direccion_fisica,void* dato,uint32_t size);
void leer_memoria(void* direccion_fisica,void* buffer,uint32_t size);
t_proceso* get_proceso(int pid);

//TERMINAR
void io_handler(int *ptr_conexion);
void iniciar_conexion_io();

//

//bool ejec_codigo_prueba();
#endif /* memoria.h*/