#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "executa.h"
#include "SmartArray.h"
#include "argus.h"

int nrTaf=1;
int max_time=0;
int pipe_time=0;
int pipes[2];
struct SmartArrayS* running;
int * pids;
int oo;

//Handler utilizado na listar
void remove_handler(int signum){
      int id;
      oo=read(pipes[0],&id,sizeof(id));
      removeElem(running,getIndexArr(running,id));
    }


//Função de ler uma linha adaptada a dependencias de outras funções
ssize_t readln2(int fd,char *linha,size_t nbyte){
    ssize_t res = 0;
    ssize_t i = 0,j=0;
    char buffer_aux[4096];
    while((res = read(fd,&buffer_aux[i],nbyte))>0 ){
        for (;i<res;i++){
            if( ((char*) buffer_aux)[i] != '\n' ){
                linha[j] = buffer_aux[i];
                j++;
            }else{
              linha[j] ='$';
              j++;
            }

        }

    }
    return j;
}

//Função com o objetivo de escrever no ficheiro o motivo pelo qual a tarefa terminou
 void atualizaModo(char *modo, int output_executa){
    switch (output_executa) {
      case 1:
      strcpy(modo,"max execução");
      break;
      case 2:
      strcpy(modo,"max inactividade");
      break;
      case 3:
      strcpy(modo,"terminada");
      break;
      default:
      strcpy(modo,"concluida");
    }
  }


