#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

void configuracion(FILE * entrada, FILE * salida);
// Declaración variables globales
int configuracion_inicial[9];
FILE * entrada, * salida; 

int main(int argc, char * argv){
    if (argc == 1){
        entrada = fopen("entrada_vacunacion.txt",O_RDONLY);
        salida = fopen("salida_vacunacion.txt",O_RDONLY);
        configuracion(entrada, salida);  
        // Uso entrada_vacunacion.txt y salida_vacunacion.txt 
    }else if (argc == 2){
        configuracion(argv[1],"salida_vacunacion.txt");
        // Uso argv[1] y salida_vacunacion.txt
    }else if (argc == 3){
        configuracion(argv[1],argv[2]);
        // Uso argv[1] y argv[2]
    }else{
        fprintf(stderr, "Error, numéro de argumentos incorrecto %s.\n", strerror(errno));
    }
}

void configuracion (FILE * entrada, FILE * salida){
    char * buffer[1024];
    int contador = 0;

    int fdescriptor = open(entrada, O_RDONLY); 
    //Comprobamos posible error en la redirección
    if (fdescriptor == -1){
        fprintf(stderr, "%s: Error, asociado al descriptor de entrada: %s.\n", entrada, strerror(errno));
        exit (1);
    }else{
        // No hay error y se escribe el descriptor
        dup2(fdescriptor,0);
        close(fdescriptor);
    } 
    while(fgets(buffer,1024,stdin)){
        configuracion_inicial[contador]= atoi(buffer); 
        printf("%i\n", configuracion_inicial[contador]);
        contador++;
        
    }
}