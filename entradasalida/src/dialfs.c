#include "dialfs.h"

FILE *archivo_bitmap,*archivo_metadata,*archivo_bloques;
int tamanio_filesystem,tamanio_restante_filesystem, tamanio_bitmap;
char* path_bitmap, *path_bloques, *dir_metadata;
void* bitmap;
void* data_bloques;
t_bitarray* bitarray_bitmap;
t_list* lista_archivos;
void* swap;

bool iniciar_archivos_dialfs(){
    if(DIALFS == config->TIPO_INTERFAZ.id)
    {
    int i;
    swap = malloc(sizeof(tamanio_filesystem));
    tamanio_bitmap= ceil(((config->BLOCK_COUNT)/8.0));
    lista_archivos = list_create();
    dir_metadata = path_resolve(config->PATH_BASE_DIALFS,DIR_METADATA);
    path_bitmap = path_resolve(config->PATH_BASE_DIALFS,PATH_BITMAP);
    path_bloques = path_resolve(config->PATH_BASE_DIALFS,PATH_BLOQUES);

    tamanio_filesystem = config->BLOCK_SIZE * config->BLOCK_COUNT;
    tamanio_restante_filesystem = tamanio_filesystem;

    if(access(path_bloques,F_OK)!=0){
        archivo_bloques = fopen(path_bloques,"w");
        for(i=0;i<tamanio_filesystem;i++)
            fprintf(archivo_bloques,"@");
        fclose(archivo_bloques);
    }
    if(access(path_bitmap,F_OK)!=0){
        archivo_bitmap = fopen(path_bitmap,"w");
        void* bitmap_aux = malloc(tamanio_bitmap);
        t_bitarray* bitarray = bitarray_create_with_mode(bitmap_aux,tamanio_bitmap,LSB_FIRST);
        for(i=0;i<config->BLOCK_COUNT;i++)
            bitarray_clean_bit(bitarray,i);
        fwrite(bitarray->bitarray,config->BLOCK_COUNT,1,archivo_bitmap);
        bitarray_destroy(bitarray);
        fclose(archivo_bitmap);
        
    }
    cargar_directorio_metadata();

    int fd_bitmap = open(path_bitmap,O_RDWR);
    int fd_data = open(path_bloques,O_RDWR);
    bitmap = mmap(NULL, tamanio_bitmap, PROT_READ | PROT_WRITE, MAP_SHARED, fd_bitmap, 0);
    data_bloques = mmap(NULL, tamanio_filesystem, PROT_READ | PROT_WRITE, MAP_SHARED, fd_data, 0);
    bitarray_bitmap= bitarray_create_with_mode(bitmap,tamanio_bitmap,LSB_FIRST);
    }
    return true;

}

bool cargar_directorio_metadata(){
    DIR *dir = opendir(dir_metadata);
    struct dirent *entry;
    
    while ((entry = readdir(dir)) != NULL) {
      if(string_contains(entry->d_name,".txt")){
          t_dialfs_metadata* metadata=obtener_metadata_txt(entry->d_name);
        list_add(lista_archivos,metadata);
      }
    }
    closedir(dir);
    return true;
}

t_dialfs_metadata* obtener_metadata_txt(char* nombre_archivo){
    t_dialfs_metadata* metadata=malloc(sizeof(t_dialfs_metadata));
    t_config* config_meta = config_create(path_resolve(dir_metadata,nombre_archivo));

    metadata->bloque_inicial=(uint32_t)config_get_int_value(config_meta,"BLOQUE_INICIAL");
    metadata->tamanio_archivo=(uint32_t)config_get_int_value(config_meta,"TAMANIO_ARCHIVO");
    metadata->nombre_archivo=string_duplicate(nombre_archivo);

    config_destroy(config_meta);
    return metadata;
}

