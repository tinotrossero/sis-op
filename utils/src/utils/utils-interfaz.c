#include "utils-interfaz.h"


void escribir_memoria_completa(t_direcciones_proceso* direcciones_fisicas_registros, char* registro_dato,int conexion_a_memoria){
	int operacion_ok;
	t_acceso_espacio_usuario* acceso_espacio_usuario;
	t_list* direcciones_registros =  direcciones_fisicas_registros->direcciones;
	t_pid_valor pid_size_total = direcciones_fisicas_registros->pid_size_total;
	uint32_t size_leido=0;
	uint32_t size_registro_pagina_actual;
    int registro_int = atoi(registro_dato);
    void* registro_puntero = &registro_int;
	int registro_reconstr;
    void* registro_puntero_recons = &registro_reconstr;
 

		void _enviar_direcciones_memoria(void* element){
		
			t_direccion_registro* direccion_registro = (t_direccion_registro*) element;
			size_registro_pagina_actual = direccion_registro->size_registro_pagina;
			
			void* dato_parcial = malloc(size_registro_pagina_actual);

			memcpy(dato_parcial, registro_puntero + size_leido,size_registro_pagina_actual);
			
			memcpy(registro_puntero_recons + size_leido, dato_parcial ,size_registro_pagina_actual);

			acceso_espacio_usuario =  acceso_espacio_usuario_create(
			pid_size_total.PID,
			direccion_registro->direccion_fisica,
			direccion_registro-> size_registro_pagina,
			dato_parcial);
			
			
			enviar_acceso_espacio_usuario(acceso_espacio_usuario,ESCRITURA_MEMORIA,conexion_a_memoria);
			size_leido += size_registro_pagina_actual;	

			operacion_ok = recibir_operacion(conexion_a_memoria);
			
			if(operacion_ok==MOV_OUT_OK){
				char* valor_memoria =recibir_mensaje(conexion_a_memoria);
				free(valor_memoria);
			}
			
			//loguear("PID: <%d> - Acción: <ESCRIBIR> - Dirección Física: <%d> - Valor: <%d>",
			//pid_size_total.PID,direccion_registro->direccion_fisica,registro_dato + size_leido);
			free(acceso_espacio_usuario);
			free(dato_parcial);
			
			
		};
		
	list_iterate(direcciones_registros, &_enviar_direcciones_memoria);

	loguear("Valor escrito: <%d>",registro_reconstr);


		
}


