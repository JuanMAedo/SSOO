#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>


// Consideramos que la aplicación tendrá 3 hilos:
// 1º - Hilo: elección al centro de vacunación que acudirá cada habitante
// 2º - Hilo: vacunaciñon del habitante x en cada centro
// 3º - Hilo: fabricas de las vacunas entregas en los 5 centros

void configuracion(char * entrada, char * salida);
void impresion_configuracion(int configuracion []);


// Declaración variables globales
int centros_vacunacion = 5;
int fabricas = 3;
int configuracion_inicial[9];
char* entrada_defecto = "entrada_vacunacion.txt";
char* salida_defecto = "salida_vacunacion.txt";

int main(int argc, char * argv[]){
    //Comprobamos las entradas de los ficheros
    if (argc == 1){// Uso entrada_vacunacion.txt y salida_vacunacion.txt 
        configuracion(entrada_defecto, salida_defecto);  
    }else if (argc == 2){// Uso argv[1] y salida_vacunacion.txt
        char* nombre_entrada = argv[1];
        configuracion(nombre_entrada,salida_defecto);
    }else if (argc == 3){// Uso argv[1] y argv[2]
        char* nombre_entrada = argv[1];
        char* nombre_salida = argv[2];
        configuracion(nombre_entrada,nombre_salida); 
    }else{
        fprintf(stderr, "Error, número de argumentos incorrecto.\n");
        exit(1);
    }
    impresion_configuracion(configuracion_inicial);
    // En el array configuracion_inicial tenemos las 9 posiciones con los parametros necesarios para la vacunación
    // A partir de ello 

    // rand()% [INTERVALO +1]+[MINIMO] cuando el número aleatorio debe estar entre 2 valores
}

void configuracion (char * entrada, char * salida){
    char buffer[1024];
    int contador = 0;
    int fdescriptor = open(entrada, O_RDONLY); 
    //Realizamos los cambios en la tabla de descrpitores necesarias para la entrada
    if (fdescriptor == -1){
        fprintf(stderr, "%s: Error, asociado al descriptor de entrada: %s.\n", entrada, strerror(errno));
        exit(1);
    }else{
        // No hay error y se escribe el descriptor
        dup2(fdescriptor,0);
        close(fdescriptor);
    } 
    //Leemos el archivo pasado por líneas y lo guardamos en un array, donde estará la configuración
    while(fgets(buffer,1024,stdin)){
        configuracion_inicial[contador] = atoi(buffer); 
        contador++;
    }
    //Comprobamos que todos los parámetros son válidos
    for(int i=0; i<9; i++){
        if(configuracion_inicial[i] < 0){
            fprintf(stderr, "Error, algún parámetro de la configuración introducido en el archivo de entrada es negativo\n");
            exit(-1);
        }else if((configuracion_inicial[i] == 0) && (i>=6)){
            //Establecemos el tiempo mínimo de reparto,reacción y desplazamiento en 1, en caso de que sea introducido a 0
            configuracion_inicial[i] = 1;
            printf("El parámetro %d ha sido establecido en 1 seg, ya que es su tiempo mínimo\n", i+1);
        }
    
    }
}

void impresion_configuracion(int configuracion []){
    printf("VACUNACIÓN EN PANDEMIA: CONFIGURACIÓN INICIAL\n");
    printf("Habitantes: %d\n", configuracion[0]);
    printf("Centros de Vacunación: %d \n", centros_vacunacion);
    printf("Fábricas de Vacunas: %d\n", fabricas);
    printf("Vacunados por tanda: %d\n", configuracion[0]/10);
    printf("Vacunas Iniciales en cada Centro: %d\n",configuracion[1]);
    printf("Vacunas totales por fábrica: %d\n", configuracion[0]/fabricas);
    printf("Mínimo número de vacunas fabricadas en cada tanda: %d\n", configuracion[2]);
    printf("Máximo número de vacunas fabricadas en cada tanda: %d\n", configuracion[3]);
    printf("Tiempo mínimo de fabricación de una tanda de vacunas: %d\n", configuracion[4]);
    printf("Tiempo máximo de fabricación de una tanda de vacunas: %d\n", configuracion[5]);
    printf("Tiempo máximo de reparto de vacunas a los centros: %d\n",configuracion[6]);
    printf("Tiempo máximo que un habitante tarda en ver que está citado para vacunarse: %d\n", configuracion[7]);
    printf("Tiempo máximo de desplazamiento del habitante al centro de vacunación: %d\n\n", configuracion[8]);
}