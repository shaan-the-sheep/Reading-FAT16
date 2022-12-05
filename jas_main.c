
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
    printf("\nBOOT SECTOR VALUES:\n");
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


// temporary array to hold all the clusters of a file - make it 200 elements for now
uint16_t cluster_ids[200];

uint32_t findClustersStartingAt(uint16_t startCluster, uint16_t* fatPtr, size_t elemsInfat1Table,
                                uint16_t* collectClusterArr)
{

    uint16_t prev_index, next_index;
    uint16_t collectArr_index=0;

    prev_index = startCluster;
    collectClusterArr[collectArr_index++] = prev_index;

    while (1) {
        next_index = fatPtr[prev_index];
        if (next_index >= 0xfff8){
            return collectArr_index;
        }
        else {
            collectClusterArr[collectArr_index++] = next_index;
            prev_index = next_index;
        }
    }
}


/**
 * @brief 
 * Parses through last modified time using bit masking
 * Returns formatted time string
 * @param wrt_time 
 * @param time_str 
 */
void parse_wrt_time(uint16_t wrt_time, char* time_str)
{
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
void parse_wrt_date(uint16_t wrt_date, char* date_str) 
{
    uint8_t day, month, year;
    day = wrt_date & 0x1F;
    month = (wrt_date >> 5) & 0xF;
    year = (wrt_date >> 9) & 0x7F;
    sprintf(date_str, "Last modified date: %d/%d/%d\n", day, month, year + 1980);
}

// Task 5
#define FILE_DATA_BUFFER_SIZE 2048
uint8_t file_data[FILE_DATA_BUFFER_SIZE];  // Hold some file data

uint16_t readFileDataAtCluster(int fp, uint16_t clusterNum,
                               uint32_t startOffsetOfDataSectionInBytes,
                               uint32_t bytesPerCluster,
                               uint32_t numBytesToRead,
                               uint8_t* dataBuffer, uint16_t dataBufferSize)
{
    if (numBytesToRead > dataBufferSize) {
        printf("Max bytes to read exceeded, reading %d bytes\n", dataBufferSize);
        numBytesToRead = dataBufferSize;
    }
    clusterNum -= 2; // Adjust cluster number

    // Empty the dataBuffer
    memset(dataBuffer, '\0', dataBufferSize);

    uint32_t read_offset = startOffsetOfDataSectionInBytes + (clusterNum * bytesPerCluster);
    // DEBUG
    //printf("In readFileDataAtCluster(): reading adjusted cluster 0x%x at byte addr: 0x%x\n",
    //              clusterNum  , read_offset);
    size_t bytes_read = readBytesAtOffset(fp, read_offset, numBytesToRead, dataBuffer);
    return bytes_read;
}

#define archive_mask    0x20
#define dir_mask        0x10
#define vol_mask        0x08
#define sys_mask        0x04
#define hidden_mask     0x02
#define readOnly_mask   0x01

#define read_only_bit   0x0
#define hidden_bit      0x01
#define system_bit      0x02
#define volume_bit      0x03
#define directory_bit   0x04
#define archive_bit     0x05

void createAttrString(uint8_t attr, char* collectAttr)
{
    collectAttr[6] = '\n';
    if(attr & archive_mask)
        collectAttr[archive_bit] = 'A';
    else
        collectAttr[archive_bit] = '-';

    if(attr & dir_mask)
        collectAttr[directory_bit] = 'D';
    else
        collectAttr[directory_bit] = '-';
    
    if(attr & vol_mask)
        collectAttr[volume_bit] = 'V';
    else
        collectAttr[volume_bit] = '-';
    
    if(attr & sys_mask)
        collectAttr[system_bit] = 'S';
    else
        collectAttr[system_bit] = '-';
    
    if(attr & hidden_mask)
        collectAttr[hidden_bit] = 'H';
    else
        collectAttr[hidden_bit] = '-';
    
    if(attr & readOnly_mask)
        collectAttr[read_only_bit] = 'R';
    else
        collectAttr[read_only_bit] = '-';
}

uint8_t checkIfRegularFile(uint8_t attr)
{
    if ((attr & vol_mask) && (attr & dir_mask))
        return 0;
    else
        return 1;
}

// Checks if empty or deleted root dir entry
uint8_t isValidDirEntry(RootDirectory dirEntry)
{
    if (dirEntry.DIR_Name[0] == 0x00 || dirEntry.DIR_Name[0] == 0xE5)
        return 0;
    else
        return 1;
}

/**
 * @brief
 * Uses bit masking to work out the attribute flags
 * If entry is for a regular file, output info
 * 
 * @param rootDirElem 
 */
void print_file_info(RootDirectory rootDirElem)
{
    // Parse dir entry and extract  the first/ starting cluster, the last modified time and date, the file attributes using a 
    // single letter for each, i.e., ADVSHR, with a – (dash/ hyphen) used to indicate an unset flag, the length 
    // of the file, and finally the filename. Output should be formatted neatly in columns

    uint16_t fstClusHi, fstClusLo, wrtTime, wrtDate;
    uint32_t fileSize;
    uint8_t dirNameArr[11];

    fstClusHi = rootDirElem.DIR_FstClusHI;
    fstClusLo = rootDirElem.DIR_FstClusLO;
    wrtTime = rootDirElem.DIR_WrtTime;
    wrtDate = rootDirElem.DIR_WrtDate;
    fileSize = rootDirElem.DIR_FileSize;
    memcpy ( &dirNameArr, &rootDirElem.DIR_Name , 11);
    // Make dir name null terminated
    for (int i = 0; i < 11; i++) {
        if (dirNameArr[i] == 0x20) {
            dirNameArr[i] = '\0';
            break;
        }
    }

    // Work out the attr flags
    char collectAttr[6+1];
    createAttrString(rootDirElem.DIR_Attr, collectAttr);

    if (!isValidDirEntry(rootDirElem))
        return;
    else if (!(collectAttr[read_only_bit] == 'R' && collectAttr[hidden_bit] == 'H' 
            && collectAttr[system_bit] == 'S' && collectAttr[volume_bit] == 'V' 
            && collectAttr[directory_bit] == '-' && collectAttr[archive_bit] == '-')) {
        if (checkIfRegularFile(rootDirElem.DIR_Attr) == 1) {
            printf("File name: %s\n", dirNameArr);
            printf("Start ClusterHi: 0x%0x\nStart ClusterLo: 0x%0x\nFile Len: 0x%0x\nAttr: %s", fstClusHi, fstClusLo, fileSize, collectAttr);
            char time_str[50];
            parse_wrt_time(rootDirElem.DIR_WrtTime, time_str);
            printf("%s\n", time_str);

            char date_str[50];
            parse_wrt_date(rootDirElem.DIR_WrtDate, date_str);
            printf("%s\n", date_str);
        }
    }
}

// Prints all data from a file (assumes it is a text file)
uint32_t printRegularFileContents(int fp,
                              uint32_t startOffsetOfDataSectionInBytes,
                              uint32_t bytesPerCluster,
                              RootDirectory rootDirEntry, uint32_t numBytesToPrint,
                              uint16_t* clusterList, uint16_t clusterListSize)
{
    // Check is a regular file
    if(!checkIfRegularFile(rootDirEntry.DIR_Attr)) 
    {
        //printf ("Not a regular file\n");
        return 0;
    }
    uint32_t bytesReadSoFar = 0;
    uint16_t startCluster = rootDirEntry.DIR_FstClusLO;
    uint16_t bytes_read; 

    // Go to each cluster and read data until we have read numBytesToPrint in total
    for (int i=0; i < clusterListSize; i++)
    {
        uint16_t thisCluster = clusterList[i];

        if (numBytesToPrint < bytesPerCluster) 
        {

            bytes_read = readFileDataAtCluster(
                                    fp, thisCluster,
                                    startOffsetOfDataSectionInBytes,
                                    bytesPerCluster,
                                    numBytesToPrint,
                                    file_data, FILE_DATA_BUFFER_SIZE);
            // We've read all the bytes. Print them
            printf("%s\n ", file_data);
            printf("\n");
        }
        else {
            // read all of this cluster
            bytes_read = readFileDataAtCluster(
                                    fp, thisCluster,
                                    startOffsetOfDataSectionInBytes,
                                    bytesPerCluster,
                                    bytesPerCluster,  // reading ALL of cluster
                                    file_data, FILE_DATA_BUFFER_SIZE);
            bytesReadSoFar += bytes_read;
            if (bytesReadSoFar >= numBytesToPrint) 
            {
                // We've read all the bytes. Print them
                printf("%s\n ", file_data);
                printf("\n");
            }
        }
    }
    return bytesReadSoFar;
}

char* FILENAME  = "fat16.img";
BootSector bs;   
#define BOOT_SECTOR_OFFSET  0

int main(void){
    // Task 1
    // Opening img
    int fp;
    fp = openFile(FILENAME);

    // Task 2
    // Reading boot sector
    readBytesAtOffset(fp, BOOT_SECTOR_OFFSET, sizeof(BootSector), &bs);

    // Task 3
    // Caluclating Fat1 offset 
    // Calculating fat1 size
    // Ptr to hold fat1 contents
    off_t fat1_offset = bs.BPB_RsvdSecCnt * bs.BPB_BytsPerSec;
    uint32_t fat1_num_bytes = bs.BPB_FATSz16 * bs.BPB_BytsPerSec;   
    uint16_t* fat1_table_ptr = (uint16_t*) malloc(fat1_num_bytes/2);
    readBytesAtOffset(fp, fat1_offset, fat1_num_bytes, fat1_table_ptr);

    // Task 4
    // Creating rootdirectory object
    // Calculating offset
    RootDirectory rootDirElem;
    off_t rootDir_offset = fat1_offset + fat1_num_bytes * bs.BPB_NumFATs;

    // Creating menu
    int task;
    while (1){
        printf("Task 2: Output Boot Sector values\nTask 3: Outut list of clusters that make up a file\nTask 4: Output the list of files in the Root Directory\nTask 5: Output contents of a file\nEnter 0 to exit\nPlease enter Task number: ");
        scanf("%d", &task);

        if (task == 2)
        {
            printBSvalues(bs);
        }
        else if (task == 3)
        {
            if (fat1_table_ptr == NULL) 
            {
                printf("Could not allocate memory for fat1\n");
                exit(1);
            }
            //reading fat1
            //readBytesAtOffset(fp, fat1_offset, fat1_num_bytes, fat1_table_ptr);
            uint16_t startingCluster;
            printf("\nEnter starting cluster: ");
            scanf("%d", &startingCluster);
            int num_clusters_found = findClustersStartingAt(startingCluster, fat1_table_ptr, fat1_num_bytes/2, cluster_ids);
            for(int i= 0; i < num_clusters_found; i++)
            {
                printf("%d ", cluster_ids[i]);
                printf("\n");
            }
        }
        else if (task == 4)
        {
            for(int i = 0; i < bs.BPB_RootEntCnt; i++) 
            {
                readBytesAtOffset(fp, rootDir_offset, sizeof(RootDirectory), &rootDirElem);
                print_file_info(rootDirElem);
                rootDir_offset += sizeof(RootDirectory);
            }
        }
        else if (task == 5)
        {
            uint32_t byteSizeOfRootDirBlock = bs.BPB_RootEntCnt * sizeof(RootDirectory);
            printf("%d\n", byteSizeOfRootDirBlock);
            uint32_t startOffsetOfRootDirSection = (bs.BPB_RsvdSecCnt + bs.BPB_NumFATs * bs.BPB_FATSz16) * bs.BPB_BytsPerSec;
            uint32_t startOffsetOfDataSection = startOffsetOfRootDirSection  + byteSizeOfRootDirBlock;
            printf("Start offset of data section: 0x%X\n", startOffsetOfDataSection);

            // Get all Regular files in RootDir
            rootDir_offset = fat1_offset + fat1_num_bytes * bs.BPB_NumFATs;
            for(int i = 0; i < bs.BPB_RootEntCnt; i++) 
            {
                readBytesAtOffset(fp, rootDir_offset, sizeof(RootDirectory), &rootDirElem);
                rootDir_offset += sizeof(RootDirectory);

                if (!isValidDirEntry(rootDirElem) || !checkIfRegularFile(rootDirElem.DIR_Attr))
                    continue;
                else {
                    // Get cluster list for this rootDirElem
                    int num_clusters_found = findClustersStartingAt(rootDirElem.DIR_FstClusLO,
                                                                fat1_table_ptr,
                                                                fat1_num_bytes/2,
                                                                cluster_ids);
                    // Debug
                    printf("Printing cluster Ids for root dir entry: %s\n", rootDirElem.DIR_Name);
                    for(int i= 0; i < num_clusters_found; i++)
                        printf("%d, ", cluster_ids[i]);
                    printf("\n");

                    printRegularFileContents(fp, startOffsetOfDataSection,
                                        bs.BPB_BytsPerSec*bs.BPB_SecPerClus,
                                        rootDirElem,
                                        rootDirElem.DIR_FileSize, // print all of file
                                        cluster_ids, num_clusters_found);
                }
            }
        }
        else if (task == 0)
        {
            return 1;
        }
    }
    close(fp);

}

