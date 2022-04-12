#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include "argus.h"


int oo;
//Função de ler a linha introduzida pelo cliente
ssize_t myreadln(int fildes, void *buf, size_t nbyte){

    char *b = buf;
    size_t i = 0;

    while(i < nbyte){
        ssize_t n = read(fildes,&b[i],1);
        if (n <= 0) break;
        if(b[i] == '\n') return (i+1);
        i++;
    }
    return i;
}


int main( int argc, char *argv[]){
  int client_to_server;
  int server_to_client;
  int n=1,comp=0,j;


  char* temp = (char*)malloc(sizeof(char*) * STRING);
  char* str = (char*)malloc(sizeof(char*) * STRING);
  char str2[STRING];
  char* str3 = (char*)malloc(sizeof(char*) * STRING);
  char** novo = (char**)malloc(sizeof(char*) * STRING);
  char * tok_bar =NULL;
  char * fo=NULL;


  const char *myfifo = "fifoF1";
  char myfifo2[100];


  pid_t pid = getpid();
  sprintf(myfifo2,"PID%d",pid);
  mkfifo(myfifo2, 0666);


  client_to_server = open(myfifo, O_WRONLY);
  //Se método de input é via consola (stdin)
  if(argc==1){
      oo=write(1, "argus$ ", strlen("argus$ "));

      while((j = myreadln(0,str,4096))>0) {
        //Processo filho envia string lida ao servidor
          if(!fork()) {
            int total=strlen(str);
              if(((char*)str)[total-1]=='\n'){
                  ((char*)str)[total-1]='\0';
                }
                comp=strlen(str)*sizeof(char);
                memset(str2,0,STRING);

                snprintf(str2, STRING,"%010d %010d %010d %s",pid,comp,1,str);

                oo=write(client_to_server,str2,strlen(str2));
                close(client_to_server);
                _exit(0);
      } else {
        //Processo pai espera resposta do servidor
              memset(str,0,j);
              int k=0;
              server_to_client = open(myfifo2, O_RDONLY);

                while (1) {
                          n = myreadln(server_to_client,str3,STRING);
                          fo=strdup(str3);
                          //Dividir string recebida pelo delimitador
                                while((tok_bar = strsep(&fo,"$")) != NULL) {
                                      novo[k] = strdup(tok_bar);
                                      strcat(novo[k],"\n");
                                      k++;
                                      }

                                if (n>0){
                                  //Apresentar ao utilizador o array acima preenchido
                                        for(int posArr=0;posArr<k;posArr++){
                                            oo=write(1,novo[posArr],strlen(novo[posArr]));
                                        }
                                        memset(str3,0,n);
                                        oo=write(1, "argus$ ", strlen("argus$ "));
                                        break;
                                        }
                          }

                          close(server_to_client);
             }
           }

           for(int tam=0;tam<STRING;tam++){
             free(novo[tam]);
              }
        free(tok_bar);
        free(str3);
        free(str);
        free(fo);
              }
  //Se método de input é via linha de comandos
  else{
    //Processo filho envia comandos recebidos em forma de string ao servidor
    if(!fork()) {
      if(((char*)argv)[argc-1]=='\n'){
        ((char*)argv)[argc-1]='\0';
        }

          for(int i=0; i<argc;i++){
              strcat(temp,argv[i]);
              strcat(temp," ");
              }

          comp=strlen(temp)*sizeof(char);
          memset(str2,0,STRING);
          snprintf(str2, STRING,"%010d %010d %010d %s",pid,comp,0,temp);


          oo=write(client_to_server,str2,strlen(str2));
          close(client_to_server);
          _exit(0);

      } else {
        //Processo pai espera resposta do servidor
        server_to_client = open(myfifo2, O_RDONLY);
        int k=0;
        while(1){
                  n = myreadln(server_to_client,str3,STRING);
                  fo=strdup(str3);
                  //Dividir string recebida pelo delimitador
                      while((tok_bar = strsep(&fo,"$")) != NULL) {
                            novo[k] = strdup(tok_bar);
                            strcat(novo[k],"\n");
                            k++;
                            }

                      if (n>0) {
                        //Apresentar ao utilizador o array acima preenchido
                               for(int posArr=0;posArr<k;posArr++){
                                    oo=write(1,novo[posArr],strlen(novo[posArr]));
                                  }
                                  memset(str3,0,n);
                                  break;
                                }

                                }

                                close(server_to_client);
              }
              for(int tam=0;tam<STRING;tam++){
                free(novo[tam]);
              }
        free(tok_bar);
        free(str3);
        free(fo);
       }

  free(temp);
  unlink(myfifo2);

  return EXIT_SUCCESS;
}
