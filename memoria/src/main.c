#include <stdlib.h>
#include <stdio.h>
#include <utils/hello.h>
#include "memoria.h"

int main(int argc, char** argv) {

    //Iniciamos mediante una funcion las config, el logger y las conexiones
    //Asignamos un flag que nos devolverá si se pudo iniciar correctamente el módulo o no.
    bool memmoria_iniciada = iniciar_memoria(argv[1]);
    if(!memmoria_iniciada){
        //En caso de que no se haya inicializado correctamente, finalizamos memoria y salimos del programa
        finalizar_memoria();
        return EXIT_FAILURE;
    } 

    //Si llega hasta acá es porque ya se ejecutó todo lo necesario
    //Finalizamos memoria y salimos del programa
    finalizar_memoria();
    return EXIT_SUCCESS;




}
