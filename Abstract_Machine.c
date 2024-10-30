#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define BLOCK_SIZE 256

typedef struct {
    int key;
    char First_Name[10];
    char Last_Name[10];
    char Discription[60];
    bool Eraser;
    int size;
} Record;

typedef struct {
    char data[BLOCK_SIZE];
    int Byte_Used;
    int Number_of_records;
} Block;

typedef struct {
    int Number_of_Blocks;
    int Number_of_Records;
} Header;

typedef struct {
    FILE *file;
    Header header;
} TOVS_file;

// Allocate a new block and initialize fields
Block *AllocBlock(TOVS_file *file) {
    Block *block = (Block*)malloc(sizeof(Block));
    if (block == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        return NULL;
    }
    memset(block->data, 0, BLOCK_SIZE); // Clear block data
    block->Number_of_records = 0;
    block->Byte_Used = 0;
    file->header.Number_of_Blocks++;
    return block;
}

// Read a block from file
int readBlock(FILE *file, int blockNumber, Block *block) {
    fseek(file, sizeof(Header) + blockNumber * sizeof(Block), SEEK_SET);
    return fread(block, sizeof(Block), 1, file);
}

// Write a block to file
void writeBlock(FILE *file, int blockNumber, Block *block) {
    fseek(file, sizeof(Header) + blockNumber * sizeof(Block), SEEK_SET);
    fwrite(block, sizeof(Block), 1, file);
}

// Set the header in the file
void setHeader(FILE *file, Header *header) {
    fseek(file, 0, SEEK_SET);
    fwrite(header, sizeof(Header), 1, file);
}

// Get the header from the file
Header getHeader(FILE *file) {
    Header header;
    fseek(file, 0, SEEK_SET);
    fread(&header, sizeof(Header), 1, file);
    return header;
}

// Open or create a file and initialize the header if necessary
FILE* Open(const char *filename, const char *mode) {
    FILE *file = fopen(filename, mode);
    if (!file && mode[0] == 'r') { // File does not exist, create it
        file = fopen(filename, "wb+");
        if (file) {
            Header header = {0, 0}; // Initialize header
            setHeader(file, &header);
        }
    } else if (file && mode[0] == 'r') { // Reading existing file
        // Read existing header
        Header header = getHeader(file);
        printf("File opened. Blocks: %d, Records: %d\n", header.Number_of_Blocks, header.Number_of_Records);
    }
    return file;
}

// Close the file
void Close(FILE *file) {
    if (file) fclose(file);
}

//testing the Abstract Machine
int main() {
    const char *filename = "testfile.dat";
    
    FILE *file = Open(filename, "rb+");
    if (!file) {
        fprintf(stderr, "Failed to open file.\n");
        return 1;
    }
    
    TOVS_file tovFile;
    tovFile.file = file;
    tovFile.header = getHeader(file);
    
    Block *block = AllocBlock(&tovFile);
    if (!block) {
        Close(file);
        return 1;
    }
    
    strcpy(block->data, "Hello, world!");
    block->Byte_Used = strlen(block->data);
    block->Number_of_records = 1;

    int blockNumber = 0;
    writeBlock(file, blockNumber, block);
    printf("Block written to file.\n");

    Block readBlockData;
    if (readBlock(file, blockNumber, &readBlockData) == 1) {
        printf("Block read from file. Data: %s\n", readBlockData.data);
        printf("Byte_Used: %d, Number_of_records: %d\n", readBlockData.Byte_Used, readBlockData.Number_of_records);
    } else {
        fprintf(stderr, "Failed to read block from file.\n");
    }
    
    setHeader(file, &tovFile.header);

    free(block);
    Close(file);

    return 0;
}
