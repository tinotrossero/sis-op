#include "kernel.h"
#include "semaphore.h"
#include <time.h>

sem_t sem_cont_grado_mp;
sem_t sem_bin_new; //Sincroniza que plp (hilo new) no actúe hasta que haya un nuevo elemento en new
sem_t sem_bin_ready; //Sincroniza que pcp no actúe hasta que haya un nuevo elemento en ready
sem_t sem_bin_exit; //Sincroniza que plp (hilo exit) no actúe hasta que haya un nuevo elemento en exit
sem_t sem_bin_cpu_libre; // Sincroniza que no haya ningun PCB ejecutando en CPU
sem_t sem_bin_controlar_quantum;

sem_t sem_bin_recibir_pcb;
sem_t sem_bin_plp_procesos_nuevos_iniciado,sem_bin_plp_procesos_finalizados_iniciado,sem_bin_planificador_corto_iniciado;

pthread_mutex_t mx_new = PTHREAD_MUTEX_INITIALIZER; // Garantiza mutua exclusion en estado_new. Podrían querer acceder consola y plp al mismo tiempo
pthread_mutex_t mx_ready = PTHREAD_MUTEX_INITIALIZER; //Garantiza mutua exclusion en estado_ready. Podrían querer acceder plp y pcp al mismo tiempo
pthread_mutex_t mx_ready_plus = PTHREAD_MUTEX_INITIALIZER; //Garantiza mutua exclusion en estado_ready. Podrían querer acceder plp y pcp al mismo tiempo
pthread_mutex_t mx_blocked = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_exit = PTHREAD_MUTEX_INITIALIZER; // Garantiza mutua exclusion en estado_exit. Podrían querer acceder consola, plp y pcp al mismo tiempo
pthread_mutex_t mx_pcb_exec = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_temp = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_grado_mult_de_mas = PTHREAD_MUTEX_INITIALIZER;

time_t tiempo_inicial, tiempo_final;

int conexion_memoria, cpu_dispatch,cpu_interrupt, kernel_escucha, conexion_io;
int cod_op_dispatch,cod_op_interrupt,cod_op_memoria;
bool planificacion_detenida = false;
bool eliminar_proceso_en_FIN_QUANTUM = false, exec_recibido = false;
int grado_multiprog_de_mas =0;

t_config_kernel* config;
t_dictionary * comandos_consola,
	*estados_dictionary,
	*estados_mutexes_dictionary, 
	*diccionario_nombre_conexion, 
	*diccionario_nombre_qblocked, 
	*diccionario_conexion_qblocked,
	*nombres_colas_dictionary,
	*tipos_de_interfaces;
t_list * lista_recursos;
t_queue* estado_new, *estado_ready, *estado_exit, *estado_ready_plus, *estado_temp;
//
t_blocked_interfaz* blocked_interfaz;
//
t_pcb* pcb_exec = NULL;
//t_list* lista_interfaces_blocked;

 // Crear el diccionario de algoritmo
t_planificador get_algoritmo(char* nombre){


	 // Crear el diccionario de algoritmos
    t_dictionary *algoritmos = dictionary_create();
	t_planificador planificador;

    // Función para agregar un algoritmo al diccionario
    void _agregar(char* _nombre, t_alg_planificador tipo,void(*funcion)(void)){
		t_planificador* planif = malloc(sizeof(t_planificador));
		planif->id = tipo;
		planif->planificar = funcion; 	
        dictionary_put(algoritmos, _nombre, planif);
    };

	_agregar("FIFO",FIFO,&planificacion_FIFO);
	_agregar("RR",RR,&planificacion_RR);
	_agregar("VRR",VRR,&planificacion_VRR);

	t_planificador* planificador_ptr= (t_planificador*)dictionary_get(algoritmos, nombre);
	if(planificador_ptr==NULL)	
	{	
		perror("Algoritmo de planificación no válido");

		exit(EXIT_FAILURE);
	}
	else planificador=*planificador_ptr;
	
	dictionary_destroy_and_destroy_elements(algoritmos,free);
	return planificador;

}

t_config_kernel* iniciar_config_kernel(char* path_config){
	t_config* _config = config_create(path_config);
	if(_config ==NULL)
		return NULL;
	t_config_kernel* config_kernel = malloc(sizeof(t_config_kernel));	

	config_kernel->PUERTO_ESCUCHA= config_get_int_value(_config,"PUERTO_ESCUCHA");
	config_kernel->IP_MEMORIA = config_get_string_value(_config,"IP_MEMORIA");
	config_kernel->PUERTO_MEMORIA = config_get_int_value(_config,"PUERTO_MEMORIA");
	config_kernel->IP_CPU = config_get_string_value(_config,"IP_CPU");
	config_kernel->PUERTO_CPU_DISPATCH = config_get_int_value(_config,"PUERTO_CPU_DISPATCH");
	config_kernel->PUERTO_CPU_INTERRUPT = config_get_int_value(_config,"PUERTO_CPU_INTERRUPT");
	config_kernel->ALGORITMO_PLANIFICACION = get_algoritmo(config_get_string_value(_config,"ALGORITMO_PLANIFICACION"));
	config_kernel->QUANTUM = config_get_int_value(_config,"QUANTUM");
	config_kernel->RECURSOS = config_get_array_value(_config,"RECURSOS");
	config_kernel->INSTANCIAS_RECURSOS = config_get_array_value(_config,"INSTANCIAS_RECURSOS");
	config_kernel->GRADO_MULTIPROGRAMACION = config_get_int_value(_config,"GRADO_MULTIPROGRAMACION_INI");
	config_kernel->PATH_SCRIPTS = config_get_string_value(_config,"PATH_SCRIPTS");

	config_kernel->config = _config;
	if(config_kernel->ALGORITMO_PLANIFICACION.planificar ==NULL)
	{	config_destroy_kernel(config_kernel);
		return NULL;
	}
	return config_kernel;
}

bool iniciar_logger_config(char* path_config){
	decir_hola(MODULO);
    logger = iniciar_logger_(MODULO,0);
	if(logger == NULL) printf("EL LOGGER NO PUDO SER INICIADO.\n");
	config = iniciar_config_kernel(path_config);
	if(config == NULL) {
		loguear_error("No se encuentra el archivo de las config");
		return false;
	}
	loguear_config();	
	return true;
}

bool inicializar_comandos(){
	comandos_consola  =  dictionary_create();
    agregar_comando(EJECUTAR_SCRIPT,"EJECUTAR_SCRIPT","[PATH]",&ejecutar_scripts_de_archivo);
    agregar_comando(INICIAR_PROCESO,"INICIAR_PROCESO","[PATH]",&iniciar_proceso);
	agregar_comando(FINALIZAR_PROCESO,"FINALIZAR_PROCESO","[PID]",&finalizar_proceso);
    agregar_comando(DETENER_PLANIFICACION,"DETENER_PLANIFICACION","[]",&detener_planificacion);
	agregar_comando(INICIAR_PLANIFICACION,"INICIAR_PLANIFICACION","[]",&iniciar_planificacion);
    agregar_comando(MULTIPROGRAMACION,"MULTIPROGRAMACION","[VALOR]",&multiprogramacion);
	agregar_comando(PROCESO_ESTADO,"PROCESO_ESTADO","[]",&proceso_estado);
    agregar_comando(EXIT,"EXIT","[]",&finalizar_consola);
	return true;
}

bool iniciar_conexion_memoria(){
	conexion_memoria = crear_conexion(config->IP_MEMORIA,config->PUERTO_MEMORIA);
	if(conexion_memoria ==-1){
		
		loguear_error("No se pudo conectar memoria");
		return false;
	} 
	return true;
}

bool iniciar_dispatch(){
	cpu_dispatch = crear_conexion(config->IP_CPU, config->PUERTO_CPU_DISPATCH);
	if(cpu_dispatch ==-1){
		
		loguear_error("No se pudo conectar cpu (dispatch)");
		return false;
	} 
	return true;
}

bool iniciar_interrupt(){
	cpu_interrupt = crear_conexion(config->IP_CPU, config->PUERTO_CPU_INTERRUPT);
	if(cpu_interrupt ==-1){	
		loguear_error("No se pudo conectar cpu (interrupt)");
		return false;
	} 
	return true;
}


bool iniciar_estados_planificacion(){

	estado_new = queue_create();
	estado_ready = queue_create();
	estado_exit = queue_create();	
	estado_temp = queue_create();	
	if(es_vrr())
		estado_ready_plus = queue_create();
	
	
	return true;
}
t_recurso* crear_recurso(char* nombre, int instancias){
	t_recurso* recurso = malloc(sizeof(t_recurso));
	recurso->recurso = nombre;
	recurso->instancias = instancias;
	recurso->cola_procesos_esperando = queue_create();
	recurso->lista_procesos_asignados = list_create();
	return recurso;
}


bool iniciar_recursos(){

	int cantidad_recursos = string_array_size(config->RECURSOS);
	lista_recursos = list_create();
	char* key;
	int value;
	
	t_recurso* recurso;
	

	//list_add(lista_recursos,recurso);
	
	for(int i=0;i < cantidad_recursos ;i++){
		key = string_array_pop(config->RECURSOS);
		char* instancia_recurso = string_array_pop(config->INSTANCIAS_RECURSOS);
		value = atoi(instancia_recurso);
		free(instancia_recurso);
		if(string_contains(key,"]"))
			key[string_length(key)-1]='\0';
		loguear("RECURSO: %s - INSTANCIAS: %d",key,value);
		/////////////////
		recurso = crear_recurso(key,value);
		list_add(lista_recursos,recurso);
		dictionary_put(estados_dictionary, key, recurso->cola_procesos_esperando);
		pthread_mutex_t* mx_recurso = malloc(sizeof(pthread_mutex_t)); 
		pthread_mutex_init(mx_recurso, NULL);
		dictionary_put(estados_mutexes_dictionary, key, mx_recurso);
		////////////////////
		loguear("RECURSO: %s - INSTANCIAS: %d",key,value);
	}
	
	return true;
}



bool iniciar_kernel(char* path_config){
	return


	iniciar_logger_config(path_config)&&
	inicializar_comandos() &&
	iniciar_servidor_kernel()&&
	iniciar_conexion_memoria()&&
	iniciar_dispatch()&&
	iniciar_interrupt()&&
	iniciar_estados_planificacion()&&
	//iniciar_colas_entrada_salida()&&
	iniciar_semaforos()&&
	inicializar_dictionario_mutex_colas()&&
	iniciar_recursos() &&
	iniciar_threads_io();
}
bool iniciar_semaforos(){
	sem_init(&sem_cont_grado_mp,0,config->GRADO_MULTIPROGRAMACION);
	sem_init(&sem_bin_new,0,0);
	sem_init(&sem_bin_ready,0,0);
	sem_init(&sem_bin_exit,0,0);
	sem_init(&sem_bin_cpu_libre,0,1);
	sem_init(&sem_bin_recibir_pcb,0,1);
	sem_init(&sem_bin_controlar_quantum,0,1);

	sem_init(&sem_bin_plp_procesos_nuevos_iniciado,0,1);
	sem_init(&sem_bin_plp_procesos_finalizados_iniciado,0,1);
	sem_init(&sem_bin_planificador_corto_iniciado,0,1);

	return true;
}

