#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#define PGMEXT "pgm"
#define SKEXT "sk"
typedef unsigned char     uint8_t;
typedef unsigned short    uint16_t;
typedef unsigned int      uint32_t;
typedef unsigned long int uint64_t;
typedef unsigned char byte;
const byte KEEPOPERAND  = (byte)63;
enum { DX = 0, DY = 1, TOOL = 2, DATA = 3};

// Tool Types
enum { NONE = 0, BLOCK = 2, COLOUR = 3, TARGETX = 4, TARGETY = 5, PAUSE = 7};

// Data structure holding the drawing state
typedef struct state { int x, y, tx, ty; unsigned char tool; unsigned int data; byte colour;} state;

typedef struct bounds {int leftBoundIndex, rightBoundIndex, topBoundIndex, botBoundIndex;} bounds;
typedef struct colourFreq {byte colour; int freq;} colourFreq;
typedef struct colourFreqList {colourFreq* arr; int len;} colourFreqList;

struct rectangle {
    int X, Y, W, H, grayVal; //x and y are respective axis values of the top left pixel of the block. The pixel 0, 0 is at the top left of the byte array.
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
    byte** rowArray; //Array of rows from top to bottom e.g. rowArray[0] = top row of pixels;
};
typedef struct PGM PGM;


void error(bool* errorVar, char* msg) {
    *errorVar = true;
    printf("%s\n", msg);
}

//Miscellaneous functions START

rectangleArr* initializeRectangleArr() {
    rectangleArr* newRectArr = malloc(sizeof(rectangleArr));
    newRectArr->len = 0;
    newRectArr->arr = malloc(0);
    return newRectArr;
}

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


void destroy2DByteArr(byte** arr, int len) {
    while (len-- > 0) {
        free(arr[len]);
    }
    free(arr);
}

void destroy2DCharArr(char** arr, int len) {
    while (len-- > 0) {
        free(arr[len]);
    }
    free(arr);
}

void destroyPGM(PGM* input) {
    destroy2DByteArr(input->rowArray, input->H);
    free(input);
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
    PGMData->rowArray = malloc(PGMData->H * sizeof(byte*));
    if (PGMData->maxVal > 255) bytesPerPixel = 2;
    //read in rest of bytes
    for (int i = 0; i < PGMData->H; i++) {
        PGMData->rowArray[i] = malloc(PGMData->W * sizeof(byte));
        for (int j = 0; j < PGMData->W; j++) {
            PGMData->rowArray[i][j] = (byte)convertNBytes(byteArr, bytesPerPixel, &index);
        }
    }
    destroyByteArray(byteArr);
    return PGMData;
}


colourFreqList* getColourFreqs(PGM* PGMData) {
    colourFreqList* colourFreqs = malloc(sizeof(colourFreqList));
    colourFreqs->len = 0;
    colourFreqs->arr = malloc(0);
    
    for (int i = 0; i < PGMData->H * PGMData->W; i++) {
        bool foundColour = false;
        for (int j = 0; j < colourFreqs->len; j++) {
            if (colourFreqs->arr[j].colour == PGMData->rowArray[i / PGMData->W][i % PGMData->W])  {
                colourFreqs->arr[j].freq += 1;
                foundColour = true;
            }
        }
        if (!foundColour) {
            //allocate memory for new colour
            colourFreqs->len++;
            colourFreqs->arr = realloc(colourFreqs->arr, colourFreqs->len * sizeof(colourFreq));
            //assign new colour
            colourFreqs->arr[colourFreqs->len-1].colour = PGMData->rowArray[i / PGMData->W][i % PGMData->W];
            //assign freq to 1 for new colour
            colourFreqs->arr[colourFreqs->len-1].freq = 1;

        }
        
    }
    return colourFreqs;
}

void swap(colourFreqList* colourFreqs, int i, int j) {
    colourFreq temp = colourFreqs->arr[j];
    colourFreqs->arr[j] = colourFreqs->arr[i];
    colourFreqs->arr[i] = temp;
}

