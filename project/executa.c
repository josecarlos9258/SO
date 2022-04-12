#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <string.h>
#include "executa.h"


int filhos_criados=0;
int monitores_criados = 0;
int * pids;
int * monitores; // vai guardar os PID dos monitores de pipes

int modo_terminacao = 0;  // 0 terminou normalmente    1 excedeu limite tempo     2 pipe inativo demasiado tempo    3 foi terminado
int oo;

void tempo(int tempo){
	if(tempo>0) alarm(tempo);
}

void mata_filhos(){

	for(int i=0; i< filhos_criados; i++){ // mata os filhos criados
		if(pids[i]>0){
			kill(pids[i],SIGKILL);
		}
	}

	for(int i=0; i< monitores_criados; i++){ // mata os filhos criados
		if(monitores[i]>0){
			kill(monitores[i],SIGKILL);
		}
	}
}


//------------------------------------------------DEFINICAO DOS HANDLERS---------------------------------------

void timeout_handler(int signum){
	if(modo_terminacao==0) modo_terminacao = 1;
	mata_filhos();
}

void pipe_timeout_handler(int signum){
	if(modo_terminacao==0) modo_terminacao = 2;
	mata_filhos();
}

void monitor_handler(int signum){
	kill(getppid(),SIGUSR1);  // envia o sinal de pipe excedeu limite de tempo ao pai
}


void terminar_handler(int signum){
	if(modo_terminacao==0) modo_terminacao = 3;
	mata_filhos();
}










char** tokenize(char* input){
    char** args = (char**) malloc(sizeof(char*));
    char* temp = strdup(input);//vai receber por exemplo cut -f7
	char* token = strtok(temp," ");
	int r=0;

	 for(int i = 0;token != NULL; i++){
        args[i] = token;
        token = strtok(NULL," ");
        args = realloc(args,sizeof(char*) * (i+2));
        r=i;
    }
    args[r+1]=NULL;
    free(token);
    return args;
}









