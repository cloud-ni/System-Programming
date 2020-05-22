#include "20170001.h"
#include "shell.h"
#include "memory.h"
#include "opcode.h"
#include "assembler.h"
#include "loader.h"
#include "execute.h"


CmdData CmdTable[CMD_NUM] = {
{ "help", "h", 0, 0, help },
{ "dir", "d", 0, 0, dir },
{ "quit", "q", 0, 0, quit },
{ "history", "hi", 0, 0, history },
{ "dump", "du", 2, 0, dump },
{ "edit", "e", 2, 0, edit },
{ "fill", "f", 3, 0, fill },
{ "reset", "reset", 0, 0, reset },
{ "opcode", "opcode", 1, 1, opcode },
{ "opcodelist", "opcodelist", 0, 0, opcodelist },
{ "assemble", "assemble", 1, 1, assemble},
{ "type", "type", 1, 1, type},
{ "symbol", "symbol", 0, 0, symbol},
{ "progaddr", "progaddr", 1, 1, progaddr},
{ "loader", "loader", 3, 3, loader},
{ "bp", "bp", 1, 1, bp},
{ "bp clear", "bp clear", 0, 0, bpclear},
{ "run", "run", 0, 0, run},
};


/* check if the command is valid command */
int isValidCmd(char* str, CmdTokens* tokens) {
	for (int i = 0; i < CMD_NUM; i++) {
		//if command found, return 1
		if (!strcmp(str, CmdTable[i].cmd1) || !strcmp(str, CmdTable[i].cmd2)) {
			tokens->cmd = CmdTable[i].cmd; //save command to tokens
			return 1;
		}
	}
	return 0;//invalid command
}
/* check if tok is a valid hex value */
int isValidHex(char *tok) {
	for (int i = 0; i < strlen(tok); i++) {
		if (isxdigit(tok[i])) {// valid hex value
			continue;
		}
		else if (tok[i] == ',') {
			if (i != strlen(tok) - 1) {//',' exists before the last character of tok
				return 0;
			}
		}
		else {// invalid value
			return 0;
		}
	}
	return 1;
}
/* check if it is long hex number */
int isLongHex(char * string) {
	for (int i = 0; i < (int)strlen(string); i++)//check every index of the string
	{
		if (string[i] >= '0'&&string[i] <= '9') {//if a char is between 0~9
			continue;
		}
		else if (string[i] >= 'a'&&string[i] <= 'f') {
			continue;
		}
		else if (string[i] >= 'A'&&string[i] <= 'F') {
			continue;
		}
		else if (string[i] == '-') {
			continue;
		}
		else {//found char that is not digit
			return 0;
		}
	}
	return 1;//every letter is hex
}
/*split instruction in to tokens, but return -1 if it is wrong instruction*/
/* check command and the number of parameters */
int SplitInstruct(char *str, CmdTokens *tokens) {
	char * tok;
	int count = 0, comma = 0, hexTmp;

	if (strlen(str) == 0) { //if the instruction is null 
		printf("Error: Invalid command\n");
		return -1;
	}
	tok = strtok(str, " ");//get the command from the input
	if (!isValidCmd(tok, tokens)) {//if the command is not valid
		printf("Error: Invalid command\n");
		return -1;
	}
	//get next part of instruction
	while (1) {
		tok = strtok(NULL, " ");
		if (tok == NULL)
			break;
		count++;//current parameter
		if (tokens->cmd == dump || tokens->cmd == edit || tokens->cmd == fill) {//if the command is about memory
			switch (count) {
			case 1://first parameter
				if (tok[strlen(tok) - 1] == ',') {//to check if comma exists at the back
					comma = 1;
				}
				if (!isValidHex(tok)) { //check if tok is valid hex value
					printf("Error: Invalid parameter(s)\n");
					return -1;
				}
				hexTmp = (int)strtol(tok, NULL, 16);
				tokens->par1 = hexTmp;//save hex to structure tokens
				break;
			case 2://second
				if (comma == 1) {
					if (tok[strlen(tok) - 1] == ',') {
						comma = 2;
					}
					if (!isValidHex(tok)) { //check if tok is valid hex value
						printf("Error: Invalid parameter(s)\n");
						return -1;
					}
					hexTmp = (int)strtol(tok, NULL, 16);
					tokens->par2 = hexTmp;//save hex to structure tokens
				}
				else { //no comma between first and second parameter
					printf("Error: Invalid parameter(s)\n");
					return -1;
				}
				break;
			case 3://third
				if (comma == 2 && tokens->cmd == fill) {
					if (!isValidHex(tok)) { //check if tok is valid hex value
						printf("Error: Invalid parameter(s)\n");
						return -1;
					}
					hexTmp = (int)strtol(tok, NULL, 16);
					tokens->par3 = hexTmp;//save hex to structure tokens
				}
				else {
					printf("Error: Invalid parameter(s)\n");
					return -1;
				}
				break;
			}
		}
		else if (count <= CmdTable[tokens->cmd].hasStrpar) {//command has string parameter
			if(count == 1)
				strcpy(tokens->strpar, tok);
			else if(count == 2)
				strcpy(tokens->strpar1, tok);
			else if (count == 3) 
				strcpy(tokens->strpar2, tok);
		}
		else { //error: too many parameters for the command
			printf("Error: Invalid parameter(s)\n");
			return -1;
		}
	}
	if (comma && count != comma + 1) {//there was a comma, but no parmeter after that
		printf("Error: Invalid parameter(s)\n");
		return -1;
	}
	tokens->param_num = count; //save the number of parameter
	if (tokens->cmd == dump) {// too many parameters
		if (count == 3) { 
			printf("Error: Invalid number of parameter(s)\n"); 
			return -1; 
		}
	}
	else if (tokens->cmd == bp) {
		if (count > 1) {
			printf("Error: Invalid number of parameter(s)\n"); 
			return -1;
		}
		if (!strcmp(tokens->strpar, "clear")) {
			tokens->param_num = 0;
			tokens->cmd = bpclear;
			memset(tokens->strpar, 0, sizeof(char)*10);
		}
		else if (!isLongHex(tokens->strpar)) {
			printf("Error: Invalid Input. Enter 'bp [hex number within program range]' or 'bp clear'.\n");
			return -1;
		}
	}
	else if (tokens->cmd == progaddr) {
		if (!isLongHex(tokens->strpar)) {
			printf("Error: Invalid parameter. It should be complete hex number.\n");
			return -1;
		}
	}
	else if (tokens->cmd == loader) {
		if (count == 0 || count > 3) {
			printf("Error: Invalid number of parameter(s)\n"); 
			return -1;
		}
	}
	else if (CmdTable[tokens->cmd].param_num != count) {// less or more parameters than it needs
		printf("Error: Invalid number of parameter(s)\n");
		return -1;
	}
	return 0;
}
/* reset input tokens */
void resetCmdTokens(CmdTokens* tok) {
	tok->cmd = null;
	tok->param_num = 0;
	tok->par1 = -1; //first hex parameter
	tok->par2 = -1;
	tok->par3 = -1;
	memset(tok->strpar, 0, sizeof(tok->strpar));
}

