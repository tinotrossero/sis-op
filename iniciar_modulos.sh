#!/bin/bash

# Función para ejecutar un comando en una nueva terminal y esperar a que se complete
function ejecutar_en_terminal {
    local directorio="$1"
    local ejecutable="$2"
    local señalizacion="$3"
    local pidfile="/tmp/$senalizacion.pid"
    
    # Cambiar al directorio especificado
    cd "$directorio"
    
    # Verificar si el archivo ejecutable existe y es un archivo regular
    if [ ! -f "$ejecutable" ]; then
        echo "El archivo $ejecutable no existe en el directorio $directorio o no es un archivo regular."
        exit 1
    fi
    
    # Ejecutar el comando y guardar el PID en un archivo
    ./"$ejecutable" &
    echo $! > "$pidfile"
}

# Iniciar proceso en memoria
ejecutar_en_terminal "./memoria" "./exec" "memoria_ready"

# Esperar a que el proceso memoria se inicie
memoria_pidfile="/tmp/memoria_ready.pid"
while [ ! -f "$memoria_pidfile" ] || ! pgrep -F "$memoria_pidfile" > /dev/null; do sleep 1; done

# Abrir terminal para proceso en cpu
gnome-terminal --working-directory="./cpu" -- ./exec &

# Esperar a que el proceso cpu se inicie
cpu_pidfile="/tmp/cpu_ready.pid"
while [ ! -f "$cpu_pidfile" ] || ! pgrep -f "./exec" > /dev/null; do sleep 1; done

# Abrir terminal para proceso en kernel
gnome-terminal --working-directory="./kernel" -- ./exec &

# Esperar a que el proceso kernel se inicie
kernel_pidfile="/tmp/kernel_ready.pid"
while [ ! -f "$kernel_pidfile" ] || ! pgrep -f "./exec" > /dev/null; do sleep 1; done

# Mensaje de finalización
echo "Todos los procesos han iniciado correctamente."