t_buffer* leer_memoria_completa(t_direcciones_proceso* direcciones_fisicas_registros,int conexion){
	
	// 
	//int response;
	//t_acceso_espacio_usuario* acceso_espacio_usuario;
	t_list* direcciones_registros =  direcciones_fisicas_registros->direcciones;
	t_pid_valor pid_size_total = direcciones_fisicas_registros->pid_size_total;
	uint32_t size_leido=0;
//	uint32_t size_registro_pagina_actual;
    t_buffer* dato_final_puntero = crear_buffer(pid_size_total.valor);
	
	/////BORRAR
//	int registro_reconstr;
   // void* registro_puntero_recons = &registro_reconstr;
	/////BORRAR


	void _enviar_direcciones_memoria(void* element){	
			t_direccion_registro* direccion_registro = (t_direccion_registro*) element;
			int size_registro_pagina_actual = direccion_registro->size_registro_pagina;
			
			
			t_acceso_espacio_usuario* acceso_espacio_usuario =  acceso_espacio_usuario_create(
			pid_size_total.PID,
			direccion_registro->direccion_fisica,
			direccion_registro->size_registro_pagina,
			NULL);		
			enviar_acceso_espacio_usuario(acceso_espacio_usuario,LECTURA_MEMORIA,conexion);
			
			free(acceso_espacio_usuario);
		//	int response = 
			recibir_operacion(conexion);
				
		//	if(response == VALOR_LECTURA_MEMORIA){
				
				void* dato_recibido = recibir_buffer(&size_registro_pagina_actual,conexion);		

				memcpy(dato_final_puntero->stream + size_leido,dato_recibido, size_registro_pagina_actual);
				
				/////BORRAR
				//memcpy(registro_puntero_recons + size_leido, dato_recibido ,size_registro_pagina_actual);
				/////BORRAR
				
				size_leido += size_registro_pagina_actual;
	
				
			//	loguear("PID: <%d> - Acción: <LEER> - Dirección Física: <%d> - Valor: <%d>",
		   // pid_size_total.PID,direccion_registro->direccion_fisica,dato_recibido);
				free(dato_recibido);
			//}
			
			
		}
		list_iterate(direcciones_registros, &_enviar_direcciones_memoria);
		/////BORRAR
		//loguear("Valor leido: <%d>",registro_reconstr);
		/////BORRAR
	return dato_final_puntero;
}
void escribir_memoria_completa_FS(uint32_t tamanio_registro,t_list* direcciones_registros, uint32_t pid, char* registro_dato,int conexion_a_memoria, op_code op_code){
	int operacion_ok;
	t_acceso_espacio_usuario* acceso_espacio_usuario;
	//t_list* direcciones_registros =  direcciones_fisicas_registros->direcciones;
	//t_pid_valor pid_size_total = tamanio_registro;
	uint32_t size_leido=0;
	uint32_t size_registro_pagina_actual;
    
	//BORRAR
	//char* registro_dato = malloc(5);
	//registro_dato = "hola";
	//BORRAR
    void* registro_puntero =(void*) registro_dato; 
	void* dato_parcial_prueba = malloc(tamanio_registro);
		void _enviar_direcciones_memoria(void* element){
		
			t_direccion_tamanio* direccion_registro = (t_direccion_tamanio*) element;
			size_registro_pagina_actual = direccion_registro->tamanio_bytes;
			
			
			void* dato_parcial = malloc(size_registro_pagina_actual);

			//BORRAR
			//void* dato_parcial_prueba = malloc(size_registro_pagina_actual+1);
			//BORRAR
			memcpy(dato_parcial, registro_puntero + size_leido,size_registro_pagina_actual);
			
			//BORRAR
			memcpy(dato_parcial_prueba + size_leido,dato_parcial,size_registro_pagina_actual);
			//BORRAR
			
			
			acceso_espacio_usuario =  acceso_espacio_usuario_create(
			pid,
			direccion_registro->direccion_fisica,
			direccion_registro-> tamanio_bytes,
			dato_parcial);
			
			
			enviar_acceso_espacio_usuario(acceso_espacio_usuario,op_code,conexion_a_memoria);
			size_leido += size_registro_pagina_actual;	

			operacion_ok = recibir_operacion(conexion_a_memoria);
			
			if(operacion_ok==MOV_OUT_OK){
				char* valor_memoria =recibir_mensaje(conexion_a_memoria);
				free(valor_memoria);
			};
			
			//loguear("PID: <%d> - Acción: <ESCRIBIR> - Dirección Física: <%d> - Valor: <%d>",
			//pid_size_total.PID,direccion_registro->direccion_fisica,registro_dato + size_leido);
			free(acceso_espacio_usuario);
			free(dato_parcial);
			
			
			
		};
		
	list_iterate(direcciones_registros, &_enviar_direcciones_memoria);
	((char*)dato_parcial_prueba)[tamanio_registro] = '\0';
	loguear("Dato prueba <%s>",(char*)dato_parcial_prueba);
//	loguear("Valor escrito: <%d>",registro_reconstr);
	free(dato_parcial_prueba);

//free(registro_dato);		
}


void escribir_memoria_completa_io(t_direcciones_proceso* direcciones_fisicas_registros, char* registro_dato,int conexion_a_memoria, op_code op_code){
	int operacion_ok;
	t_acceso_espacio_usuario* acceso_espacio_usuario;
	t_list* direcciones_registros =  direcciones_fisicas_registros->direcciones;
	t_pid_valor pid_size_total = direcciones_fisicas_registros->pid_size_total;
	uint32_t size_leido=0;
	uint32_t size_registro_pagina_actual;
    
	//BORRAR
	//char* registro_dato = malloc(5);
	//registro_dato = "hola";
	//BORRAR
    void* registro_puntero =(void*) registro_dato; 
	void* dato_parcial_prueba = malloc(pid_size_total.valor);
		void _enviar_direcciones_memoria(void* element){
		
			t_direccion_registro* direccion_registro = (t_direccion_registro*) element;
			size_registro_pagina_actual = direccion_registro->size_registro_pagina;
			
			
			void* dato_parcial = malloc(size_registro_pagina_actual);

			//BORRAR
			//void* dato_parcial_prueba = malloc(size_registro_pagina_actual+1);
			//BORRAR
			memcpy(dato_parcial, registro_puntero + size_leido,size_registro_pagina_actual);
			
			//BORRAR
			memcpy(dato_parcial_prueba + size_leido,dato_parcial,size_registro_pagina_actual);
			//BORRAR
			
			
			acceso_espacio_usuario =  acceso_espacio_usuario_create(
			pid_size_total.PID,
			direccion_registro->direccion_fisica,
			direccion_registro-> size_registro_pagina,
			dato_parcial);
			
			
			enviar_acceso_espacio_usuario(acceso_espacio_usuario,op_code,conexion_a_memoria);
			size_leido += size_registro_pagina_actual;	

			operacion_ok = recibir_operacion(conexion_a_memoria);
			
			if(operacion_ok==MOV_OUT_OK){
				char* valor_memoria =recibir_mensaje(conexion_a_memoria);
				free(valor_memoria);
			};
			
			//loguear("PID: <%d> - Acción: <ESCRIBIR> - Dirección Física: <%d> - Valor: <%d>",
			//pid_size_total.PID,direccion_registro->direccion_fisica,registro_dato + size_leido);
			free(acceso_espacio_usuario);
			free(dato_parcial);
			
			
			
		};
		
	list_iterate(direcciones_registros, &_enviar_direcciones_memoria);
	((char*)dato_parcial_prueba)[pid_size_total.valor] = '\0';
	loguear("Dato prueba <%s>",(char*)dato_parcial_prueba);
//	loguear("Valor escrito: <%d>",registro_reconstr);
	free(dato_parcial_prueba);

//free(registro_dato);		
};