void inicializar_nombres_colas(){
	nombres_colas_dictionary = dictionary_create();

	void _agregar(t_codigo_estado codigo,char* nombre){
		char* key = string_itoa(codigo);
		dictionary_put(nombres_colas_dictionary,key,nombre);
		free(key);
	};

	
	_agregar(NEW,"NUEVO");
	_agregar(READY,"READY");
	_agregar(EXEC,"EXEC");
	_agregar(EXIT_STATE,"FINALIZADO");
	_agregar(TEMP,"TEMPORAL");
	if(es_vrr())
		_agregar(READY_PLUS,"READY_PLUS");
}

bool inicializar_dictionario_mutex_colas(){
	estados_dictionary = dictionary_create();
	estados_mutexes_dictionary = dictionary_create();
	tipos_de_interfaces = dictionary_create();

 
	void _agregar(t_codigo_estado codigo,void* estado,t_dictionary* diccionario){
		char* clave = string_itoa(codigo);
		dictionary_put(diccionario,clave,estado);
		free(clave);
	};
	void _agregar_interfaz(op_code codigo,void* estado,t_dictionary* diccionario){
		char* clave = string_itoa(codigo);
		dictionary_put(diccionario,clave,estado);
		free(clave);
	};

	void _agregar_estado(t_codigo_estado codigo,void* estado){
		_agregar(codigo,estado,estados_dictionary);
	};

	void _agregar_mx(t_codigo_estado codigo,pthread_mutex_t* mx){
		_agregar(codigo,mx,estados_mutexes_dictionary);
	};

	_agregar_interfaz(IO_STDIN_READ,"STDIN",tipos_de_interfaces);
	_agregar_interfaz(IO_STDOUT_WRITE,"STDOUT",tipos_de_interfaces);
	_agregar_interfaz(IO_GEN_SLEEP,"GENERICA",tipos_de_interfaces);
	_agregar_interfaz(FILE_SYSTEM,"DIALFS",tipos_de_interfaces);
	// _agregar_interfaz(IO_FS_DELETE,"DIALFS",tipos_de_interfaces);
	// _agregar_interfaz(IO_FS_TRUNCATE,"DIALFS",tipos_de_interfaces);
	// _agregar_interfaz(IO_FS_WRITE,"DIALFS",tipos_de_interfaces);
	// _agregar_interfaz(IO_FS_READ,"DIALFS",tipos_de_interfaces);

	






	_agregar_estado(NEW,estado_new);
	_agregar_estado(READY,estado_ready);
	_agregar_estado(EXEC,pcb_exec);
	_agregar_estado(EXIT_STATE,estado_exit);
	_agregar_estado(TEMP,estado_temp);

	_agregar_mx(NEW,&mx_new);
	_agregar_mx(READY,&mx_ready);
	_agregar_mx(EXEC,&mx_pcb_exec);
	_agregar_mx(EXIT_STATE,&mx_exit);
	_agregar_mx(TEMP,&mx_temp);

	if(es_vrr()){
		_agregar_estado(READY_PLUS,estado_ready_plus);
		_agregar_mx(READY_PLUS,&mx_ready_plus);
	}

	inicializar_nombres_colas();

	return true;
}

void liberar_diccionario(t_dictionary* diccionario){
	if(diccionario)
	dictionary_destroy(diccionario);
}

void liberar_diccionario_mx_estados(){
   t_dictionary* diccionario = estados_mutexes_dictionary;
   t_list* nombres_recuros = get_nombres_recursos();
	void _liberar(char*key,void* elem){
		bool es_igual_a(void*elem){
			return strcmp(key,elem)==0;
		}
		bool es_interfaz = dictionary_has_key(diccionario_nombre_conexion,key);
		bool es_recurso =  list_any_satisfy(nombres_recuros,&es_igual_a);
		
		if(es_interfaz||es_recurso)
		free(elem);
	}
	dictionary_iterator(diccionario,&_liberar);
	liberar_diccionario(diccionario);
	list_destroy(nombres_recuros);
}


void liberar_diccionario_colas(){
	liberar_diccionario(estados_dictionary);		
	liberar_diccionario_mx_estados();
	liberar_diccionario(nombres_colas_dictionary);
	liberar_diccionario(tipos_de_interfaces);

	
}

void liberar_diccionarios_interfaces(){
	if(diccionario_nombre_conexion)
		dictionary_destroy_and_destroy_elements(diccionario_nombre_conexion,free);
	liberar_diccionario(diccionario_nombre_qblocked);
	if(diccionario_conexion_qblocked)
	dictionary_destroy_and_destroy_elements(diccionario_conexion_qblocked,blocked_interfaz_destroy);
}

void bloquear_mutex_colas(){
	void _bloquear(char* _,void* element){
		pthread_mutex_t* mutex = (pthread_mutex_t*)element;
		pthread_mutex_lock(mutex);
	};
	dictionary_iterator(estados_mutexes_dictionary,_bloquear);
}

void desbloquear_mutex_colas(){
	void _desbloquear(char* _,void* element){
		pthread_mutex_t* mutex = (pthread_mutex_t*)element;
		pthread_mutex_unlock(mutex);
	};
	dictionary_iterator(estados_mutexes_dictionary,_desbloquear);
}


bool pcb_es_id (void* elem, uint32_t pid){
	t_pcb* pcb = (t_pcb*)elem;
	return pcb->PID == pid;
}


t_queue* buscar_cola_de_pcb(uint32_t pid){

	bool _es_id (void* elem){
		return pcb_es_id(elem,pid);
	};

	bool _es_del_pcb(void* elem){
		t_queue* cola = (t_queue*)elem;
		return (!list_is_empty(cola->elements)) && list_any_satisfy(cola->elements,_es_id);
	};

	t_list* estados_inicializados = get_estados_inicializados();
	t_queue* cola = list_find(estados_inicializados,_es_del_pcb);
	list_destroy(estados_inicializados);
	return cola;
}

t_pcb* buscar_pcb_en_cola(t_queue* cola,uint32_t pid){

	bool _es_id (void* elem){
		return pcb_es_id(elem,pid);
	}
	return list_find(cola->elements,_es_id);
}

bool esta_en_exec(uint32_t pid){
	return pcb_exec!=NULL && pcb_exec->PID==pid;
}

t_pcb_query* buscar_pcb_sin_bloqueo(uint32_t pid){
	t_queue* cola = NULL;
	if(!esta_en_exec(pid))
		cola = buscar_cola_de_pcb(pid);

	t_pcb_query* pcb_query = malloc(sizeof(t_pcb_query));
	pcb_query->estado = cola;
	if(cola)
		pcb_query->pcb = buscar_pcb_en_cola(cola, pid);
	else
		pcb_query->pcb = esta_en_exec(pid)?pcb_exec:NULL;
		
	// if(pcb_query->pcb)
	// 	loguear_warning("PID ENCONTRADO: %d", pcb_query->pcb->PID);
	// else
	// 	loguear_warning("PID NO ENCONTRADO: %d",pid);
	//pcb_query->pcb = cola!=NULL? buscar_pcb_en_cola(cola,pid):pcb_exec;	
	return pcb_query;
}

t_pcb_query* buscar_pcb(uint32_t pid){
	t_pcb_query* pcb_query;
	bloquear_mutex_colas();
	pcb_query = buscar_pcb_sin_bloqueo(pid);
	desbloquear_mutex_colas();
	return pcb_query;
}

bool estado_inicializado(void* elem){
	t_queue* cola = (t_queue*)elem;
	return cola!= NULL;
}

/// @brief 
	//Retorna una lista de t_queue* con las colas de estados
/// @return 
t_list* get_estados(){return dictionary_elements(estados_dictionary);}

t_list* get_estados_inicializados(){
	t_list* estados = get_estados();
	t_list* estados_filtrados = list_filter(estados,&estado_inicializado);
	list_destroy(estados);
	return estados_filtrados;
}


bool iniciar_planificadores(){
	pthread_t thread_plp_new;
	pthread_t thread_plp_exit;
	pthread_t thread_pcp;

	pthread_create(&thread_plp_new,NULL, (void*)plp_procesos_nuevos,NULL);
	pthread_create(&thread_plp_exit,NULL, (void*)plp_procesos_finalizados,NULL);
	pthread_create(&thread_pcp,NULL,(void*)planificador_corto,NULL);
	
	pthread_detach(thread_plp_new);
	if (thread_plp_new == -1){
		loguear_error("No se pudo iniciar el planificador de largo plazo de procesos nuevos.");
		return false;
	}
	pthread_detach(thread_plp_exit);
	if (thread_plp_exit == -1){
		loguear_error("No se pudo iniciar el planificador de largo plazo.");
		return false;
	}
	pthread_detach(thread_pcp);
	if (thread_pcp == -1){
		loguear_error("No se pudo iniciar el planificador de corto plazo de procesos finalizados.");
		return false;
	}
	return true;
}

t_queue* get_cola_pcb(t_pcb* pcb){
	return NULL;
}

//Este método se llama cuando se inicia un proceso
void plp_procesos_nuevos(){
	while(1){		
			sem_wait(&sem_bin_new); //Bloquea plp hasta que aparezca un proceso
			sem_wait(&sem_bin_plp_procesos_nuevos_iniciado); 
			sem_wait(&sem_cont_grado_mp); //Se bloquea en caso de que el gradodemultiprogramación esté lleno
			
			bool proceso_new_a_ready = cambio_de_estado(estado_new, estado_ready,&mx_new,&mx_ready);
			if(proceso_new_a_ready){
				
				sem_post(&sem_bin_ready);
				//loguear_warning("plp_procesos_nuevos - GMPDMás_%d",grado_multiprog_de_mas);
				loguear("El proceso ingresó correctamente a la lista de ready");
				
			}
			else {
				loguear_warning("No hay procesos en New para plp_procesos_nuevos");
				//loguear_warning("plp_procesos_nuevos - GMPDMás_%d",grado_multiprog_de_mas);
				//sem_post(&sem_bin_new);
			}	
			sem_post(&sem_bin_plp_procesos_nuevos_iniciado); 
	}
}


