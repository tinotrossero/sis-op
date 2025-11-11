#include "utils-commons.h"
int ultimo_pid=0;


t_pcb* pcb_create(char* path_programa){

	t_pcb* pcb = malloc(sizeof(t_pcb));
	 memset(pcb, 0, sizeof(t_pcb));
	pcb->PID = ultimo_pid++;
	pcb->archivos_abiertos = list_create();
	pcb->registros_cpu = inicializar_registros();
	pcb->program_counter = 0;    
	pcb->quantum = 0; // En caso de necesitarlo, el planificador de corto plazo lo inicializará con el valor adecuado 
    pcb->path = string_duplicate(path_programa);
   	return pcb;
}
t_pcb* pcb_create_copy(char* path_programa){

	t_pcb* pcb = malloc(sizeof(t_pcb));
	pcb->archivos_abiertos = list_create();
	pcb->registros_cpu = inicializar_registros();
	pcb->program_counter = 0;    
	pcb->quantum = 0; // En caso de necesitarlo, el planificador de corto plazo lo inicializará con el valor adecuado 
    pcb->path = string_duplicate(path_programa);
   	return pcb;
}

t_pid_valor* pid_value_create(uint32_t pid, uint32_t valor){
	t_pid_valor* pid_atributo = malloc(sizeof(t_pid_valor));
	pid_atributo->PID = pid;
	pid_atributo->valor = valor;
	return pid_atributo;
}

t_acceso_espacio_usuario* acceso_espacio_usuario_create(uint32_t PID, uint32_t direccion, uint32_t size_registro,void* valor){
	t_acceso_espacio_usuario* acceso_espacio_usuario = malloc(sizeof(t_acceso_espacio_usuario));
	acceso_espacio_usuario->PID = PID;
	acceso_espacio_usuario->direccion_fisica = direccion;
	//acceso_espacio_usuario->bytes_restantes_en_frame = bytes_restantes;
	acceso_espacio_usuario->size_registro = size_registro;
	//acceso_espacio_usuario->size_registro = (valor!=NULL) ? (uint32_t)(strlen(valor)+1) : (uint32_t)0;
	acceso_espacio_usuario->registro_dato = valor;
	return acceso_espacio_usuario;
}

void acceso_espacio_usuario_destroy(t_acceso_espacio_usuario* acceso){
	if(acceso !=NULL) {
		if(acceso->registro_dato)
			free(acceso->registro_dato);
		free(acceso);
	}
}

t_registros_cpu* inicializar_registros(){
    t_registros_cpu* registros = calloc(1, sizeof(t_registros_cpu));

    if (registros == NULL){
        loguear_error("No se pudieron iniciar los registros correctamente");
        return NULL;
    }

    return registros;
}

bool es_codigo_valido(int code){
	return code>=MENSAJE&&code<NO_CODE;
}


t_pcb* pcb_create_quantum(char* path_programa,int quantum){
	t_pcb* pcb = pcb_create(path_programa);
	pcb->quantum=quantum;
	return pcb;
}

void pcb_destroy(t_pcb* pcb){
	list_destroy(pcb->archivos_abiertos);
	free(pcb->registros_cpu);
    free(pcb->path);
	free(pcb);
}

void loguear_registros(t_registros_cpu* registros){
	loguear("Reg AX: %d",registros->AX);
	loguear("Reg BX: %d",registros->BX);
	loguear("Reg CX: %d",registros->CX);
	loguear("Reg DX: %d",registros->DX);
	loguear("Reg EAX: %d",registros->EAX);
	loguear("Reg EBX: %d",registros->EBX);
	loguear("Reg ECX: %d",registros->ECX);
	loguear("Reg EDX: %d",registros->EDX);
	loguear("Reg SI: %d",registros->SI);
	loguear("Reg DI: %d",registros->DI);
}

