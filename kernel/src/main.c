#include <stdlib.h>
#include <stdio.h>
#include <utils/hello.h>
#include "kernel.h"
#include "test.h"

int main_kernel(int argc, char** argv) {

    //Iniciamos mediante una funcion las config, el logger y las conexiones
    //Asignamos un flag que nos devolverá si se pudo iniciar el módulo o no.

    bool modulo_iniciado = iniciar_kernel(argv[1]);
   // En caso de que no se haya inicializado, finalizamos el programa
    if(!modulo_iniciado){
        finalizar_kernel();
        return EXIT_FAILURE;
    } 
    iniciar_planificadores();
    iniciar_consola();
    

    finalizar_kernel();

    return EXIT_SUCCESS;
}





// TESTS
int main(int argc, char** argv) {
        if(argc > 1 && strcmp(argv[1],"-test")==0)
        run_tests();
    else 
        main_kernel(argc,argv);
}

