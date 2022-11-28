#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>

typedef struct __attribute__((__packed__)) {
uint8_t BS_jmpBoot[ 3 ]; // x86 jump instr. to boot code
uint8_t BS_OEMName[ 8 ]; // What created the filesystem
uint16_t BPB_BytsPerSec; // Bytes per Sector
uint8_t BPB_SecPerClus; // Sectors per Cluster
uint16_t BPB_RsvdSecCnt; // Reserved Sector Count
uint8_t BPB_NumFATs; // Number of copies of FAT
uint16_t BPB_RootEntCnt; // FAT12/FAT16: size of root DIR
uint16_t BPB_TotSec16; // Sectors, may be 0, see below
uint8_t BPB_Media; // Media type, e.g. fixed
uint16_t BPB_FATSz16; // Sectors in FAT (FAT12 or FAT16)
uint16_t BPB_SecPerTrk; // Sectors per Track
uint16_t BPB_NumHeads; // Number of heads in disk
uint32_t BPB_HiddSec; // Hidden Sector count
uint32_t BPB_TotSec32; // Sectors if BPB_TotSec16 == 0
uint8_t BS_DrvNum; // 0 = floppy, 0x80 = hard disk
uint8_t BS_Reserved1; 
uint8_t BS_BootSig; // Should = 0x29
uint32_t BS_VolID; // 'Unique' ID for volume
uint8_t BS_VolLab[ 11 ]; // Non zero terminated string
uint8_t BS_FilSysType[ 8 ]; // e.g. 'FAT16 ' (Not 0 term.)
} BootSector;

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

void printArray(uint16_t arr[], int len){
    for (int i = 0; i < len; i++) {
        printf("%" PRIu16 "\n", arr[i]);
    }
    printf("\n");
}

BootSector bootSectorReader(int fp, off_t byteOffset, int bytesToRead, BootSector buffer){
    lseek(fp, byteOffset, SEEK_SET); // jumps to byte offset 
    read(fp, &buffer, bytesToRead); // reads into buffer    
    return buffer;
}

uint16_t* fatReader(int fp, BootSector bs, uint16_t* buffer){
    lseek(fp, (bs.BPB_RsvdSecCnt * bs.BPB_BytsPerSec), SEEK_SET);
    read(fp, buffer, bs.BPB_BytsPerSec * bs.BPB_SecPerClus); 
    return buffer;
}

uint16_t* clustersInFile(uint16_t startingCluster, uint16_t* fatArr, uint16_t* clustersArr){
    uint16_t cluster = startingCluster;
    int i = 0;
    if (cluster == 0){
        cluster = 1;
    }
    //printf("%" PRIu16 "\n", cluster);
    clustersArr[i] = cluster;
    i++;
    while (cluster < 0xfff8){
        if (cluster == 0){
            cluster = 1;
        } else{
            cluster = fatArr[cluster];
            if (cluster == 0){
                cluster = 1;
            }  
        } 
        //printf("%" PRIu16 "\n", cluster);
            clustersArr[i] = cluster;
            i++;
    }
    return clustersArr;
} 

int main(void){
    int fp = openFile("fat16.img");
    BootSector bs;
    bs = bootSectorReader(fp, 0, sizeof(bs), bs); //reads Boot Sector into bs 

    uint16_t* fatArr; 
    uint16_t* clustersArr;
    fatArr = fatReader(fp, bs, fatArr); //navigates to first fat and reads into array

    uint16_t startingCluster; //user input
    printf("Enter starting cluster: ");
    scanf("%" PRIu16, &startingCluster);

    clustersArr = clustersInFile(startingCluster, fatArr, clustersArr); //prints clusters in file from starting cluster

    close(fp);
}