void plp_procesos_finalizados(){
	while(1){
					
			sem_wait(&sem_bin_exit);
			
			sem_wait(&sem_bin_plp_procesos_finalizados_iniciado); 
			
			t_pcb* pcb = pop_estado_get_pcb(estado_temp,&mx_temp);
			
			eliminar_proceso_en_memoria(pcb);
			
			push_proceso_a_estado(pcb,estado_exit,&mx_exit);
			
			pthread_mutex_lock(&mx_grado_mult_de_mas);
			if(grado_multiprog_de_mas>0)
			{		
				grado_multiprog_de_mas--;
				
			}
			else
				{	
					
					if(grado_multiprog_de_mas<0)
					{	grado_multiprog_de_mas++;
						
					}
					else{ 
						sem_post(&sem_cont_grado_mp);
					}
				}
			
			pthread_mutex_unlock(&mx_grado_mult_de_mas);
			
			sem_post(&sem_bin_plp_procesos_finalizados_iniciado); 
		
	}
}


//Muestra por pantalla el valor actual del semáforo
//Ejemplo: //loguear_semaforo("sem_bin_planificador_corto_iniciado: %d\n",&sem_bin_planificador_corto_iniciado);	
void loguear_semaforo(char* texto,sem_t* semaforo){
	int sval;
	sem_getvalue(semaforo,&sval);
	printf(texto,sval);

}
int get_sem_grado_value(){
	int sval;
	sem_getvalue(&sem_cont_grado_mp,&sval);
	return sval;
}

void planificador_corto(){
	while(1){					
			sem_wait(&sem_bin_ready); //Hay que ver si tiene que estar acá. En este caso se considera que cada replanificación pasa por aca
			sem_wait(&sem_bin_planificador_corto_iniciado); 
			ejecutar_planificacion();
			// Esperar la vuelta del PCB
			//sem_post(&sem_bin_recibir_pcb);
			recibir_pcb_de_cpu();			
			/* Cuando recibe un pcb con centexto finalizado, lo agrega a la cola de exit  */
			sem_post(&sem_bin_planificador_corto_iniciado); 
		
	}
}
void liberar_pcb_exec(){
	pthread_mutex_lock(&mx_pcb_exec);
	pcb_exec = NULL;
	pthread_mutex_unlock(&mx_pcb_exec);
}

t_pcb_query * recibir_pcb_y_actualizar(t_paquete* paquete){

	t_pcb* pcb_recibido = recibir_pcb(paquete);  
	t_pcb_query* pcb_query = buscar_pcb(pcb_recibido->PID);
	reemplazar_pcb_con(pcb_query->pcb,pcb_recibido);

	pcb_destroy(pcb_recibido);
	return pcb_query;
}



void modificar_quantum_restante(t_pcb* pcb){
	tiempo_final = time(NULL);
	if ( (difftime(tiempo_final,tiempo_inicial)*1000) < config->QUANTUM ){
		pcb->quantum = pcb->quantum - ((int)difftime(tiempo_final,tiempo_inicial)*1000);
	}
}




void io_gen_sleep(int pid,char** splitter){
	char pid_mas_unidades [20];
	sprintf(pid_mas_unidades,"%u",pid);
	strcat(pid_mas_unidades," ");
	strcat(pid_mas_unidades, splitter[1]);

	loguear("IO_GEN_SLEEP -> Interfaz:%s Unidades:%s", splitter[0], splitter[1]);
	void *ptr_conexion = dictionary_get(diccionario_nombre_conexion, splitter[0]);
	int conexion_io = *(int *)ptr_conexion;

	enviar_texto(pid_mas_unidades,
				IO_GEN_SLEEP,
				conexion_io);
	loguear("Peticion a IO enviada");
	string_array_destroy(splitter);
}

void io_std(int pid,t_paquete* paquete_IO, char* nombre_interfaz){
	void *ptr_conexion = dictionary_get(diccionario_nombre_conexion, nombre_interfaz);
	int conexion_io = *(int *)ptr_conexion;
	enviar_paquete(paquete_IO,conexion_io);
	loguear("Peticion a IO enviada");
	paquete_destroy(paquete_IO);
}


void asignar_recurso(t_pcb* pcb,t_recurso* recurso){
	bool pcb_del_recurso(void* elem){

	t_proceso_instancia* pid_instacia = (t_proceso_instancia*) elem;
	return pid_instacia->PID == pcb->PID;

	};
	t_proceso_instancia* pid_instancia = (t_proceso_instancia*)list_find(recurso->lista_procesos_asignados,&pcb_del_recurso);
	if(pid_instancia)
		pid_instancia->instancias++;
	else{
		pid_instancia = malloc(sizeof(t_proceso_instancia));
		pid_instancia ->instancias=1;
		pid_instancia->PID = pcb->PID;
		list_add(recurso->lista_procesos_asignados,pid_instancia);
	}
		
	loguear("INSTANCIA ASIGNADA A <%s> - PID:<%d> - INSTANCIAS_POSEIDAS: <%d>",recurso->recurso,pcb->PID,pid_instancia->instancias);
	
}

void liberar_instancia(t_pcb* pcb_recibido,t_recurso* recurso){

	bool pcb_del_recurso(void* elem){

	t_proceso_instancia* pid_instacia = (t_proceso_instancia*) elem;
	return pid_instacia->PID == pcb_recibido->PID;

	};

	t_proceso_instancia* pid_instancia = (t_proceso_instancia*)list_find(recurso->lista_procesos_asignados,&pcb_del_recurso);
	if(pid_instancia!=NULL && pid_instancia->instancias > 0){
		pid_instancia->instancias--;
		recurso->instancias++;
		
		return;

	}
	loguear_error("NO HAY INSTANCIAS ASIGNADAS EN PID:<%d> PARA <%s>",pcb_recibido->PID,recurso->recurso);
}

void liberar_recurso(t_pcb* pcb_recibido,t_recurso* recurso){

	bool pcb_del_recurso(void* elem){

	t_proceso_instancia* pid_instacia = (t_proceso_instancia*) elem;
	return pid_instacia->PID == pcb_recibido->PID;

	};
	t_proceso_instancia* pid_instancia = (t_proceso_instancia*)list_find(recurso->lista_procesos_asignados,&pcb_del_recurso);
	if(pid_instancia!=NULL){
		recurso->instancias = recurso->instancias + pid_instancia->instancias;
		list_remove_element(recurso->lista_procesos_asignados,pid_instancia);
		loguear("SE LIBERA PID: <%d> - INSTANCIAS: <%d> - RECURSO: <%s>",
		pcb_recibido->PID,
		pid_instancia->instancias,
		recurso->recurso
		);
		free(pid_instancia);
	}

}


void rec_handler_exec(t_pcb* pcb_recibido){
	//int instancias_disponibles;
	int cod_op_rec = recibir_operacion(cpu_dispatch);		
	char* peticion = recibir_mensaje(cpu_dispatch);
	t_recurso *recurso = obtener_recurso(peticion);
	free(peticion);
	if(recurso == NULL){
		pasar_a_exit(pcb_recibido);
		loguear("PID: <%d> - Estado Anterior: <EXEC> - Estado Actual: <EXIT>",pcb_recibido->PID); // LOG MINIMO Y OBLIGATORIO
		loguear("Finaliza el proceso <%d> - Motivo: <INVALID_RESOURCE>",pcb_recibido->PID); // LOG MINIMO Y OBLIGATORIO
		return;
	}
	switch(cod_op_rec){
		case WAIT:
		recurso->instancias--;
		if(recurso->instancias < 0){
			// queue_push(recurso->cola_procesos_esperando, pcb_recibido);
			proceso_a_estado(pcb_recibido,recurso->cola_procesos_esperando,dictionary_get(estados_mutexes_dictionary,recurso->recurso));
			loguear("PID: <%d> - Estado Anterior: <EXEC> - Estado Actual: <BLOCKED>",pcb_recibido->PID);// LOG MINIMO Y OBLIGATORIO
			loguear("PID: <%d> - Bloqueado por: <%s>", pcb_recibido->PID,recurso->recurso); // LOG MINIMO Y OBLIGATORIO
			//Bloquear proceso
		}
		else{
			//Devolver el proceso a ejecutar
			asignar_recurso(pcb_recibido,recurso);
			a_ready(pcb_recibido);
		}
		break;

		case SIGNAL:
		// recurso->instancias++;
		
		liberar_instancia(pcb_recibido,recurso);
		// pthread_mutex_t* mutex_cola = dictionary_get(estados_mutexes_dictionary,recurso->recurso);
		// pthread_mutex_lock(mutex_cola);
		// bloquear_mutex_colas();
		if(!queue_is_empty(recurso->cola_procesos_esperando)){
			
			
			t_pcb* pcb_liberado=queue_pop(recurso->cola_procesos_esperando);
			asignar_recurso(pcb_liberado,recurso);
			// a_ready(pcb_liberado);

			a_ready_sin_mutex(pcb_liberado);
			
		}
		// desbloquear_mutex_colas();
		loguear("SIGNAL DE PID:<%d> - RECURSO: <%s> - INSTANCIAS_DISPONIBLES: <%d>",pcb_recibido->PID,
		recurso->recurso,
		recurso->instancias);
		a_ready(pcb_recibido);

		break;
	}


}

bool admite_operacion(op_code cod, char* interfaz){
	char* clave = string_itoa(cod);
	// loguear("DICCIONARIO: %s", (char*)dictionary_get(tipos_de_interfaces, clave));
	// loguear("INTERFAZ: %s", interfaz);
	char* key_d = (char*)dictionary_get(tipos_de_interfaces, clave);
	// loguear("VALORRRR: %d", strcmp(key_d, clave));
	free(clave);

	return strncmp(key_d, interfaz,4 ) == 0;
}

void limpiar_buffer(int cod_op_io){
	switch(cod_op_io){
		case IO_GEN_SLEEP:
			recibir_operacion(cpu_dispatch); 
			char* mensaje = recibir_mensaje(cpu_dispatch);
			free(mensaje);
			break;
		case IO_STDIN_READ:
			t_paquete* paquete_ior = recibir_paquete(cpu_dispatch);
			paquete_destroy(paquete_ior);
			break;
		case IO_STDOUT_WRITE:
			t_paquete* paquete_iow = recibir_paquete(cpu_dispatch);
			paquete_destroy(paquete_iow);
			break;
		case FILE_SYSTEM:
			t_paquete* paquete_fs = recibir_paquete(cpu_dispatch);
			paquete_destroy(paquete_fs);
			break;
	}
}

