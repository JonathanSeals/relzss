/*
cc relzss.c lzss.c -o relzss
*/

#include <stdio.h>
#include "lzss.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#ifndef OSSwapInt32
#define OSSwapInt32 __builtin_bswap32
#endif

int compLZSS(void *compressedDATA, uint32_t *compressedDataSize, void *uncompressedDATA, uint32_t uncompressedDataSize) {
    uint32_t adler32 = 0;
    
    adler32 = lzadler32(uncompressedDATA, uncompressedDataSize);
    
    uint8_t *cmpLen;
    
    cmpLen = malloc(uncompressedDataSize+(uncompressedDataSize/0x40));
    
    uint32_t result = (uint32_t)((compress_lzss(compressedDATA, (uncompressedDataSize*2), uncompressedDATA, uncompressedDataSize)-cmpLen)+*compressedDataSize);
    
    free(cmpLen);
    
    void *nb = malloc(result+0x180);
    bzero(nb, result+0x180);
    
    memcpy(nb+0x180, compressedDATA, result);
    
    *(uint64_t*)nb = 0x73737A6C706D6F63; //complzss
    *(uint32_t*)(nb+0x8) = OSSwapInt32(adler32); //adler32
    *(uint32_t*)(nb+0xC) = OSSwapInt32(uncompressedDataSize); //uncompressed data size
    *(uint32_t*)(nb+0x10) = OSSwapInt32(result); //compressed data size
    *(uint32_t*)(nb+0x14) = OSSwapInt32(0x1); //always 1
    
    memcpy(compressedDATA, nb, result+0x180);
    
    free(nb);
    
    *compressedDataSize = result;
    
    return 0;
}


int reLzss(void *filebuf, uint32_t len, void *outBuf, uint32_t *outBufLen) {
    void *compressedData = malloc(len);
    
    uint32_t *len2 = (uint32_t*)malloc(sizeof(uint32_t));
    
    *len2 = len;
    
    compLZSS(compressedData, len2, filebuf, len);
    
    if (!(*len2 && len)) {
        return -1;
    }
    
    *outBufLen = (*len2+0x180);
    
    bzero(outBuf, (*outBufLen-0x180));
    
    memcpy(outBuf, compressedData, *len2+0x180);
    
    free(compressedData);
    
    return 0;
}

int main(int argc, const char **argv) {
    
    if (argc < 3) {
        printf("Usage: relzss inFile outFile (monitor)\n");
        return -1;
    }
    
    char *inFile = argv[1];
    char *outFile = argv[2];
    char *monitorFile = 0;
    
    if (argc > 3) {
        monitorFile = argv[3];
    }
    
    FILE *inf = fopen(inFile, "r");
    
    if (!inf) {
        printf("Error opening %s\n", inFile);
        return -1;
    }
    
    fseek(inf, 0, SEEK_END);
    size_t inLen = ftell(inf);
    fseek(inf, 0, SEEK_SET);
    
    if (!inLen) {
        printf("%s was empty\n", inFile);
        fclose(inf);
        return -1;
    }
    
    void *inBuf = (void*)malloc(inLen);
    if (!inBuf) {
        printf("Failed to allocate memory for %s\n", inFile);
        fclose(inf);
        return -1;
    }
    bzero(inBuf, inLen);
    
    size_t read = fread(inBuf, 1, inLen, inf);
    
    void *outBuf = (void*)malloc(inLen);
    bzero(outBuf, inLen);
    
    int ret = reLzss(inBuf, inLen, outBuf, &inLen);
    
    if (monitorFile) {
        FILE *monitorfd = fopen(monitorFile, "r");
        if (monitorfd) {
            
            fseek(monitorfd, 0, SEEK_END);
            size_t monitorLen = ftell(monitorfd);
            fseek(monitorfd, 0, SEEK_SET);
            
            void *monitorBuf = (void*)malloc(monitorLen);
            bzero(monitorBuf, monitorLen);
            
            read = fread(monitorBuf, 1, monitorLen, monitorfd);
            
            fclose(monitorfd);
            
            if (read != monitorLen) {
                printf("Error opening monitor file\n");
                free(monitorBuf);
            }
            else {
                outBuf = realloc(outBuf, (inLen+monitorLen));
                memcpy((outBuf+inLen), monitorBuf, monitorLen);
                free(monitorBuf);
                inLen = (inLen+monitorLen);
            }
            
        }
        else {
            printf("Error opening monitor file\n");
        }
    }
    
    FILE *outf = fopen(outFile, "w");
    if (!outf) {
        printf("Failed to write %s\n", outFile);
        return -1;
    }
    
    size_t written = fwrite(outBuf, 1, inLen, outf);
    
    if (written == inLen) {
        printf("Success\n");
    }
    else {
        printf("Error: wrote %lu bytes, wanted %lu\n", written, inLen);
    }
    
    return 0;
}
