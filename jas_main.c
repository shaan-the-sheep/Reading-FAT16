
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

typedef struct __attribute__((__packed__)) {
uint8_t DIR_Name[ 11 ]; // Non zero terminated string
uint8_t DIR_Attr; // File attributes
uint8_t DIR_NTRes; // Used by Windows NT, ignore
uint8_t DIR_CrtTimeTenth; // Tenths of sec. 0...199
uint16_t DIR_CrtTime; // Creation Time in 2s intervals
uint16_t DIR_CrtDate; // Date file created
uint16_t DIR_LstAccDate; // Date of last read or write
uint16_t DIR_FstClusHI; // Top 16 bits file's 1st cluster
uint16_t DIR_WrtTime; // Time of last write
uint16_t DIR_WrtDate; // Date of last write
uint16_t DIR_FstClusLO; // Lower 16 bits file's 1st cluster
uint32_t DIR_FileSize; // File size in bytes
} RootDirectory;

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


void printBSvalues(BootSector bs){
    printf("BOOT SECTOR VALUES:\n");
    printf("Bytes per sec: %" PRIu16 "\n", bs.BPB_BytsPerSec);
    printf("Sectors per cluster: %" PRIu8 "\n", bs.BPB_SecPerClus);
    printf("Reserved sector count: %" PRIu16 "\n", bs.BPB_RsvdSecCnt);
    printf("Num of FATs: %" PRIu8 "\n", bs.BPB_NumFATs);
    printf("Size of root DIR: %" PRIu16 "\n", bs.BPB_RootEntCnt);
    printf("Num of sectors: %" PRIu16 "\n", bs.BPB_TotSec16);
    printf("Sectors in FAT: %" PRIu16 "\n", bs.BPB_FATSz16);
    printf("Sectors if totsec16 = 0: %" PRIu32 "\n", bs.BPB_TotSec32);
    printf("Non zero terminated string: %.*s\n", 11, bs.BS_VolLab);
}

ssize_t readBytesAtOffset(int fd, off_t offset, size_t numBytesToRead,
                            void* buffPtrToFill){
    off_t ret = lseek(fd, offset, SEEK_SET);
    if (ret == -1) {
        printf("Error reading Fie\n");
        exit(1);
    }

    ssize_t bytes_read = read(fd, buffPtrToFill, numBytesToRead);
    if (bytes_read == -1) {
        printf("Error reading Fie\n");
        exit(1);
    }
    return bytes_read;
}

BootSector bs;   // The boot-sector
#define BOOT_SECTOR_OFFSET  0

// temporary array to hold all the clusters of a file - make it 1024 elements for now
uint16_t cluster_ids[1024];

#ifdef DONTDO
uint32_t findClustersStartingAt(uint16_t startCluster, uint16_t* fatPtr, size_t elemsInfat1Table,
                                uint16_t* collectClusterArr)
{
    uint16_t ca_indx = 0;
    uint16_t cluster_value;

    // Find the startCluster
    for(int i = 0 ; i < elemsInfat1Table; ++i)
    {
        if (fatPtr[i] == startCluster){
            // Found start cluster
            cluster_value = fatPtr[i];
            collectClusterArr[ca_indx] = startCluster;
            do {
                c++;


            } while (fatPtr[i] < 0xfff8);
        }
    }
    printf("\n");

    return 0;
}
#endif

char* FILENAME  = "fat16.img";

int main(void){

    int fp;

    // Task 2: Read boot sector
    fp = openFile(FILENAME);
    ssize_t bytesRead = readBytesAtOffset(fp, BOOT_SECTOR_OFFSET, sizeof(BootSector), &bs);
    printf("read boot sector, Bytes read from file: %d\n", bytesRead);
    // Print bootsector values
    printBSvalues(bs);

    // Task 3: load a copy of the first FAT into memory and that, given a starting
    // cluster number, you can produce an ordered list of all the clusters that make up 
    // a file

    // Calculate offset of fat1
    printf("Byte Size of bootsector: %d\n", sizeof(BootSector));
    off_t fat1_offset = bs.BPB_RsvdSecCnt * bs.BPB_BytsPerSec;
    printf("Offset on fat1 table: %d\n", fat1_offset);

    // Calulate total size of fat1 in bytes
    uint32_t fat1_num_bytes = bs.BPB_FATSz16 * bs.BPB_BytsPerSec;
    printf("fat1_num_bytes: %d\n", fat1_num_bytes);

    // Allocate dynamic array to hold fat1 (16 bit ints)
    uint16_t* fat1_table_ptr = (uint16_t*) malloc(fat1_num_bytes/2);
    if (fat1_table_ptr == NULL) {
        printf("Could not allocate memory for fat1\n");
        exit(1);
    }

    bytesRead = readBytesAtOffset(fp, fat1_offset, fat1_num_bytes, fat1_table_ptr);

    // findClustersStartingAt(0, fat1_table_ptr, fat1_num_bytes/2, cluster_ids);

    // PRINT MY FAT TABLE
    for (int i = 0; i < 30; i++)
    {
        printf("%02X, ", fat1_table_ptr[i]);
    }

    // Task 4: 
    off_t rootDir_offset = fat1_offset + fat1_num_bytes * bs.BPB_NumFATs;
    printf("root dir offset: %d\n", rootDir_offset);
    printf("%d\n",  bs.BPB_RsvdSecCnt + bs.BPB_NumFATs * bs.BPB_FATSz16);

    close(fp);


}