void io_handler_exec(t_pcb* pcb_recibido){
	int cod_op_io = recibir_operacion(cpu_dispatch);		
	char* nombre_interfaz = recibir_mensaje(cpu_dispatch);
	//t_paquete* paquete_IO = recibir_paquete(cpu_dispatch);
	//char* tipo_interfaz = string_new();
	loguear("NOMBRE DE LA INTERFAZ: %s", nombre_interfaz);
	if(!existe_interfaz(nombre_interfaz)){
		free(nombre_interfaz);
		loguear_warning("NOMBRE INCORRECTO.");
		loguear("PID: <%d> - Estado Anterior: <EXEC> - Estado Actual: <EXIT>", pcb_recibido->PID); // LOG MINIMO Y OBLIGATORIO	
		loguear("Finaliza el proceso <%d> - Motivo: <INVALID_INTERFACE>",pcb_recibido->PID); // LOG MINIMO Y OBLIGATORIO		
		limpiar_buffer(cod_op_io);
		pasar_a_exit(pcb_recibido);		
		//paquete_destroy(paquete_IO);
		return;
	}


	
	t_blocked_interfaz* interfaz = dictionary_get(diccionario_nombre_qblocked,nombre_interfaz);
	//char *tipo_interfaz = interfaz->tipo_de_interfaz;
	proceso_a_estado(pcb_recibido, interfaz->estado_blocked, interfaz->mx_blocked);
	loguear("PID: <%d> - Estado Anterior: <EXEC> - Estado Actual: <BLOCKED>", pcb_recibido->PID); // LOG MINIMO Y OBLIGATORIO
	loguear("PID: <%d> - Bloqueado por: <%s>", pcb_recibido->PID,nombre_interfaz); // LOG MINIMO Y OBLIGATORIO
	if(!admite_operacion(cod_op_io, interfaz->tipo_de_interfaz)){ 
		loguear_warning("NO SE ADMITE OPERACION.");
		loguear("Finaliza el proceso <%d> - Motivo: <INVALID_INTERFACE>",pcb_recibido->PID); // LOG MINIMO Y OBLIGATORIO		
		limpiar_buffer(cod_op_io);
		pasar_a_exit(pcb_recibido);
		free(nombre_interfaz);
		return; 
	}
	switch(cod_op_io){
		case IO_GEN_SLEEP:
			recibir_operacion(cpu_dispatch);
			char* peticion = recibir_mensaje(cpu_dispatch);
			char** splitter = string_split(peticion," ");
			free(peticion);
			// loguear_warning("ENTRO AL SWITCH, PID: %s",splitter[0]);
			io_gen_sleep(pcb_recibido->PID,splitter);
			break;
		case IO_STDIN_READ:
			t_paquete* paquete_ior = recibir_paquete(cpu_dispatch);
			io_std(pcb_recibido->PID, paquete_ior,nombre_interfaz);
			break;
		case IO_STDOUT_WRITE:
			t_paquete* paquete_iow = recibir_paquete(cpu_dispatch);
			io_std(pcb_recibido->PID, paquete_iow,nombre_interfaz);
			break;
		case FILE_SYSTEM:
			//recibir_operacion(cpu_dispatch);
			t_paquete* paquete_fs = recibir_paquete(cpu_dispatch);
			//t_operacion_fs* operacion_fs = recibir_op_fs(cpu_dispatch);
			io_fs(pcb_recibido->PID,paquete_fs,nombre_interfaz);
			break;
		default:
			limpiar_buffer(cod_op_io);
			pasar_a_exit(pcb_recibido);			
			break;
	}
	free(nombre_interfaz);
}



void finalizar_proceso_consola_exec(t_pcb* pcb_recibido){
	pasar_a_exit(pcb_recibido);	
	sem_post(&sem_bin_controlar_quantum);
}

void fin_de_quantum_exec(t_pcb* pcb_recibido,t_queue* estado){
	if(estado)
		return;

	if(exec_recibido)
	{	pasar_a_exit(pcb_recibido);	
		loguear("PID: <%d> - Estado Anterior: <EXEC> - Estado Actual: <EXIT>", pcb_recibido->PID); // LOG MINIMO Y OBLIGATORIO			
		loguear("Finaliza el proceso <%d> - Motivo: <SUCCESS>",pcb_recibido->PID); // LOG MINIMO Y OBLIGATORIO
		sem_post(&sem_bin_controlar_quantum);
		return;
	}

	if (es_vrr())
		pcb_recibido->quantum = config->QUANTUM;
	
	proceso_a_estado(pcb_recibido, estado_ready,&mx_ready); 
	loguear("PID: <%d> - Estado Anterior: <EXEC> - Estado Actual: <READY>", pcb_recibido->PID); // LOG MINIMO Y OBLIGATORIO	
	loguear("Cola Ready / Ready Prioridad");
	loguear_pids(estado_ready,&mx_ready);
	if(es_vrr()){
		loguear("Cola Ready Plus / Ready Plus Prioridad");
		loguear_pids(estado_ready_plus,&mx_ready_plus);		
	}
	sem_post(&sem_bin_ready);
	sem_post(&sem_bin_controlar_quantum);
}

void gestionar_operacion_de_cpu(op_code cod_op,t_pcb* pcb_recibido,t_queue* estado){
	switch (cod_op)
		{
			case FINALIZAR_PROCESO_POR_CONSOLA:
				finalizar_proceso_consola_exec(pcb_recibido);
				loguear("PID: <%d> - Estado Anterior: <EXEC> - Estado Actual: <EXIT>", pcb_recibido->PID); // LOG MINIMO Y OBLIGATORIO
				loguear("Finaliza el proceso <%d> - Motivo: <INTERRUPTED_BY_USER>",pcb_recibido->PID); // LOG MINIMO Y OBLIGATORIO
				break;
			case CPU_EXIT:
				loguear("PID: <%d> - Estado Anterior: <EXEC> - Estado Actual: <EXIT>", pcb_recibido->PID); // LOG MINIMO Y OBLIGATORIO
				loguear("Finaliza el proceso <%d> - Motivo: <SUCCESS>",pcb_recibido->PID); // LOG MINIMO Y OBLIGATORIO
				pasar_a_exit(pcb_recibido);			 
				break;
			case FIN_QUANTUM:
				fin_de_quantum_exec(pcb_recibido,estado);
				// LOGS ADENTRO DE FUNCION
				break;
			case IO_HANDLER:
				io_handler_exec(pcb_recibido);
				// LOGS ADENTRO DE FUNCION
				break;
			case REC_HANDLER:
				rec_handler_exec(pcb_recibido);
				// LOGS ADENTRO DE FUNCION
				break;
			case OUT_OF_MEMORY:
				loguear("PID: <%d> - Estado Anterior: <EXEC> - Estado Actual: <EXIT>", pcb_recibido->PID); // LOG MINIMO Y OBLIGATORIO
				loguear_error("Finaliza el proceso <%d> - Motivo: <OUT_OF_MEMORY> ", pcb_recibido->PID); // LOG MINIMO Y OBLIGATORIO
				pasar_a_exit(pcb_recibido);
				break;
			default:
				break;
		}

}

bool fue_finalizado(uint32_t pid){
	return (encontrar_en_lista(pid,estado_temp, &mx_temp)!=NULL) 
		|| (encontrar_en_lista(pid,estado_exit, &mx_exit)!=NULL);
}

void recibir_pcb_de_cpu(){
	// loguear_warning("Intento recibir de CPU!");
	t_paquete *paquete = recibir_paquete(cpu_dispatch);
	int cod_op = paquete->codigo_operacion;
	// loguear("Cod op CPU: %d", cod_op);
	t_pcb_query* pcb_query = recibir_pcb_y_actualizar(paquete);
	t_pcb* pcb_recibido = pcb_query->pcb;
	paquete_destroy(paquete);
	// if(cod_op == FIN_QUANTUM && pcb_recibido->PID != pcb_exec->PID){
	// 	loguear_warning("ENTRA POR ACÁ, PID recibido: %d.", pcb_recibido->PID  );
	// 	loguear_warning("ENTRA POR ACÁ, PID exec: %d.", pcb_exec->PID  );
	// 	eliminar_proceso_en_FIN_QUANTUM = true;
	// 	sem_post(&sem_bin_cpu_libre);
	// 	//sem_post(&sem_bin_recibir_pcb);
	// 	free(pcb_query);
	// 	exec_recibido = false;
	// 	return;
	// }
	
	if(es_vrr()) modificar_quantum_restante(pcb_recibido);
	
	
	// PAUSAR POR DETENER PLANI	
	sem_wait(&sem_bin_recibir_pcb);
	liberar_pcb_exec();
	// loguear_warning("Frenado en recibir_pcb_de_cpu.");
	
	if(!fue_finalizado(pcb_recibido->PID))	
		gestionar_operacion_de_cpu(cod_op,pcb_recibido,pcb_query->estado);

	sem_post(&sem_bin_cpu_libre);
	sem_post(&sem_bin_recibir_pcb);
	free(pcb_query);
	exec_recibido = false;
	
}


// void config_kernel_destroy(t_config_kernel* config){

// 	string_array_destroy(config->RECURSOS);
// 	string_array_destroy(config->INSTANCIAS_RECURSOS);
// 	config_destroy(config->config);
// 	free(config);
// }

void loguear_config(){

	loguear("IP_MEMORIA: %s",config->IP_MEMORIA);
	loguear("PUERTO_MEMORIA: %d",config->PUERTO_MEMORIA);
	loguear("QUANTUM: %d",config->QUANTUM);
	loguear("IP_CPU: %s",config->IP_CPU);
	loguear("PUERTO DISPATCH: %d",config->PUERTO_CPU_DISPATCH);
	loguear("PUERTO INTERRUPT: %d",config->PUERTO_CPU_DISPATCH);
	loguear("CANT. RECURSOS: %d", string_array_size(config->RECURSOS));
}

void loguear_pids(t_queue* cola,pthread_mutex_t* mx_estado){

	pthread_mutex_lock(mx_estado);
	char lista_pids[1000]= "[";
	char* pid_string;
	void _agregar_a_lista(void* elem){
		t_pcb* pcb = (t_pcb*)elem;
		pid_string = string_itoa(pcb->PID);
		strcat(lista_pids, pid_string);
		strcat(lista_pids, ",");
		free(pid_string);
	}
	list_iterate(cola->elements, _agregar_a_lista);
	size_t len = strlen(lista_pids);
	if(len > 1) lista_pids[len - 1] = '\0';
	strcat(lista_pids, "]");
	loguear("%s",lista_pids);
	//free(pid_string);
	pthread_mutex_unlock(mx_estado);
}
// void imprimir_cola(t_queue *cola, const char *estado) {

// 	void _imprimir_estado(void* elem){
// 		t_pcb* pcb = (t_pcb*)elem;
// 		 printf("| %-10d | %-20s | %-15s |\n", pcb->PID,pcb->path ,estado);
// 	}
//     printf("Estado: %s\n", estado);
//     printf("| %-10s | %-20s | %-15s |\n", "PID", "Nombre", "Estado");
//     printf("|------------|----------------------|-----------------|\n");

// 	list_iterate(cola->elements,_imprimir_estado);

//     printf("|------------|----------------------|-----------------|\n");
// }

char* leer_texto_consola(){
	
	char* input = readline(">");
	 if (input != NULL && *input != '\0')
        add_history(input); // Agrega la entrada al historial de readline
    
    return input;

}

