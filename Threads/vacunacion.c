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
#define TANDAS_VACUNACION 10

void configuracion(char * entrada, char * salida);
void impresion_configuracion(int configuracion []);
void impresion_estadisticas();
void * habitante(void *num);
void * fabrica(void *num);
void calc_porcentajes();

// Declaro el centro de vacunación y la fábrica como un tipo de dato para un manejo de los recursos más fácilmente
typedef struct {
    int vacunas_disponibles;
    int lista_espera;
    int vacunados;
} centro_vacunacion;

typedef struct {
    int vacunas_a_fabricar;
    int vacunas_entregadas;
    int centros_entregados[CENTROS_VACUNACION];
} t_fabrica;

// DECLARACIÓN DE VARIABLES GLOBALES

// Para los hilos:
pthread_mutex_t mutex[CENTROS_VACUNACION];
pthread_cond_t espera[CENTROS_VACUNACION];

// Para las fábricas y centros de vacunación:
centro_vacunacion centros_vacunacion[CENTROS_VACUNACION];
t_fabrica fabricas[FABRICAS];
double  porcentajes[CENTROS_VACUNACION];
int habitantes_vacunados,habitantes_por_tanda;

// Para entrada y salida:
int configuracion_inicial[9], vacunas_a_fabricar;
char* entrada_defecto = "entrada_vacunacion.txt";
char* salida_defecto = "salida_vacunacion.txt";


int main(int argc, char * argv[]){
    // Variables usadas por el main del programa
    int * habitantes_id, * fabricas_id;
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

    //Reservo el array dinámico con los índices de cada habitante, y compruebo que no revase la memoria o que halla fallo en la reserva.
    //Además creo un array con los índices para luego poder controlar si se ha creado o no el hilo de ese habitante  
    if ((habitantes_id = (int *) malloc(sizeof(int) * configuracion_inicial[0]))== NULL){
		puts("Error de memoria\n");
		exit(1);
    }
    // Creo mutex y cond igual al nº de centros de vacunación. Esto se debe a que el centro de vacunación va a poseer todos los recursos necesarios a modificar
    //pthread_mutexattr_setrobust;
    for(int i = 0; i < CENTROS_VACUNACION; i++) {
		pthread_mutex_init(&mutex[i],NULL);
        pthread_cond_init(&espera[i], NULL);
    }
    // Creo las fábricas
	for(int i = 0; i < FABRICAS; i++) {
        fabricas_id[i] = i+1;
        pthread_create(&th,NULL,fabrica,(void*)&fabricas_id[i]);
	}
    // Creo los habitantes
    for (int j = 0; j < TANDAS_VACUNACION; j++){
        for(int i = (habitantes_por_tanda * j); i < (habitantes_por_tanda * (j+1)); i++) {
            habitantes_id[i] = i+1;
            pthread_create(&th,NULL,habitante,(void*)&habitantes_id[i]);    
        }
        // Hasta que la tanda anterior no ha sido vacuanda, no puedo mandar a los siguientes habitantes
        while(habitantes_vacunados != habitantes_por_tanda * (j+1)){ 
        }
    }
    // Hasta que toda la pobación no haya sido vacunada, debo esperar a que finalicen todos los hilos
    while (habitantes_vacunados != configuracion_inicial[0]){
    }
    // Destruyo los mutex y mutex_cond
    for(int i = 0; i < CENTROS_VACUNACION; i++) {
		pthread_mutex_destroy(&mutex[i]);
        pthread_cond_destroy(&espera[i]);
    }
    // Imprimo las estadísticas
    printf("\n******** VACUNACIÓN FINALIZADA ********\n\n");
	impresion_estadisticas();
}