bool io_fs_create(char* nombre_archivo){

    char* path_metadata =string_new();
    path_metadata = path_resolve(dir_metadata,nombre_archivo);

    bool buscar_archivo(void* elem){
        t_dialfs_metadata* metadata = (t_dialfs_metadata*) elem;
        return !strcmp(nombre_archivo, metadata->nombre_archivo);
    };
    if(list_any_satisfy(lista_archivos, &buscar_archivo)){
        loguear_warning("Ya existe un archivo creado con ese nombre.");
        return true;
    };
    t_dialfs_metadata* metadata = create_metadata(nombre_archivo);
    list_add(lista_archivos, metadata);
    loguear("Se agrega el archivo %s",nombre_archivo);
    
    // fwrite(&metadata,sizeof(t_dialfs_metadata),1,archivo_metadata);
    editar_archivo_metadata(path_metadata,metadata);

    free(path_metadata);
    return true;

}



bool io_fs_delete(char* nombre_archivo, uint32_t pid){
    
    bool buscar_archivo(void* elem){
        t_dialfs_metadata* metadata = (t_dialfs_metadata*) elem;
        return !strcmp(nombre_archivo, metadata->nombre_archivo);
    };

    char* path_metadata =string_new();
    path_metadata = path_resolve(config->PATH_BASE_DIALFS,DIR_METADATA);
    path_metadata = path_resolve(path_metadata,nombre_archivo);
    t_dialfs_metadata* metadata_delete = (t_dialfs_metadata*)list_find(lista_archivos,&buscar_archivo);
    truncar_bitmap(metadata_delete,0, pid);
    list_remove_element(lista_archivos,metadata_delete);
    remove(path_metadata);
    // free(metadata_delete->nombre_archivo);
    free(metadata_delete);
    return true;
}

bool io_fs_truncate(char* nombre_archivo,uint32_t tamanio_final, uint32_t pid){

    bool buscar_archivo(void* elem){
        t_dialfs_metadata* metadata = (t_dialfs_metadata*) elem;     
        return !strcmp(nombre_archivo, metadata->nombre_archivo);
    };
    char* path_metadata =string_new();
    path_metadata = path_resolve(dir_metadata,nombre_archivo);
    t_dialfs_metadata* metadata = (t_dialfs_metadata*)list_find(lista_archivos,&buscar_archivo);
    truncar_bitmap(metadata,tamanio_final, pid);
    metadata->tamanio_archivo = tamanio_final;
    editar_archivo_metadata(path_metadata,metadata);
    
    return true;

}

// typedef struct{
//     char* nombre_archivo;
//     t_direcciones_proceso* direcciones_proceso;
//     uint32_t registro_puntero_archivo;

// }t_peticion_fs;

bool io_fs_read(t_operacion_fs* peticion_fs){
    bool encontrar_archivo(void* elem){
        t_dialfs_metadata* metadata_elem = (t_dialfs_metadata*)elem;
        return (!strcmp(peticion_fs->nombre_archivo,metadata_elem->nombre_archivo));
    };
    t_dialfs_metadata* metadata_write = list_find(lista_archivos,&encontrar_archivo);
    void* stream_memoria = malloc(peticion_fs->tamanio_registro);
    uint32_t byte_inicial_lectura = config->BLOCK_SIZE * metadata_write->bloque_inicial + peticion_fs->registro_puntero;
    memcpy(stream_memoria,data_bloques+ byte_inicial_lectura,peticion_fs->tamanio_registro);
    escribir_memoria_completa_FS(peticion_fs->tamanio_registro, peticion_fs->direcciones, peticion_fs->pid,stream_memoria,conexion_memoria, ESCRITURA_MEMORIA);
    free(stream_memoria);

    return true;
} 

bool io_fs_write(t_operacion_fs* peticion_fs){
    t_buffer* buffer =leer_memoria_completa_FS(peticion_fs->tamanio_registro, peticion_fs->direcciones, peticion_fs->pid,conexion_memoria, LECTURA_MEMORIA);
    bool encontrar_archivo(void* elem){
        t_dialfs_metadata* metadata_elem = (t_dialfs_metadata*)elem;
        return (!strcmp(peticion_fs->nombre_archivo,metadata_elem->nombre_archivo));
    };
    t_dialfs_metadata* metadata_write = list_find(lista_archivos,&encontrar_archivo);
    uint32_t byte_inicial_escritura = config->BLOCK_SIZE * metadata_write->bloque_inicial + peticion_fs->registro_puntero;
    // memcpy(&data_bloques[byte_inicial_escritura],buffer->stream,peticion_fs->tamanio_registro);
    memcpy(data_bloques + byte_inicial_escritura,buffer->stream,peticion_fs->tamanio_registro);
    return true;
}