t_comando_consola* comando_consola_create(op_code_kernel code,char* nombre,char* params,bool(*funcion)(char**)){
    t_comando_consola* comando = malloc(sizeof(t_comando_consola));
    comando->comando = code;
    comando->parametros =  string_duplicate(params);
    comando->funcion = funcion;
    comando->nombre= string_duplicate(nombre);

    return comando;
}


void agregar_comando(op_code_kernel code,char* nombre,char* params,bool(*funcion)(char**)){
    void _agregar_comando_(char* texto){
		 t_comando_consola* comando = comando_consola_create(code,nombre,params,funcion);
        dictionary_put(comandos_consola,texto,comando);				
    };
	 char* code_str = string_itoa(code);
    _agregar_comando_(code_str);
	free(code_str);
    _agregar_comando_(nombre);
}


void imprimir_valores_leidos(char** substrings){
	
	void imprimir_valor(char* leido){
		//loguear("substring[%d] vale:%s\n",index++,leido);
	};
	string_iterate_lines(substrings,imprimir_valor);
}


bool parametros_ejecutar_script_validos(char** parametros){
	bool validado = string_array_size(parametros)==2;
	
	if(!validado)
		printf("\tEJECUTAR_SCRIPT debe recibir 1 parámetro:\n\tPath (string)\n");

	return validado;
}



t_list* get_instrucciones_kernel(char* archivo){
	return get_instrucciones(config->PATH_SCRIPTS,archivo);
}

void ejecutar_sript(void* script){
	
	printf("Ejecutando script...%s\n",(char*)script);
	ejecutar_comando_consola((char*)script);
}

bool ejecutar_scripts_de_archivo(char** parametros){
	
	imprimir_valores_leidos(parametros);

	if(!parametros_ejecutar_script_validos(parametros))
		return false;

	char *path = string_duplicate(parametros[1]);

	t_list* instrucciones_sript = get_instrucciones_kernel(path);
	free(path);

	if(instrucciones_sript!=NULL)
	{	list_iterate(instrucciones_sript,ejecutar_sript);
		list_destroy_and_destroy_elements(instrucciones_sript,free);
	}
	
	return true;

}

bool existe_comando(char* comando){
   return (dictionary_has_key(comandos_consola,comando));
}

int ejecutar_comando_consola(char*params){

	char** parametros = string_split(params," ");  
	char* comando = parametros[0]; 
	string_to_upper(comando);
	t_comando_consola* comando_consola = NULL;
	int numero_comando = -1;
	if(existe_comando(comando)){
		comando_consola = dictionary_get(comandos_consola,comando);
		numero_comando = comando_consola->comando;
		if(comando_consola->comando != EXIT){
       		comando_consola->funcion(parametros);
		}
	}
	
	string_array_destroy(parametros);
	return numero_comando;
}

bool crear_proceso_en_memoria(t_pcb* pcb){
	enviar_pcb(pcb,CREACION_PROCESO,conexion_memoria); // Enviar proceso a memoria para que inicialice 
	op_code operacion = recibir_operacion(conexion_memoria);
	switch (operacion)
	{
	case CREACION_PROCESO:
		char* mensaje = recibir_mensaje(conexion_memoria);
		//("OK: %s",mensaje);
		free(mensaje);
		break;
	case CREACION_PROCESO_FALLIDO:
		char* mensaje_falla = recibir_mensaje(conexion_memoria);
		//loguear_error("No se pudo crear el proceso: %s",mensaje_falla);
		free(mensaje_falla);
		return false;
		break;
	default:
		break;
	}
	
	return true;
}

// Se pueden unificar estas dos funciones en un solo Switch??????

bool eliminar_proceso_en_memoria(t_pcb* pcb){
	//loguear_warning("eliminar_proceso_en_memoria");
	enviar_pcb(pcb,ELIMINACION_PROCESO,conexion_memoria); // Enviar proceso a memoria para que inicialice 
	
	op_code operacion = recibir_operacion(conexion_memoria);
	switch (operacion)
	{
	case ELIMINACION_PROCESO:
		char* mensaje = recibir_mensaje(conexion_memoria);
		//loguear("OK: %s",mensaje);
		free(mensaje);
		break;
	case ELIMINACION_PROCESO_FALLIDO:
		char* mensaje_falla = recibir_mensaje(conexion_memoria);
		loguear_error("No se pudo eliminar el proceso: %s",mensaje_falla);
		free(mensaje_falla);
		return false;
		break;
	default:
		break;
	}

	return true;
}

bool maneja_quantum(){return (es_rr()||es_vrr());}

bool iniciar_proceso(char** parametros){
	
	bool parametros_iniciar_proceso_validos(char** parametros){
		bool validado = 
		string_array_size(parametros)==2;
		
		if(!validado)
			printf("\tINICIAR_PROCESO debe recibir 1 parámetro:\n\tPath (string)\n");

		return validado;
	};

	//loguear("iniciando proceso...");
	imprimir_valores_leidos(parametros);	

	bool parametros_validos = parametros_iniciar_proceso_validos(parametros);

	if(parametros_validos){
	
		char *path = string_duplicate(parametros[1]);
		//loguear("PATH: %s",path);
		t_pcb* pcb = pcb_create(path);   // Se crea el PCB
		if(maneja_quantum())
			pcb->quantum = config->QUANTUM;
		
		bool proceso_creado = crear_proceso_en_memoria(pcb);
		if(proceso_creado){
			push_proceso_a_estado(pcb,estado_new,&mx_new); //Pasa el PCB a New
			sem_post(&sem_bin_new);
			loguear("Se crea el proceso <%d> en NEW", pcb->PID);// LOG MINIMO Y OBLIGATORIO
		}
		else pcb_destroy(pcb);

		free(path);
	}
			
	return parametros_validos;
}


void proceso_a_estado(t_pcb* pcb, t_queue* estado,pthread_mutex_t* mx_estado){

	pthread_mutex_lock(mx_estado);
	queue_push(estado,pcb);
	pthread_mutex_unlock(mx_estado);
}

t_validacion validacion_parametros_finalizar(char** parametros){
	t_validacion validacion;
	validacion.resultado = string_array_size(parametros)==2;
	validacion.descripcion = NULL;
	if(!validacion.resultado)
		validacion.descripcion = "\tINICIAR_PROCESO debe recibir 1 parámetro:\n\tPath (string)\n";

	return validacion;
}

bool finalizar_proceso(char** substrings){	
	
		t_validacion validacion = validacion_parametros_finalizar(substrings);
		if(validacion.resultado){
			imprimir_valores_leidos(substrings);

			uint32_t pid = atoi(substrings[1]);
			
		//	bool eliminado = eliminar_proceso(pid);
			crear_hilo_eliminar_proceso(pid);
			//if(eliminado)
			
		}
		else printf(validacion.descripcion,"");

		return validacion.resultado;
}



void listar_comandos(){
	printf("Los comandos disponibles son:\n");
	char* decoracion= "\t\t*****************************************************\n";
	printf("%s",decoracion);
	int cantidad = dictionary_size(comandos_consola)/2;


	for (int i = 0; i < cantidad; i++) {
		char* id = string_itoa(i);
		t_comando_consola* comando = dictionary_get(comandos_consola,id);
        printf("\t%d. %s %s\n",i ,comando->nombre ,comando->parametros);
		free(id);
    }
	printf("%s",decoracion);
}

bool iniciar_planificacion(char** substrings){
	if(planificacion_detenida){
		sem_post(&sem_bin_plp_procesos_nuevos_iniciado);
		sem_post(&sem_bin_plp_procesos_finalizados_iniciado);
		sem_post(&sem_bin_planificador_corto_iniciado);
		sem_post(&sem_bin_recibir_pcb);

		planificacion_detenida = false;
	}
	return true;
}

void detener_plani_nuevos(){
	sem_wait(&sem_bin_plp_procesos_nuevos_iniciado);
}
void detener_plani_finalizados(){
	sem_wait(&sem_bin_plp_procesos_finalizados_iniciado);
}
void detener_plani_corto(){
	sem_wait(&sem_bin_planificador_corto_iniciado);
}
void detener_recibir_pcb(){
	sem_wait(&sem_bin_recibir_pcb);
}

void hilo_detencion(void* detenedor){
	pthread_t thread_detener_plani;
	pthread_create(&thread_detener_plani,NULL, detenedor,NULL);
	pthread_detach(thread_detener_plani);
}

void* _detener(){
		
		hilo_detencion(&detener_plani_nuevos);
		hilo_detencion(&detener_plani_finalizados);
		hilo_detencion(&detener_plani_corto);
		hilo_detencion(&detener_recibir_pcb);	

		return NULL;
	};
bool detener_planificacion(char** substrings){

	if(!planificacion_detenida){
		planificacion_detenida = true;
		pthread_t thread_detener_plani;
		pthread_create(&thread_detener_plani,NULL, &_detener,NULL);
		pthread_detach(thread_detener_plani);
	}
	return true;
}
void* hilo_multiprogramacion_ajuste(void* diferencia_ptr){
	int diferencia = *(int*)diferencia_ptr;
	pthread_mutex_lock(&mx_grado_mult_de_mas);
	grado_multiprog_de_mas = grado_multiprog_de_mas - diferencia;
	free(diferencia_ptr);
	
	
	if (grado_multiprog_de_mas < 0){
		
		
		for( ;grado_multiprog_de_mas < 0 ; grado_multiprog_de_mas++){	
			
			sem_post(&sem_cont_grado_mp);
			
		}
		
		pthread_mutex_unlock(&mx_grado_mult_de_mas);
	}
	else {		
		
		for( ;grado_multiprog_de_mas >0; grado_multiprog_de_mas--){
		
			pthread_mutex_unlock(&mx_grado_mult_de_mas);
			sem_wait(&sem_cont_grado_mp);
			pthread_mutex_lock(&mx_grado_mult_de_mas);
			
		}		
		for( ;grado_multiprog_de_mas < 0 ; grado_multiprog_de_mas++)
		{	
			sem_post(&sem_cont_grado_mp);
			
		}
		
		pthread_mutex_unlock(&mx_grado_mult_de_mas);
	}
	return NULL;
}

bool multiprogramacion(char** substrings){
	loguear("El grado de multiprogramación anterior es: %d",config->GRADO_MULTIPROGRAMACION);
	if(string_array_size(substrings)>0){
		char* valor = substrings[1];
		if(is_numeric(valor)){
			char* endptr;
			int number, number_anterior;
			number_anterior = config->GRADO_MULTIPROGRAMACION;
			number = strtol(valor, &endptr, 10);
			config->GRADO_MULTIPROGRAMACION = number;
			loguear("El nuevo grado de multiprogramación es: %d",config->GRADO_MULTIPROGRAMACION);
			int diferencia = number - number_anterior;
			if(diferencia == 0)
				loguear("El nuevo grado de multiprogramacion es igual al anterior");			
			else
			{	int* valor_prt = malloc(sizeof(int));
					*valor_prt = diferencia;
				pthread_t thread_multiprogramacion;
				pthread_create(&thread_multiprogramacion,NULL , hilo_multiprogramacion_ajuste,(void*)(valor_prt));
				pthread_detach(thread_multiprogramacion);
			}
		}
		else loguear_error("El valor: %s no es un grado de multiprogramación válido.",valor );
	}
	else loguear_error("No se especificó ningún grado de multiprogramación");

	
	return true;
}



