#include "displayfull.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define PGMEXT "pgm"
#define SKEXT "sk"
typedef unsigned char byte;

enum { DX = 0, DY = 1, TOOL = 2, DATA = 3};

// Tool Types
enum { NONE = 0, BLOCK = 2, COLOUR = 3, TARGETX = 4, TARGETY = 5, PAUSE = 7};

// Data structure holding the drawing state
typedef struct state { int x, y, tx, ty; unsigned char tool; unsigned int data;} state;


struct rectangle {
    int X, Y, W, H, RGBAVal; //x and y are respective axis values of the top left pixel of the block. The pixel 0, 0 is at the top left of the byte array.
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

bool isMultiplePowerTwo(int input, int base) {
    while (input > base) {
        base *= 2;
    }
    if (input == base) return true;
    else return false;
}
void doublerealloc(byteArray* byteArr) {
    //printf("len: %d\n", byteArr->len);
    if (isMultiplePowerTwo(byteArr->len, 24)) {
        //printf("reallocating\n");
        byteArr->bytes = realloc(byteArr->bytes, byteArr->len * 2);
        //printf("2len: %d\n", byteArr->len * 2);
    }
}
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

void destroy2DArr(void** arr, int len) {
    while (len-- > 0) {
        free(arr[len]);
    }
    free(arr);
}

void destroyByteArray(byteArray* byteArr) {
    free(byteArr->bytes);
    free(byteArr);
}
int getByteFileLen(FILE* byteFile) {
    fseek(byteFile, 0L, SEEK_END);
    int output = ftell(byteFile);
    rewind(byteFile);
    return output;
}

byteArray* readByteFile(char* fileName) {
    FILE* inputFile = fopen(fileName, "r");
    byteArray* byteArr = malloc(sizeof(byteArray));
    byteArr->len = getByteFileLen(inputFile);
    byteArr->bytes = malloc(byteArr->len);
    fread(byteArr->bytes, byteArr->len, 1, inputFile);
    fclose(inputFile);
    return byteArr;
}

int readASCIIBytes(byteArray* byteArr, int* index) {
    byteArray ASCIIChars;
    ASCIIChars.bytes = malloc(24);
    ASCIIChars.len = 0;
    while (byteArr->bytes[*index] != (uint8_t)' ' && byteArr->bytes[*index] != (uint8_t)'\n') {
        ASCIIChars.len++;
        doublerealloc(&ASCIIChars);
        ASCIIChars.bytes[ASCIIChars.len-1] = byteArr->bytes[*index];
        (*index)++;
    }
    (*index)++;
    int output = 0;
    for (int i = 0; i < ASCIIChars.len; i++) {
        output += (ASCIIChars.bytes[i] - '0') * pow(10, ASCIIChars.len-i-1);
    }
    free(ASCIIChars.bytes);
    return output;
}

unsigned int convertNBytes(byteArray* byteArr, int n, int* index) { //converts n consecutive bytes to integer
    unsigned int output = 0;
    while (n-- > 0) {
        output = (output << 8) | (unsigned int)byteArr->bytes[(*index)++];
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
    PGMData->rowArray = malloc(PGMData->H * sizeof(unsigned short*));
    if (PGMData->maxVal > 255) bytesPerPixel = 2;
    //read in rest of bytes
    for (int i = 0; i < PGMData->H; i++) {
        PGMData->rowArray[i] = malloc(PGMData->W * sizeof(unsigned short));
        for (int j = 0; j < PGMData->W; j++) {
            PGMData->rowArray[i][j] = (unsigned short)convertNBytes(byteArr, bytesPerPixel, &index);
        }
    }
    destroyByteArray(byteArr);
    return PGMData;
}

rectangleArr* RLE(PGM* PGMData) {
    rectangleArr* blocks = malloc(sizeof(rectangleArr));
    blocks->len = PGMData->H * PGMData->W;
    blocks->arr = malloc(blocks->len * sizeof(rectangle));
    for (int i = 0; i < PGMData->H; i++) {
        for (int j = 0; j < PGMData->W; j++) { 
            rectangle block;
            block.X = j;
            block.Y = i;
            block.W = 1;
            block.H = 1;
            block.RGBAVal = PGMData->rowArray[i][j];
            blocks->arr[i*PGMData->W + j] = block;
        }
    }
    destroy2DArr(PGMData->rowArray, PGMData->H);
    free(PGMData);
    return blocks;
}

byte setTOOL(int operand) {
    return (TOOL << 6) | (byte)operand;
}

byte setDATA(int operand) {
    return (DATA << 6) | (byte)operand;
}

byte setDY(int operand) {
    return (DY << 6) | operand;
}

void addByte(byte input, byteArray* outputByteArr) {
    if (outputByteArr->len == 24) {
        printf("");
    }
    doublerealloc(outputByteArr);
    outputByteArr->bytes[outputByteArr->len] = input;
    outputByteArr->len++;
}

void setDataInt(int intVal, byteArray* outputByteArr) {
    byte val = (intVal >> (6 * 5)) & (byte)3;
    addByte(setDATA(val), outputByteArr);
    for (int i = 4; i >= 0; i--) {
        //printf("data: %d\n", i);
        val = (intVal >> (6 * i)) & (byte)63;
        addByte(setDATA(val), outputByteArr);
    }
}

void setTarget(int val, bool isX, byteArray* outputByteArr) {
    setDataInt(val, outputByteArr);
    if(isX) addByte(setTOOL(TARGETX), outputByteArr);
    else addByte(setTOOL(TARGETY), outputByteArr);
}

int convertGrayToRGBA(unsigned int grayScaleVal) {
    unsigned int RGBAVal = 0;
    for (int i = 0; i < 4; i++) {
        RGBAVal = (RGBAVal << 8) | grayScaleVal;
    }
    return RGBAVal;
}
void setColour(unsigned int grayScaleVal, byteArray* outputByteArr) {
    unsigned int RGBAVal = convertGrayToRGBA(grayScaleVal);
    setDataInt(RGBAVal, outputByteArr);
    addByte(setTOOL(COLOUR), outputByteArr);
}

void setPause(byteArray* outputByteArr, int len) {
    setDataInt(len, outputByteArr);
    addByte(setTOOL(PAUSE), outputByteArr);
}
byteArray* rectangleArrToSK(rectangleArr* blockArray) {
    byteArray* SKBytes = malloc(sizeof(byteArray));
    SKBytes->bytes = malloc(24);
    SKBytes->len = 0;
    for (int i = 0; i < blockArray->len; i++) {
        //printf("i: %d\nlen: %d\n", i, SKBytes->len);
        //set tool to NONE
        addByte(setTOOL(NONE), SKBytes);
        //setx
        setTarget(blockArray->arr[i].X, true, SKBytes);
        //sety
        setTarget(blockArray->arr[i].Y, false, SKBytes);
        //apply changes to x and y
        addByte(setDY(0), SKBytes);
        //set tool to block
        addByte(setTOOL(BLOCK), SKBytes);
        //set target x
        setTarget(blockArray->arr[i].X + blockArray->arr[i].W, true, SKBytes);
        //set target y
        setTarget(blockArray->arr[i].Y + blockArray->arr[i].H, false, SKBytes);
        //set colour
        setColour(blockArray->arr[i].RGBAVal, SKBytes);
        //draw
        addByte(setDY(0), SKBytes);
    }
    //setPause(SKBytes, 1000000);
    free(blockArray->arr);
    free(blockArray);
    return SKBytes;
}

byteArray* convertPGMToSK(char* fileName) {
    PGM* PGMData = readPGM(fileName);
    rectangleArr* blockArray = RLE(PGMData);
    byteArray* SKBytes = rectangleArrToSK(blockArray);
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
    if ((file = fopen(fileName, "r"))) {
            fclose(file);
            return true;
        }
    return false;
}

char* getFileType(char* fileName) {
    int outputLen = 0;
    char** fileNameComponents = split(fileName, ".", &outputLen);
    char* extension = malloc(sizeof(char) * (strlen(*(fileNameComponents + (outputLen-1))) + 1));
    strcpy(extension, *(fileNameComponents + (outputLen-1)));
    destroy2DArr(fileNameComponents, outputLen);
    return extension;
}

void writeToFile(byteArray* byteArr, char* fileName, char* inputFileType) {
    //setting the extension for the output file
    char outputFileName[strlen(fileName) + 2];
    outputFileName[0] = 0;
    int outputLen = 0;
    char** components = split(fileName, ".", &outputLen);
    strcat(outputFileName, components[0]);
    strcat(outputFileName, ".");
    if (strcmp(inputFileType, PGMEXT) == 0) strcat(outputFileName, SKEXT);
    else strcat(outputFileName, PGMEXT);
    destroy2DArr(components, outputLen);
    //writing to the file
    FILE* outputFile;
    if((outputFile = fopen(outputFileName, "wb+")) != NULL){
        fwrite(byteArr->bytes, 1, byteArr->len, outputFile);
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
    if (!errorOccured) {
        writeToFile(bytes, fileName, fileType);
        destroyByteArray(bytes);
    }
    free(fileType);
}

//UNIT TESTS START
void testWriteToFile() {
    byteArray testbytes;
    testbytes.len = 1;
    testbytes.bytes = malloc(2);
    testbytes.bytes[0] = 'f';
    writeToFile(&testbytes, "testfile.pgm", PGMEXT);
    free(testbytes.bytes);
}

void runUnitTests() {
    testWriteToFile();
    printf("all tests pass\n");
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
        printf("too many arguments\n");
        printf("usage ./converter filepath\n");
    }
    return 0;
}
