#include "utils-tests.h"

void resumen_tests(){
    // Ejecutar todas las pruebas
    CU_basic_run_tests();

    CU_basic_show_failures(CU_get_failure_list());


     // Colorear el resumen
     if (CU_get_number_of_tests_run()==0){    
        printf(ANSI_COLOR_RED);  
        printf("No corrieron los tests.\n");
     }

    else if (CU_get_number_of_tests_run() > 0 &&
        CU_get_number_of_tests_run() >= CU_get_number_of_failures() &&
        CU_get_number_of_failures() == 0) {
        printf(ANSI_COLOR_GREEN); // Color verde brillante
        printf("Todos los tests pasaron correctamente.\n");
    } else {
        printf(ANSI_COLOR_RED); // Color rojo brillante
        printf("Algunos tests fallaron.\n");
    }
    printf(ANSI_COLOR_RESET); // Restablecer el color


    // Limpiar el registro de pruebas
    CU_cleanup_registry();
}