#include "displayfull.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define PGMEXT "pgm"
#define SKEXT "sk"
typedef unsigned char byte;

enum { DX = 0, DY = 1, TOOL = 2, DATA = 3};

// Tool Types
enum { NONE = 0, BLOCK = 2, COLOUR = 3, TARGETX = 4, TARGETY = 5 };

// Data structure holding the drawing state
typedef struct state { int x, y, tx, ty; unsigned char tool; unsigned int data;} state;


struct rectangle {
    int x, y, width, height; //x and y are respective axis values of the top left pixel of the block. The pixel 0, 0 is at the top left of the byte array.
};
typedef struct rectangle rectangle;

struct rectangleArr {
    rectangle* arr;
    int len;
};
typedef struct rectangleArr rectangleArr;

struct byteArray {
    byte* bytes;
    int len;
};
typedef struct byteArray byteArray;

struct PGM {
    int H, W, maxVal; //HEIGHT, WIDTH, maximum value for a colour
    unsigned short** rowArray; //Array of rows from top to bottom e.g. rowArray[0] = top row of pixels;
};
typedef struct PGM PGM;

void error(bool* errorVar, char* msg) {
    *errorVar = true;
    printf("%s\n", msg);
}

//Miscellaneous functions START
char** split(char* input, const char* delimiter, int* outputlength) {
  //char* splitInput = (char*)malloc((strlen(input) + 1) * sizeof(char));
  char splitInput[(strlen(input) + 1)];
  splitInput[0] = 0;
  strcat(splitInput, input);
  char** output = (char**)malloc(sizeof(char*));
  char* substring = strtok(splitInput, delimiter);
  if (substring != NULL) *outputlength = 1;
  //*output = substring;
  *output = (char*)malloc((1+strlen(substring)) * sizeof(char));
  strcpy(*output, substring);
  while (substring != NULL) {
    substring = strtok(NULL, delimiter);
    if (substring != NULL) {
      output = realloc(output, ++(*outputlength) * sizeof(char*));
      *(output + *outputlength - 1) = (char*)malloc((1+strlen(substring)) * sizeof(char));
      strcpy(*(output + *outputlength - 1), substring);
      //*(output + *outputlength - 1) = substring;
    }
  }
  //free(splitInput);
  return output;
}

int getByteFileLen(File* byteFile) {
    fseek(byteFile, 0L, SEEK_END);
    int output = ftell(fp);
    rewind(fp);
}

byteArray* readByteFile(char* fileName) {
    FILE* inputFile = fopen(fileName, "r");
    byteArray* byteArr = malloc(sizeof(byteArray));
    byteArr->len = getByteFileLen(inputFile);
    byteArr->bytes = malloc(bytearr->len);
    fread(byteArr->bytes, bytearr->len, 1, inputFile);
    fclose(inputFile);
    return byteArray;
}

int readASCIIBytes(byteArray* byteArr, int* index) {
    byteArray ASCIIChars;
    ASCIIChars.bytes = malloc(0);
    while (byteArr->bytes[*index] != (uint8_t)' ' || byteArr->bytes[*index] != (uint8_t)'\n') {
        ASCIIChars.len++;
        ASCIIChars.bytes = realloc(ASCIIChars.bytes, ASCIIChars.len);
        ASCIIChars.bytes[ASCIIChars.len-1] = byteArr->bytes[*index];
        *index++;
    }
    *index++;
    int output = 0;
    int i = 0;
    while(i < ASCIIChars.len) {
        output += (ASCIIChars.bytes[i] - '0') * pow(10, ASCIIChars.len-1)
    }
    free(ASCIIChars.bytes);
    return output;
}

unsigned int convertNBytes(byteArray* byteArr, int n int* index) { //converts n consecutive bytes to integer
    unsigned int output = 0;
    while (n-- > 0) {
        output = (output << 8) | (unsigned int)byteArr[*index++];
    }
    return output;
}
//Miscellaneous functions END

PGM* readPGM(char* fileName) {
    //initializing byteArray and PGMData output
    PGM* PGMData = malloc(sizeof(PGM));
    byteArray* byteArr = readByteFile(fileName);
    int index = 3;
    //read in intial conditions
    PGMData->W = readASCIIBytes(byteArr, &index);
    PGMData->H = readASCIIBytes(byteArr, &index);
    PGMData->maxVal = readASCIIBytes(byteArr, &index);
    int bytesPerPixel = 1;
    PGMData->rowArray = malloc(PGMData->H) * sizeof(unsigned short*);
    if (PGMData->maxVal > 255) bytesPerPixel = 2;
    //read in rest of bytes
    for (int i = 0; i < PGMData->H; i++) {
        PGMData[i] = malloc(PGMData->W);
        for (int j = 0; j < PGMData->W; j++) {
            PGMData->rowArray[i][j] = (unsigned short)convertNBytes(byteArr, bytesPerPixel, &index);
        }
    }
    return PGMData;
}

rectangleArr* RLE(PGM* PGMData) {

    return NULL;
}

byteArray* blockArrToSK(rectangleArr* blockArray) {

    return NULL;
}

byteArray* convertPGMToSK(char* fileName) {
    PGM* PGMData = readPGM(fileName);
    rectangleArr* blockArray = RLE(PGMData);
    byteArray* SKBytes = blockArrToSK(blockArray);
    return SKBytes;
}



byteArray* convertSKToPGM(char* fileName) {
    /*FILE* byteFile = fopen(filename, "rb");
    byte currentByte = fgetc(byteFile);
    int index = 0;
    while (!feof(byteFile) && !s->end) {
      if (index >= s->start) {
        obey(d, s, currentByte);
      }
      currentByte = fgetc(byteFile);
      index++;
}*/
    return NULL;
}

bool checkFileExists(char* fileName) {
    FILE *file;
    if (file = fopen(fileName, "r")) 
        {
            fclose(file);
            return true;
        }
    return false;
}

char* getFileType(char* fileName) {
    int* outputLen = 0;
    char** fileNameComponents = split(fileName, ".", outputLen);
    return fileNameComponents[outputLen-1];
}

void writeToFile(byteArray* bytes, char* fileName, char* inputFileType) {
    //setting the extension for the output file
    char outputFileName[strlen(fileName) + 2] = 0;
    int* outputLen = 0;
    strcat(outputFileName, split(fileName, ".", outputLen)[0]);
    strcat(outputFileName, ".");
    if (strcmp(inputFileType, PGMEXT) == 0) strcat(outputFileExt, SKEXT);
    else strcat(outputFileName, PGMEXT);
    
    //writing to the file
    FILE* outputFile;
    if((outputFile = fopen(outputFileName, "wb+")) != NULL){
    fwrite(bytes->bytes, 1, bytes->len, outputFile);
    fclose(outputFile);
}
    
}

void convertFile(int n, char* fileName) {
    bool* errorOccured = false;
    checkFileExists(fileName);
    char* fileType = getFileType(fileName);
    byteArray* bytes;
    if (strcmp(fileType, PGMEXT) == 0) bytes = convertPGMToSK(fileName);
    else if (strcmp(fileType, SKEXT) == 0) bytes = convertSKToPGM(fileName);
    else error(errorOccured, "Invalid filetype");
    if (!errorOccured) writeToFile(bytes, fileName, fileType);
}

//UNIT TESTS START
void runUnitTests() {

}
//UNIT TESTS END
int main(int n, char* args[n]) {
    if (n == 2) {
        convertFile(n, args[1]);
    }
    else if (n == 1) {
        runUnitTests();
    }
    else {
        printf("too many arguments\n")
        printf("usage ./converter filepath\n");
    }
    return 0;
}
