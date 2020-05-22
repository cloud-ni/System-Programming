#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#define INSTRUCT_LEN 1000
#define CMD_LEN 50
#define CMD_NUM 25
#define TOKENS 4
#define STRPAR_LEN 50 //2nd parameter length
#define SYMTAB_LEN 39//the number of indices in symbol table
#define MEMORY_SIZE 0x100000// Memory size is 1Mbyte
#define ASMTOK_LEN 30 //length of each token in .asm file
#define ASMLINE_LEN 100 //length of each line in .asm file
#define TEXTREC_LEN 30 //length of each text record line
#define MODIFY_LEN 10000//length of modify part in .obj file
#define BPLIST_LEN 50 //breakpoint list length

unsigned char Mem[MEMORY_SIZE];//one char = one byte

typedef enum {
	help, dir, quit, history, dump, edit, fill, reset, opcode, opcodelist,
	assemble, type, symbol, progaddr, loader, bp, bpclear, run, null
} Cmd;

typedef struct CmdData {
	char cmd1[CMD_LEN];
	char cmd2[CMD_LEN];
	int param_num;
	int hasStrpar; //has string parmeter: 1
	Cmd cmd;
} CmdData;

typedef struct CmdTokens {
	Cmd cmd; //command
	int param_num; //number of parameter(cmd not included)
	int par1; //first hex parameter
	int par2;
	int par3;
	char strpar[STRPAR_LEN]; //string paramater
	char strpar1[STRPAR_LEN];
	char strpar2[STRPAR_LEN];
} CmdTokens;

int isValidHex(char *tok); //check if the parameter is a hex value
int isValidCmd(char* str, CmdTokens* tokens); //check if the command is a valid command
// split the instruction into tokens and check if it has enough parameters
int SplitInstruct(char *str, CmdTokens *tokens);
void resetCmdTokens(CmdTokens* tok); // refresh CmdTokens
int runCmd(CmdTokens *tokens); //call functions according to the command
int isLongHex(char * string);