void loguear_pcb(t_pcb* pcb){
//	printf("LOGUEO PCB\n");
	loguear("PID: %d",pcb->PID);
	loguear("program_counter: %d",pcb->program_counter);
	loguear("Prioridad: %d",pcb->prioridad);
	loguear("Quantum: %d", pcb->quantum);
	loguear_registros(pcb->registros_cpu);
	loguear("Cant. de archivos abiertos: %d",list_size( pcb->archivos_abiertos));
    loguear("PATH: %s",pcb->path);
//	printf("==========================----- \n");

//	printf("FIN LOGUEO PCB\n");
}

void loguear_pid_value(t_pid_valor* pid_atributo){
	printf("LOGUEO PID VALUE\n");
	loguear("PID: %d",pid_atributo->PID);
	loguear("Valor: %d",pid_atributo->valor);
	printf("=============================== \n");

}

bool is_numeric(const char* str) {
    if (str == NULL || *str == '\0') 
        return false;
    for (int i = 0; str[i] != '\0'; i++) 
        if (!isdigit(str[i])) 
            // If any character s not a digit, return false.
            return false;
    return true;
}


char* path_resolve(char* directorio, char* nombre_archivo)
{
   char* path = string_duplicate(directorio);
	int path_size = strlen(path);;
	if(path[path_size - 1] != '/')
		string_append(&path, "/");
	string_append(&path, nombre_archivo);
	return path;
}

char * uint_a_string(uint num){
    char* string = malloc(sizeof(char) * 20);
	sprintf(string,"%u", num);
    return string;
}

void quitar_salto_linea(char* linea){
	if(linea!=NULL){
		int len = strlen(linea);
		if(linea[len - 1] == '\n')
		{  //Eliminamos el salto de línea
			linea[len - 1] = '\0';
			len--;
		}
		if(linea[len - 1] == '\r')
			//Eliminamos el retorno de carro.
		linea[len - 1] = '\0';
	}
}

FILE* abrir_archivo(char* directorio,char* nombre_archivo){
char* path = path_resolve(directorio,nombre_archivo);
	
	FILE *archivo;

	archivo = fopen(path, "r");
	

	if (archivo == NULL)
	{
		perror("Error al abrir el archivo");
		loguear_error("No se pudo abrir el archivo %s \n", path);
		free(path);
		return NULL;
	}
	free(path);
	return archivo;
}


t_list* get_instrucciones(char* directorio,char *nombre_archivo)
{
	t_list *lista_instrucciones = list_create();
	FILE *archivo = abrir_archivo(directorio,nombre_archivo);
	
	if (archivo == NULL)		
	{	list_destroy(lista_instrucciones);
		return NULL;
	}
 	char *linea = NULL;
    size_t line_size = 0;
    ssize_t read;

    while ((read = getline(&linea, &line_size, archivo)) != -1) {
        quitar_salto_linea(linea);
        char *inst = string_duplicate(linea);
        if (inst) {
            list_add(lista_instrucciones, inst);
        } else {
            free(inst);
        }
    }

    free(linea);
    fclose(archivo);
    return lista_instrucciones;
}

char *leer_linea_i(FILE *archivo, int posicion) {
    char *linea = NULL;
    size_t longitud = 0;
    ssize_t caracteres_leidos;
    int num_linea = 0;

    // Leer líneas hasta llegar a la posición deseada
    while (num_linea <= posicion && (caracteres_leidos = getline(&linea, &longitud, archivo)) != -1) 
        num_linea++;
    

    // Si la posición especificada excede el número de líneas en el archivo
    if (num_linea < posicion) {
        free(linea);
        return NULL;
    }

    // Si la línea se leyó correctamente, devolverla
    return linea;
}

char* get_linea_archivo(char* directorio,char* nombre_archivo,int posicion){

	FILE *archivo = abrir_archivo(directorio,nombre_archivo);
	if (archivo == NULL)		
		return NULL;
	char *linea = leer_linea_i(archivo, posicion);
	quitar_salto_linea(linea);			
	fclose(archivo);

	return linea;

}

bool es_exit(void *comando)
{
	char *instruccion = (char*)comando;
	return string_equals_ignore_case(instruccion, (char *)EXIT_PROGRAM);
}

