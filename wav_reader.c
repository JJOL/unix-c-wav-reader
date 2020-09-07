/**
 * Author: Juan Jose Olivera
 * Name: WAV Reader
 * Description: A custom implemented WAV reader program
 * 
 * Version: 0.1.0
 * 
 * Using RIFF WAV Spec: https://www.lpi.tel.uva.es/~nacho/docencia/ing_ond_1/trabajos_01_02/formatos_audio_digital/html/wavformat.htm
 * Additional RIFF Spec for LIST segment: https://www.recordingblogs.com/wiki/list-chunk-of-a-wave-file
 * 
 * */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

#define HEAD_ID_LEN 4
#define HEAD_SIZE_LEN 4
#define MS_PMC_TAG 0x0001
#define MAX_STR_LEN 256


typedef unsigned int uint;
typedef unsigned char uchar;
typedef unsigned short WORD;
typedef unsigned int DWORD;

typedef struct riff_header {
    uchar id[4];
    uint  size;
} riff_header;

typedef struct wav_fmt {
    WORD wFormatTag;
    WORD wChannels;
    DWORD dwSamplesPerSec;
    DWORD dwAvgBytesPerSec;
    WORD wBlockAlign;

    // Specific for Microsoft PCM Format Tag
    WORD nBitsPerSample;
} wav_fmt;

typedef struct riff_list_info {
    char name[MAX_STR_LEN];
} riff_list_info;


int readHeader(int fd, riff_header *header);
int readHeaderTest(int fd, riff_header *header, char *str);

void pFmtError(char *str) {
    fprintf(stderr, "Invalid encoded format on %s!\n", str);
}

void strCleanZeros(char *str, uint len);


// Segment Processing Functions
int processFmtSegment(int fd, riff_header *header, wav_fmt *wavFmtInfo);
int processLISTSegment(int fd, riff_header *header, riff_list_info *riffListInfo);
int processDataSegment(int fd, riff_header *header, uint *dataLength);


int main(int argc, char *argv[])
{
    char temp_buff[MAX_STR_LEN];

    // Open File
    int wavFd;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return -1;
    }
    if ((wavFd = open(argv[1], O_RDONLY)) < 0) {
        fprintf(stderr, "Can not open file '%s'\n", argv[1]);
        return -2;
    }

    // RIFF WAVE Format Validation
    int contentSize;
    riff_header tempHeader;

    if (readHeaderTest(wavFd, &tempHeader, "RIFF") < 0) {
        pFmtError("RIFF Header");
        return -2;
    }
    contentSize = tempHeader.size;

    if (readHeaderTest(wavFd, &tempHeader, "WAVE") < 0) {
        pFmtError("RIFF Header");
        return -2;
    }
    lseek(wavFd, -4, SEEK_CUR); // Rewind to the last 4 bytes

    

    // Segments Processing
    wav_fmt wavFmtInfo;
    riff_list_info riffListInfo;
    uint dataLength;
    
    printf("Processing Segments\n");
    while ((readHeader(wavFd, &tempHeader)) >= 0)
    {
        if (strncmp(tempHeader.id, "fmt ", HEAD_ID_LEN) == 0) {
            printf("- Processing 'fmt '...\n");
            processFmtSegment(wavFd, &tempHeader, &wavFmtInfo);
        }
        else if (strncmp(tempHeader.id, "LIST", HEAD_ID_LEN) == 0) {
            printf("- Processing 'LIST'...\n");
            processLISTSegment(wavFd, &tempHeader, &riffListInfo);
        }
        else if (strncmp(tempHeader.id, "data", HEAD_ID_LEN) == 0) {
            printf("- Processing 'data'...\n");
            processDataSegment(wavFd, &tempHeader, &dataLength);
        } else {
            strncpy(temp_buff, tempHeader.id, HEAD_ID_LEN);
            temp_buff[HEAD_ID_LEN] = '\0';
            printf("- Unknown segment '%s'...\n", temp_buff);
            // Unknow Processing
            // TODO: Move Segment Length Bytes to next one
            lseek(wavFd, tempHeader.size, SEEK_CUR);
        }
    }
    printf("Done Processing!\n");
    printf("\n\n");

    
    // Info Printing
    uint bytesPerSample = wavFmtInfo.nBitsPerSample / 8;
    uint calcSeconds = ((((dataLength / bytesPerSample)) / wavFmtInfo.wChannels) / wavFmtInfo.dwSamplesPerSec);

    printf("WAV Info\n");
    printf("- Fmt Tag: 0x%04hx\n", wavFmtInfo.wFormatTag); // Hex Format: http://personal.ee.surrey.ac.uk/Personal/R.Bowden/C/printf.html
    printf("- Channels: %hu\n", wavFmtInfo.wChannels);
    printf("- Samples Per Sec: %u\n", wavFmtInfo.dwSamplesPerSec);
    printf("- Avg. Bytes Per Sec: %u\n", wavFmtInfo.dwAvgBytesPerSec);
    printf("- Bits Per Sample: %hu\n", wavFmtInfo.nBitsPerSample);
    printf("- Author: %s\n", riffListInfo.name);
    printf("\n");
    printf("- Data Byte Length: %u\n", dataLength);
    printf("- Duration: %us\n", calcSeconds);

    // Stat Information
    struct stat fileStat;
    fstat(wavFd, &fileStat);
    printf("Number of links to file '%s': %lu\n", argv[1], fileStat.st_nlink);
    printf("File size: %lu\n", fileStat.st_size);


    // Close File
    close(wavFd);
    return 0;
}



