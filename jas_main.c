
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
 * @brief 
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

/**
 * @brief 
 * Prints out boot sector values BootSector struct 
 * @param bs 
 */
void printBSvalues(BootSector bs){
    printf("BOOT SECTOR VALUES:\n");
    printf("Bytes per sec: %d\n", bs.BPB_BytsPerSec);
    printf("Sectors per cluster: %d\n", bs.BPB_SecPerClus);
    printf("Reserved sector count: %d\n", bs.BPB_RsvdSecCnt);
    printf("Num of FATs: %d\n", bs.BPB_NumFATs);
    printf("Size of root DIR: %d\n", bs.BPB_RootEntCnt);
    printf("Num of sectors: %d\n", bs.BPB_TotSec16);
    printf("Sectors in FAT: %d\n", bs.BPB_FATSz16);
    printf("Sectors if totsec16 = 0: %d\n", bs.BPB_TotSec32);
    printf("Non zero terminated string: %.*s\n", 11, bs.BS_VolLab);
    printf("\n");
}

/**
 * @brief 
 * Jumps to byte offset in file
 * Reads bytes from file to buffer 
 * Returns buffer
 * Prints error message if unable to read
 * @param fd 
 * @param offset 
 * @param numBytesToRead 
 * @param buffPtrToFill 
 * @return ssize_t 
 */
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


/**
 * @brief 
 * Parses through last modified time using bit masking
 * Returns formatted time string
 * @param wrt_time 
 * @param time_str 
 */
void parse_wrt_time(uint16_t wrt_time, char* time_str){
    uint8_t secs, mins, hrs;
    secs = wrt_time & 0x1F;
    mins = (wrt_time >> 5) & 0x3F;
    hrs = (wrt_time >> 11) & 0x1F;
    sprintf(time_str, "Last modified time: %d:%d:%d", hrs, mins, secs);
}

/**
 * @brief 
 * Parses through last modified date using bit masking
 * Returns formatted date string
 * @param wrt_date 
 * @param date_str 
 */
void parse_wrt_date(uint16_t wrt_date, char* date_str){
    uint8_t day, month, year;
    day = wrt_date & 0x1F;
    month = (wrt_date >> 5) & 0xF;
    year = (wrt_date >> 9) & 0x7F;
    sprintf(date_str, "Last modified date: %d/%d/%d\n", day, month, year + 1980);

}

/**
 * @brief
 * Uses bit masking to work out the attribute flags
 * If entry is for a regular file, output info
 * 
 * @param rootDirElem 
 */
void print_file_info(RootDirectory *rootDirElem){
    uint16_t fstClusHi, fstClusLo, wrtTime, wrtDate, attr;
    uint32_t fileSize;

    attr = rootDirElem->DIR_Attr;
    char collectAttr[6];
    if(attr & 0x20){
        collectAttr[5] = 'A';
    }else{
        collectAttr[5] = '-';
    }
    if(attr & 0x10){
        collectAttr[4] = 'D';
    }else{
        collectAttr[4] = '-';
    }
    if(attr & 0x08){
        collectAttr[3] = 'V';
    }else{
        collectAttr[3] = '-';
    }
    if(attr & 0x04){
        collectAttr[2] = 'S';
    }else{
        collectAttr[2] = '-';
    }
    if(attr & 0x02){
        collectAttr[1] = 'H';
    }else{
        collectAttr[1] = '-';
    }
    if(attr & 0x01){
        collectAttr[0] = 'R';
    }else{
        collectAttr[0] = '-';
    }
    if (rootDirElem->DIR_Name[0] == 0x00 || rootDirElem->DIR_Name[0] == 0xE5){
        return;
    }
    if (collectAttr[3] == '-' && collectAttr[4] == '-'){
        if (!(collectAttr[0] == 'R' && collectAttr[1] == 'H' && collectAttr[2] == 'S' && collectAttr[3] == 'V')){ 
            printf("File name: %.*s\n", 11, rootDirElem->DIR_Name);
            printf("Start ClusterHi: %0x, Start ClusterLo: %0x, file Len: %0x, attr: %s\n", rootDirElem->DIR_FstClusHI, rootDirElem->DIR_FstClusLO, rootDirElem->DIR_FileSize, collectAttr);
            char time_str[50];
            parse_wrt_time(rootDirElem->DIR_WrtTime, time_str);
            printf("%s\n", time_str);

            char date_str[50];
            parse_wrt_date(rootDirElem->DIR_WrtDate, date_str);
            printf("%s\n", date_str);
        }
    }
}

char* FILENAME  = "fat16.img";
BootSector bs;   
#define BOOT_SECTOR_OFFSET  0

int main(void){
    int fp = openFile(FILENAME);
    // Task 2:
    ssize_t bytesRead = readBytesAtOffset(fp, BOOT_SECTOR_OFFSET, sizeof(BootSector), &bs);
    printBSvalues(bs);

    // Task 3:
    // Calculate offset of fat1
    off_t fat1_offset = bs.BPB_RsvdSecCnt * bs.BPB_BytsPerSec;

    // Calulate total size of fat1 in bytes
    uint32_t fat1_num_bytes = bs.BPB_FATSz16 * bs.BPB_BytsPerSec;

    // Allocate dynamic array to hold fat1 
    uint16_t* fat1_table_ptr = (uint16_t*) malloc(fat1_num_bytes/2);
    if (fat1_table_ptr == NULL) {
        printf("Could not allocate memory for fat1\n");
        exit(1);
    }
    bytesRead = readBytesAtOffset(fp, fat1_offset, fat1_num_bytes, fat1_table_ptr);

    // Task 4: 
    off_t rootDir_offset = fat1_offset + fat1_num_bytes * bs.BPB_NumFATs;

    // Read a dir entry at a time
    RootDirectory rootDirElem;
    for(int i = 0; i < (bs.BPB_RootEntCnt); i++) {
        readBytesAtOffset(fp, rootDir_offset + i*sizeof(rootDirElem), sizeof(RootDirectory), &rootDirElem);
        print_file_info(&rootDirElem);
    } 
    close(fp);
}