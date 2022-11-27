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

BootSector sectorReader(int fp, off_t byteOffset, int bytesToRead, BootSector buffer){
    lseek(fp, byteOffset, SEEK_CUR); // jumps to byte offset 
    read(fp, &buffer, bytesToRead); // reads into buffer    
    return buffer;
}

int main(void){
    int fp = openFile("fat16.img");
    BootSector bs;
    bs = sectorReader(fp, 0, sizeof(bs), bs);
    //printf("%x\n",bs.BS_BootSig);

    //go to FAT table
    lseek(fp, sizeof(BootSector) + (bs.BPB_RsvdSecCnt * bs.BPB_BytsPerSec), SEEK_SET); //skips reserved sectors
    
    //reading first FAT
    int bytesPerClust = bs.BPB_BytsPerSec * bs.BPB_SecPerClus;
    int bytesPerFAT = bs.BPB_FATSz16 * bs.BPB_BytsPerSec;
    uint16_t array[bytesPerClust];
    read(fp, array, bytesPerClust); 

    uint16_t startingCluster;
    printf("Enter starting cluster: ");
    scanf("%" PRIu16, &startingCluster);
    uint16_t i = startingCluster;
    uint16_t a = 0;
    printf("%" PRIu16 "\n", startingCluster);
    while (i < 0xfff8){
        if (i == 0){
            i = 1;
        }
        i = array[i]; //indexing starting from 0?
        printf("%" PRIu16 "\n", i);
    }

    //printArray(array, bytesPerClust);
    //printf("%" PRIu16 "\n", array[0]);

    close(fp);
}