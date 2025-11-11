#include "utils-server.h"


int iniciar_servidor(int puerto)
{
	int socket_servidor;

	//parámetros que nos guardarán las direcciones para que la máquina entienda
	struct addrinfo hints, *servinfo; 


	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	char* puerto_char = (char*)malloc(sizeof(char) * 20); 
	sprintf(puerto_char, "%d", puerto);
	getaddrinfo(NULL, puerto_char, &hints, &servinfo);
	free(puerto_char);

	// Creamos el socket de escucha del servidor
	socket_servidor= socket(servinfo->ai_family,servinfo->ai_socktype,servinfo->ai_protocol);

	int optval = 1;
    setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
	// Asociamos el socket a un puerto
	bind(socket_servidor,servinfo->ai_addr,servinfo->ai_addrlen);
	// Escuchamos las conexiones entrantes
	listen(socket_servidor,SOMAXCONN);

	freeaddrinfo(servinfo);
	return socket_servidor;
}

int esperar_cliente(int socket_servidor)
{

	// Aceptamos un nuevo cliente
	int socket_cliente=accept(socket_servidor,NULL,NULL);

	loguear("Se conecto un cliente!");

	return socket_cliente;
}

int recibir_operacion(int socket_cliente)
{
	int cod_op;
	if(recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
		return cod_op;
	else
	{
		close(socket_cliente);
		return -1;
	}
}

void* recibir_buffer(int* size, int socket_cliente)
{
	void * buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

char* recibir_mensaje(int socket_cliente)
{
	int size;
	char* buffer = recibir_buffer(&size, socket_cliente);
	// log_info(logger, "Me llegó el mensaje %s", buffer);
	return buffer;
}


t_paquete* recibir_paquete(int socket_cliente)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	memset(paquete, 0, sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));

	// Inicializar el buffer a cero
		
    memset(paquete->buffer, 0, sizeof(t_buffer));
	// Primero recibimos el codigo de operacion
	recv(socket_cliente, &(paquete->codigo_operacion), sizeof(op_code), 0);

	if(paquete->codigo_operacion==-1)
		return paquete;

	// Después ya podemos recibir el buffer. Primero su tamaño seguido del contenido
	recv(socket_cliente, &(paquete->buffer->size), sizeof(uint32_t), 0);
	if(paquete->buffer->size>0){
		paquete->buffer->stream = malloc(paquete->buffer->size);
		recv(socket_cliente, paquete->buffer->stream, paquete->buffer->size, 0);
	}
	else paquete->buffer->stream = NULL;


	return paquete;
}

t_list* recibir_lista(int socket_cliente)
{
	int size;
	int desplazamiento = 0;
	void * buffer;
	t_list* valores = list_create();
	int tamanio;

	buffer = recibir_buffer(&size, socket_cliente);
	while(desplazamiento < size)
	{
		memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		char* valor = malloc(tamanio);
		memcpy(valor, buffer+desplazamiento, tamanio);
		desplazamiento+=tamanio;
		list_add(valores, valor);
	}
	free(buffer);
	return valores;
}

void recibir_de_buffer(void* lugar_destino,t_buffer* buffer,size_t tam){
	memcpy(lugar_destino,buffer->stream+buffer->desplazamiento,tam);
	buffer->desplazamiento+=tam;
}

t_pcb* recibir_pcb(t_paquete* paquete)
{
	t_pcb* pcb = pcb_create_copy("");
	t_buffer* buffer = paquete->buffer;
	buffer->desplazamiento = sizeof(uint32_t);
	uint32_t path_size ;

	recibir_de_buffer(&pcb->PID,buffer,sizeof(uint32_t));
	recibir_de_buffer(&pcb->prioridad, buffer, sizeof(uint8_t));
	recibir_de_buffer(&pcb->program_counter, buffer, sizeof(uint32_t));
	recibir_de_buffer(&pcb->quantum, buffer, sizeof(uint32_t));
	recibir_de_buffer(&pcb->registros_cpu->AX, buffer, sizeof(uint8_t));
	recibir_de_buffer(&pcb->registros_cpu->BX, buffer, sizeof(uint8_t));
	recibir_de_buffer(&pcb->registros_cpu->CX, buffer, sizeof(uint8_t));
	recibir_de_buffer(&pcb->registros_cpu->DX, buffer, sizeof(uint8_t));
	recibir_de_buffer(&pcb->registros_cpu->EAX, buffer, sizeof(uint32_t));	
	recibir_de_buffer(&pcb->registros_cpu->EBX, buffer, sizeof(uint32_t));		
	recibir_de_buffer(&pcb->registros_cpu->ECX, buffer, sizeof(uint32_t));	
	recibir_de_buffer(&pcb->registros_cpu->EDX, buffer, sizeof(uint32_t));	
	recibir_de_buffer(&pcb->registros_cpu->SI, buffer, sizeof(uint32_t));	
	recibir_de_buffer(&pcb->registros_cpu->DI, buffer, sizeof(uint32_t));	
	recibir_de_buffer(&path_size, buffer, sizeof(uint32_t));

	//free(pcb->path);
	pcb->path = realloc(pcb->path,path_size);
	recibir_de_buffer(pcb->path, buffer, path_size);

	return pcb;
}


