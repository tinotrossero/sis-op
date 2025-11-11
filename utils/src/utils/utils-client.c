#include "utils-client.h"


void* serializar_paquete(t_paquete* paquete, int bytes)
{
	void *magic = malloc(bytes);
    if (magic == NULL) {
        perror("Error al asignar memoria para la serialización");
        return NULL;
    }

    int desplazamiento = 0;

    // Copiar el código de operación
    memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
    desplazamiento += sizeof(int);

    // Copiar el tamaño del buffer
    memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
    desplazamiento += sizeof(int);

    // Copiar el contenido del buffer
    memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);

    return magic;
}


int crear_conexion(char *ip, int puerto)
{
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	char* puerto_char = (char*)malloc(sizeof(char) * 20); 
	sprintf(puerto_char, "%d", puerto);
	getaddrinfo(ip, puerto_char, &hints, &server_info);
	free(puerto_char);

	// Ahora vamos a crear el socket.
	int socket_cliente = socket(server_info->ai_family,
								server_info->ai_socktype,
								server_info->ai_protocol);

	// Ahora que tenemos el socket, vamos a conectarlo
	int connect_res=connect(socket_cliente,server_info->ai_addr,server_info->ai_addrlen);	
	freeaddrinfo(server_info);
	if(connect_res<0){
		socket_cliente = connect_res;
		perror("Error en la conexión del cliente a un servidor.");
	}

	return socket_cliente;
}


void enviar_stream(void*stream,int size,int socket,op_code codigo_operacion){
	t_paquete* paquete = crear_paquete(codigo_operacion);

	if (paquete == NULL) {
        perror("Error en enviar_stream al crear_paquete");
        return;
    }  
					   	
	agregar_a_paquete(paquete,stream,size);	
	
	enviar_paquete(paquete,socket);
	
	eliminar_paquete(paquete);
}

void _enviar_texto(char* texto,op_code operacion,int socket){
	int size = strlen(texto) + 1;
	enviar_stream(texto,size,socket,operacion);

}
void enviar_texto(char* texto,op_code operacion,int socket){

																					 
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = operacion;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(texto) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, texto, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);

}

void enviar_mensaje(char* mensaje, int socket)
												
{
	enviar_texto(mensaje,MENSAJE,socket);
	
}

t_buffer* crear_buffer(size_t size)
{
	t_buffer* buffer = malloc(sizeof(t_buffer));
    if (buffer == NULL) {
        perror("Error al alocar memoria para t_buffer");
        return NULL;
    }
    buffer->size = size;
    buffer->stream = calloc(1, buffer->size);
    buffer->desplazamiento = 0;

    return buffer;
}



t_paquete* crear_paquete(op_code codigo_operacion)
{
	 t_paquete* paquete = malloc(sizeof(t_paquete));
    if (paquete == NULL) {
        perror("Error al alocar memoria para t_paquete");
        return NULL;
    }
	 memset(paquete, 0, sizeof(t_paquete)); // Inicializa todos los bytes
    paquete->codigo_operacion = codigo_operacion;
    paquete->buffer = crear_buffer(0);
    if (paquete->buffer == NULL) {
        free(paquete);
        return NULL;
    }

    return paquete;
}

void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio)
{
   if (paquete == NULL || paquete->buffer == NULL || valor == NULL) {
        perror("Error: Paquete, buffer o valor es NULL");
        return;
    }

    void* nuevo_stream = realloc(paquete->buffer->stream, paquete->buffer->size + sizeof(int) + tamanio);
    if (nuevo_stream == NULL) {
        perror("Error al reasignar memoria para el stream del paquete");
        return;
    }

    paquete->buffer->stream = nuevo_stream;
    memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
    memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);
    paquete->buffer->size += sizeof(int) + tamanio;							  
								 
}

void enviar_paquete(t_paquete* paquete, int socket_cliente)
{
	 if (paquete == NULL || paquete->buffer == NULL || paquete->buffer->stream == NULL) {
        perror("Error: Paquete o buffer es NULL");
        return;
    }

    int bytes = paquete->buffer->size + 2*sizeof(int);
    void* a_enviar = serializar_paquete(paquete, bytes);

    if (a_enviar == NULL) {
        perror("Error al serializar el paquete");
        return;
    }

    ssize_t bytes_enviados = send(socket_cliente, a_enviar, bytes, 0);
    if (bytes_enviados == -1) {
        perror("Error al enviar el paquete");
    } else if (bytes_enviados != bytes) {
        fprintf(stderr, "Advertencia: no se enviaron todos los bytes del paquete\n");
    }

    free(a_enviar);					  
}


