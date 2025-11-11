#include "generica.h"

void io_gen_sleep(int unidades_de_trabajo){
    usleep((config->TIEMPO_UNIDAD_TRABAJO)*unidades_de_trabajo);
}


