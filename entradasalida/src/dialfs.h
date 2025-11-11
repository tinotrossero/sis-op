#ifndef dialfs_h
#define dialfs_h

#include "entradasalida.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#define PATH_BITMAP "bitmap.dat"
#define PATH_BLOQUES "bloques.dat"
#define DIR_METADATA "metadata/"


typedef struct 
{   
    char* nombre_archivo;
    uint32_t bloque_inicial;
    uint32_t tamanio_archivo;
    

} t_dialfs_metadata;

t_dialfs_metadata* obtener_metadata_txt(char* nombre_archivo);


bool iniciar_archivos_dialfs();//Control de bloques de datos
// bool compactacion();
bool io_fs_create(char* nombre_archivo);
bool io_fs_delete(char* nombre_archivo, uint32_t pid);
bool io_fs_truncate(char* nombre_archivo, uint32_t tamanio_final, uint32_t pid);
bool io_fs_write(t_operacion_fs* operacion_fs);
bool io_fs_read(t_operacion_fs* operacion_fs);

bool actualizar_bitmap(t_bitarray* bitmap);
bool editar_archivo_metadata(char* path_metadata,t_dialfs_metadata* metadata);

t_dialfs_metadata* create_metadata(char* nombre_archivo);
t_dialfs_metadata* compactacion(t_dialfs_metadata* metadata);

bool cargar_directorio_metadata();
// t_bitarray* obtener_bitmap();
int asignar_bloque_inicial();
bool truncar_bitmap(t_dialfs_metadata* metadata, uint32_t tamanio_final, uint32_t pid);
void loguear_bitmap(t_bitarray* bitmap);
extern FILE *archivo_bitmap,*archivo_metadata,*archivo_bloques;
extern int tamanio_filesystem, tamanio_bitmap;
extern void* bitmap, *data_bloques;
extern char* path_bitmap, *path_bloques, *dir_metadata;
extern void* swap;
extern t_list* lista_archivos;
extern t_bitarray* bitarray_bitmap;

#endif