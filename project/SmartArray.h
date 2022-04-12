#ifndef SMARTARRAYS_H
#define SMARTARRAYS_H

struct SmartArrayS{
    int tam;
    char **array;
};

struct SmartArrayS* initSmartArrayS();

void freeSmartArrayS(struct SmartArrayS *array);

void addToArrayS (struct SmartArrayS *q, char *elem);

void removeElem(struct SmartArrayS *q, int index);

void printArrayS(struct SmartArrayS* arr);

int getIndex(char* s);

int getIndexArr(struct SmartArrayS *arr, int index);

#endif