t_buffer* leer_memoria_completa_FS(uint32_t tamanio_registro,t_list* direcciones_registros, uint32_t pid,int conexion,op_code op_code){
	
	//t_list* direcciones_registros =  direcciones_fisicas_registros->direcciones;
	//t_pid_valor pid_size_total = direcciones_fisicas_registros->pid_size_total;
	uint32_t size_leido=0;

    t_buffer* dato_final_puntero = crear_buffer(tamanio_registro);
	
	/////BORRAR
//	int registro_reconstr;
   
	/////BORRAR


	void _enviar_direcciones_memoria(void* element){	
					t_direccion_tamanio* direccion_registro = (t_direccion_tamanio*) element;
					int size_registro_pagina_actual = direccion_registro->tamanio_bytes;
					
					
					t_acceso_espacio_usuario* acceso_espacio_usuario =  acceso_espacio_usuario_create(
					pid,
					direccion_registro->direccion_fisica,
					direccion_registro->tamanio_bytes,
					NULL);		
					enviar_acceso_espacio_usuario(acceso_espacio_usuario,op_code,conexion);
					
					free(acceso_espacio_usuario);
				//	int response = 
					recibir_operacion(conexion);
						
				//	if(response == VALOR_LECTURA_MEMORIA){
						
						void* dato_recibido = recibir_buffer(&size_registro_pagina_actual,conexion);		
						
						
						
						memcpy(dato_final_puntero->stream + size_leido,dato_recibido, size_registro_pagina_actual);
						


						size_leido += size_registro_pagina_actual;
			
						free(dato_recibido);
						
					//}
					
					
				};
		list_iterate(direcciones_registros, &_enviar_direcciones_memoria);
		
	return dato_final_puntero;
}

t_buffer* leer_memoria_completa_io(t_direcciones_proceso* direcciones_fisicas_registros,int conexion,op_code op_code){
	
	t_list* direcciones_registros =  direcciones_fisicas_registros->direcciones;
	t_pid_valor pid_size_total = direcciones_fisicas_registros->pid_size_total;
	uint32_t size_leido=0;

    t_buffer* dato_final_puntero = crear_buffer(pid_size_total.valor);
	
	/////BORRAR
//	int registro_reconstr;
   
	/////BORRAR


	void _enviar_direcciones_memoria(void* element){	
			t_direccion_registro* direccion_registro = (t_direccion_registro*) element;
			int size_registro_pagina_actual = direccion_registro->size_registro_pagina;
			
			
			t_acceso_espacio_usuario* acceso_espacio_usuario =  acceso_espacio_usuario_create(
			pid_size_total.PID,
			direccion_registro->direccion_fisica,
			direccion_registro->size_registro_pagina,
			NULL);		
			enviar_acceso_espacio_usuario(acceso_espacio_usuario,op_code,conexion);
			
			free(acceso_espacio_usuario);
		//	int response = 
			recibir_operacion(conexion);
				
		//	if(response == VALOR_LECTURA_MEMORIA){
				
				void* dato_recibido = recibir_buffer(&size_registro_pagina_actual,conexion);		
				
				
				
				memcpy(dato_final_puntero->stream + size_leido,dato_recibido, size_registro_pagina_actual);

				size_leido += size_registro_pagina_actual;
	
				free(dato_recibido);
				
			//}
			
			
		}
		list_iterate(direcciones_registros, &_enviar_direcciones_memoria);
		/////BORRAR
		//loguear("Valor leido: <%d>",registro_reconstr);
		/////BORRAR
	return dato_final_puntero;
}