int executa(int num_comandos, char * argv[], int max_time, int pipe_time){ // argv[] vai ser um array por exemplo [p1 ...  ,  p2 ...  , p3 ...]

	tempo(max_time); //recebe o tempo maximo de execucao

	pids = (int*)malloc(sizeof(int) * num_comandos);
	monitores = (int*)malloc(sizeof(int) * (num_comandos-1));


	char ** argumentos;
	int pipe_fd[num_comandos-1][2]; // vamos ter 'num_comandos-1' pipe
	int pipes_monitores [num_comandos-1][2]; //vai guardar os pipes usados pelos monitores para enviarem o seu output
	int status;
	char * buffer[1024];



	//Handlers ---------------------------------------------------------------------------------------------
	if (signal(SIGALRM, timeout_handler) == SIG_ERR){
		perror("ocorreu um erro no sinal de alarm");
		exit(1);
	}
	if (signal(SIGUSR1, pipe_timeout_handler) == SIG_ERR){
		perror("ocorreu um erro no sinal de USR1");
		exit(1);
	}

	if (signal(SIGINT, terminar_handler) == SIG_ERR){
		perror("ocorreu um erro no sinal de INT");
		exit(1);
	}

	//Caso em que so temos 1 comando
	if(num_comandos == 1){
		if((pids[filhos_criados++]=fork())==0){
			argumentos = tokenize(argv[0]);
			execvp(argumentos[0], argumentos);
			_exit(1);
		}

			//Verifica se sai ou nao
		if(modo_terminacao!=0){
			free(pids);
			free(monitores);
			return modo_terminacao;
		}

	}

	



    //________________________________________________CASO GERAL____________________________________________________________________________
	//Caso em que temos mais que 1 comando (caso geral)
	else{

		//Criamos pipe 0
		if(pipe(pipe_fd[0])<0){
			perror("pipe[0]");
			exit(1);
		}

		//Criamos o pipe do monito 0
		if(pipe(pipes_monitores[0])<0){
			perror("pipe[0]");
			exit(1);
		}


		// Crio o 1º filho
		if( (pids[filhos_criados++]=fork()) ==0){

			close(pipe_fd[0][0]); // nao precisamos de ler do primeiro pipe
			dup2(pipe_fd[0][1],1);  //vai escrever para o primeiro pipe
			close(pipe_fd[0][1]);
			argumentos = tokenize(argv[0]);
			execvp(argumentos[0], argumentos); // o output disto é escrito no pipe
			_exit(1);

		}

		close(pipe_fd[0][1]);

		//Crio o 1º monitor
		if( (monitores[monitores_criados++]=fork()) ==0){

			if (signal(SIGALRM, monitor_handler) == SIG_ERR){
				perror("ocorreu um erro no sinal de alarm");
				exit(1);
			}
			int bytes_read=0;

			tempo(pipe_time);
			while( (bytes_read=read(pipe_fd[0][0],&buffer,1024))>0 ){ // vou ler do do pipe para o buffer
			 	alarm(0); // cancelo o alarm
				oo=write(pipes_monitores[0][1],buffer,bytes_read); //escrevo para o pipe do monitor
			}
			_exit(1);

		}

		close(pipe_fd[0][0]);
		close(pipes_monitores[0][1]);


		waitpid(pids[filhos_criados-1],&status,0);
		waitpid(monitores[monitores_criados-1],&status,0);


		//Verifica se sai ou nao
		if(modo_terminacao!=0){
			free(pids);
			free(monitores);
			return modo_terminacao;
		}




		for(int i = 1; i<num_comandos-1; i++){  //Vai ate ao penultimo comando num_comandos-2

			//Criamos o pipe
			if(pipe(pipe_fd[i])<0){
				perror("pipe");
				exit(1);
			}
			//Criamos o pipe do monitor
			if(pipe(pipes_monitores[i])<0){
				perror("pipe[0]");
				exit(1);
			}


			//Crio o filho
			if((pids[filhos_criados++]=fork())==0){

				dup2(pipes_monitores[i-1][0],0); //vou ler do pipe anterior
				close(pipes_monitores[i-1][0]); // fecha descritor de leitura do pipe anterior

				dup2(pipe_fd[i][1],1); // vou escrever no proximo pipe
				close(pipe_fd[i][1]); //fecha o descritor de escrita do pipe anterior
				argumentos = tokenize(argv[i]); // vai receber aqui um comando e os seus argumentos e parti los --- tokenizer
				execvp(argumentos[0], argumentos);

				_exit(1);
			}

			close(pipes_monitores[i-1][0]); // ja ngm precisa de ler deste pipe
			close(pipe_fd[i][1]); // fecha o descritor de escrita do pipe anterior


			//Crio o monitor
			if((monitores[monitores_criados++]=fork())==0){

				if (signal(SIGALRM, monitor_handler) == SIG_ERR){
					perror("ocorreu um erro no sinal de alarm");
					exit(1);
				}
				int bytes_read=0;

				tempo(pipe_time);

				while( (bytes_read=read(pipe_fd[i][0],&buffer,1024))>0 ){ // vou ler do do pipe para o buffer
				 	alarm(0); // cancelo o alarm
					oo=write(pipes_monitores[i][1],buffer,bytes_read); //escrevo para o pipe do monitor
				}
				_exit(1);
			}

			close(pipe_fd[i][0]);
			close(pipes_monitores[i][1]);


			waitpid(pids[i],&status,0);
			waitpid(monitores[i],&status,0);

			//Verifica se sai ou nao
			if(modo_terminacao!=0){
				free(pids);
				free(monitores);
				return modo_terminacao;
			}

		}






		//Criamos o ultimo filho
		if((pids[filhos_criados++]=fork())==0){


			dup2(pipes_monitores[num_comandos-2][0],0); //vai ler do ultimo pipe
			close(pipes_monitores[num_comandos-2][0]);

			argumentos = tokenize(argv[num_comandos-1]);   //isto esta a foder o esquema wtff
			execvp(argumentos[0], argumentos);
			_exit(1);
		}
		close(pipes_monitores[num_comandos-2][0]);

		waitpid(pids[filhos_criados-1],&status,0);
		waitpid(monitores[monitores_criados-1],&status,0);


	}


	free(pids);
	free(monitores);


	return modo_terminacao; //executou sem problemas

}
