#include <stdlib.h>
#include <stdio.h>
#include <utils/hello.h>
#include "entradasalida.h"

int main(int argc, char* argv[]) {   
   
    // Iniciamos mediante una funcion las config, el logger y las conexiones
    // Asignamos un flag que nos devolverá si se pudo iniciar correctamente el módulo o no.
    bool flag_iniciar_io = iniciar_io(argv[1],argv[2]);
    if(flag_iniciar_io == false){
        //En caso de que no se haya inicializado correctamente, finalizamos memoria y salimos del programa
        finalizar_io();
        return EXIT_FAILURE;
    }
    recibir_io();

    //Si llega hasta acá es porque ya se ejecutó todo lo necesario
    //Finalizamos memoria y salimos del programa
    finalizar_io();
  
    // iniciar_log_config(argv[1],argv[2]);
    // iniciar_archivos_dialfs();
    // io_fs_create("fede_crack.txt");
    // io_fs_create("fede_crack_II.txt");
    return EXIT_SUCCESS;
}


// int main() {
//     // Write C code here
//     int t = 1;
//     char* cadena = malloc(t);
//     fgets(cadena,t,stdin);
//     scanf("%s", cadena);
//     printf("%s\n", cadena);
//     printf("%d\n", sizeof(char));
//     printf("%d", sizeof("hola"));
    
//     return 0;
// }