void sortDescending(colourFreqList* colourFreqs) {

    for (int i = 0; i < colourFreqs->len-1; i++) { 
        for (int j = 0; j < colourFreqs->len-i-1; j++) {
            if (colourFreqs->arr[j].freq < colourFreqs->arr[j+1].freq) {
                //printf("[j] [j+1] %d %d\n", colourFreqs->arr[j].freq, colourFreqs->arr[j+1].freq);
                swap(colourFreqs, j, j+1);
                //printf("[j] [j+1] %d %d\n", colourFreqs->arr[j].freq, colourFreqs->arr[j+1].freq);
            }
        }
    }
}

PGM* copyPGMMetaData(PGM* PGMData) {
    PGM* newPGM = malloc(sizeof(PGM));
    //copying metadata of original PGM (that was taken from the file)
    newPGM->W = PGMData->W;
    newPGM->H = PGMData->H;
    newPGM->maxVal = PGMData->maxVal;
    newPGM->rowArray = malloc(newPGM->H * sizeof(byte*));
    //iterating through every row
    for (int i = 0; i < newPGM->H; i++) {
        //mallocing every row
        newPGM->rowArray[i] = malloc(newPGM->W * sizeof(byte));
        //setting every pixel to 0;
        for (int j = 0; j < newPGM->W; j++) {
            newPGM->rowArray[i][j] = (byte)0;
        }
    }
    return newPGM;
}

int getNumOfPixelsInMask(PGM* pixelMask) {
    int num = 0;
    for (int i = 0; i < pixelMask->H * pixelMask->W; i++) {
        if (pixelMask->rowArray[i / pixelMask->W][i % pixelMask->W] == (byte)1) {
            num++;
        }
    }

    return num;
}
PGM* updateCurrentPixelColourMask(PGM* pixelMask, PGM* PGMData, byte currentColour) { 
    
    for (int i = 0; i < PGMData->H * PGMData->W; i++) {
        if (PGMData->rowArray[i / PGMData->W][i % PGMData->W] == currentColour) {
            pixelMask->rowArray[i / PGMData->W][i % PGMData->W] = (byte)1;
        }
        
    }
    return pixelMask;
}

int findPixelInMask(PGM* pixelMask) {
    for (int i = 0; i < pixelMask->H * pixelMask->W; i++) {
        if (pixelMask->rowArray[i / pixelMask->W][i % pixelMask->W] == (byte)1) {
            return i;
        }
    }
    return 1;

}

bool isIndexOutsidePGM(int index, PGM* PGMMask) {
    if (index == PGMMask->H || index == -1) return true;
    return false;
}
bool ContainsIllegal(PGM* illegalPixels, bounds* blockBounds) {
    if (isIndexOutsidePGM(blockBounds->topBoundIndex, illegalPixels)) return true;
    if (isIndexOutsidePGM(blockBounds->botBoundIndex, illegalPixels)) return true;
    if (isIndexOutsidePGM(blockBounds->leftBoundIndex, illegalPixels)) return true;
    if (isIndexOutsidePGM(blockBounds->rightBoundIndex, illegalPixels)) return true;
    for (int i = blockBounds->topBoundIndex; i <= blockBounds->botBoundIndex; i++) {
        for (int j = blockBounds->leftBoundIndex; j <= blockBounds->rightBoundIndex; j++) {

            if (illegalPixels->rowArray[i][j] == (byte)1) {
                return true;
            }
        }
        
    }
    
    return false;
}

void maxOut(PGM* illegalPixels, bounds* outputBounds, bool widthFirst) {

    while (!ContainsIllegal(illegalPixels, outputBounds)) {
        if (widthFirst) outputBounds->leftBoundIndex--;
        else outputBounds->topBoundIndex--;
    }
    
    if (widthFirst) outputBounds->leftBoundIndex++;
    else outputBounds->topBoundIndex++;

    while (!ContainsIllegal(illegalPixels, outputBounds)) {
        if (widthFirst) outputBounds->rightBoundIndex++;
        else outputBounds->botBoundIndex++;
    }
    if (widthFirst) outputBounds->rightBoundIndex--;
    else outputBounds->botBoundIndex--;

}