void eliminar_paquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

void liberar_conexion(int socket_cliente)
{
	 close(socket_cliente);
}

void agregar_a_buffer(t_buffer* buffer,const void* lugar_origen,size_t tam){
	memcpy(buffer->stream+buffer->desplazamiento,lugar_origen,tam);
	buffer->desplazamiento+=tam;
}

void* serializar_pid_value(t_pid_valor* pid_value,int* size){
	*size = sizeof(uint32_t) *2 ;
	t_buffer* buffer = crear_buffer(*size);
	agregar_a_buffer(buffer, &pid_value->PID, sizeof(uint32_t));
	agregar_a_buffer(buffer, &pid_value->valor, sizeof(uint32_t));
	
	void * stream = buffer->stream;
	free(buffer);

	return stream;

}

void* serializar_pcb(t_pcb* pcb,int* size)
{	
	
	uint32_t path_size = strlen(pcb->path)+ 1;
	//		(PID,pc,quantum,size del path) + registros + prioridad + path_size
	*size = sizeof(uint32_t) *10 +  + sizeof(uint8_t) *5 + path_size ;
	t_buffer* buffer = crear_buffer(*size);

								

	agregar_a_buffer(buffer, &pcb->PID, sizeof(uint32_t));
	agregar_a_buffer(buffer, &pcb->prioridad, sizeof(uint8_t));
	agregar_a_buffer(buffer, &pcb->program_counter, sizeof(uint32_t));
	agregar_a_buffer(buffer, &pcb->quantum, sizeof(uint32_t));
	agregar_a_buffer(buffer, &pcb->registros_cpu->AX, sizeof(uint8_t));
	agregar_a_buffer(buffer, &pcb->registros_cpu->BX, sizeof(uint8_t));
	agregar_a_buffer(buffer, &pcb->registros_cpu->CX, sizeof(uint8_t));
	agregar_a_buffer(buffer, &pcb->registros_cpu->DX, sizeof(uint8_t));
	agregar_a_buffer(buffer, &pcb->registros_cpu->EAX, sizeof(uint32_t));
	agregar_a_buffer(buffer, &pcb->registros_cpu->EBX, sizeof(uint32_t));
	agregar_a_buffer(buffer, &pcb->registros_cpu->ECX, sizeof(uint32_t));
	agregar_a_buffer(buffer, &pcb->registros_cpu->EDX, sizeof(uint32_t));
	agregar_a_buffer(buffer, &pcb->registros_cpu->SI, sizeof(uint32_t));
	agregar_a_buffer(buffer, &pcb->registros_cpu->DI, sizeof(uint32_t));
	agregar_a_buffer(buffer, &path_size, sizeof(uint32_t));
	agregar_a_buffer(buffer, pcb->path, path_size);

 
								 
														
	void * stream = buffer->stream;
	free(buffer);

	return stream;
}

void enviar_pcb(t_pcb* pcb,op_code operacion,int socket){
	int size;
	void* stream = serializar_pcb(pcb,&size);									
					 
	enviar_stream(stream,size,socket,operacion);
	free(stream);
}

															
void enviar_pid_value(t_pid_valor* pid_value,op_code operacion,int socket){
	int size;
	void* stream = serializar_pid_value(pid_value,&size);									
					 
	enviar_stream(stream,size,socket,operacion);
	free(stream);
}