// Reading Functions
int readHeader(int fd, riff_header *header)
{
    if (read(fd, header->id, HEAD_ID_LEN) <= 0) {
        return -2;
    }
    if (read(fd, &header->size, HEAD_SIZE_LEN) <= 0) {
        return -2;
    }

    return 0;
}
int readHeaderTest(int fd, riff_header *header, char *str)
{
    if (readHeader(fd, header) < 0 || strncmp(header->id, str, HEAD_ID_LEN) != 0) {
        return -1;
    }
    return 0;
}

void strCleanZeros(char *str, uint len)
{
    uint i;
    for (i = 0; i < len; i++) {
        if (str[i] == 0)
            str[i] = ' ';
    }
}


// Segment Processing Functions
int processFmtSegment(int fd, riff_header *header, wav_fmt *wavFmtInfo)
{
    int segSize = header->size;

    read(fd, &wavFmtInfo->wFormatTag, sizeof(WORD));
    read(fd, &wavFmtInfo->wChannels,  sizeof(WORD));
    read(fd, &wavFmtInfo->dwSamplesPerSec,  sizeof(DWORD));
    read(fd, &wavFmtInfo->dwAvgBytesPerSec, sizeof(DWORD));
    read(fd, &wavFmtInfo->wBlockAlign, sizeof(WORD));
    
    if (wavFmtInfo->wFormatTag == MS_PMC_TAG) // If tag is Microsoft PCM Tag, read bitsPerSample
        read(fd, &wavFmtInfo->nBitsPerSample, sizeof(WORD));

    return 0;
}

int processLISTSegment(int fd, riff_header *header, riff_list_info *riffListInfo)
{
    char subtype[4];
    read(fd, subtype, 4);
    if (strncmp(subtype, "INFO", 4) == 0) {
        uint infoSize = header->size - 4;
        read(fd, riffListInfo->name, infoSize);

        // Clean String
        strCleanZeros(riffListInfo->name, infoSize);
        riffListInfo->name[infoSize] = '\0';
    }
    return 0;
}

int processDataSegment(int fd, riff_header *header, uint *dataLength)
{
    *dataLength = header->size;
    //lseek(fd, 0, SEEK_END);
    lseek(fd, *dataLength, SEEK_CUR);
    return 0;
}