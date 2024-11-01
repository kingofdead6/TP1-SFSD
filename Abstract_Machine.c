#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define BLOCK_SIZE 256
#define DELIMITER "|"

typedef struct {
    int key;
    char First_Name[10];
    char Last_Name[10];
    char Description[60];
    bool Eraser;
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

Block *AllocBlock(TOVS_file *file) {
    Block *block = (Block*)malloc(sizeof(Block));
    if (!block) {
        fprintf(stderr, "Memory allocation failed.\n");
        return NULL;
    }
    memset(block->data, 0, BLOCK_SIZE);
    block->Number_of_records = 0;
    block->Byte_Used = 0;
    file->header.Number_of_Blocks++;
    return block;
}

int readBlock(FILE *file, int blockNumber, Block *block) {
    fseek(file, sizeof(Header) + blockNumber * sizeof(Block), SEEK_SET);
    return fread(block, sizeof(Block), 1, file);
}

void writeBlock(FILE *file, int blockNumber, Block *block) {
    fseek(file, sizeof(Header) + blockNumber * sizeof(Block), SEEK_SET);
    fwrite(block, sizeof(Block), 1, file);
    printf("Block %d written with %d bytes used.\n", blockNumber, block->Byte_Used);
}

void setHeader(FILE *file, Header *header) {
    fseek(file, 0, SEEK_SET);
    fwrite(header, sizeof(Header), 1, file);
}

Header getHeader(FILE *file) {
    Header header;
    fseek(file, 0, SEEK_SET);
    fread(&header, sizeof(Header), 1, file);
    return header;
}

void Record_to_String(Record rec, char *s) {
    sprintf(s, "%d,%s,%s,%s,%d%s",
            rec.key, rec.First_Name, rec.Last_Name, rec.Description,
            rec.Eraser ? 1 : 0, DELIMITER);
}

void String_to_Record(const char *s, Record *rec) {
    sscanf(s, "%d,%9[^,],%9[^,],%59[^,],%d", 
           &rec->key, rec->First_Name, rec->Last_Name,
           rec->Description, (int*)&rec->Eraser);
}

void insertRecord(TOVS_file *file, Record rec) {
    char recordStr[BLOCK_SIZE];
    Record_to_String(rec, recordStr);
    int recordLen = strlen(recordStr);

    Block block;
    Record records[1000];
    int totalRecords = 0;
    bool isDuplicate = false;

    for (int blockNumber = 0; blockNumber < file->header.Number_of_Blocks; blockNumber++) {
        readBlock(file->file, blockNumber, &block);
        char *token = strtok(block.data, DELIMITER);

        while (token != NULL) {
            Record existingRecord;
            String_to_Record(token, &existingRecord);

            if (!existingRecord.Eraser && existingRecord.key == rec.key) {
                isDuplicate = true;
            }

            records[totalRecords++] = existingRecord;
            token = strtok(NULL, DELIMITER);
        }
    }

    if (isDuplicate) {
        printf("Duplicate record with key %d found. Insertion skipped.\n", rec.key);
        return;
    }

    int insertPos = 0;
    while (insertPos < totalRecords && records[insertPos].key < rec.key) {
        insertPos++;
    }

    for (int i = totalRecords; i > insertPos; i--) {
        records[i] = records[i - 1];
    }
    records[insertPos] = rec;
    totalRecords++;

    int blockNumber = 0;
    memset(&block, 0, sizeof(Block));
    block.Number_of_records = 0;
    block.Byte_Used = 0;

    int currentRecordIndex = 0;

    while (currentRecordIndex < totalRecords) {
        char tempStr[BLOCK_SIZE];
        Record_to_String(records[currentRecordIndex], tempStr);
        int tempLen = strlen(tempStr);

        if (block.Byte_Used + tempLen + (block.Number_of_records > 0 ? strlen(DELIMITER) : 0) > BLOCK_SIZE) {
            writeBlock(file->file, blockNumber++, &block);
            memset(&block, 0, sizeof(Block));
            block.Number_of_records = 0;
            block.Byte_Used = 0;
        }

        if (block.Number_of_records > 0) {
            strcat(block.data, DELIMITER);
            block.Byte_Used += strlen(DELIMITER);
        }

        strcat(block.data, tempStr);
        block.Byte_Used += tempLen;
        block.Number_of_records++;

        currentRecordIndex++;
    }

    if (block.Byte_Used > 0) {
        writeBlock(file->file, blockNumber, &block);
    }

    file->header.Number_of_Blocks = blockNumber + 1;
    file->header.Number_of_Records = totalRecords;
    setHeader(file->file, &file->header);
}

void Display_Header(TOVS_file *file) {
    printf("The number of blocks here is  : %d \n", file->header.Number_of_Blocks);
    printf("The number of records here is  : %d \n", file->header.Number_of_Records);
}

void display_File(TOVS_file *file) {
    Display_Header(file);
    Block block;
    for (int i = 0; i < file->header.Number_of_Blocks; i++) {
        readBlock(file->file, i, &block);
        char *token = strtok(block.data, DELIMITER);
        
        printf("Block %d:\n", i);
        while (token != NULL) {
            Record existingRecord;
            String_to_Record(token, &existingRecord);

            if (!existingRecord.Eraser) {
                printf("  %s\n", token);
            }
            token = strtok(NULL, DELIMITER);
        }
    }
}

void Display_block(TOVS_file *file, int num_of_block) {
    Block block;
    readBlock(file->file, num_of_block, &block);
    char *token = strtok(block.data, DELIMITER);
    
    printf("Contents of Block %d:\n", num_of_block);
    while (token != NULL) {
        Record existingRecord;
        String_to_Record(token, &existingRecord);

        if (!existingRecord.Eraser) {
            printf("  %s\n", token);
        }
        token = strtok(NULL, DELIMITER);
    }
}

TOVS_file *OpenTOVS(const char *filename, const char *mode) {
    FILE *file = fopen(filename, mode);
    if (!file && mode[0] == 'r') {
        file = fopen(filename, "wb+");
        if (file) {
            Header header = {0, 0};
            setHeader(file, &header);
        }
    }
    TOVS_file *tovsFile = (TOVS_file*)malloc(sizeof(TOVS_file));
    tovsFile->file = file;
    tovsFile->header = getHeader(file);
    return tovsFile;
}

void CloseTOVS(TOVS_file *file) {
    setHeader(file->file, &file->header);
    if (file->file) fclose(file->file);
    free(file);
}

int searchRecordByKey(TOVS_file *file, int key, int *blockNumber, int *position) {
    Block block;

    for (int i = 0; i < file->header.Number_of_Blocks; i++) {
        readBlock(file->file, i, &block);
        char *token = strtok(block.data, DELIMITER);

        int pos = 0;
        while (token != NULL) {
            Record existingRecord;
            String_to_Record(token, &existingRecord);

            if (!existingRecord.Eraser && existingRecord.key == key) {
                *blockNumber = i;
                *position = pos;
                return 1;
            }

            token = strtok(NULL, DELIMITER);
            pos++;
        }
    }

    return 0;
}

int logicalDeleteRecord(TOVS_file *file, int key) {
    Block block;
    bool found = false;

    for (int i = 0; i < file->header.Number_of_Blocks; i++) {
        readBlock(file->file, i, &block);
        char *token = strtok(block.data, DELIMITER);
        char newBlockData[BLOCK_SIZE] = {0};
        int pos = 0;

        while (token != NULL) {
            Record existingRecord;
            String_to_Record(token, &existingRecord);

            if (existingRecord.key == key && !existingRecord.Eraser) {
                existingRecord.Eraser = true;
                found = true;
            }

            char recordStr[BLOCK_SIZE];
            Record_to_String(existingRecord, recordStr);
            if (pos > 0) strcat(newBlockData, DELIMITER);
            strcat(newBlockData, recordStr);
            pos++;

            token = strtok(NULL, DELIMITER);
        }

        if (found) {
            strncpy(block.data, newBlockData, BLOCK_SIZE);
            writeBlock(file->file, i, &block);
            return 1;
        }
    }

    return 0;
}
int physicalDeleteRecordByKey(TOVS_file *file, int key) {
    Block block;
    Record records[1000]; 
    int totalRecords = 0;
    bool found = false;

    for (int i = 0; i < file->header.Number_of_Blocks; i++) {
        readBlock(file->file, i, &block);
        char *token = strtok(block.data, DELIMITER);

        while (token != NULL) {
            Record existingRecord;
            String_to_Record(token, &existingRecord);

            if (existingRecord.key == key) {
                found = true;
            } else {
                records[totalRecords++] = existingRecord;
            }
            token = strtok(NULL, DELIMITER);
        }
    }

    if (!found) {
        printf("Record with key %d not found for physical deletion.\n", key);
        return 0;
    }

    int blockNumber = 0;
    memset(&block, 0, sizeof(Block));
    block.Number_of_records = 0;
    block.Byte_Used = 0;

    for (int i = 0; i < totalRecords; i++) {
        char tempStr[BLOCK_SIZE];
        Record_to_String(records[i], tempStr);
        int tempLen = strlen(tempStr);

        if (block.Byte_Used + tempLen + (block.Number_of_records > 0 ? 1 : 0) > BLOCK_SIZE) {
            writeBlock(file->file, blockNumber++, &block);
            memset(&block, 0, sizeof(Block)); 
        }

        if (block.Number_of_records > 0) {
            strcat(block.data, DELIMITER);
            block.Byte_Used += strlen(DELIMITER);
        }

        strcat(block.data, tempStr);
        block.Byte_Used += tempLen;
        block.Number_of_records++;
    }

    if (block.Byte_Used > 0) {
        writeBlock(file->file, blockNumber, &block);
    }

    file->header.Number_of_Blocks = blockNumber + 1;
    file->header.Number_of_Records = totalRecords;
    setHeader(file->file, &file->header);

    printf("Record with key %d physically deleted.\n", key);
    return 1; 
}
void initialLoad(TOVS_file *file, int rate) {
    for (int i = 1; i <= rate; i++) {
        Record rec;
        rec.key = i;
        snprintf(rec.First_Name, sizeof(rec.First_Name), "First%d", i);
        snprintf(rec.Last_Name, sizeof(rec.Last_Name), "Last%d", i);
        snprintf(rec.Description, sizeof(rec.Description), "Record number %d", i);
        rec.Eraser = false; // Set to false since we're adding new records

        insertRecord(file, rec);
    }
    printf("Initial load complete with %d records.\n", rate);
}
int main() {
    TOVS_file *tovsFile = OpenTOVS("tovs.dat", "rb+");
    if (!tovsFile->file) {
        fprintf(stderr, "Failed to open file.\n");
        return 1;
    }

    // Sample records for testing
     Record records[10] = {
        {1, "John", "Doe", "First record", false},
        {2, "Jane", "Doe", "Second record", false},
        {3, "Alice", "Smith", "Third record", false},
        {4, "Bob", "Brown", "Fourth record", false},
        {5, "Charlie", "Johnson", "Fifth record", false},
        {6, "Eve", "Davis", "Sixth record", false},
        {7, "Frank", "Wilson", "Seventh record", false},
        {8, "Grace", "Lee", "Eighth record", false},
        {9, "Hank", "Miller", "Ninth record", false},
        {10, "Ivy", "Moore", "Tenth record", false}
    };
/*
    // Insert records
    for (int i = 0; i < 10; i++) {
        insertRecord(tovsFile, records[i]);
    }*/
    // Display_block(tovsFile, 0);
    // Display_block(tovsFile, 1 );
    int initialRate = 100;
    initialLoad(tovsFile, initialRate);

     display_File(tovsFile);

    int blockNumber, position , searchingnum;
    searchingnum = 12 ;
    if (searchRecordByKey(tovsFile, searchingnum, &blockNumber, &position)) {
        printf("Record found in Block %d at Position %d\n", blockNumber, position+1);
    } else {
        printf("Record with key %d was not found.\n" ,searchingnum);
    }


     int keyToDelete = 5; 
    if (logicalDeleteRecord(tovsFile, keyToDelete)) {
        printf("Record with key %d logically deleted.\n", keyToDelete);
    } else {
        printf("Record with key %d not found.\n", keyToDelete);
    }

     if (physicalDeleteRecordByKey(tovsFile, keyToDelete)) {
        printf("Record with key %d has been physically deleted.\n", keyToDelete);
    } else {
        printf("Record with key %d not found.\n", keyToDelete);
    }

    display_File(tovsFile);
    CloseTOVS(tovsFile);
    return 0;
}