void* serializar_acceso_espacio_usuario(t_acceso_espacio_usuario* acceso_espacio_usuario,int* size){
	// uint32_t tamanio_dato = ((uint32_t)strlen(acceso_espacio_usuario->registro_dato)+(uint32_t)1);
	
	*size = sizeof(uint32_t) * 3;
	if (acceso_espacio_usuario->registro_dato!=NULL){
		// *size = *size +  (strlen(acceso_espacio_usuario->registro_dato)+1); 
		*size = *size + acceso_espacio_usuario->size_registro;
	}
	t_buffer* buffer = crear_buffer(*size);
	agregar_a_buffer(buffer, &acceso_espacio_usuario->PID, sizeof(uint32_t));
	agregar_a_buffer(buffer, &acceso_espacio_usuario->direccion_fisica, sizeof(uint32_t));
	//agregar_a_buffer(buffer, &acceso_espacio_usuario->bytes_restantes_en_frame, sizeof(uint32_t));
	agregar_a_buffer(buffer, &acceso_espacio_usuario->size_registro, sizeof(uint32_t));
	if( acceso_espacio_usuario->registro_dato!=NULL){
	agregar_a_buffer(buffer, acceso_espacio_usuario->registro_dato, acceso_espacio_usuario->size_registro);
	}
	void * stream = buffer->stream;
	free(buffer);
	
	return stream;

}
/*

void* _serializar_acceso_espacio_usuario(t_acceso_espacio_usuario* acceso_espacio_usuario,int* size){
	// uint32_t tamanio_dato = ((uint32_t)strlen(acceso_espacio_usuario->registro_dato)+(uint32_t)1);
	
	*size = sizeof(uint32_t) * 3 + (strlen(acceso_espacio_usuario->registro_dato)+1);
	t_buffer* buffer = crear_buffer(*size);
	agregar_a_buffer(buffer, &acceso_espacio_usuario->PID, sizeof(uint32_t));
	agregar_a_buffer(buffer, &acceso_espacio_usuario->direccion_fisica, sizeof(uint32_t));
	//agregar_a_buffer(buffer, &acceso_espacio_usuario->bytes_restantes_en_frame, sizeof(uint32_t));
	agregar_a_buffer(buffer, &acceso_espacio_usuario->size_registro, sizeof(uint32_t));
	// if(tamanio_dato)
	agregar_a_buffer(buffer, acceso_espacio_usuario->registro_dato, strlen(acceso_espacio_usuario->registro_dato)+1);
	void * stream = buffer->stream;
	free(buffer);
	
	return stream;

}



void _enviar_acceso_espacio_usuario(t_acceso_espacio_usuario* acceso_espacio_usuario,op_code operacion,int socket){
	int size;
	void* stream = _serializar_acceso_espacio_usuario(acceso_espacio_usuario,&size);									
					 
	enviar_stream(stream,size,socket,operacion);
	free(stream);
}
*/

void enviar_acceso_espacio_usuario(t_acceso_espacio_usuario* acceso_espacio_usuario,op_code operacion,int socket){
	int size;
	void* stream = serializar_acceso_espacio_usuario(acceso_espacio_usuario,&size);									
					 
	enviar_stream(stream,size,socket,operacion);
	free(stream);
}



void _enviar_stream_(void* texto,int size,int socket,op_code operacion){

																					 
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = operacion;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = size;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, texto, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);
	
	//int valor_int;
	//void* valor_int_puntero= &valor_int;
	//memcpy(valor_int_puntero,texto,size);

	//loguear("Valor entero: <%d>",valor_int);
	
	
	send(socket, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);

}

void enviar_direcciones_proceso(t_direcciones_proceso* direcciones_proceso,op_code operacion,int socket){
	int size;
	void* stream = serializar_direcciones_proceso(direcciones_proceso,&size);									
					 
	enviar_stream(stream,size,socket,operacion);
	free(stream);
}
void* serializar_direcciones_proceso(t_direcciones_proceso* direcciones_proceso,int* size){
	
	t_list* lista = direcciones_proceso->direcciones;
	uint32_t cant_direcciones = list_size(lista);
	*size = sizeof(uint32_t) * 4 + cant_direcciones * (2*sizeof(uint32_t));

	t_buffer* buffer = crear_buffer(*size);
	

	agregar_a_buffer(buffer, &direcciones_proceso->pid_size_total.PID, sizeof(uint32_t));
	agregar_a_buffer(buffer, &direcciones_proceso->pid_size_total.valor, sizeof(uint32_t));
	agregar_a_buffer(buffer, &cant_direcciones, sizeof(uint32_t));
	
	for(int i=0;i<cant_direcciones;i++)
	{	
		t_id_valor* id_valor = (t_id_valor*)list_get(lista,i);
		agregar_a_buffer(buffer, &id_valor->id, sizeof(uint32_t));
		agregar_a_buffer(buffer, &id_valor->valor, sizeof(uint32_t));
	
	}
	void * stream = buffer->stream;
	free(buffer);
	
	return stream;
}


				  
											

 