#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>

#define CENTROS_VACUNACION 5
#define FABRICAS 3

void configuracion(char * entrada, char * salida);
void impresion_configuracion(int configuracion []);
void * habitante(void *num);
void * fabrica(void *num);

typedef struct {
	int id_centro;
    int vacunas_disponibles;
    int lista_espera;
} centro_vacunacion;

// Declaración variables globales
int configuracion_inicial[9], vacunas_a_fabricar;
char* entrada_defecto = "entrada_vacunacion.txt";
char* salida_defecto = "salida_vacunacion.txt";
centro_vacunacion centros_vacunacion[CENTROS_VACUNACION];

int main(int argc, char * argv[]){
    int * habitantes_id;
    pthread_t th;
    

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
    }else{//Error con el número de argumentos
        fprintf(stderr, "Error, número de argumentos incorrecto.\n");
        exit(1);
    }
    // En el array configuracion_inicial tenemos las 9 posiciones con los parametros necesarios para la vacunación
    impresion_configuracion(configuracion_inicial);

    // rand()% [INTERVALO +1]+[MINIMO] cuando el número aleatorio debe estar entre 2 valores

    //Reservo el array dinámico con los índices de cada habitante, y compruebo que no revase la memoria o que halla fallo en la reserva.
    //Además creo un array con los índices para luego poder controlar si se ha creado o no el hilo de ese habitante  
    if ((habitantes_id = (int *) malloc(sizeof(int) * configuracion_inicial[0]))== NULL){
		puts("Error de memoria\n");
		exit(1);
    }
    for(int i = 0; i < configuracion_inicial[0]; i++) {
		habitantes_id[i] = i+1;// De esta manera empiezo en 1 hasta el nº total de habitantes, no en 1 menos todo
    }
    
    // La 1º sección, los habitantes se irán colocando en los diferentes centros de vacunación
    // La 2º sección, los habitantes serán vacunados y los que no puedan serlo por falta de vacunas se pondrán a la espera
    // Por otro lado, tendrá 3 hilos que corresponden a la fabricación de vacunas
    // Para crear un hilo --> pthread_create(pthread_t *tid, pthread_attr_t *attr,void *funcion, void *param)
    // Seguramente necesitemos mutex/semaforos, por lo que mirar en teoría la cabecera

}



void *habitante(void * num){	
	int fil_id = *(int *)num;
    int aleatorio;
	
    sleep(rand() % (configuracion_inicial[7]) + (1));
    aleatorio = (rand() % (5) + (1));
    pthread_mutex_lock(&centros_vacunacion[aleatorio]);
    printf("Habitante %d elige centro de vacunacion %d\n", fil_id,aleatorio);
    centros_vacunacion[aleatorio].lista_espera += 1;
    pthread_mutex_unlock(&centros_vacunacion[aleatorio]); 
    sleep(rand() % (configuracion_inicial[8]) + (1));
    //
    pthread_mutex_lock(&centros_vacunacion[aleatorio]); // Espera para poder modificar el estado		
		while(centros_vacunacion[aleatorio].vacunas_disponibles < 1)
			pthread_cond_wait(&centros_vacunacion[aleatorio], &mutex);
    printf("Habitante %d vacunado en el centro %d\n", fil_id,aleatorio);
    centros_vacunacion[aleatorio].lista_espera -= 1;
    centros_vacunacion[aleatorio].vacunas_disponibles -= 1;
    pthread_mutex_unlock(&centros_vacunacion[aleatorio]); 



}

void *fabrica(void * num){

    //pthread_cond_signal(&espera[fil_id]);
};

void configuracion (char * entrada, char * salida){
    char buffer[1024];
    int contador = 0;
    int fdescriptor = open(entrada, O_RDONLY); 
    //Realizamos los cambios en la tabla de descriptores necesarias para la entrada
    if (fdescriptor == -1){
        fprintf(stderr, "%s: Error asociado al descriptor de entrada: %s.\n", entrada, strerror(errno));
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
    // Descontamos las vacunas que ya existen en los centros de vacunación de las totales a fabricar
    vacunas_a_fabricar = configuracion_inicial[0] - (CENTROS_VACUNACION * configuracion_inicial[1]);
    // Damos las vacunas iniciales a cada centro
    for (int i = 0; i < CENTROS_VACUNACION; i++){
        centros_vacunacion[i].vacunas_disponibles = configuracion_inicial[i];
        centros_vacunacion[i].id_centro = i+1; 
    }
}
void impresion_configuracion(int configuracion []){
    printf("VACUNACIÓN EN PANDEMIA: CONFIGURACIÓN INICIAL\n");
    printf("Habitantes: %d\n", configuracion[0]);
    printf("Centros de Vacunación: %d \n", CENTROS_VACUNACION);
    printf("Fábricas de Vacunas: %d\n", FABRICAS);
    printf("Vacunados por tanda: %d\n", configuracion[0]/10);
    printf("Vacunas Iniciales en cada Centro: %d\n",configuracion[1]);
    printf("Vacunas totales por fábrica: %d\n", vacunas_a_fabricar/FABRICAS);
    printf("Mínimo número de vacunas fabricadas en cada tanda: %d\n", configuracion[2]);
    printf("Máximo número de vacunas fabricadas en cada tanda: %d\n", configuracion[3]);
    printf("Tiempo mínimo de fabricación de una tanda de vacunas: %d\n", configuracion[4]);
    printf("Tiempo máximo de fabricación de una tanda de vacunas: %d\n", configuracion[5]);
    printf("Tiempo máximo de reparto de vacunas a los centros: %d\n",configuracion[6]);
    printf("Tiempo máximo que un habitante tarda en ver que está citado para vacunarse: %d\n", configuracion[7]);
    printf("Tiempo máximo de desplazamiento del habitante al centro de vacunación: %d\n\n", configuracion[8]);
}