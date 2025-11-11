#include "test.h"

// Función que suma dos números enteros
int sum(int a, int b) {
    return a + b;
}

void crear_procesos(){

    queue_push(estado_new,  pcb_create_quantum("programa1",2));
     queue_push(estado_new,  pcb_create_quantum("programa2",3));

     queue_push(estado_ready,  pcb_create_quantum("programa3",3));
    // queue_push(estado_blocked,  pcb_create_quantum("programa4",5));
        queue_push(estado_ready,  pcb_create_quantum("programa5",2));

      pcb_exec =  pcb_create("programa6");
      pcb_exec->quantum = 0;
       //queue_push(estado_ready_plus,  pcb_create("programa7"));
    
}


bool iniciar_kernel_prueba(){
 return   iniciar_logger_config("./kernel.config")&&
	    inicializar_comandos()&&
        iniciar_estados_planificacion()&&
        iniciar_dispatch()&&
        iniciar_interrupt();
}

// Función de inicialización de las pruebas
int init_suite(void) {
     
    system("clear");
   
    if(  iniciar_kernel_prueba()){
    crear_procesos();
    iniciar_consola();

    }
    
    return 0;
}

// Función de limpieza de las pruebas
int clean_suite(void) {

    return 0; // Todo está bien
}

// Test unitario para la función sum
void test_sum() {
    CU_ASSERT_EQUAL(sum(1, 2), 3);
    CU_ASSERT_EQUAL(sum(-1, 1), 0);
    CU_ASSERT_EQUAL(sum(0, 0), 0);
    CU_ASSERT_EQUAL(sum(10, -5), 5);
}
void es_FIFO()
{
    CU_ASSERT_EQUAL(config->ALGORITMO_PLANIFICACION.id,RR);
    ejecutar_planificacion();
}

void envia_pcb()
{
   // ejecutar_proceso();
}

int run_tests() {
    // Ejecutar el test
    
   // Inicializar el registro de pruebas
    CU_initialize_registry();

    // Añadir la suite de pruebas al registro
    CU_pSuite suite = CU_add_suite("Suite de Pruebas", init_suite, clean_suite);

    // Añadir las pruebas a la suite
    CU_add_test(suite, "test_sum", test_sum);   
   // CU_add_test(suite, "es_fifo", es_FIFO); 
    CU_add_test(suite,"envia_pcb",envia_pcb);


    resumen_tests();


    return 0;
}