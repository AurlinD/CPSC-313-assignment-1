#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

#include "printRoutines.h"

#define ERROR_RETURN -1
#define SUCCESS 0

char *instructions[12] = {"halt", "nop", "rrmovq", "irmovq", "rmmovq", "mrmovq", "opq", "jxx", "call", "ret", "pushq", "popq"};
char *cmovXX[7] = {"rrmovq", "cmovle", "cmovl", "cmove", "cmovne", "cmovge", "cmovg"};
char *OPq[7] = {"addq", "subq", "andq", "xorq", "mulq", "divq", "modq"};
char *jXX[7] = {"jmp", "jle", "jl", "je", "jne", "jge", "jg"};
char *reg[16] = {"%rax", "%rcx", "%rdx", "%rbx", "%rsp", "%rbp", "%rsi", "%rdi", "%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14", "NO REGISTER"};

int main(int argc, char **argv) {
  
  FILE *machineCode, *outputFile;
  long currAddr = 0; 

  // Verify that the command line has an appropriate number
  // of arguments

  if (argc < 2 || argc > 4) {
    fprintf(stderr, "Usage: %s InputFilename [OutputFilename] [startingOffset]\n", argv[0]);
    return ERROR_RETURN;
  }

  // First argument is the file to read, attempt to open it 
  // for reading and verify that the open did occur.
  machineCode = fopen(argv[1], "rb");

  if (machineCode == NULL) {
    fprintf(stderr, "Failed to open %s: %s\n", argv[1], strerror(errno));
    return ERROR_RETURN;
  }

  // Second argument is the file to write, attempt to open it for
  // writing and verify that the open did occur. Use standard output
  // if not provided.
  outputFile = argc <= 2 ? stdout : fopen(argv[2], "w");
  
  if (outputFile == NULL) {
    fprintf(stderr, "Failed to open %s: %s\n", argv[2], strerror(errno));
    fclose(machineCode);
    return ERROR_RETURN;
  }

  // If there is a 3rd argument present it is an offset so convert it
  // to a numeric value.
  if (4 == argc) {
    errno = 0;
    currAddr = strtol(argv[3], NULL, 0);
    if (errno != 0) {
      perror("Invalid offset on command line");
      fclose(machineCode);
      fclose(outputFile);
      return ERROR_RETURN;
    }
  }

  fprintf(stderr, "Opened %s, starting offset 0x%lX\n", argv[1], currAddr);
  fprintf(stderr, "Saving output to %s\n", argc <= 2 ? "standard output" : argv[2]);

  // Comment or delete the following lines and this comment before
  // handing in your final version.
  // if (currAddr) printPosition(outputFile, currAddr);
  // printInstruction(outputFile);

  // Your code starts here.
  readMemFile(machineCode, outputFile, currAddr);

  
  fclose(machineCode);
  fclose(outputFile);
  return SUCCESS;
}