void imprimir_cola(t_queue *cola, const char *estado) {

	void _imprimir_estado(void* elem){
		t_pcb* pcb = (t_pcb*)elem;
		 printf("| %-10d | %-20s | %-15s |\n", pcb->PID,pcb->path ,estado);
	}
    printf("Estado: %s\n", estado);
    printf("| %-10s | %-20s | %-15s |\n", "PID", "Nombre", "Estado");
    printf("|------------|----------------------|-----------------|\n");

	list_iterate(cola->elements,_imprimir_estado);

    printf("|------------|----------------------|-----------------|\n");
}

/*
void imprimir_cola_recursos(){
	char* nombre_cola=malloc(30);
	t_recurso* recurso = malloc(sizeof(t_recurso));
	for(int i=0;i<list_size(lista_recursos);i++){
		recurso= list_get(lista_recursos,i);
		sprintf(nombre_cola,"%s - BLOCKED",recurso->recurso);
		imprimir_cola(recurso->cola_procesos_esperando,nombre_cola);
		
	}
	free(nombre_cola);
}
*/

bool es_planificacion(t_alg_planificador algoritmo){
	return config->ALGORITMO_PLANIFICACION.id == algoritmo;
}

bool es_rr(){
	return es_planificacion(RR);
}
bool es_vrr(){
	return es_planificacion(VRR);
}

char* get_nombre_estado(char* clave){
	char* nombre;
	if(dictionary_has_key(nombres_colas_dictionary,clave))
		nombre = string_duplicate(dictionary_get(nombres_colas_dictionary,clave));
	else nombre = string_duplicate(clave);

	return nombre;

}

void imprimir_estado(char* clave,void* element){
	t_queue* estado;	
	char* nombre_estado = get_nombre_estado(clave);
	
	if(element!=NULL && element!=pcb_exec){	
		estado = (t_queue*)element;
		imprimir_cola(estado,nombre_estado);		
	}
	else
	{		
		t_queue* estado_exec = queue_create();
		pthread_mutex_lock(&mx_pcb_exec);
		if(pcb_exec!=NULL)
			queue_push(estado_exec,pcb_exec);
		pthread_mutex_unlock(&mx_pcb_exec);
		imprimir_cola(estado_exec,nombre_estado);
		queue_destroy(estado_exec);
	}
	
	free(nombre_estado);

}

bool proceso_estado(){
	// t_queue* estado_exec = queue_create();
	// pthread_mutex_lock(&mx_pcb_exec);
	// if(pcb_exec!=NULL)
	// 	queue_push(estado_exec,pcb_exec);
	// pthread_mutex_unlock(&mx_pcb_exec);

	dictionary_iterator(estados_dictionary,&imprimir_estado);
	// imprimir_cola(estado_new, "Nuevo");
    // imprimir_cola(estado_ready, "Listo");
	// imprimir_todas las colas de blocked
	//imprimir_cola(estado_exec, "Ejecutando");	
	// imprimir_cola(estado_exit, "Finalizado");
	// imprimir_cola(estado_temp, "Temporal");
	//imprimir_cola_recursos();
	if( es_vrr()) imprimir_cola(estado_ready_plus,"Listo VRR");	
	
	return true;
}


bool finalizar_consola(char** parametros){
	loguear("Consola finalizada.");	
	return false;
}


void iniciar_consola(){
	 using_history(); // Activa la funcionalidad de historial de readline
	char *cadenaLeida;
	int comando = -1;
	 while (comando == -1 || comando != EXIT) {
		listar_comandos();
        cadenaLeida =  leer_texto_consola();
		comando = ejecutar_comando_consola(cadenaLeida);
		free(cadenaLeida);
    }
	 rl_clear_history(); // Limpia el historial de readline después de usar la interfaz de línea de comandos
}

void ejecutar_planificacion(){
	config->ALGORITMO_PLANIFICACION.planificar();
}

void ejecutar_proceso(){

	if(pcb_exec){
		//loguear("Se debe enviar el pcb en exec a la cpu PID: %d",pcb_exec->PID);
		enviar_pcb(pcb_exec,EJECUTAR_PROCESO,cpu_dispatch);
	}else
	 loguear_warning("No hay ningún proceso para enviar a ejecutar.");
	pthread_mutex_unlock(&mx_pcb_exec);
	
}

void pcb_a_exec(t_pcb* pcb){
	if(pcb != NULL){
		pthread_mutex_lock(&mx_pcb_exec);
		pcb_exec = pcb;		
		ejecutar_proceso();
	}
}

t_pcb* ready_a_exec(){	
	sem_wait(&sem_bin_cpu_libre); // Verificamos que no haya nadie en CPU
	t_pcb* pcb = NULL;
	//int size = queue_size(estado_ready);
	//loguear_warning("La cola ready tiene: %d",size);
	pcb = pop_estado_get_pcb(estado_ready,&mx_ready);
	loguear("PID: <%d> - Estado Anterior: <READY> - Estado Actual: <EXEC>", pcb->PID); // LOG MINIMO Y OBLIGATORIO
	pcb_a_exec(pcb);
	return pcb;
}

void planificacion_FIFO(){
	//loguear("Planificando por Fpcb_enviadoIFO");
	ready_a_exec();
}

void controlar_quantum (t_pcb* pcb_enviado){
	t_pcb pcb;
	memcpy(&pcb,pcb_enviado,sizeof(t_pcb));
	if(config->QUANTUM)
	{	eliminar_proceso_en_FIN_QUANTUM=false;
		//eliminar_proceso_en_FIN_QUANTUM2 = false;
		usleep(pcb.quantum *1000);	
		//loguear("PID: <%d> - Esperando sem_wait(&sem_bin_controlar_quantum)",pcb.PID);
		sem_wait(&sem_bin_controlar_quantum);	
		//loguear("PID: <%d> - &sem_bin_controlar_quantum activado",pcb.PID);
		if(!eliminar_proceso_en_FIN_QUANTUM){
			pthread_mutex_lock(&mx_pcb_exec);
			if(pcb_exec != NULL && pcb_exec->PID==pcb.PID){
				enviar_texto("FIN_QUANTUM",FIN_QUANTUM,cpu_interrupt);
				loguear("PID: <%d> - Desalojado por fin de Quantum",pcb.PID);// LOG MINIMO Y OBLIGATORIO
				
			}
			pthread_mutex_unlock(&mx_pcb_exec);
		}
		sem_post(&sem_bin_controlar_quantum);
	}
}

void crear_hilo_quantum(t_pcb* pcb){
	pthread_t thread_quantum;
	pthread_create(&thread_quantum,NULL, (void*)controlar_quantum,pcb);
	
	pthread_detach(thread_quantum);
	if(es_vrr()){
		tiempo_inicial = time(NULL);
	}
	if (thread_quantum == -1)
		loguear_error("No se pudo iniciar el hilo de quantum para el PID: %d",pcb->PID);	
	
}

void crear_hilo_eliminar_proceso(uint32_t pid){
	pthread_t thread_eliminar;

    uint32_t* pid_ptr = malloc(sizeof(uint32_t));
	*pid_ptr = pid;
	pthread_create(&thread_eliminar,NULL, (void*)eliminar_proceso,pid_ptr);
	
	pthread_detach(thread_eliminar);
	if (thread_eliminar == -1)
		loguear_error("No se pudo iniciar el hilo de eliminar proceso: %d",pid);	
	
}





void planificacion_RR(){
	//loguear("Planificando por Round Robbin");
	t_pcb* pcb = ready_a_exec();
	if(pcb)
	crear_hilo_quantum(pcb);
}

// Para VRR, en caso de que el proceso no ejecute todo su quantum se realiza la
// validacion correspondiente en kernel para saber a qué cola mandar el pcb
// una manera de distinguir entre IO y fin de proceso es el codigo de op
// que le manda cpu.

t_pcb*  ready_plus_a_exec(){
	t_pcb* pcb = pop_estado_get_pcb(estado_ready_plus,&mx_ready_plus);
	loguear("PID: <%d> - Estado Anterior: <READY_PLUS> - Estado Actual: <EXEC>",pcb->PID); // LOG MINIMO Y OBLIGATORIO
	pcb_a_exec(pcb);
	return pcb;
}


void planificacion_VRR(){
	//loguear("Planificando por Virtual Round Robbin");	
	t_pcb* pcb;
	if(!queue_is_empty(estado_ready_plus))
		pcb = ready_plus_a_exec();	
	else
		pcb = ready_a_exec();
	if(pcb)
	crear_hilo_quantum(pcb);
}

////// MODIFICACIONES DE ESTADO

bool cambio_de_estado(t_queue* estado_origen, t_queue* estado_destino,pthread_mutex_t* sem_origen,pthread_mutex_t* sem_destino){	
	bool transicion = (!queue_is_empty(estado_origen)) && transicion_valida(estado_origen, estado_destino);
	if(transicion){
		pthread_mutex_lock(sem_origen);

 		t_pcb* pcb = queue_pop(estado_origen);
		push_proceso_a_estado(pcb,estado_destino,sem_destino);
		if (estado_origen == estado_new){
			loguear("PID: <%d> - Estado Anterior: <NEW> - Estado Actual: <READY>",pcb->PID); // LOG MINIMO Y OBLIGATORIO
			loguear("Cola Ready / Ready Prioridad");
			loguear_pids(estado_ready,&mx_ready);
			if(es_vrr()){
				loguear("Cola Ready Plus / Ready Plus Prioridad");
				loguear_pids(estado_ready_plus,&mx_ready_plus);		
			}
		}
		pthread_mutex_unlock(sem_origen);
	}
	return transicion;
}
// Convertir en matriz (Como en sintaxis con Roxy :) 
bool transicion_valida(t_queue* estado_origen,t_queue* estado_destino){
	if (estado_destino==estado_new || estado_origen==estado_exit){
		return false;
	}
	if(estado_destino==estado_ready && estado_origen==estado_exit){
		return false;
	}	
	return true;
}

void push_proceso_a_estado(t_pcb* pcb, t_queue* estado,pthread_mutex_t* mx_estado){
	pthread_mutex_lock(mx_estado);
	queue_push(estado,pcb);
	pthread_mutex_unlock(mx_estado);
}