t_operacion_fs* obtener_op_fs(uint32_t pid, char* nmb_archivo,t_list* direcciones,uint32_t tamanio_registro, uint32_t puntero_archivo, uint32_t tamanio_truncate,op_code cod_op){
	t_operacion_fs* operacion_fs = malloc(sizeof(t_operacion_fs));
	operacion_fs->nombre_archivo = nmb_archivo;
	operacion_fs->cod_op = cod_op;
	operacion_fs->registro_puntero = puntero_archivo;
	operacion_fs->direcciones = direcciones;
	operacion_fs->tamanio_truncate = tamanio_truncate;
	operacion_fs->pid = pid;
	operacion_fs->tamanio_registro = tamanio_registro;

	return operacion_fs;

}


void operacion_fs_destroy(t_operacion_fs* operacion_fs){
	list_destroy_and_destroy_elements(operacion_fs->direcciones,free);
	free(operacion_fs->nombre_archivo);
	free(operacion_fs);
}

void enviar_operacion_fs(t_operacion_fs* operacion,op_code op,int socket){
	int size;
	void* stream = serializar_operacion_fs(operacion,&size);	
				 
	enviar_stream(stream,size,socket,op);
	free(stream);
}


void* serializar_operacion_fs(t_operacion_fs* operacion_fs,int* size){
	
	//t_list* lista = operacion_fs->direcciones_proceso->direcciones;
	t_list* lista = operacion_fs->direcciones;
	char* nombre = operacion_fs->nombre_archivo;
	uint32_t long_char; 
	uint32_t cant_direcciones = list_size(lista);
	*size = sizeof(op_code) + sizeof(uint32_t) * 4 + sizeof(uint32_t) + (strlen(nombre)+1) + sizeof(uint32_t) + cant_direcciones * (2*sizeof(uint32_t));
													// TAMAÑO char*                      TAMAÑO direcciones_proceso
	long_char = strlen(nombre)+1;
	t_buffer* buffer = crear_buffer(*size);
	
	agregar_a_buffer(buffer, &operacion_fs->cod_op, sizeof(op_code));
	agregar_a_buffer(buffer, &operacion_fs->pid, sizeof(uint32_t));
	agregar_a_buffer(buffer, &operacion_fs->tamanio_registro, sizeof(uint32_t));
	agregar_a_buffer(buffer, &operacion_fs->registro_puntero, sizeof(uint32_t));
	agregar_a_buffer(buffer, &operacion_fs->tamanio_truncate, sizeof(uint32_t));
	agregar_a_buffer(buffer, &long_char, sizeof(uint32_t)); // TAMAÑO CHAR*
	agregar_a_buffer(buffer, nombre, long_char);
	agregar_a_buffer(buffer, &cant_direcciones, sizeof(uint32_t)); // TAMAÑO/CANTIDAD DIRECCIONES
	
	for(int i=0;i<cant_direcciones;i++)
	{	
		t_direccion_tamanio* direc = (t_direccion_tamanio*)list_get(lista,i);  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		agregar_a_buffer(buffer, &direc->direccion_fisica, sizeof(uint32_t)); // REVISAR
		agregar_a_buffer(buffer, &direc->tamanio_bytes, sizeof(uint32_t)); // 
															// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	};
	void * stream = buffer->stream;
	free(buffer);
	//free(nombre);
	
	return stream;
}

t_operacion_fs* recibir_op_fs(t_paquete* paquete)
{
	uint32_t tam_nombre;
	uint32_t cant_direcciones;
	t_operacion_fs* op_fs = malloc(sizeof(t_operacion_fs));
	op_fs->direcciones = list_create();
	
	t_buffer* buffer = paquete->buffer;
	buffer->desplazamiento = sizeof(op_code); // PORQUE YA HICIMOS recibir_operacion(cod_op)
	void _recibir(void* lugar_destino,size_t tam){
		recibir_de_buffer(lugar_destino,buffer,tam);
	};

	_recibir(&op_fs->cod_op,sizeof(op_code));
	_recibir(&op_fs->pid,sizeof(uint32_t));
	_recibir(&op_fs->tamanio_registro,sizeof(uint32_t));
	_recibir(&op_fs->registro_puntero,sizeof(uint32_t));
	_recibir(&op_fs->tamanio_truncate,sizeof(uint32_t));
	_recibir(&tam_nombre,sizeof(uint32_t));
	op_fs->nombre_archivo = malloc(tam_nombre);  // ES UN CHAR*, DEBEMOS RESERVAR EL ESPACIO
	_recibir(op_fs->nombre_archivo,tam_nombre);
	_recibir(&cant_direcciones,sizeof(uint32_t));

	for(int i=0;i<cant_direcciones;i++)
	{	
		t_direccion_tamanio* direc = malloc(sizeof(t_direccion_tamanio));
		_recibir(&direc->direccion_fisica,sizeof(uint32_t));
		_recibir(&direc->tamanio_bytes,sizeof(uint32_t));
		list_add(op_fs->direcciones,direc);
	};
	paquete_destroy(paquete);
	return op_fs;
}