bounds getBounds(PGM* PGMData, PGM* unfilledRequiredPixels, PGM* illegalPixels, int currentPixelIndex, bool widthFirst) {
    bounds output;
    output.leftBoundIndex = currentPixelIndex % PGMData->W;
    output.rightBoundIndex = output.leftBoundIndex;
    output.topBoundIndex = currentPixelIndex / PGMData->W;
    output.botBoundIndex = output.topBoundIndex;

    maxOut(illegalPixels, &output, widthFirst);
    maxOut(illegalPixels, &output, !widthFirst);
    return output;
}

int getPixelsWithinBounds(PGM* unfilledRequiredPixels, bounds maxedBounds) {
    int num = 0;
    for (int i = maxedBounds.topBoundIndex; i < maxedBounds.botBoundIndex; i++) {
        for (int j = maxedBounds.leftBoundIndex; j < maxedBounds.rightBoundIndex; j++) {
            if (unfilledRequiredPixels->rowArray[i][j] == (byte)1) num++;
        }
    }

    return num;
}

void setPGMMaskToBounds(PGM* outputPGM, bounds maxedBounds) {
    for (int i = maxedBounds.topBoundIndex; i <= maxedBounds.botBoundIndex; i++) {
        for (int j = maxedBounds.leftBoundIndex; j <= maxedBounds.rightBoundIndex; j++) {
            outputPGM->rowArray[i][j] = (byte)1;
        }
    }
}

bounds findLargestRectangle(PGM* PGMData, PGM* unfilledRequiredPixels, PGM* illegalPixels, int currentPixelIndex, PGM* largestRectanglePGM) {

    bounds widthMaxedOutBounds = getBounds(PGMData, unfilledRequiredPixels, illegalPixels, currentPixelIndex, true);
    int widthMaxedOutFirst = getPixelsWithinBounds(unfilledRequiredPixels, widthMaxedOutBounds);

    bounds colMaxedOutBounds = getBounds(PGMData, unfilledRequiredPixels, illegalPixels, currentPixelIndex, false);
    int colMaxedOutFirst = getPixelsWithinBounds(unfilledRequiredPixels, colMaxedOutBounds);

    bounds output;
    if (widthMaxedOutFirst > colMaxedOutFirst) {
        setPGMMaskToBounds(largestRectanglePGM, widthMaxedOutBounds);
        output = widthMaxedOutBounds;
    }
    else {
        setPGMMaskToBounds(largestRectanglePGM, colMaxedOutBounds);
        output = colMaxedOutBounds;
        
    }
    //strip rows and columns
    return output;
}

void mergePixelColourMask(PGM* base, PGM* overlay) {
    //where the new block (overlay) has pixels == 1, and base has pixels == 1, the pixels are set to 0 in base
    //base is unfilledRequiredPixels and overlay is largestRectanglePGM
    //so we are trying to remove the pixels which have been filled by the rectangle
    for (int i = 0; i < base->H; i++) {
        for (int j = 0; j < base->W; j++) {
            if (overlay->rowArray[i][j] == (byte)1) //printf("i j %d %d\n", i, j);
            if (base->rowArray[i][j] == (byte)1 && overlay->rowArray[i][j] == (byte)1) {
                base->rowArray[i][j] = (byte)0;
                //printf("they have pixels in common!!!!\n");
            }
        }
    }

}

rectangle boundsToRectangle(byte colour, bounds inputBounds) {
    rectangle output;
    output.X = inputBounds.leftBoundIndex;
    output.Y = inputBounds.topBoundIndex;
    output.W = inputBounds.rightBoundIndex - inputBounds.leftBoundIndex + 1;
    output.H = inputBounds.botBoundIndex - inputBounds.topBoundIndex + 1;
    //printf("X Y %d %d\n", output.X, output.Y);
    output.grayVal = colour;
    return output;
}

void addRectangle(rectangleArr* blockList, rectangle block) {
    blockList->len++;
    blockList->arr = realloc(blockList->arr, blockList->len * sizeof(rectangle));
    blockList->arr[blockList->len-1] = block;
}

