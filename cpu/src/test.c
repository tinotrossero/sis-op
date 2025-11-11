#include "test.h"
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>


bool test_conexion_memoria(char* path_config){
		return
	iniciar_log_config(path_config) &&
	iniciar_registros_cpu()	&&
	// iniciar_dispatch()&&
	iniciar_conexion_memoria();
	
}

void test_instrucciones(){
    // bool flag_fetch=fetch(pcb);
  /*  registros_cpu->AX= 10;
    registros_cpu->IR="SUB AX 5";
    bool flag_decode = decode();
    if(flag_decode == false){ loguear_error("%s",registros_cpu->IR); return EXIT_FAILURE;}
    loguear("ID:%s P1:%i P2:%i",
    registros_cpu->INSTID,
    *(uint32_t*)registros_cpu->PARAM1,
    *(uint32_t*)registros_cpu->PARAM2);
    bool flag_exe = execute();
    if(!flag_exe){loguear_error("No se pudo ejecutar"); return EXIT_FAILURE;}
    loguear("AX: %d", registros_cpu->AX);*/
}

// Función que suma dos números enteros
int sum(int a, int b) {
    return a + b;
}


// Función de inicialización de las pruebas
int init_suite(void) {

     bool cpu_iniciada = test_conexion_memoria("./cpu.config");
    if(!cpu_iniciada){ 
        finalizar_cpu();
        return EXIT_FAILURE;
        }
    //Limpiamos la consola    
    system("clear");
    return 0; // Todo está bien
}

// Función de limpieza de las pruebas
int clean_suite(void) {
 
 finalizar_cpu();
    return 0; // Todo está bien
}

// Test unitario para la función sum
void test_sum() {
    CU_ASSERT_EQUAL(sum(1, 2), 3);
    CU_ASSERT_EQUAL(sum(-1, 1), 0);
    CU_ASSERT_EQUAL(sum(0, 0), 0);
    CU_ASSERT_EQUAL(sum(10, -5), 5);
}

void test_proxima_instruccion(){
    t_pcb* pcb = pcb_create("programa1.txt");

    CU_ASSERT_STRING_EQUAL(pedir_proxima_instruccion(pcb), "hola");
    pcb->program_counter++;
    CU_ASSERT_STRING_EQUAL(pedir_proxima_instruccion(pcb), "que");
    pcb->program_counter++;
    CU_ASSERT_STRING_EQUAL(pedir_proxima_instruccion(pcb), "tal");

    pcb_destroy(pcb);
}

void test_ciclo_de_instruccion(){
    t_pcb* pcb = pcb_create("programa5.txt");
    iniciar_registros_cpu();
    ciclo_de_instruccion(pcb);

    pcb_destroy(pcb);
}


int run_tests() {
    // Ejecutar el test
    
   // Inicializar el registro de pruebas
    CU_initialize_registry();

    // Añadir la suite de pruebas al registro
    CU_pSuite suite = CU_add_suite("Suite de Pruebas", init_suite, clean_suite);

    // Añadir las pruebas a la suite
    // CU_add_test(suite, "test_sum", test_sum);
    CU_add_test(suite, "test_ciclo_de_instruccion", test_ciclo_de_instruccion);

    resumen_tests();
    //  test_instrucciones();

    return 0;
}