t_pcb* pop_estado_get_pcb(t_queue* estado,pthread_mutex_t* mx_estado){
	pthread_mutex_lock(mx_estado);
	t_pcb* pcb;
	if(!queue_is_empty(estado))
		pcb = queue_pop(estado);
	pthread_mutex_unlock(mx_estado);
	return pcb;
}

// LIBERAR Y/O DESTRUIR ELEMENTOS Y PROCESOS

void config_destroy_kernel(t_config_kernel * config){
	config_destroy(config->config);
	if(config->INSTANCIAS_RECURSOS)
	string_array_destroy(config->INSTANCIAS_RECURSOS);
	if(config->RECURSOS)
	string_array_destroy(config->RECURSOS);
	free(config);
}

void liberar_colas(){

	void liberar_pcb(void *pcb){
		pcb_destroy((t_pcb*)pcb);
	};

	void liberar_cola(t_queue* cola){
		if(cola!=NULL)
			queue_destroy_and_destroy_elements(cola,liberar_pcb);
	};

	liberar_cola(estado_exit);
	liberar_cola(estado_new);
	liberar_cola(estado_ready);
	if(es_vrr()){
		liberar_cola(estado_ready_plus);
	}
	liberar_cola(estado_temp);
	if(pcb_exec!=NULL)
		pcb_destroy(pcb_exec);
	
}

void liberar_semaforos(){
	sem_destroy(&sem_cont_grado_mp);
	sem_destroy(&sem_bin_new);
	sem_destroy(&sem_bin_ready);
	sem_destroy(&sem_bin_exit);
	sem_destroy(&sem_bin_cpu_libre);
	sem_destroy(&sem_bin_recibir_pcb);

	pthread_mutex_destroy(&mx_new);
	pthread_mutex_destroy(&mx_ready);
	if(es_vrr()) pthread_mutex_destroy(&mx_ready_plus);
	pthread_mutex_destroy(&mx_exit);
	pthread_mutex_destroy(&mx_pcb_exec);
	pthread_mutex_destroy(&mx_temp);
}

void liberar_comando(void* c){
	 t_comando_consola* comando = (t_comando_consola*)c;
	 if(comando){
		if(comando->parametros)
		free(comando->parametros);
		if(comando->nombre)
		free(comando->nombre);
		 free(comando);
		comando=NULL;
	 }
}
void liberar_comandos(){
	
	if(comandos_consola!=NULL) 
		dictionary_destroy_and_destroy_elements(comandos_consola,liberar_comando);
}


void finalizar_kernel(){
	

	conexion_memoria = crear_conexion(config->IP_MEMORIA,config->PUERTO_MEMORIA);
	char* texto = string_new();
	string_append(&texto,"KERNEL");
	string_append(&texto," ");
	string_append(&texto,"KERNEL");

	if(conexion_memoria ==-1){
		loguear_error("No se pudo conectar memoria");
	//	free(texto);
		//return false;
	} 
//	sprintf(texto,"%s %s",config->NOMBRE,devuelve_tipo_en_char(config->TIPO_INTERFAZ.id));
	enviar_texto(texto,NUEVA_IO,conexion_memoria);
	free(texto);

	if (conexion_memoria != -1) liberar_conexion(conexion_memoria);
	if (cpu_dispatch != -1) liberar_conexion(cpu_dispatch);
	if (cpu_interrupt != -1) liberar_conexion(cpu_interrupt);
	if(logger!=NULL) log_destroy(logger);
	liberar_comandos();
	liberar_colas();
	liberar_semaforos();	
	liberar_diccionario_colas();
	liberar_diccionarios_interfaces();
	if(config!=NULL) config_destroy_kernel(config);

	



}





bool iniciar_servidor_kernel(){
    //Iniciamos el servidor con el puerto indicado en la config
	kernel_escucha = iniciar_servidor(config->PUERTO_ESCUCHA);
	if(kernel_escucha == -1){
		loguear_error("El servidor no pudo ser iniciado");
		return false;
	}
	loguear("El Servidor iniciado correctamente");
	return true;
}


void pasar_a_temp_sin_bloqueo(t_pcb_query* pcb_query){
	if(pcb_query->estado)
		list_remove_element(pcb_query->estado->elements,pcb_query->pcb);
	else
		pcb_exec = NULL;
	
	queue_push(estado_temp,pcb_query->pcb);

	
}

bool eliminar_proceso(uint32_t* pid_ptr){
	uint32_t pid = *pid_ptr;
	free(pid_ptr);
	bool eliminado = true;
	//loguear_warning("grado de multiprogramación antes del if: %d\n", get_sem_grado_value());
	bloquear_mutex_colas();

	t_pcb_query* pcb_query = buscar_pcb_sin_bloqueo(pid);	
	if(pcb_query->pcb!=NULL)
		devolver_recursos(pcb_query->pcb,pcb_query->estado);
	if(pcb_query->pcb==NULL){
		free(pcb_query);
		desbloquear_mutex_colas();
		eliminado = false;
		loguear_warning("PCB NO ENCONTRADO");
	}
	else if((pcb_query->estado==estado_temp||pcb_query->estado==estado_exit)){	 //SI YA FUE ELIMINADO	
		desbloquear_mutex_colas();
		free(pcb_query);	
		eliminado = false;		
	}
	else if(pcb_exec!=NULL && pcb_query->estado==NULL){ // SI ESTA EN EXEC
		sem_wait(&sem_bin_controlar_quantum);
		exec_recibido = true;
		enviar_texto("FINALIZAR PROCESO",FINALIZAR_PROCESO_POR_CONSOLA,cpu_interrupt);
		free(pcb_query);
		eliminar_proceso_en_FIN_QUANTUM = true;	
		desbloquear_mutex_colas();
	}
	else if(pcb_query->estado==estado_ready||pcb_query->estado==estado_ready_plus){ // SI ESTA EN READY O READY+

		sem_wait(&sem_bin_ready);	
		
		pasar_a_temp_sin_bloqueo(pcb_query);
		if(pcb_query->estado==estado_ready_plus){
			loguear("PID: <%d> - Estado Anterior: <READY_PLUS> - Estado Actual: <EXIT>", pcb_query->pcb->PID); // LOG MINIMO Y OBLIGATORIO
		}else{
			loguear("PID: <%d> - Estado Anterior: <READY> - Estado Actual: <EXIT>", pcb_query->pcb->PID); // LOG MINIMO Y OBLIGATORIO
		}
		free(pcb_query);
		desbloquear_mutex_colas();
		sem_post(&sem_bin_exit);
		
	}	
	else if(pcb_query->estado==estado_new){ //SI ESTA EN NEW
		//sem_post(&sem_bin_new);
		list_remove_element(estado_new->elements,pcb_query->pcb);
		queue_push(estado_exit,pcb_query->pcb);
		loguear("PID: <%d> - Estado Anterior: <NEW> - Estado Actual: <EXIT>", pcb_query->pcb->PID); // LOG MINIMO Y OBLIGATORIO
		free(pcb_query);
		//loguear_warning("grado de multiprogramación antes del if: %d\n", get_sem_grado_value());
		//eliminar_proceso_en_memoria(pcb_query->pcb);
		//oguear_warning("eliminar_proceso - New - get_sem_grado_value:%d",get_sem_grado_value());
		
		pthread_mutex_lock(&mx_grado_mult_de_mas);
		//loguear_warning("eliminar_proceso - New grado_multiprog_de_mas:%d",grado_multiprog_de_mas);
		if(grado_multiprog_de_mas>0)
			{	//grado_multiprog_de_mas--;
				//loguear_warning("eliminar_proceso - New grado_multiprog_de_mas:%d",grado_multiprog_de_mas);
				pthread_mutex_unlock(&mx_grado_mult_de_mas);
				//loguear_warning("eliminar_proceso - New sem_wait(&sem_bin_new)"  );
				//sem_wait(&sem_bin_new);
				
			}	
			else {
				if(grado_multiprog_de_mas<0){
					sem_post(&sem_cont_grado_mp);
					//loguear_warning("eliminar_proceso - New sem_post get_sem_grado_value:%d",get_sem_grado_value());
					grado_multiprog_de_mas++;
					//loguear_warning("eliminar_proceso - New grado_multiprog_de_mas:%d",grado_multiprog_de_mas);
				}
				pthread_mutex_unlock(&mx_grado_mult_de_mas);
				//loguear_warning("eliminar_proceso - New - get_sem_grado_value:%d",get_sem_grado_value());
			}	
		//loguear_warning("eliminar_proceso - desbloquear_mutex_colas()");
		desbloquear_mutex_colas();	
		
	}	
	else {	// SI ESTA EN BLOCKED? 
		loguear("PID: <%d> - Estado Anterior: <BLOCKED> - Estado Actual: <EXIT>", pcb_query->pcb->PID); // LOG MINIMO Y OBLIGATORIO
		pasar_a_temp_sin_bloqueo(pcb_query);
		free(pcb_query);
		desbloquear_mutex_colas();
		sem_post(&sem_bin_exit);
	}
	
	if(eliminado)
		loguear("Finaliza el proceso <%d> - Motivo <INTERRUPTED_BY_USER>",pid);// LOG MINIMO Y OBLIGATORIO
	return eliminado;

}


bool eliminar_proceso_en_lista(uint32_t pid_buscado,t_queue* estado_buscado ,pthread_mutex_t* mutex_estado_buscado){
	t_pcb* pcb_buscado;
	if (encontrar_en_lista(pid_buscado,estado_buscado, mutex_estado_buscado)){
		pcb_buscado = encontrar_en_lista(pid_buscado,estado_buscado, mutex_estado_buscado);
		pthread_mutex_lock(mutex_estado_buscado);
		if (list_remove_element(estado_buscado->elements, pcb_buscado)) loguear("Se removio el PCB buscado");
		pthread_mutex_unlock(mutex_estado_buscado);
		pasar_a_exit(pcb_buscado);
		return true;
	}
	return false;
}


t_pcb* encontrar_en_lista(uint32_t pid_buscado,t_queue* estado_buscado ,pthread_mutex_t* mutex_estado_buscado){

	bool es_el_pcb (t_pcb* pcb){return pid_buscado == pcb->PID;};

	t_pcb* pcb_encontrado = NULL;
	pthread_mutex_lock(mutex_estado_buscado);
	pcb_encontrado = list_find(estado_buscado->elements, (void*) es_el_pcb);
	pthread_mutex_unlock(mutex_estado_buscado);
	return pcb_encontrado;
}

void pasar_a_exit(t_pcb* pcb){
	//loguear_warning("ESTOY POR DEVOLVER RECURSOS DEL PID <%d>",pcb->PID);
	t_pcb_query* pcb_query = buscar_pcb(pcb->PID);
	t_queue* cola = NULL;
	if(pcb_query->pcb)
		cola = pcb_query->estado;

	devolver_recursos(pcb,cola);
	free(pcb_query);
	proceso_a_estado(pcb, estado_temp,&mx_temp); 

	void loguear_cantidad(void* elem){
		t_recurso* rec= (t_recurso*)elem;
		loguear_warning("Recurso %s, cant: %d",rec->recurso,rec->instancias);
	}

	list_iterate(lista_recursos,&loguear_cantidad);
	
	sem_post(&sem_bin_exit);
}