int readMemFile(FILE * machineCode, FILE * outputFile, int currAddr) {

  fseek(machineCode, 0, SEEK_END);
  long machineCodeSize = ftell(machineCode);
  fseek(machineCode, currAddr, SEEK_SET);

  char *workingInstruction = malloc(sizeof(char) * 16);

  int first = true;
  int consecutiveHalt = 0;
  while(true) {
    int currByte = fread(workingInstruction, 1, 1, machineCode); 
    if (feof(machineCode)) break; // breaks out of while loop if EOF

    unsigned char iCode = workingInstruction[0] >> 4 & 0xF;
    unsigned char iFun = workingInstruction[0] & 0xF;

    unsigned char valA;
    unsigned char valB;
    long valC;
    long quad;
    char byte;

    switch (iCode) {
      //halt case
      case 0:
        // invalid, handle in default
        if (iFun != 0) goto default_case;

        if (!first) {
          if (consecutiveHalt == 0) {
            fprintf(outputFile, "    %-8s                    # 0x00\n", "halt");
          }
          consecutiveHalt++;
        }

        currAddr++;
        break;

      //nop case
      case 1:
        handlePos(outputFile, currAddr, first, consecutiveHalt);

        // invalid, handle in default
        if (iFun != 0) goto default_case;

        first = false;
        consecutiveHalt = 0;
        currAddr++;
        fseek(machineCode, -1, SEEK_CUR);
        currByte = fread(workingInstruction, 1, 1, machineCode);

        fprintf(outputFile, "    %-8s                    # 0x10\n", instructions[iCode]);
        break;

      //rmovq and cmovXX case
      case 2:
        handlePos(outputFile, currAddr, first, consecutiveHalt);

        first = false;
        consecutiveHalt = 0;
        currAddr += 2;
        fseek(machineCode, -1, SEEK_CUR);
        currByte = fread(workingInstruction, 1, 2, machineCode);
        valA = workingInstruction[1] >> 4 & 0xF;
        valB = workingInstruction[1] & 0xF;

        // invalid, handle in default
        if (iFun < 0 || iFun > 6 || valA == 0xF || valB == 0xF) goto default_case;

        fprintf(outputFile, "    %-8s %s, %s                    # 0x2%i%x%x\n", cmovXX[iFun], reg[valA], reg[valB], iFun, valA, valB);
        break;

      //irmovq case
      case 3:
        handlePos(outputFile, currAddr, first, consecutiveHalt);

        first = false;
        consecutiveHalt = 0;
        currAddr += 10;
        fseek(machineCode, -1, SEEK_CUR);
        currByte = fread(workingInstruction, 1, 2, machineCode);

        valA = workingInstruction[1] >> 4 & 0xF;
        valB = workingInstruction[1] & 0xF;

        fread(workingInstruction, 1, 8, machineCode);
        memcpy(&valC, workingInstruction, sizeof(long));

        // invalid, handle in default
        if (iFun != 0 || valA != 0xF || valB == 0xF) goto default_case;

        fprintf(outputFile, "    %-8s $0x%llx, %s                    # 0x30f%x", instructions[iCode], valC, reg[valB], valB);


        hexHelper(outputFile, workingInstruction);
        break;

      //rmmovq case
      case 4:
        handlePos(outputFile, currAddr, first, consecutiveHalt);

        first = false;
        consecutiveHalt = 0;
        currAddr += 10;
        fseek(machineCode, -1, SEEK_CUR);
        currByte = fread(workingInstruction, 1, 2, machineCode);

        valA = workingInstruction[1] >> 4 & 0xF;
        valB = workingInstruction[1] & 0xF;


        fread(workingInstruction, 1, 8, machineCode);
        memcpy(&valC, workingInstruction, sizeof(long));

        // invalid, handle in default
        if (iFun != 0 || valA == 0xF || valB == 0xF) goto default_case;

        fprintf(outputFile, "    %-8s %s, 0x%llx(%s)                    # 0x40%x%x", instructions[iCode], reg[valA], valC, reg[valB], valA, valB);

        hexHelper(outputFile, workingInstruction);
        break;

      //mrmovq case
      case 5:
        handlePos(outputFile, currAddr, first, consecutiveHalt);

        first = false;
        consecutiveHalt = 0;
        currAddr += 10;
        fseek(machineCode, -1, SEEK_CUR);
        currByte = fread(workingInstruction, 1, 2, machineCode);

        valA = workingInstruction[1] >> 4 & 0xF;
        valB = workingInstruction[1] & 0xF;


        fread(workingInstruction, 1, 8, machineCode);
        memcpy(&valC, workingInstruction, sizeof(long));

        // invalid, handle in default
        if (iFun != 0 || valA == 0xF || valB == 0xF) goto default_case;

        fprintf(outputFile, "    %-8s 0x%llx(%s), %s                    # 0x50%x%x", instructions[iCode], valC, reg[valB], reg[valA], valA, valB);

        hexHelper(outputFile, workingInstruction);
        break;

      //OPq case
      case 6:
        handlePos(outputFile, currAddr, first, consecutiveHalt);

        first = false;
        consecutiveHalt = 0;
        currAddr += 2;
        fseek(machineCode, -1, SEEK_CUR);
        currByte = fread(workingInstruction, 1, 2, machineCode);

        valA = workingInstruction[1] >> 4 & 0xF;
        valB = workingInstruction[1] & 0xF;

        // invalid, handle in default
        if (iFun < 0 || iFun > 6 || valA == 0xF || valB == 0xF) goto default_case;

        fprintf(outputFile, "    %-8s %s, %s                    # 0x6%i%x%x\n", OPq[iFun], reg[valA], reg[valB], iFun, valA, valB);
        break;

      //jXX case
      case 7:
        handlePos(outputFile, currAddr, first, consecutiveHalt);

        first = false;
        consecutiveHalt = 0;
        currAddr += 9;
        fseek(machineCode, -1, SEEK_CUR);
        currByte = fread(workingInstruction, 1, 1, machineCode);

        fread(workingInstruction, 1, 8, machineCode);
        memcpy(&valC, workingInstruction, sizeof(long));

        // invalid, handle in default
        if (iFun < 0 || iFun > 6) goto default_case;

        fprintf(outputFile, "    %-8s 0x%llx                    # 0x7%x", jXX[iFun], valC, iFun);

        hexHelper(outputFile, workingInstruction);
        break;

      //call case
      case 8:
        handlePos(outputFile, currAddr, first, consecutiveHalt);

        first = false;
        consecutiveHalt = 0;
        currAddr += 9;
        fseek(machineCode, -1, SEEK_CUR);
        currByte = fread(workingInstruction, 1, 1, machineCode);

        fread(workingInstruction, 1, 8, machineCode);
        memcpy(&valC, workingInstruction, sizeof(long));

        // invalid, handle in default
        if (iFun != 0) goto default_case;

        fprintf(outputFile, "    %-8s 0x%llx                    # 0x80", instructions[iCode], valC);

        hexHelper(outputFile, workingInstruction);
        break;

      //ret case
      case 9:
        handlePos(outputFile, currAddr, first, consecutiveHalt);

        first = false;
        consecutiveHalt = 0;
        currAddr += 1;
        fseek(machineCode, -1, SEEK_CUR);
        currByte = fread(workingInstruction, 1, 1, machineCode);

        // invalid, handle in default
        if (iFun != 0) goto default_case;

        fprintf(outputFile, "    %-8s                    # 0x90\n", instructions[iCode]);
        break;

      //pushq case
      case 0xA:
        handlePos(outputFile, currAddr, first, consecutiveHalt);

        first = false;
        consecutiveHalt = 0;
        currAddr += 2;
        fseek(machineCode, -1, SEEK_CUR);
        currByte = fread(workingInstruction, 1, 2, machineCode);

        valA = workingInstruction[1] >> 4 & 0xF;
        valB = workingInstruction[1] & 0xF;

        // invalid, handle in default
        if (iFun != 0 || valA == 0xF || valB == 0xF) goto default_case;

        fprintf(outputFile, "    %-8s %s                    # 0xa0%xf\n", instructions[iCode], reg[valA], valA);
        break;

      //popq case
      case 0xB:
        handlePos(outputFile, currAddr, first, consecutiveHalt);

        first = false;
        consecutiveHalt = 0;
        currAddr += 2;
        fseek(machineCode, -1, SEEK_CUR);
        currByte = fread(workingInstruction, 1, 2, machineCode);

        valA = workingInstruction[1] >> 4 & 0xF;
        valB = workingInstruction[1] & 0xF;

        // invalid, handle in default
        if (iFun != 0 || valA == 0xF || valB == 0xF) goto default_case;

        fprintf(outputFile, "    %-8s %s                    # 0xb0%xf\n", instructions[iCode], reg[valA], valA);
        break;

      default:
      default_case:
        first = false;
        consecutiveHalt = 0;
        fseek(machineCode, currAddr, SEEK_SET);

        if (currAddr % 8 == 0 && (machineCodeSize - currAddr) >= 8) {
          fread(workingInstruction, 1, 8, machineCode);
          memcpy(&quad, workingInstruction, sizeof(long));
          fprintf(outputFile, "    %-8s 0x%llx                    # 0x", ".quad", quad);

          hexHelper(outputFile, workingInstruction);

          currAddr += 8;

        } else if (currAddr % 8 != 0 || machineCodeSize - currAddr < 8) {
          fread(workingInstruction, 1, 1, machineCode);
          memcpy(&byte, workingInstruction, sizeof(char));
          fprintf(outputFile, "    %-8s 0x%hhx                        # 0x%02x\n", ".byte", byte, (unsigned char)workingInstruction[0]);

          currAddr++;
        }
        break;
    }
  }
  return SUCCESS;
}

int handlePos(FILE *outputFile, int currAddr, int first, int consecutiveHalt) {
  if ((currAddr != 0) && (first || consecutiveHalt > 1)) {
    return fprintf(outputFile, "\n.pos 0x%lx \n", currAddr);
  }
}

int hexHelper(FILE *outputFile, char workingInstruction[]) {
  for (int i = 0; i < 8; i++) fprintf(outputFile, "%02x", (unsigned char)workingInstruction[i]);
  fprintf(outputFile, "\n");
}