void *habitante(void * num){
    // variables usadas por el thread	
	int fil_id = *(int *)num;
    int aleatorio;

    // INICIO

    aleatorio = (rand() % CENTROS_VACUNACION); // El centro de vacunación será entre el 0 - 4
	// El habitante tarda un tiempo random en reaccionar al mensaje de ser vacunado
    sleep(rand() % (configuracion_inicial[7]) + (1));

    // El habitante elige un centro de vacunación y se pone en la lista de espera de ese centro ( de ahí el mutex usado)
    pthread_mutex_lock(&mutex[aleatorio]);

    // SECCIÓN CRÍTICA
    printf("Habitante %d elige centro de vacunacion %d\n", fil_id,aleatorio+1);
    centros_vacunacion[aleatorio].lista_espera += 1;
    // FIN SECCIÓN CRÍTICA
    pthread_mutex_unlock(&mutex[aleatorio]); 

    // El habitante tarda un tiempo random en acudir al centro de vacunación
    sleep(rand() % (configuracion_inicial[8]) + (1));

    // Se va a modificar el nº de vacunas, el nº de vacunados y la lista de espera, por ello el mutex
    pthread_mutex_lock(&mutex[aleatorio]);
    // En caso de que no se disponga de vacunas, se espera a que el centro las reciba de la fábrica --> condición de espera
    while(centros_vacunacion[aleatorio].vacunas_disponibles < 1){
        pthread_cond_wait(&espera[aleatorio], &mutex[aleatorio]);
    }    
    // SECCIÓN CRÍTICA       
    printf("Habitante %d vacunado en el centro %d\n", fil_id,aleatorio+1);
    centros_vacunacion[aleatorio].lista_espera -= 1;
    centros_vacunacion[aleatorio].vacunas_disponibles -= 1;
    centros_vacunacion[aleatorio].vacunados += 1;
    habitantes_vacunados += 1;
    // FIN SECCIÓN CRÍTICA
    pthread_mutex_unlock(&mutex[aleatorio]); 
    

    //Destrucción del hilo, ya que el habitante ya se ha vacunado y salida con 0 al ser correcta
    pthread_exit(0);
}