rectangleArr* constructBlocks(byte colour, PGM* illegalPixels, PGM* PGMData) {
    
    rectangleArr* currentBlocks = malloc(sizeof(rectangleArr));
    currentBlocks->arr = malloc(0);
    currentBlocks->len = 0;
    //find all pixels of the current colour that haven't been filled yet
    PGM* unfilledRequiredPixels = copyPGMMetaData(PGMData);
    updateCurrentPixelColourMask(unfilledRequiredPixels, PGMData, colour);
    int numOfRequiredPixels = getNumOfPixelsInMask(unfilledRequiredPixels);
    while (numOfRequiredPixels != 0) {
        //printMaskPercentageAndCount(unfilledRequiredPixels);
        int currentPixelIndex = findPixelInMask(unfilledRequiredPixels);
        //form largest rectangle that contains currentPixel and does not contain illegal pixels
        PGM* largestRectanglePGM = copyPGMMetaData(PGMData);
        bounds largestRectangleBounds = findLargestRectangle(PGMData, unfilledRequiredPixels, illegalPixels, currentPixelIndex, largestRectanglePGM);
        
        mergePixelColourMask(unfilledRequiredPixels, largestRectanglePGM);
        destroyPGM(largestRectanglePGM);
        numOfRequiredPixels = getNumOfPixelsInMask(unfilledRequiredPixels);

        rectangle largestRectangle = boundsToRectangle(colour, largestRectangleBounds);
        addRectangle(currentBlocks, largestRectangle);
    }
    destroyPGM(unfilledRequiredPixels);
    return currentBlocks;
}

void concatenateBlocks(rectangleArr* permList, rectangleArr* tempList) {
    permList->arr = realloc(permList->arr, (permList->len + tempList->len) * sizeof(rectangle));
    for (int i = 0; i < tempList->len; i++) {
        permList->arr[permList->len] = tempList->arr[i];
        permList->len++;
    }
}

rectangleArr* RLE(PGM* PGMData) {
    rectangleArr* blocks = malloc(sizeof(rectangleArr));
    blocks->len = 0;
    blocks->arr = malloc(0);
    colourFreqList* colourFreqs = getColourFreqs(PGMData);
    sortDescending(colourFreqs);
    PGM* illegalPixels = copyPGMMetaData(PGMData);
    for (int i = 0; i < colourFreqs->len; i++) {
        byte currentColour = colourFreqs->arr[i].colour;
        rectangleArr* currentColourBlocks = constructBlocks(currentColour, illegalPixels, PGMData);
        concatenateBlocks(blocks, currentColourBlocks);
        free(currentColourBlocks->arr);
        free(currentColourBlocks);
        //printMaskPercentageAndCount(illegalPixels);
        updateCurrentPixelColourMask(illegalPixels, PGMData, currentColour);
    }
    free(colourFreqs->arr);
    free(colourFreqs);
    destroyPGM(illegalPixels);
    destroyPGM(PGMData);
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
    doublerealloc(outputByteArr);
    outputByteArr->bytes[outputByteArr->len] = input;
    outputByteArr->len++;
}

