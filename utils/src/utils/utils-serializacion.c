#include "utils-serializacion.h"

void buffer_destroy(t_buffer* buffer){
	if(buffer!=NULL){
		if(buffer->stream!=NULL)
			free(buffer->stream);

		free(buffer);
	}

}
void paquete_destroy(t_paquete* paquete) {
    if (paquete) {
        buffer_destroy(paquete->buffer);
       /* if (paquete->buffer) {
            if (paquete->buffer->stream) {
                free(paquete->buffer->stream);
            }
            free(paquete->buffer);
        }*/
        free(paquete);
    }
}

t_peticion_generica* recibir_peticion_generica(t_paquete* paquete){

    t_peticion_generica* peticion=malloc(sizeof(t_peticion_generica));

    /*Acá van todos los Recibir buffer*/


    return peticion;
}

t_peticion_generica* crear_peticion_generica(t_pcb*pcb_recibido,t_buffer* buffer_recibido){
	t_peticion_generica* peticion = malloc(sizeof(t_peticion_generica));

	/**/

	return peticion;
}

void enviar_peticion(t_peticion_generica*){

loguear_warning("Enviar petición todavía no fue implementada");
};
