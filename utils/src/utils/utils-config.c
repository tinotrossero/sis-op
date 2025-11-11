#include "utils-config.h"

char* construir_path_config(char* nombre_archivo){
    if(nombre_archivo!=NULL)
    return construir_path(nombre_archivo,".config");
    else
        return NULL;
}

t_config* iniciar_config(char* nombre_archivo)
{
     char* path = construir_path_config(nombre_archivo);

    if(path==NULL)	{		
        loguear( "No se pudo crear el config! No se especificó un nombre de archivo");
        exit(EXIT_FAILURE);
    };

   
	t_config* nuevo_config;
	char *current_dir = getcwd(NULL, 0);
    printf("El directorio actual es %s\n", current_dir);
    free(current_dir);
	nuevo_config = config_create(path);
    free(path);

    if(nuevo_config==NULL)	{		
        loguear( "No se pudo crear el config! %s",nombre_archivo);
        exit(EXIT_FAILURE);
    };


	return nuevo_config;
}