t_validacion* validacion_new(){
	t_validacion* validacion = malloc(sizeof(t_validacion));
	validacion->resultado= false;
	return validacion;
	
}
void liberar_pcb_contenido(t_pcb* pcb) {
    if (pcb->registros_cpu != NULL) {
        free(pcb->registros_cpu);
    }
    if (pcb->path != NULL) {
        free(pcb->path);
    }
}
void reemplazar_pcb_con(t_pcb* destino,t_pcb* origen) {
    liberar_pcb_contenido(destino); // Liberar la memoria del destino

    // Asignar el contenido del origen al destino
    destino->PID = origen->PID;
    destino->program_counter = origen->program_counter;
    destino->prioridad = origen->prioridad;
    destino->quantum = origen->quantum;

    // Asignar registros_cpu
    destino->registros_cpu = (t_registros_cpu*)malloc(sizeof(t_registros_cpu));
    if (destino->registros_cpu != NULL) {
        memcpy(destino->registros_cpu, origen->registros_cpu, sizeof(t_registros_cpu));
    }

    // Asignar path
    if (origen->path != NULL) {
        destino->path = (char*)malloc(strlen(origen->path) + 1);
        if (destino->path != NULL) {
            strcpy(destino->path, origen->path);
        }
    } else {
        destino->path = NULL;
    }
}

t_direccion_tamanio* direccion_tamanio_new(uint32_t direccion,uint32_t size){
	t_direccion_tamanio* direccion_tamanio = malloc(sizeof(t_direccion_tamanio));
	direccion_tamanio->direccion_fisica = direccion;
	direccion_tamanio->tamanio_bytes =size;

	return direccion_tamanio;
}


int list_find_index(t_list* self, bool(*condition)(void*)) {
    if (self == NULL || condition == NULL) {
        return -1;
    }

    t_link_element *current_element = self->head;
    int index = 0;

    while (current_element != NULL) {
        if (condition(current_element->data)) {
            return index;
        }
        current_element = current_element->next;
        index++;
    }

    return -1;  // No se encontró ningún elemento que cumpla la condición
}

bool is_true(void* element){
   return element!=NULL && *(bool*)element;
}

bool is_false(void* element){
   return !is_true(element);
}


t_direcciones_proceso* direcciones_proceso_create(uint32_t pid,uint32_t tamanio){
	t_direcciones_proceso* direcciones_proceso = malloc(sizeof(t_direcciones_proceso));
	direcciones_proceso->pid_size_total.PID = pid;
	direcciones_proceso->pid_size_total.valor=tamanio;
	direcciones_proceso->direcciones = list_create();
	return direcciones_proceso;
}

t_id_valor* id_valor_new(uint32_t id,uint32_t valor){
	t_id_valor* id_valor = malloc(sizeof(t_id_valor));
	id_valor->id = id;
	id_valor->valor =valor;

	return id_valor;
}

t_direccion_registro* direccion_registro_new(uint32_t direccion,uint32_t size){
	t_direccion_registro* direccion_registro = malloc(sizeof(t_direccion_registro));
	direccion_registro->direccion_fisica = direccion;
	direccion_registro->size_registro_pagina =size;

	return direccion_registro;
}

void direcciones_proceso_destroy(t_direcciones_proceso* direcciones_proceso){
	list_destroy_and_destroy_elements(direcciones_proceso->direcciones,free);
	free(direcciones_proceso);
}

void loguear_direccion_proceso(t_direcciones_proceso* dir_proceso){

	loguear("PID proceso:%d",dir_proceso->pid_size_total.PID);
	loguear("Tam proceso:%d",dir_proceso->pid_size_total.valor);
	loguear("--------%s--------","direcciones");
	for(int i=0;i<list_size(dir_proceso->direcciones);i++){
		t_direccion_registro* id_valor = (t_direccion_registro* )list_get(dir_proceso->direcciones,i);
		loguear("Direcc: %d - Tam: %d", id_valor->direccion_fisica,id_valor->size_registro_pagina);
	}
	loguear("--------%s--------","fin direcciones");

}