bool truncar_bitmap(t_dialfs_metadata* metadata, uint32_t tamanio_final, uint32_t pid){

    //Le resto 1 byte al archivo para que no me cree bloques demás y que no reste negativo en caso de ser 0
    uint32_t tam_archivo = (metadata->tamanio_archivo == 0)? 0 : metadata->tamanio_archivo - 1;
    tamanio_final=(tamanio_final == 0)? 0: tamanio_final-1; 

    //Calculo la posicion inicial y final de los bloques a asignar/desasignar
    uint32_t cantidad_bloques_inicial = tam_archivo/ config->BLOCK_SIZE;
    uint32_t cantidad_bloques_final = tamanio_final/ config->BLOCK_SIZE;
    uint32_t posicion_inicial=metadata->bloque_inicial + cantidad_bloques_inicial;
    uint32_t posicion_final= metadata->bloque_inicial + cantidad_bloques_final;

    //Si se reduce el tamaño del archivo, le saco los bloques
    if(tamanio_final < metadata->tamanio_archivo){
        for(uint32_t i=posicion_final;i <= posicion_inicial;i++)
            bitarray_clean_bit(bitarray_bitmap,i);
    }
    //Si se quiere ampliar el tamaño del archivo...
    if(tamanio_final > metadata->tamanio_archivo){
        //Primero tenemos que ver que haya espacio en el filesystem
        if(tamanio_final > tamanio_restante_filesystem){
            loguear_error("No hay espacio suficiente para ampliar el archivo <%s>",metadata->nombre_archivo);
            return false;
        }
        //Luego que se le puedan asignar los bloques contiguos necesarios
        for(uint32_t i=posicion_inicial+1;i <= posicion_final;i++){

            //Si no se puede...
            if(bitarray_test_bit(bitarray_bitmap,i)){
                //Compactamos
                loguear("PID: <%d> - Inicio Compactacion.", pid);
                metadata = compactacion(metadata);
                loguear("PID: <%d> - Fin Compactacion.", pid);
                //Recalculamos las posiciones de los bloques
                posicion_inicial=metadata->bloque_inicial + cantidad_bloques_inicial;
                posicion_final= metadata->bloque_inicial + cantidad_bloques_final;
                break;
            }

        }
        //Luego agregamos los bloques 
        for(uint32_t i=posicion_inicial;i <= posicion_final;i++)
            bitarray_set_bit(bitarray_bitmap,i);
    }
    loguear_bitmap(bitarray_bitmap);
    return true;

}