void startServer(){
   int client_to_server, server_to_client, n, pids_criados=0;
   running = initSmartArrayS();
   pids = (int*)malloc(sizeof(int) * STRING);

   const char *myfifo = "fifoF1";
   char myfifo2[100];
   mkfifo(myfifo, 0666);
   client_to_server = open(myfifo, O_RDONLY);

   if(pipe(pipes)<0){
       perror("pipes");
       exit(1);
       }

        while(1) {

       int i=0, k=0, quantos=0,size_flag;
       char* buf = (char*)malloc(sizeof(char*) * STRING);
       char* line = (char*)malloc(sizeof(char*) * STRING);
       char* pointer_space=NULL;
       char* aux = NULL;
       char* fo=NULL;
       char * queresver=NULL;
       char* pointer_barra = NULL;
       int buffer_size = 4096;
       char buffer[buffer_size];
       char* strTaf = (char*)malloc(sizeof(char*) * STRING);
       char* ajuda = (char*)malloc(sizeof(char*) * STRING);
       char** token_space = (char**)malloc(sizeof(char*) * STRING);
       char** token_pipes = (char**)malloc(sizeof(char*) * STRING);
       char** novo = (char**)malloc(sizeof(char*) * STRING);
       char modo[20];
       char ls[100];

         if((n = read(client_to_server,buf,STRING)) > 0) {

           fo=strdup(buf);
           //Guardar a string recebida em array utilizando como token espaço
           while(((pointer_space = strsep(&fo," ")) != NULL)) {
             token_space[i++] = strdup(pointer_space);
                }

                //Interpretação da string recebida
                int pid = atoi(token_space[0]);
                int size_userStr=atoi(token_space[1]);
                int metodoIput= atoi(token_space[2]);

                if(metodoIput==1){
                    size_flag = sizeof(char)*strlen(token_space[3]);
                  } else{
                    size_flag = sizeof(char)*strlen(token_space[4]) + sizeof(char)*strlen(token_space[3])+1;
                  }

          //Garante que o próximo token recebe a informação que necessita: a | b | c...
           aux=strdup(buf+(n-(size_userStr-size_flag)));
           queresver=strdup(aux);

           //Guardar a string recebida em array utilizando como token barra
           while(((pointer_barra = strsep(&aux,"|")) != NULL)) {
                token_pipes[k++] = strdup(pointer_barra);
                }
                quantos=k;


           //nomeia o fifo de resposta
           sprintf(myfifo2,"PID%d",pid);
           server_to_client=open(myfifo2,O_WRONLY);

           //Interpretação da flag introduzida pelo cliente
           //Tempo de inactividade
           if(((metodoIput==1) && (strcmp(token_space[3],TEMP_INAC)==0)) || ((metodoIput==0) && (strcmp(token_space[4],"-i")==0)))  {
               pipe_time = atoi(token_space[4+(1-metodoIput)]);
               sprintf(ls,"Tempo máximo de inactividade definido com sucesso! $\n\n");
               oo=write(server_to_client,ls,strlen(ls));
               close(server_to_client);
            }

            //Tempo de execucao
            if(((metodoIput==1) && (strcmp(token_space[3],TEMP_EXEC)==0)) || ((metodoIput==0) && (strcmp(token_space[4],"-m")==0)))  {
              max_time = atoi(token_space[4+(1-metodoIput)]);
              sprintf(ls,"Tempo máximo de execução definido com sucesso! $\n\n");
              oo=write(server_to_client,ls,strlen(ls));
              close(server_to_client);
            }

            //Executar
            if(((metodoIput==1) && (strcmp(token_space[3],EXEC)==0)) || ((metodoIput==0) && (strcmp(token_space[4],"-e")==0))) {

              if (signal(SIGUSR2, remove_handler) == SIG_ERR){
                      perror("ocorreu um erro no sinal de USR2");
                    }

                int bytes_read=0;
                char temp[STRING];
                char sender[100];

                //Adicionar ao array dinâmico
                sprintf(temp,"#%d: %s$",nrTaf,queresver);
                addToArrayS(running, temp);

                //Envio mensagem ao cliente
                sprintf(sender,"nova tarefa #%d$\n\n",nrTaf);
                oo=write(server_to_client,sender,strlen(sender));
                close(server_to_client);

                //É criado um filho para cada tarefa a executar
                if((pids[pids_criados++]=fork())==0){

                  int write_to_file[2], output_executa=0;

                    if(pipe(write_to_file)<0){
                        perror("pipe[0]");
                        exit(1);
                        }

                  //Escrita do output da executa para o ficheiro output.txt
                  int fd_w=open("output.txt",O_CREAT | O_WRONLY | O_APPEND,0700);
                  dup2(fd_w,1);
                  output_executa=executa(quantos,token_pipes,max_time,pipe_time);
                  close(fd_w);
                  atualizaModo(modo,output_executa);

                  //Construção da string para escrita no ficheiro tarefas.txt
                  snprintf(strTaf, STRING,"%c%d%c %s%c %s %s",'#',nrTaf,',',modo,':',queresver,"\n");
                  oo=write(write_to_file[1],strTaf,(strlen(strTaf)*sizeof(char)));
                  close(write_to_file[1]);

                  //Escrita no ficheiro tarefas.txt por intermédio de um pipe
                  int fd_tarefa_w= open("tarefas.txt",O_CREAT | O_WRONLY | O_APPEND,0700);
                  while((bytes_read=read(write_to_file[0],&buffer,buffer_size))>0){
                        oo=write(fd_tarefa_w,&buffer,bytes_read);
                        }
                  close(fd_tarefa_w);
                  close(write_to_file[0]);
                  oo=write(pipes[1],&nrTaf,sizeof(nrTaf));

                  //Envia sinal para remover do array dinâmico
                  kill(getppid(),SIGUSR2);
                  exit(0);
                  }
                  nrTaf++;
              }


            // Listar tarefas em execucao
            if(((metodoIput==1) && (strcmp(token_space[3],LISTAR)==0)) || ((metodoIput==0) && (strcmp(token_space[4],"-l")==0)))  {
              if(running->tam > 0){
                char strBuf[STRING] = "";
                //Concatena a informação do array numa unica string para ser enviada para o cliente
                for(int i = 0; i<running->tam; i++){
                  strcat(strBuf,running->array[i]);
                }
                strcat(strBuf,"$");
                oo=write(server_to_client,strBuf,STRING);
                close(server_to_client);
              }
              else{
                oo=write(server_to_client,"Não há tarefas a correr\n",26);
                close(server_to_client);
              }
            }

            // Terminar uma dada tarefa
            if(((metodoIput==1) && (strcmp(token_space[3],TERM)==0)) || ((metodoIput==0) && (strcmp(token_space[4],"-t")==0)))  {
              int tarTerm = atoi(token_space[4+(1-metodoIput)]);
              //verifica se número de tarefa existe
              if((tarTerm >0) && tarTerm<=pids_criados){
                kill(pids[tarTerm-1],SIGINT);

                oo=write(server_to_client,"Tarefa terminada!\n",26);
                close(server_to_client);
                }else{
              oo=write(server_to_client,"Erro!Tarefa inválida!\n",26);
              close(server_to_client);
                }
            }


            // Histórico de tarefas
            if(((metodoIput==1) && (strcmp(token_space[3],HIST)==0)) || ((metodoIput==0) && (strcmp(token_space[4],"-r")==0)))  {
               int fd_r_hist=open("tarefas.txt",O_RDONLY);
               //Lê do ficheiro tarefas.txt todas as tarefas lá presentes e envia ao cliente
               int nrlidos= readln2(fd_r_hist,line,STRING);
                  line[nrlidos++]='\n';
                  oo=write(server_to_client,line,nrlidos);

                  close(fd_r_hist);
                  close(server_to_client);
            }

            // Ajuda
            if((metodoIput==1) && (strcmp(token_space[3],AJU)==0))  {
             snprintf(ajuda, STRING,"%s %s$%s %s$%s %s$%s$%s %s$%s$\n",TEMP_INAC,"segs",TEMP_EXEC,"segs",EXEC,"p1 | p2 ... | pn",LISTAR,TERM,"l",HIST);
             oo=write(server_to_client,ajuda,strlen(ajuda));
             close(server_to_client);
            }


            if((metodoIput==0) && (strcmp(token_space[4],"-h")==0))  {

             snprintf(ajuda, STRING,"%s: %s %s$%s: %s %s$%s: %s %s$%s: %s$%s: %s %s$%s: %s$\n",TEMP_INAC,"-i","n",TEMP_EXEC,"-m","n",EXEC,"-e","p1 | p2 ... | pn",LISTAR,"-l",TERM,"-t","n",HIST,"-r");
             oo=write(server_to_client,ajuda,strlen(ajuda));
             close(server_to_client);
            }
            else{
              oo=write(server_to_client,"Comando inválido! Insira novamente.\n",50);
              close(server_to_client);
            }
            memset(buf,0,n);

      }
      for(int tam=0;tam<STRING;tam++){
        free(token_pipes[tam]);
        free(token_space[tam]);
        free(novo[tam]);
      }
      free(pointer_space);
      free(buf);
      free(aux);
      free(ajuda);
      free(line);
      free(pointer_barra);
      free(queresver);
      free(fo);
      free(strTaf);

     }
     free(pids);
     freeSmartArrayS(running);

     close(client_to_server);
     unlink(myfifo);
     unlink(myfifo2);

}



int main() {
  startServer();
  return EXIT_SUCCESS;
}