/* run command */
int runCmd(CmdTokens *tokens) {
	int error = 0;
	switch (tokens->cmd) {
	case(help):
		helpCmd();
		break;
	case(dir):
		dirCmd();
		break;
	case(quit):
		quitCmd();
		break;
	case(history):
		historyCmd();
		break;
	case(dump):
		error = dumpMem(tokens);
		break;
	case(edit):
		error = editMem(tokens);
		break;
	case(fill):
		error = fillMem(tokens);
		break;
	case(reset):
		resetMem();
		break;
	case(opcode):
		error = getOpcode(tokens);
		break;
	case(opcodelist):
		listOpcode();
		break;
	case(assemble):
		error = assembleAsm(tokens);
		break;
	case(type):
		error = typeCmd(tokens);
		break;
	case(symbol):
		listSymbol();
		break;
	case(progaddr):
		error = setProaddr(tokens);
		break;
	case(loader):
		error = loadObj(tokens);
		break;
	case(bp):
		if (tokens->param_num == 0) {
			printBp();
		}
		else {
			error = setBp(tokens);
		}
		break;
	case(bpclear):
		clearBp();
		break;
	case(run):
		error = runProgram();
		break;
	default:
		error = -1;
		break;
	}
	return error;
}

int main(int argc, char *argv[]) {
	char str[INSTRUCT_LEN];
	char tmp[INSTRUCT_LEN];
	int splited, error;
	CmdTokens tokens;

	createHash(); //create hash table for opcode
	SymList = NULL;//initailize symbol list
	BpLen = 0;//initailize breakpoint length
	//initialize
	ProgAddr = 0;
	ProgLen = 0;
	ExeAddr = 0;
	ExeProgAddr = 0;
	memset(Mem, 0, sizeof(Mem[0]) * MEMORY_SIZE);

	while (1) {
		error = 0;
		printf("sicsim> ");
		fflush(stdin);
		memset(str, 0, INSTRUCT_LEN);
		fgets(str, INSTRUCT_LEN, stdin);
		str[strlen(str) - 1] = '\0';
		memset(tmp, 0, INSTRUCT_LEN);
		strncpy(tmp, str, strlen(str) + 1);
		resetCmdTokens(&tokens);//reset command tokens
		splited = SplitInstruct(str, &tokens);//split instructions
		if (splited == -1) {//error 
			continue;
		}
		if (tokens.cmd == history) {//if the command is history, add first
			addHistory(tmp);
		}
		error = runCmd(&tokens);// run command
		if (error == 0 && tokens.cmd != history) {//if the instructions is valid add to the history
			addHistory(tmp);
		}
	}

	free(SymList);//free symbol list
	free(BpList);//free breakpoint list
	return 0;
}