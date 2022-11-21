#include <stdio.h>
#include <stdlib.h>

void printArray(int arr[], int len){
    for (int i = 0; i < len; i++) {     
        printf("%d ", arr[i]);     
    }
    printf("\n");     
}

int* sectorReader(FILE *fptr, int byteOffset, int bytesToRead){
    int* a;
    a = (int*)malloc(bytesToRead * sizeof(int)); //dynamically creating array to hold bytes read
    fseek(fptr, byteOffset, SEEK_SET); // jumps to byte offset from beginning of file
    fread(a, sizeof *a, bytesToRead, fptr); // reads into a
    return a;
}


int main(void){
    FILE *fptr;
    fptr = fopen("test.txt","r");
    printArray(sectorReader(fptr, 3, 5), 5); //printing bytes read (a)
    fclose(fptr);
}