void *fabrica(void * num){
    // variables usadas por el thread	
    int fil_id = *(int *)num;
    int aleatorio,vacunas_asignadas, vacunas_a_entregar_aux[CENTROS_VACUNACION];
    
    //INICIO

    //Realizo el bucle siempre que queden vacunas por fabricar
    while(fabricas[fil_id-1].vacunas_a_fabricar > 0){
        // En caso de poder tener fallo con el nº de vacunas, establezco que las vacunas fabricadas en la última tanda son exactamente las que le faltan por fabricar a la fábrica
        if (fabricas[fil_id-1].vacunas_a_fabricar < configuracion_inicial[3]){ 
            aleatorio =  fabricas[fil_id-1].vacunas_a_fabricar;
        }else {
        // En otro caso, la fabricación será un nº aleatorio entre los datos de configuración dados    
            aleatorio = rand() % (configuracion_inicial[3] - configuracion_inicial[2] + 1) + configuracion_inicial[2];
        }

        // Tiempo de fabricación de las vacunas
        sleep(rand() % (configuracion_inicial[5] - configuracion_inicial[4] + 1) + configuracion_inicial[4]);
        // Actualizo el stock de mi fábrica
        // Tiempo que tardo en repartir las vacunas a los centros
        sleep(rand() % (configuracion_inicial[6]) + 1);

        // Paro todos los centros de vacunación para ver la lista de espera que tienen en ese momento y poder dar prioridad
        for( int j = 0; j < CENTROS_VACUNACION; j++){
            pthread_mutex_lock(&mutex[j]); 
        }

        // SECCIÓN CRÍTICA
        vacunas_asignadas = 0;
        for( int j = 0; j < CENTROS_VACUNACION; j++){
            // Actualizamos la demanda existente en los centros de vacunación
            if(centros_vacunacion[j].lista_espera > (aleatorio - vacunas_asignadas)){
                vacunas_a_entregar_aux[j] = (aleatorio - vacunas_asignadas);
                vacunas_asignadas = aleatorio;
                break;
            }
            vacunas_a_entregar_aux[j] = centros_vacunacion[j].lista_espera;
            vacunas_asignadas += vacunas_a_entregar_aux[j];
        }// En caso de que sobren vacunas atendiendo a la lista de espera, se reparten aleatoriamente
        while (vacunas_asignadas < aleatorio){
            vacunas_a_entregar_aux[rand() % CENTROS_VACUNACION] += 1;
            vacunas_asignadas += 1;
        }   
        // Ya se sabe como se reparte, y se procede a ello
        printf("Fábrica %d prepara %d vacunas\n", fil_id,aleatorio);
        for( int j = 0; j < CENTROS_VACUNACION; j++){
            for (int i = 0; i < CENTROS_VACUNACION; i++){
               if (vacunas_a_entregar_aux[j] < 0){
                   vacunas_a_entregar_aux[j] = 0;
               }
            }
            printf("Fábrica %d entrega %d vacunas al centro %d \n", fil_id,vacunas_a_entregar_aux[j] ,j+1);
            fabricas[fil_id-1].centros_entregados[j] += vacunas_a_entregar_aux[j];
            centros_vacunacion[j].vacunas_disponibles += vacunas_a_entregar_aux[j];
        }
        // Actualizo la información de las vacunas fabricadas
        fabricas[fil_id-1].vacunas_a_fabricar -= aleatorio;
        fabricas[fil_id-1].vacunas_entregadas += aleatorio;
        // Doy las señales de que ya hay nuevas vacunas para el mutex_cond
        for( int i = 0; i < CENTROS_VACUNACION; i++){
            pthread_cond_broadcast(&espera[i]); 
        }

        // FIN SECCIÓN CRÍTICA
        for( int i = 0; i < CENTROS_VACUNACION; i++){
            pthread_mutex_unlock(&mutex[i]); 
        }        
    }
    printf("La fábrica %d ya ha terminado su producción\n", fil_id);

    //Destrucción del hilo, ya que el habitante ya se ha vacunado y salida con 0 al ser correcta
    pthread_exit(0);
}


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
        centros_vacunacion[i].vacunas_disponibles = configuracion_inicial[1];
        centros_vacunacion[i].vacunados = 0;
        centros_vacunacion[i].lista_espera = 0; 
    }
    // Damos los valores iniciales a cada fábrica
    for (int i = 0; i < FABRICAS; i++){
        fabricas[i].vacunas_entregadas = 0;
        fabricas[i].vacunas_a_fabricar = vacunas_a_fabricar/FABRICAS;
    }
    //Damos los valores iniciales a los porcentajes
    for (int i = 0; i < CENTROS_VACUNACION; i++){
        porcentajes[i] = 0;
    }
    // Damos valor a la variable usada para el control de la generación de tandas
    habitantes_por_tanda = configuracion_inicial[0]/TANDAS_VACUNACION;

}

void impresion_configuracion(int configuracion []){
    printf("VACUNACIÓN EN PANDEMIA: CONFIGURACIÓN INICIAL\n");
    printf("Habitantes: %d\n", configuracion[0]);
    printf("Centros de Vacunación: %d \n", CENTROS_VACUNACION);
    printf("Fábricas de Vacunas: %d\n", FABRICAS);
    printf("Vacunados por tanda: %d\n", habitantes_por_tanda);
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


void impresion_estadisticas(){
    for(int i = 0; i< CENTROS_VACUNACION; i++){
        printf("Se han vacunado en el Centro %d: %d \n",i+1,centros_vacunacion[i].vacunados);
    }

    
    for(int i = 0; i< FABRICAS; i++){
        printf("La fábrica %d ha fabricado %d vacunas \n",i+1,vacunas_a_fabricar/FABRICAS);   
        for(int j = 0; j < CENTROS_VACUNACION; j++){
            printf("La fábrica %d ha entregado al centro %d: %d vacunas \n",i+1,j+1,fabricas[i].centros_entregados[j]);
        }
    }
}