t_id_valor* recibir_id_value(t_paquete* paquete)
{
	t_id_valor* id_value = malloc(sizeof(t_id_valor));
	t_buffer* buffer = paquete->buffer;
	buffer->desplazamiento = sizeof(uint32_t);
	
	recibir_de_buffer(&id_value->id,buffer,sizeof(uint32_t));
	recibir_de_buffer(&id_value->valor,buffer,sizeof(uint32_t));

	return id_value;

}

t_id_valor_string* recibir_id_value_string(t_paquete* paquete)
{
	t_id_valor_string* id_value = malloc(sizeof(t_id_valor_string));
	t_buffer* buffer = paquete->buffer;
	buffer->desplazamiento = sizeof(uint32_t);

	int string_size;
	
	recibir_de_buffer(&id_value->id,buffer,sizeof(uint32_t));
	recibir_de_buffer(&string_size,buffer,sizeof(uint32_t));
	recibir_de_buffer(&id_value->valor,buffer,string_size);

	return id_value;

}



t_pid_valor* recibir_pid_value(t_paquete* paquete)
{
	t_id_valor* id_value = recibir_id_value(paquete);
	t_pid_valor* pid_value = malloc(sizeof(t_pid_valor));
	pid_value->PID = id_value->id;
	pid_value->valor = id_value->valor;
	free(id_value);

	return pid_value;

}

t_acceso_espacio_usuario* recibir_acceso_espacio_usuario(t_paquete* paquete)
{
	
	t_acceso_espacio_usuario* acceso_espacio_usuario = malloc(sizeof(t_acceso_espacio_usuario));
	t_buffer* buffer = paquete->buffer;
	buffer->desplazamiento = sizeof(uint32_t);
	
	recibir_de_buffer(&acceso_espacio_usuario->PID,buffer,sizeof(uint32_t));
	recibir_de_buffer(&acceso_espacio_usuario->direccion_fisica,buffer,sizeof(uint32_t));
	//recibir_de_buffer(&acceso_espacio_usuario->bytes_restantes_en_frame,buffer,sizeof(uint32_t));
	recibir_de_buffer(&acceso_espacio_usuario->size_registro,buffer,sizeof(uint32_t));

	if (acceso_espacio_usuario->size_registro>0)
	{	acceso_espacio_usuario->registro_dato = malloc(acceso_espacio_usuario->size_registro);
		recibir_de_buffer(acceso_espacio_usuario->registro_dato,buffer,acceso_espacio_usuario->size_registro);
		//recibir_de_buffer(acceso_espacio_usuario->registro_dato,buffer,strlen(acceso_espacio_usuario->registro_dato)+1);
	}
	return acceso_espacio_usuario;

}


t_direcciones_proceso* recibir_direcciones_proceso(t_paquete* paquete)
{
	t_buffer* buffer = paquete->buffer;
	buffer->desplazamiento = sizeof(uint32_t);
	void _recibir(void* lugar_destino,size_t tam){
		recibir_de_buffer(lugar_destino,buffer,tam);
	}
	int cant_direcciones;
	t_direcciones_proceso* direcciones_proceso = direcciones_proceso_create(0,0);
	
	_recibir(&direcciones_proceso->pid_size_total.PID,sizeof(uint32_t));
	_recibir(&direcciones_proceso->pid_size_total.valor,sizeof(uint32_t));
	_recibir(&cant_direcciones,sizeof(uint32_t));
	for(int i=0;i<cant_direcciones;i++)
	{	t_id_valor* id_valor = malloc(sizeof(t_id_valor));
		_recibir(&id_valor->id,sizeof(uint32_t));
		_recibir(&id_valor->valor,sizeof(uint32_t));
		list_add(direcciones_proceso->direcciones,id_valor);
	}

	return direcciones_proceso;
}