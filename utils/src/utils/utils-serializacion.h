#ifndef utils_serializacion_h
#define utils_serializacion_h
#include "utils-commons.h"

typedef struct
{
	int size;
	void* stream;
	int desplazamiento;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

void buffer_destroy(t_buffer* buffer);
void paquete_destroy(t_paquete* paquete);
void enviar_peticion(t_peticion_generica*);
t_peticion_generica* recibir_peticion_generica(t_paquete*);
t_peticion_generica* crear_peticion_generica(t_pcb*,t_buffer*);

#endif