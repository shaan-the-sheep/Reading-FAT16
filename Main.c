#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

void printArray(int arr[], int len){
    for (int i = 0; i < len; i++) {     
        printf("%d ", arr[i]);     
    }
    printf("\n");     
}

int* sectorReader(int fptr, off_t byteOffset, int bytesToRead){
    int* a;
    a = (int*)malloc(bytesToRead * sizeof(int)); //dynamically creating array to hold bytes read
    lseek(fptr, byteOffset, SEEK_SET); // jumps to byte offset from beginning of file
    read(fptr, a, bytesToRead); // reads into a
    return a;
}

int main(void){
    int fptr;
    fptr = open("fat16.img",1);
    printArray(sectorReader(fptr, 3, 5), 5); //printing bytes read (a)
    close(fptr);
}