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
void redireccion_bg(tline *linea);
void redireccion_1comando(tline *linea);
void redireccion_varios_comandos( tline * linea);
void comando_cd(tline *linea);
void comando_jobs(tline *linea);
void comando_fg(tline *linea);

int main(void){   
    // Declarar variables 
    char buffer[1024],buffer_cwd[1024];
    tline * linea_leida;
    int fallo_comand_novalido = 0;
    //Ignorar las señales
    signal(SIGINT, SIG_IGN);
    signal (SIGQUIT, SIG_IGN);
    //Guardar valores originales de las Entradas y Salidas estándar
    int red_entrada = dup(fileno(stdin));
    int red_salida = dup(fileno(stdout));
    int red_error = dup(fileno(stderr));
    //Imprimir desde el directorio en el que ejecutamos la Minishell
    getcwd(buffer_cwd,1024);
    printf("%s/msh> ",buffer_cwd);	
    //Bucle a la escucha de lo que le entra
    while (fgets(buffer, 1024, stdin)) {
        linea_leida = tokenize(buffer);
        if (linea_leida == NULL) {
		continue;
	}if (linea_leida->redirect_input != NULL) {
		printf("Redirección de entrada: %s\n", linea_leida->redirect_input);
            redireccion_entrada(linea_leida);
	}if (linea_leida->redirect_output != NULL) {
		printf("Redirección de salida: %s\n", linea_leida->redirect_output); 
    		redireccion_salida(linea_leida); 
	}if (linea_leida->redirect_error != NULL) {
		printf("Redirección de error: %s\n", linea_leida->redirect_error);
    		redireccion_error(linea_leida);
	}if (linea_leida->ncommands >=1){
            //Comprobación de mandatos internos de la Bash que se piden desarrollar
            if(strcmp(linea_leida->commands[0].argv[0],"cd") == 0){
                comando_cd(linea_leida);  
            }else if(strcmp(linea_leida->commands[0].argv[0],"fg") == 0){
                comando_fg(linea_leida);
            }else if(strcmp(linea_leida->commands[0].argv[0],"jobs") == 0){
                comando_jobs(linea_leida);    
            //Comprobación de mandatos externos de la Bash
            }else{
                for (int i = 0; i<linea_leida->ncommands; i++) {
                    //Comprobamos con las rutas de los mandatos que estos son válidos
                    if(linea_leida->commands[i].filename == NULL){
                        fallo_comand_novalido = 1;    
                    }
                }if (fallo_comand_novalido != 1){
                    if (linea_leida->ncommands == 1){
                        redireccion_1comando(linea_leida);
                    }else{
                        redireccion_varios_comandos(linea_leida);  
                    }
                }else { fprintf(stderr, "Error, algún comando introducido es erróneo: %s.\n", strerror(errno)); }
            fallo_comand_novalido = 0;
            }
        }
        // Restablecer los descriptores por si han sido modificados 		
	if(linea_leida->redirect_input != NULL ){
		dup2(red_entrada ,0);	
	}
	if(linea_leida->redirect_output != NULL ){
		dup2(red_salida ,1);	
	}
	if(linea_leida->redirect_error != NULL ){
		dup2(red_error ,2);	
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
            fprintf(stderr, "%s: Error, asociado al descriptor de entrada: %s.\n", linea->redirect_input, strerror(errno));
            exit (1);
        }else{
            // No hay error y se escribe el descriptor
            dup2(fdescriptor,0);
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
            fprintf(stderr, "%s: Error, asociado al descriptor de salida: %s.\n", linea->redirect_output, strerror(errno));;
            exit (1);
        }else{
            // No hay error y se escribe el descriptor
            dup2(fdescriptor,1);
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
            fprintf(stderr, "%s: Error, asociado al descriptor de error: %s.\n", linea->redirect_error, strerror(errno));;
            exit (1);
        }else{
            // No hay error y se escribe el descriptor
            dup2(fdescriptor,2);
            close(fdescriptor);
        }
    }    
}
void redireccion_bg(tline * linea){
    if(linea->background == 1){
        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
    }else{
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT,SIG_DFL);    
    }
}
void redireccion_1comando(tline *linea){
    pid_t pid;
    int status;
    pid = fork();
    if (pid < 0){
        fprintf(stderr, "Fallo al ejecutar el fork() : %s\n", strerror(errno));
        exit(1);
    }else if(pid == 0){ // Código del hijo
        //Cambio las señales en el hijo, dado que el Padre tiene que mantener ignorando las SIGINT y SIGQUIT
        redireccion_bg(linea);
        execvp(linea->commands[0].filename, linea->commands->argv);
         // Si ejecuta esta parte del código, implica fallo en el execvp
        fprintf(stderr, "Error al ejecutar el comando %s : %s\n", linea->commands[0].argv[0] , strerror(errno));
        exit(1);
    } 
    wait(&status);
} 
void redireccion_varios_comandos(tline *linea){
    pid_t all_pids[linea->ncommands];
    int i,pipes[linea->ncommands - 1][2];
    for( i = 0; i < linea->ncommands - 1; i++){
        if(pipe(pipes[i]) < 0){
            fprintf(stderr, "Falló crear el pipe %s/n" , strerror(errno));
        }    
    }
    for( i = 0; i < linea->ncommands; i++){
        all_pids[i] = fork();
        if(all_pids[i] < 0){
            fprintf(stderr, "Falló el fork() %s\n" , strerror(errno));
            exit(1);
        } else if(all_pids[i] == 0){
            //Si el mandato se ejecuta en bg, el bg afecta a todos los procesos 
            redireccion_bg(linea);
            for(int j=0; j<(linea->ncommands - 1); j++) {
                //Comprobamos en que hijo está, y por tanto, que pipes puede cerrar completamente
                if( ((j != i && j != (i-1))) || ((j != i && j != (i - 1))) && (((linea->ncommands - 1) == i) && (j < i))) {
                    close(pipes[j][1]);
                    close(pipes[j][0]);
                }
            }
            //En caso de que hijo sea, cierra la I/O de las pipes que no interesa, y reescribe en el descriptor que use   
            if(i == 0){
                close(pipes[0][0]);
                dup2(pipes[0][1],1);
            }else if(i > 0 && i < (linea->ncommands - 1)){  
                close(pipes[i-1][1]);
                close(pipes[i][0]);
		dup2(pipes[i-1][0],0);
		dup2(pipes[i][1],1);   
            }else if((linea->ncommands - 1) == i){
                close(pipes[i-1][1]);
                dup2(pipes[i-1][0],0);   
            }
            //Común a todos los procesos, la ejecución y, si lo hubiese, nos indica el fallo   
            execvp(linea->commands[i].filename, linea->commands[i].argv); 
		    fprintf(stderr,"%s: Error al ejecutar el mandato en el proceso hijo\n",linea->commands[i].filename);
            exit(1);
        }
    }
    //Cerramos todos los pipes en el padre
    for(i = 0; i <linea->ncommands - 1; i++){ 
        close(pipes[i][1]);
        close(pipes[i][0]);
    }
    //Esperamos a que todos los hijos terminen
    for(i = 0; i < linea->ncommands; i++){ 
        waitpid(all_pids[i],NULL,0);
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

}

void comando_jobs(tline * linea){}
