#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Estrutura de dados
struct SmartArrayS{
    int tam;
    char **array;
};

//Função que inicializa o array (inicia o array com apenas um espaço livre)
struct SmartArrayS* initSmartArrayS(){
    struct SmartArrayS* q;
    q = malloc(sizeof(struct SmartArrayS));
    q->tam = 0;
    q->array = malloc(sizeof(char *));
    return q;
};

//Função que dá free ao array
void freeSmartArrayS(struct SmartArrayS *array){
    int i = 0;
    for(i = 0; i < array->tam;i++){
      free(array->array[i]);
   }
   free(array->array);
}

//Adiciona um elemento ao array
void addToArrayS (struct SmartArrayS *q, char *elem){
    q->array = realloc(q->array,sizeof(char *)*(q ->tam+1));
    q->array[q->tam] = strdup(elem);
    q->tam++;
}

//Função que dado um comando devolve o numero da tarefa
int getIndex(char* s){
    char str[8];
    int i;
    for(i = 1; s[i]!=':'; i++){
        str[i-1] = s[i];
    }
    str[i-1] = '\0';
    return atoi(str);
}

//Função que dado o numero da tarefa devolve o indice do array onde está essa tarefa
int getIndexArr(struct SmartArrayS *arr, int index){
    for(int i = 0; i<arr->tam; i++){
        if(getIndex(arr->array[i]) == index){
            return i;
        }
    }
    return -1;
}

//Remove um elemento do array dado um indice
void removeElem(struct SmartArrayS *q, int index)
{
    for(int i = index; i < q->tam-1; i++){
        q->array[i] = strdup(q->array[i+1]);
    }
    q->tam--;
    free(q->array[q->tam]);
}
