#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

/**
 * Opens file using open()
 * Prints error message if file could not be opened
 * @param filename 
 * @return int 
 */
int openFile(char* filename){
    int fp = open(filename, O_RDONLY);
    if (fp == -1) {
        printf("Error opening file\n");
        exit(1);
    }
    return fp;
}

void sectorReader(int fp, off_t byteOffset, int bytesToRead){
    char *buffer = (char *) calloc(500, sizeof(char));
    lseek(fp, byteOffset, SEEK_CUR); // jumps to byte offset from beginning of file
    read(fp, buffer, bytesToRead); // reads into buffer
    printf("The bytes read are [%s]\n",buffer);
    free(buffer);    
}


int main(void){
    int fp = openFile("fat16.img");
    sectorReader(fp, 0, 100000);
    close(fp);
}