void setDataInt(int intVal, byteArray* outputByteArr) {
    byte val = (intVal >> (6 * 5)) & (byte)3;
    bool nonZeroHasOccured = false;
    if (val != 0) nonZeroHasOccured = true;
    if (nonZeroHasOccured) addByte(setDATA(val), outputByteArr);
    for (int i = 4; i >= 0; i--) {
        val = (intVal >> (6 * i)) & (byte)63;
        if (val != 0) nonZeroHasOccured = true;
        if (nonZeroHasOccured) addByte(setDATA(val), outputByteArr);
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
        if (i != 0) {
            if (blockArray->arr[i].grayVal != blockArray->arr[i-1].grayVal) {      
                setColour(blockArray->arr[i].grayVal, SKBytes);
            }
        }
        else {
            setColour(blockArray->arr[i].grayVal, SKBytes);
        }
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

//SKTOPGM
state *newState() {
  state* s = malloc(sizeof(state));
  s->x = 0;
  s->y = 0;
  s->tx = 0;
  s->ty = 0;
  s->tool = 0;
  s->data = 0;
  s->colour = (byte)0;
  return s;
}

unsigned int getData(state* s) {
  int output = s->data;
  s->data = 0;
  return output;
}

int getOpcode(byte b) {
  return (int)(b >> 6); // this is a placeholder only
}

// Extract an operand (-32..31) from the rightmost 6 bits of a byte.
int getOperand(byte b) {
  int output = 0;
  if (b & (byte)pow(2, 5)) output = (b | (~KEEPOPERAND)); //set leading bits to 1 if input is two's complement negative 
  else output = b & KEEPOPERAND; //if two's complement is positive then set leading bits to 0
  return output;
}

void drawBlock(PGM* outputPGM, int x, int y, int w, int h, byte colour) {
    for (int i = x; i < x+w; i++) {
        for (int j = y; j < y+h; j++) {
            outputPGM->rowArray[j][i] = colour;
        }
    }
}
void draw(PGM* outputPGM, state* s) {
  switch(s->tool) {
    case NONE:
      break;
    case BLOCK:
      drawBlock(outputPGM, s->x, s->y, s->tx - s->x, s->ty - s->y, s->colour);
      break;
  }
  s->x = s->tx;
  s->y = s->ty;
  }

void runDX(state* s, int operand) {
  s->tx += operand;
}

void runDY(PGM* outputPGM, state* s, int operand) {
  s->ty += operand;
  draw(outputPGM, s);
}


void changeTOOL(state* s, int operand) {
  switch(operand) {
    case COLOUR:
    {
      s->colour = getData(s);
      break;
    }
    case TARGETX:
      s->tx = getData(s);
      break;

    case TARGETY:
      s->ty = getData(s);
      break;

    default:
      s->tool = operand;
      break;
  }
}

void runDATA(state* s, int operand) {
  byte input = (byte) operand;
  if (operand < 0) {
    input = ~input + 1; //convert negative two's complement int to positive two's complement int 
    input = ((~input) & KEEPOPERAND) + 1; //convert positive two's complement int to negative two's complement 6 bit number by setting the 2 leading bits to 0
  }
  //if positive then no change needs to be made
  s->data = (s->data << 6) | input;
}

void obey(PGM* outputPGM, state *s, byte op) {
  int opcode = getOpcode(op);
  int operand = getOperand(op);
  switch (opcode) {
    case DX:
      runDX(s, operand);
      break;
    case DY:
      runDY(outputPGM, s, operand);
      break;
    case TOOL:
      changeTOOL(s, operand);
      break;
    case DATA:
      runDATA(s, operand);
      break;
  }


}
PGM* initializePGM() {
    PGM* newPGM = malloc(sizeof(PGM));
    //copying metadata of original PGM (that was taken from the file)
    newPGM->W = 200;
    newPGM->H = 200;
    newPGM->maxVal = 255;
    newPGM->rowArray = malloc(newPGM->H * sizeof(byte*));
    //iterating through every row
    for (int i = 0; i < newPGM->H; i++) {
        //mallocing every row
        newPGM->rowArray[i] = malloc(newPGM->W * sizeof(byte));
        //setting every pixel to 0;
        for (int j = 0; j < newPGM->W; j++) {
            newPGM->rowArray[i][j] = (byte)0;
        }
    }
    return newPGM;
}

byteArray* convertDataToByteArray(PGM* input) {
    const char header[16] = "P5 200 200 255\n";
    byteArray* output = malloc(sizeof(byteArray));
    output->len = input->H * input->W;
    output->bytes = malloc((output->len + strlen(header)) * sizeof(byte));
    for (int i = 0; i < strlen(header); i++) {
        output->bytes[i] = header[i];
    }
    for (int i = 0; i < input->H  * input->W; i++) {
        output->bytes[i + strlen(header)] = input->rowArray[i / input->W][i % input->W];
    }
    destroyPGM(input);
    return output;
}
byteArray* convertSKToPGM(char* fileName) {
    PGM* data = initializePGM();
    state *s = newState();
    FILE* byteFile = fopen(fileName, "rb");
    byte currentByte = fgetc(byteFile);
    while (!feof(byteFile)) {
        obey(data, s, currentByte);
        currentByte = fgetc(byteFile);
    }
    free(s);
    return convertDataToByteArray(data);
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
    destroy2DCharArr(fileNameComponents, outputLen);
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
    printf("File %s has been written.", outputFileName);
    destroy2DCharArr(components, outputLen);
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
