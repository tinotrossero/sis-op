#include "utils-logger.h"

pthread_mutex_t mx_log = PTHREAD_MUTEX_INITIALIZER;
t_log* logger;


char* construir_path(char* nombre_archivo,char* extension){
    char* path_char =  "";    
    char* path =malloc(strlen(path_char)+strlen(nombre_archivo)+strlen(extension) +1);
        if (path == NULL) {
        fprintf(stderr, "Memory allocation failed in construir_path.\n");
        exit(EXIT_FAILURE);
    }
    strcpy(path, path_char);
    strcat(path,nombre_archivo);
    strcat(path,extension);
    return path;
}

char* construir_path_log(char* nombre_archivo){
    return construir_path(nombre_archivo,".log");
}

t_log* iniciar_logger_(char* nombre_archivo, int mostrar_consola)
{
    char *titulo = string_duplicate(nombre_archivo);
    string_to_upper(titulo);
    char* path = construir_path_log(nombre_archivo);
	t_log* nuevo_logger =log_create(path,titulo,mostrar_consola,LOG_LEVEL_INFO);
    free(path);
    free(titulo);

	return nuevo_logger;
}

t_log* iniciar_logger(char* nombre_archivo)
{
   return iniciar_logger_(nombre_archivo,1);
}



void loguear(const char* message_template, ...) {
	
	char*message;
	va_list arguments;
   	va_start(arguments, message_template);		
	message = string_from_vformat(message_template, arguments);	
	va_end(arguments);		

	pthread_mutex_lock(&mx_log);
	log_info(logger,message,"");
	pthread_mutex_unlock(&mx_log);

	free(message);

}

void loguear_error(const char* message_template, ...) {
	
	char*message;
	va_list arguments;
   	va_start(arguments, message_template);	
	message = string_from_vformat(message_template, arguments);		
	va_end(arguments);		

	pthread_mutex_lock(&mx_log);
	log_error(logger,message,"");
	pthread_mutex_unlock(&mx_log);

	free(message);

}

void loguear_warning(const char* message_template, ...) {
	
	char*message;
	va_list arguments;
   	va_start(arguments, message_template);		
	message = string_from_vformat(message_template, arguments);	
	va_end(arguments);		
	

	pthread_mutex_lock(&mx_log);
	log_warning(logger,message,"");
	pthread_mutex_unlock(&mx_log);

	free(message);

}