t_dialfs_metadata* compactacion(t_dialfs_metadata* metadata){
    usleep((config->RETRASO_COMPACTACION)*1000);

    bool ordenar_por_bloque_inicial(void* elem1,void* elem2){
        t_dialfs_metadata* metadata_sort_1 = (t_dialfs_metadata*) elem1;
        t_dialfs_metadata* metadata_sort_2 = (t_dialfs_metadata*) elem2;
        
        return metadata_sort_1->bloque_inicial < metadata_sort_2->bloque_inicial;
    };

    void correr_archivos(void* elem){
        t_dialfs_metadata* metadata_elem = (t_dialfs_metadata*) elem;
        uint32_t tamanio_final_elem = (metadata_elem->tamanio_archivo==0)? 0: metadata_elem->tamanio_archivo -1;
        uint32_t bloque_final_elem= metadata_elem->bloque_inicial + (tamanio_final_elem / config->BLOCK_SIZE);
        int bit_libre_inicial = asignar_bloque_inicial();
        int bit_index = bit_libre_inicial;
        void* puntero_destino;
        void* puntero_origen;
        int byte_destino,byte_origen;
        int bloque_index;
        if(bit_index < metadata_elem->bloque_inicial){
            for(bloque_index=(int)metadata_elem->bloque_inicial;bloque_index<=bloque_final_elem;bloque_index++){
                byte_destino = (bit_index * config->BLOCK_SIZE);
                byte_origen=(bloque_index * config->BLOCK_SIZE);
                puntero_destino=data_bloques + byte_destino;
                puntero_origen=data_bloques + byte_origen;
                memcpy(puntero_destino,puntero_origen,config->BLOCK_SIZE);
                bitarray_set_bit(bitarray_bitmap,bit_index);
                bitarray_clean_bit(bitarray_bitmap,bloque_index);
                bit_index++;
            }

            metadata_elem->bloque_inicial = (uint32_t) bit_libre_inicial;
            editar_archivo_metadata(path_resolve(dir_metadata,metadata_elem->nombre_archivo),metadata_elem);
        
        }
        else
            bitarray_clean_bit(bitarray_bitmap,bit_index);
    };
    uint32_t tamanio_final_metadata = (metadata->tamanio_archivo==0)? 0: metadata->tamanio_archivo -1;
    uint32_t bloque_final_metadata= metadata->bloque_inicial + (tamanio_final_metadata / config->BLOCK_SIZE);
    t_list* lista_ordenada = list_sorted(lista_archivos,&ordenar_por_bloque_inicial);
    uint32_t i;
    list_remove_element(lista_ordenada,metadata);
    for(i=metadata->bloque_inicial;i<=bloque_final_metadata;i++){
        bitarray_clean_bit(bitarray_bitmap,i);
    }
    memcpy(swap, data_bloques + (metadata->bloque_inicial * config->BLOCK_SIZE),metadata->tamanio_archivo);

    list_iterate(lista_ordenada,&correr_archivos);
    metadata->bloque_inicial = (uint32_t)asignar_bloque_inicial();
    memcpy(data_bloques + (metadata->bloque_inicial * config->BLOCK_SIZE),swap,metadata->tamanio_archivo);
    editar_archivo_metadata(path_resolve(dir_metadata,metadata->nombre_archivo),metadata);
    

    return metadata;
}


bool editar_archivo_metadata(char* path_metadata,t_dialfs_metadata* metadata){
    archivo_metadata = fopen(path_metadata,"w");
    char* txt = malloc(50);
    sprintf(txt,"BLOQUE_INICIAL=%d\nTAMANIO_ARCHIVO=%d",metadata->bloque_inicial,metadata->tamanio_archivo);
    txt_write_in_file(archivo_metadata,txt);
    fclose(archivo_metadata);
    free(txt);
    return true;
}


t_dialfs_metadata* create_metadata(char* nombre_archivo){
    t_dialfs_metadata* metadata = malloc(sizeof(t_dialfs_metadata));
    metadata->bloque_inicial= (uint32_t)asignar_bloque_inicial();
    metadata->tamanio_archivo = 0;
    metadata->nombre_archivo= string_duplicate(nombre_archivo);
    return metadata;

}


int asignar_bloque_inicial(){
   int i;
   bool flag_test_bit;
   for(i=0;i<config->BLOCK_COUNT;i++){
        flag_test_bit=bitarray_test_bit(bitarray_bitmap,i);
        if(!flag_test_bit){
            bitarray_set_bit(bitarray_bitmap,i);
            return i;
        }
   }
    return -1;

}

// t_bitarray* obtener_bitmap(){
//     archivo_bitmap = fopen(path_bitmap,"r");
//     char* bitmap = malloc(config->BLOCK_COUNT);
//     fread(bitmap,config->BLOCK_COUNT,1, archivo_bitmap);
//     fclose(archivo_bitmap);
//     return bitarray_create_with_mode(bitmap,config->BLOCK_COUNT,LSB_FIRST);
// }



void loguear_bitmap(t_bitarray* bitmap){
    loguear("|    N  | v |");
    loguear("-------------");
    for(int i=0;i<config->BLOCK_COUNT;i++)
        loguear("| %4d  | %1d |",i,bitarray_test_bit(bitmap,i));
}