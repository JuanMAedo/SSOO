#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "parser.h"
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>


void redireccion_entrada(tline *linea);
void redireccion_salida(tline *linea);
void redireccion_error(tline *linea);
void redireccion_background(tline *linea);
void redireccion_comando(tline *linea);
void comando_cd(tline *linea);
void comando_jobs(tline *linea);
void comando_fg(tline *linea);

int main(void){
    
    // Declarar variables 
    char buffer[1024];
    char buffer_cwd[1024];
    tline * linea_leida;
    int i;
    int fallo_comand_novalido = 0;

    //Ignorar las señales
    signal(SIGINT, SIG_IGN);
	signal (SIGQUIT, SIG_IGN);

    //Imprimir desde el directorio en el que ejecutamos la Minishell
    getcwd(buffer_cwd,1024);
    printf("%s/msh> ",buffer_cwd);	

    //Bucle a la escucha de lo que le entra
	while (fgets(buffer, 1024, stdin)) {

        linea_leida = tokenize(buffer);

        if (linea_leida == NULL) {
			continue;
		}if (linea_leida->redirect_input != NULL) {
			printf("redirección de entrada: %s\n", linea_leida->redirect_input);
            redireccion_entrada(linea_leida);
		}if (linea_leida->redirect_output != NULL) {
            redireccion_salida(linea_leida);
			printf("redirección de salida: %s\n", linea_leida->redirect_output);  
		}if (linea_leida->redirect_error != NULL) {
			printf("redirección de error: %s\n", linea_leida->redirect_error);
            redireccion_error(linea_leida);
		} if (linea_leida->ncommands >=1){
            //Comprobación de mandatos internos de la Bash que se piden desarrollar
            if(strcmp(linea_leida->commands[0].argv[0],"cd") == 0){
                comando_cd(linea_leida);  
            }else if(strcmp(linea_leida->commands[0].argv[0],"fg") == 0){
                comando_fg(linea_leida);
            }else if(strcmp(linea_leida->commands[0].argv[0],"jobs") == 0){
                comando_jobs(linea_leida);    
            }   
            //Comprobación de mandatos externos de la Bash
            else{
                for (i=0; i<linea_leida->ncommands; i++) {
                    //Comprobamos con las rutas de los mandatos que estos son válidos
                    if(linea_leida->commands[i].filename == NULL){
                        fallo_comand_novalido = 1;    
                    }
                }if (fallo_comand_novalido != 1){
                    redireccion_comando(linea_leida);
                }else{
                    fprintf(stderr, "Error, algún comando introducido es erróneo; %s.\n", strerror(errno));
                }
                fallo_comand_novalido = 0;
            }
        }
        //Imprimir desde el directorio en el que ejecutamos la Minishell
        getcwd(buffer_cwd,1024);
		printf("%s/msh> ",buffer_cwd);	
    }
    return 0;
}

void redireccion_entrada(tline *linea){
    // Comprobamos que exista la redirección
    if(linea->redirect_input != NULL){
       // Abrimos el descriptor del mandato con la redirección
        int fdescriptor = open(linea->redirect_input, O_RDONLY); 
        //Comprobamos posible error en la redirección
        if (fdescriptor == -1){
            // Error al abrir el descriptor
            fprintf(stderr, "%s: Error, asociado al descriptor de entrada: %s.\n", linea->redirect_input, strerror(errno));
            exit (1);
        }else {
            // No hay error y se escribe el descriptor
            dup2(fdescriptor,0);
            // Cerramos el descriptor
            close(fdescriptor);
        }
    }
}

void redireccion_salida(tline * linea){
    // Comprobamos que exista la redirección
    if(linea->redirect_output != NULL){
       // Abrimos el descriptor del mandato con la redirección
        int fdescriptor = open(linea->redirect_output, O_CREAT | O_APPEND | O_WRONLY, S_IRUSR); 
        //Comprobamos posible error en la redirección
        if (fdescriptor == -1){
            // Error al abrir el descriptor
            fprintf(stderr, "%s: Error, asociado al descriptor de salida: %s.\n", linea->redirect_output, strerror(errno));;
            exit (1);
        }else {
            // No hay error y se escribe el descriptor
            dup2(fdescriptor,1);
            // Cerramos el descriptor
            close(fdescriptor);
        }
    }   
}

void redireccion_error(tline * linea){
    // Comprobamos que exista la redirección
    if(linea->redirect_error != NULL){
       // Abrimos el descriptor del mandato con la redirección
        int fdescriptor = open(linea->redirect_error, O_WRONLY | O_APPEND | O_CREAT , S_IRUSR); 
        //Comprobamos posible error en la redirección
        if (fdescriptor == -1){
            // Error al abrir el descriptor
            fprintf(stderr, "%s: Error, asociado al descriptor de error: %s.\n", linea->redirect_error, strerror(errno));;
            exit (1);
        }else {
            // No hay error y se escribe el descriptor
            dup2(fdescriptor,2);
            // Cerramos el descriptor
            close(fdescriptor);
        }
    }    
}

void redireccion_background(tline * linea){

}

void redireccion_comando(tline *linea){

    printf("Comando válido\n");
    if (linea->background) {
        printf("Ejecuta en background\n");
    }
}

void comando_cd(tline * linea){
    //En caso de no pasar argumentos, cd se posiciona en $HOME
    if(linea->commands[0].argc == 1){
        chdir(getenv("HOME"));
    }else if( chdir(linea->commands[0].argv[1]) != 0){
        fprintf(stderr, "Error al ejecutar '%s %s' : %s\n" , linea->commands[0].argv[0],linea->commands[0].argv[1], strerror(errno));
    }
}

void comando_fg(tline * linea){
    pid_t pid_aux;
    pid_aux = linea->commands[0].argv[1];
    if (pid_aux == 0){
        //Hacer fg sobre el último mandato en bg
    }else if (//recorrer lista y si coincide con pid_aux){
        
    }else{ // El recorrido nos indica que no corresponde el pid a ningún proceso en bg existente
        printf(stderr, "Error al ejecutar '%s %s' : %s\n" , linea->commands[0].argv[0],linea->commands[0].argv[1], strerror(errno));
    }
}

void comando_jobs(tline * linea){}