void devolver_recursos(t_pcb * pcb_saliente,t_queue* cola){	
	bool tiene_pcb_saliente(void* elem){
		t_recurso* recurso = (t_recurso*) elem;
		bool pcb_del_recurso(void* elem){

			t_proceso_instancia* pid_instancia = (t_proceso_instancia*) elem;
			return pid_instancia->PID == pcb_saliente->PID;

		};
		return list_any_satisfy(recurso->lista_procesos_asignados,&pcb_del_recurso);
	};
	void restaurar_recursos(void* elem){
			t_recurso* recurso = (t_recurso*)elem;
			liberar_recurso(pcb_saliente,recurso);
			
			if(!queue_is_empty(recurso->cola_procesos_esperando)){
				
				
				t_pcb* pcb_liberado=queue_pop(recurso->cola_procesos_esperando);
				
				asignar_recurso(pcb_liberado,recurso);

				a_ready_sin_mutex(pcb_liberado);
				
				
			}
			// pthread_mutex_unlock(mutex_cola);
			
	};

	bool esta_en_la_cola(void* elem){
		t_recurso* recurso = (t_recurso*)elem;
		return recurso->cola_procesos_esperando == cola;
	};

	if(cola){
		t_recurso* recurso = list_find(lista_recursos,&esta_en_la_cola);
		if(recurso)
			recurso->instancias++;
	}
	t_list* lista_filtrada_recursos = list_filter(lista_recursos,&tiene_pcb_saliente);
	if(!list_is_empty(lista_filtrada_recursos))
		list_iterate(lista_filtrada_recursos,&restaurar_recursos);
	list_destroy(lista_filtrada_recursos);
}



bool iniciar_threads_io(){
	pthread_t thread_io_conexion;
	pthread_create(&thread_io_conexion,NULL, (void*) iniciar_conexion_io,NULL);
	pthread_detach(thread_io_conexion);

	return true;
}

void blocked_interfaz_destroy(void* elemento ){
	t_blocked_interfaz* blocked_interfaz = (t_blocked_interfaz*)elemento;
	if(blocked_interfaz!=NULL){
		if(blocked_interfaz->estado_blocked!=NULL)
			queue_destroy(blocked_interfaz->estado_blocked);
		
		free(blocked_interfaz);
	}
}

void iniciar_conexion_io(){
	diccionario_nombre_conexion = dictionary_create();
	diccionario_nombre_qblocked = dictionary_create();
	diccionario_conexion_qblocked = dictionary_create();
	//
	// lista_interfaces_blocked = list_create();
	//
	while (1){
		t_blocked_interfaz* blocked_interfaz = malloc(sizeof(t_blocked_interfaz));
		blocked_interfaz->mx_blocked = malloc(sizeof(pthread_mutex_t));
		bool aceptar_interfaz=true;

		int mutex = pthread_mutex_init(blocked_interfaz->mx_blocked, NULL);
		if(mutex == -1){
			loguear_warning("El mutex no se inicio correctamente.");
			//jump al principio?
			aceptar_interfaz=false;
		}
		
		pthread_t thread;
    	int *fd_conexion_ptr = malloc(sizeof(int));
    	*fd_conexion_ptr = esperar_cliente(kernel_escucha);
		if(*fd_conexion_ptr == -1){ 
			loguear_warning("No se puso establecer la conexion con el cliente(I/O).");
			free(fd_conexion_ptr);
			// JUmp al principio?
			aceptar_interfaz=false;
		}
		//char* nombre_interfaz = malloc(16);
		if(aceptar_interfaz){
			//op_code cod_op = recibir_operacion()
			char** splitter_io = recibir_io(*fd_conexion_ptr);
			loguear_warning("NOMBRE DE LA IO: %s", splitter_io[0]);
			loguear_warning("TIPO DE INTERFAZ: %s", splitter_io[1]);
			char* nombre_interfaz = splitter_io[0];
			char* tipo_interfaz = splitter_io[1]; 
			if(nombre_interfaz !=NULL && !existe_interfaz(nombre_interfaz)){
				blocked_interfaz -> estado_blocked = queue_create();
				blocked_interfaz ->tipo_de_interfaz = tipo_interfaz;
				
				//
				char* string_conexion = string_itoa(*fd_conexion_ptr);
				loguear("Bienvenido %s",nombre_interfaz);
				dictionary_put(diccionario_nombre_conexion,nombre_interfaz,fd_conexion_ptr);
				dictionary_put(diccionario_nombre_qblocked,nombre_interfaz, blocked_interfaz);
				dictionary_put(diccionario_conexion_qblocked,string_conexion, blocked_interfaz);
				////
				dictionary_put(estados_dictionary,nombre_interfaz,blocked_interfaz->estado_blocked);
				dictionary_put(estados_mutexes_dictionary,nombre_interfaz,blocked_interfaz->mx_blocked);
				////

				//
				// list_add(lista_interfaces_blocked,blocked_interfaz);
				//
				pthread_create(&thread,NULL, (void*) io_handler,(int*)(fd_conexion_ptr));
				//							
				pthread_detach(thread);
				free(string_conexion);
			
			}
			else if(nombre_interfaz){
				loguear_error("La interfaz %s ya se había conectado.",nombre_interfaz);
			}
			if(nombre_interfaz){
				free(nombre_interfaz);
				//free(tipo_interfaz);
			}
		}
	}

}

bool existe_interfaz(char* nombre_interfaz){
	return (!dictionary_is_empty(diccionario_nombre_conexion) && dictionary_has_key(diccionario_nombre_conexion,nombre_interfaz));
}

t_recurso* obtener_recurso(char* recurso/*Nombre*/){
	t_recurso *recurso_lista;
	for(int i=0;i< list_size(lista_recursos);i++){
		recurso_lista = list_get(lista_recursos,i);
		if(!strcmp(recurso,recurso_lista->recurso))
			return recurso_lista;
	}
	return NULL;
}

char **recibir_io(int conexion){
	
	if(recibir_operacion(conexion) == NUEVA_IO){
		char* mensaje = recibir_mensaje(conexion);
		loguear_warning("Llego el mensaje %s", mensaje);
		char** splitter = string_split(mensaje," ");
		free(mensaje);
		return splitter;
	}
	else loguear_error("RECIBÍ VACÍO");
	
	return NULL;
}



bool le_queda_quantum(t_pcb* pcb){
	return pcb->quantum != config->QUANTUM;
	//VERIFICAR DESPUES DE HACER VIRTUAL ROUND ROBIN
}

void a_ready(t_pcb* pcb){
	if(es_vrr()){
		if(le_queda_quantum(pcb)){
			push_proceso_a_estado(pcb,estado_ready_plus,&mx_ready_plus);
			loguear("PID: <%d> - Estado Anterior: <BLOCKED> - Estado Actual: <READY_PLUS>", pcb->PID); // LOG MINIMO Y OBLIGATORIO
			loguear("Cola Ready / Ready Prioridad");
			loguear_pids(estado_ready,&mx_ready);
			loguear("Cola Ready Plus / Ready Plus Prioridad");
			loguear_pids(estado_ready_plus,&mx_ready_plus);		
		}
		else{
			push_proceso_a_estado(pcb,estado_ready,&mx_ready);
			loguear("PID: <%d> - Estado Anterior: <BLOCKED> - Estado Actual: <READY>", pcb->PID); // LOG MINIMO Y OBLIGATORIO
			loguear("Cola Ready / Ready Prioridad");
			loguear_pids(estado_ready,&mx_ready);
		}
	}
	else
		push_proceso_a_estado(pcb,estado_ready,&mx_ready);
	sem_post(&sem_bin_ready);
}
void a_ready_sin_mutex(t_pcb* pcb){
	if(es_vrr()){
		if(le_queda_quantum(pcb))
			queue_push(estado_ready_plus,pcb);
		else
			queue_push(estado_ready,pcb);
	}
	else
		queue_push(estado_ready,pcb);
	sem_post(&sem_bin_ready);
}
void io_handler(int *ptr_conexion){
	while(1){
		int conexion = *ptr_conexion;
		recibir_operacion(conexion);
		//loguear_warning("LLego el cod op %d", cod_operacion); // COMENTAR 
		char* pid_tipo_char = recibir_mensaje(conexion);
		//loguear_warning("Llego el mensaje %s", mensaje); //COMENTAR

		//char** splitter = string_array_new();
		//splitter = string_split(mensaje," ");
		int pid_a_manejar = atoi(pid_tipo_char);
		char* string_conexion = string_itoa(conexion);
		//loguear_warning("Antes del get del diccionario");
		t_blocked_interfaz* interfaz = dictionary_get(diccionario_conexion_qblocked,string_conexion);
		free(string_conexion);
		t_pcb_query* pcb_query = buscar_pcb(pid_a_manejar);
		if(pcb_query->estado==NULL||pcb_query->estado==estado_temp||pcb_query->estado==estado_exit){
				free(pcb_query);
				return;
		}
		t_pcb* pcb = pcb_query->pcb;
		bool quitar_elem_de_blocked = list_remove_element(interfaz -> estado_blocked->elements,pcb);
		if(quitar_elem_de_blocked){
				//loguear_warning("Ya se popeo el PCB con PID: %d", pcb->PID);
				a_ready(pcb);
		}
		free(pcb_query);
	}
}

t_list* get_nombres_recursos(){	
	void* _get_nombre(void* elem){
		t_recurso* recurso = (t_recurso*)elem;
		return recurso->recurso;
	}

	t_list* lista;
	if(lista_recursos==NULL || list_is_empty(lista_recursos))
		lista= list_create();
	else 
		lista=	list_map(lista_recursos,&_get_nombre);

	 return lista;
}


void io_fs(uint32_t pid, t_paquete* paquete,char* nombre_interfaz){
	loguear_warning("Entra al case");
	//loguear_warning("Uso del FS -> Interfaz:%s Nombre archivo:%s", nombre_interfaz, operacion_fs->nombre_archivo);
	void *ptr_conexion = dictionary_get(diccionario_nombre_conexion, nombre_interfaz);
	int conexion_io = *(int *)ptr_conexion;
	// char* pid_aux = string_itoa(pid);

	//enviar pid con cod op del tipo de operacion
	//enviar_texto(pid_aux,operacion_fs->cod_op,conexion_io);
	// enviar_mensaje(pid_aux, conexion_io);
	// enviar paquete con operacion_fs
	enviar_paquete(paquete, conexion_io);
	paquete_destroy(paquete);
	loguear_warning("Peticion a IO enviada");
}