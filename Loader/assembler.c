#include "20170001.h"
#include "opcode.h"
#include "assembler.h"

AsmSrcNode* AsmSrc = NULL;//linked list for assembly code

typedef enum Directive { START, END, BYTE, WORD, RESB, RESW, BASE, NOBASE }Directive;
char directives[8][8] = { "START", "END", "BYTE", "WORD", "RESB", "RESW", "BASE", "NOBASE" };
char registers[10][3] = { "A", "X", "L", "B", "S", "T", "F", "", "PC", "SW" };

int ProgramLen;
int SymbolNum;

/* create AsmSrc node of current line in assembly code
and insert it into AsmSrc linked list */
AsmSrcNode* insertAsmSrc(char* line, char* label, char* opcode, char* operand, char* operand2) {
	AsmSrcNode *cur, *newNode;
	newNode = (AsmSrcNode*)calloc(1, sizeof(AsmSrcNode));//create new node
	/* initialize new node */
	if (line[strlen(line) - 1] == '\n') {//delete \n at the end
		line[strlen(line) - 1] = '\0';
	}
	if (line[0] == '\t') {//change \t to 4 spaces
		strcpy(newNode->line, "    ");
		strcpy(newNode->line + 4, line + 1);
	}
	else {
		strcpy(newNode->line, line);//copy source line
	}
	strcpy(newNode->label, label);//copy label
	newNode->loc = -1;//default
	newNode->next = NULL;
	strcpy(newNode->operand2, operand2);//copy operand2

	//find out if the format is format4 or not
	if (opcode[0] == '+') { //format4
		newNode->format = 4;
		strcpy(newNode->opcode, opcode + 1);//copy opcode without +
	}
	else { //others format not known
		newNode->format = -1;
		strcpy(newNode->opcode, opcode);//copy opcode
	}

	//find out addressing mode & copy operands
	if (operand[0] == '#') {//immediate addressing
		newNode->mode = immediate;
		strcpy(newNode->operand, operand + 1);//copy operand without #
	}
	else if (operand[0] == '@') {//indirect addressing
		newNode->mode = indirect;
		strcpy(newNode->operand, operand + 1);//copy operand without @
	}
	else if (operand[0] == '\0') {//no operand => RSUB
		newNode->mode = simple;//for RSUB
		newNode->operand[0] = '\0';
	}
	else {//simple addressing
		newNode->mode = simple;
		strcpy(newNode->operand, operand);//save whole operand
	}

	/*connect newNode node to AsmSrc linked list*/
	if (AsmSrc == NULL) {//if new node is first node
		AsmSrc = newNode;
		return newNode;
	}
	cur = AsmSrc;
	while (cur->next) {//find the last node of the linked list
		cur = cur->next;
	}
	cur->next = newNode;//connect
	return newNode;
}

/* reset AsmSrc linked list */
void clearAsmSrc(void) {
	AsmSrcNode *cur, *next;
	cur = AsmSrc;
	while (cur) {
		next = cur->next;
		free(cur);
		cur = next;
	}
	AsmSrc = NULL;
	return;
}

/* hash function for symbol table */
int SymTableFunc(char* sym) {
	unsigned long val = 5381;
	while (*sym != '\0') {//visit each character
		val = 33 * val + *sym;
		sym++;
	}
	return (int)(val % SYMTAB_LEN);
}
/* create new slot and insert it into SymTable */
void insertSymTable(char* symb, int addr) {
	SymSlot *cur, *prev;
	int index;
	index = SymTableFunc(symb); //get hash value
	cur = prev = SymTable[index]; //point to the first slot
	while (cur) { //get the last slot in the current index of hash table
		prev = cur;
		cur = cur->next;
	}
	//make new slot
	cur = (SymSlot*)malloc(sizeof(SymSlot));
	strcpy(cur->symb, symb);
	cur->addr = addr;
	cur->next = NULL;
	//link new slot to appropriate place
	if (prev)
		prev->next = cur;
	else
		SymTable[index] = cur;
	SymbolNum++;//increment the number of symbols
	return;
}
/* search label in the symbol table
1: found 0: not found*/
int searchSymtable(char *label) {
	SymSlot *cur;
	int index;
	index = SymTableFunc(label);//get hash value
	cur = SymTable[index];//first slot of the index
	while (cur) {//visit each slot in the same index
		if (!strcmp(cur->symb, label)) {//label found in the table
			return 1;
		}
		cur = cur->next;
	}
	return 0;//label not found in the talbe
}
/* reset SymTable */
void clearSymTable() {
	SymSlot *cur, *next;
	for (int i = 0; i < SYMTAB_LEN; i++) {
		cur = SymTable[i];
		while (cur) {
			next = cur->next;
			free(cur);
			cur = next;
		}
		SymTable[i] = 0;
	}
	return;

}

/* tokenize current line in assembly code
to opcode, label, operand*/
void splitAsmLine(char* line, char* label, char* opcode, char* operand, char* operand2) {
	char str1[ASMTOK_LEN] = { 0 }, str2[ASMTOK_LEN] = { 0 }, str3[ASMTOK_LEN] = { 0 }, str4[ASMTOK_LEN] = { 0 };
	char* strptr;
	sscanf(line, "%s %s %s %s", str1, str2, str3, str4);//get each token in the line
	if (*str3 == '\0' && *str1 != '.') {//if the line does not have label & not a comment line
		*label = '\0'; //initialize label
		strcpy(opcode, str1); //save opcode
		strcpy(operand, str2); //save operand
		*operand2 = '\0';
	}
	else if (str2[strlen(str2) - 1] == ',') {//if there are two operands
		str2[strlen(str2) - 1] = '\0';
		*label = '\0'; //initialize label
		strcpy(opcode, str1); //save opcode
		strcpy(operand, str2); //save 1st operand
		strcpy(operand2, str3);//save 2nd operand
	}
	else if (str3[strlen(str3) - 1] == ',') {//if label is included and there are two operands
		str3[strlen(str3) - 1] = '\0';
		strcpy(label, str1);
		strcpy(opcode, str2);
		strcpy(operand, str3);
		strcpy(operand2, str4);
	}
	else { //if the line has all 3 fields
		strcpy(label, str1);
		strcpy(opcode, str2);
		strcpy(operand, str3);
		*operand2 = '\0';
	}
	//if opcode = constant, there may be spaces between words
	if (!strcmp(opcode, "BYTE")) {
		strptr = strstr(line, operand);
		sprintf(operand, "%s", strptr);//print whole string into operand
		if (operand[strlen(operand) - 1] == '\n') {//remove \n at the back
			operand[strlen(operand) - 1] = '\0';
		}
		*operand2 = '\0';
	}
	return;
}

/* check if the opcode matches the directive */
int isOpcode(int dir, AsmSrcNode* cur) {
	if (strcmp(cur->opcode, directives[dir]) == 0) {
		return 1;
	}
	else
		return 0;
}

/* pass1 function */
int pass1(FILE* fp) {
	char line[ASMLINE_LEN];
	char label[ASMTOK_LEN] = { 0 }, opcode[3 * ASMTOK_LEN] = { 0 }, operand[ASMTOK_LEN] = { 0 }, operand2[ASMTOK_LEN] = { 0 };
	AsmSrcNode* cur;
	int LC = -1;//initialize location counter
	HashSlot* opslot;
	int startAddr;

	//read first line of input file 
	if (!fgets(line, ASMLINE_LEN, fp)) {
		printf("File is Empty\n");
		return -1;
	}
	splitAsmLine(line, label, opcode, operand, operand2);//tokenize current line
	cur = insertAsmSrc(line, label, opcode, operand, operand2);//add new line to AsmSrc linked list
	if (isOpcode(START, cur)) {//opcode == START
		startAddr = (int)strtol(cur->operand, NULL, 16);//save starting address
		LC = (int)strtol(cur->operand, NULL, 16);//initialized LC to starting address
		cur->loc = LC;
	}
	else {//no START opcode
		LC = 0;
		cur->loc = LC;
	}
	while (1) {
		if (fgets(line, ASMLINE_LEN, fp) == NULL) {//read line and check if it is empty
			break;
		}
		splitAsmLine(line, label, opcode, operand, operand2);//tokenize current line
		cur = insertAsmSrc(line, label, opcode, operand, operand2);//add new line to AsmSrc linked list
		cur->loc = LC;//save location counter of current line

		if (isOpcode(END, cur)) {//last line
			break;
		}
		//comment, base lines do not affect location counter & do not have address
		if (label[0] == '.') {//this is comment line
			continue;
		}
		if (isOpcode(BASE, cur)) {//base information
			continue;
		}

		//check symbol
		if (label[0] != '\0') {//symbol exists in the label
			if (searchSymtable(cur->label)) {//label found in the symtable
				printf("Error: Duplicate Symbol\n");//error 
				return -1;//terminate
			}
			else {//label not found int the symtable
				insertSymTable(cur->label, cur->loc);
			}
		}

		//check opcode
		opslot = searchHash(cur->opcode);//search opcode table for opcode
		if (opslot) {//found opcode in opcode table
			if (cur->format == 4) {//+ was added infront of opcode
				LC += 4;//add instruction length to LC
			}
			else {//no + in front of opcode
				cur->format = opslot->format;//save format
				LC += opslot->format;//add instruction length to LC
			}
		}
		else if (isOpcode(WORD, cur)) {//opcode = 'WORD', add 3
			if (atoi(cur->operand) > 0XFFFFFF) {//over 3byte, more than one word
				printf("Error: Operand for WORD Exceeds One Word\n");
				return -1;
			}
			else {
				LC += 3;//one word
			}
		}
		else if (isOpcode(RESW, cur)) {//opcode = 'RESW'
			LC += 3 * (atoi(cur->operand));
		}
		else if (isOpcode(RESB, cur)) {//opcode = 'RESB'
			LC += atoi(cur->operand);
		}
		else if (isOpcode(BYTE, cur)) {//opcode = 'BYTE'
			if (cur->operand[0] == 'C') {//character(ascii code)
				for (int i = strlen(cur->operand) - 1; i >= 0; i--) {
					if (cur->operand[i] == '\'') {//find last '
						LC += i - 2;//a letter = one byte
						break;
					}
				}
			}
			else if (cur->operand[0] == 'X') {//hex
				for (int i = 2, j = 0, k = 0; ; i++) {//visit each char in operand
					if (cur->operand[i] == '\'') {//last char
						if (k == 0) {//the value is 0
							LC += 1;
						}
						else {// the value is nonzero
							LC += (j + 1) / 2;//two letters = one byte
						}
						break;
					}
					if (cur->operand[i] == ' ') {//skip space
						continue;
					}
					if (k == 0) {//0 in the front can be removed
						if (cur->operand[i] == '0') {
							continue;
						}
						else {
							k = 1;
						}
					}
					j++;//increment 
				}
			}
			else {//wrong operand
				printf("Error: Invalid Operand for BYTE opcode\n");
				return -1;
			}
		}
		else {//wrong opcode
			printf("Error: Invalid Operation Code\n");
			return -1;
		}
	}
	ProgramLen = LC - startAddr;//save program length to global variable ProgramLen
	return 0;//succesfully ended
}

/* get register number
returns -1 if not found */
int getRegNum(char* reg) {
	for (int i = 0; i <= 9; i++) {//go through each index if registers
		if (i == 7) continue;//no register in number 7
		if (!strcmp(reg, registers[i])) {//register found
			return i;//return register number
		}
	}
	return -1;//register not found
}

/* get symbol address
returns -1 if not found */
int getSymAddr(char * sym) {
	SymSlot *cur;
	int index;
	index = SymTableFunc(sym);//get hash value
	cur = SymTable[index];//first slot of the index
	while (cur) {//visit each slot in the same index
		if (!strcmp(cur->symb, sym)) {//sym found in the table
			return cur->addr;
		}
		cur = cur->next;
	}
	return -1;//sym not found in the talbe
}

/* check if the string is number */
int isNumber(char * string) {
	for (int i = 0; i < (int)strlen(string); i++)//check every index of the string
	{
		if (string[i] >= '0'&&string[i] <= '9') {//if a char is between 0~9
			continue;
		}
		else {//found char that is not digit
			return 0;
		}
	}
	return 1;//every letter is digit
}

/*pass2 function*/
int pass2(FILE *fp_l, FILE* fp_o) {
	AsmSrcNode* cur = AsmSrc;
	int lineNum = 5;//line number for .lst file
	int textStart = 0;//marks starting address of current line in .obj file
	int variable = 0;//if opcode is variable set this flag
	HashSlot* opslot;//points to opcode table slot
	int opcode_ni, xbpe = 0;//opcode_ni = first 2byte of object code. xbpe = next 1byte
	int reg1, reg2;//register number
	int targetAddr, baseAddr = -1, disp;//used for format 3/4
	char text[TEXTREC_LEN + 100] = { 0 };//buffer for each line in .obj file
	char tmp[5];
	char* modify = (char*)calloc(100, sizeof(char));//modify part in .obj file
	int firstExe = -1;//first executable address
	char tmpOperand[2 * ASMTOK_LEN] = { 0 }, tmpOpcode[ASMTOK_LEN] = { 0 };

	//write first line to .lst and .obj file
	fprintf(fp_l, "%-4d   %04X   %-10s%-10s%-15s\n", lineNum, cur->loc, cur->label, cur->opcode, cur->operand);
	
	fprintf(fp_o, "H%s  %06X%06X\n", cur->label, (int)strtol(cur->operand, NULL, 16), ProgramLen);
	text[0] = 'T';//save T in textline
	sprintf(text + 1, "%06X", cur->loc);//save start location 
	text[7] = -1; text[8] = -1;//save space for line legth
	textStart = cur->loc;//save start location

	while (1) {

		// go to next line
		cur = cur->next;
		lineNum += 5;//renew line number
		xbpe = 0;//reset xbpe
		memset(tmpOperand, 0, 2 * ASMTOK_LEN);//initialize
		memset(tmpOpcode, 0, 2 * ASMTOK_LEN);

		if (isOpcode(END, cur)) {//end 
			break;
		}

		if (cur->label[0] == '.') {//comment line
			//print to list file
			fprintf(fp_l, "%-4d          %s\n", lineNum, cur->line);
			continue;
		}
		if (isOpcode(BASE, cur)) {//base information
			baseAddr = getSymAddr(cur->operand);//set base address
			fprintf(fp_l, "%-4d          %-10s%-10s%-15s\n", lineNum, cur->label, cur->opcode, cur->operand);//print to list file
			continue;
		}
		if (isOpcode(NOBASE, cur)) {//reset base information
			baseAddr = -1;
			fprintf(fp_l, "%-4d          %-10s%-10s%-15s\n", lineNum, cur->label, cur->opcode, cur->operand);//print to list file
			continue;
		}

		//set variable flag
		if (isOpcode(RESW, cur) || isOpcode(RESB, cur)) {//variable line
			variable++;
		}
		else {// non-variable line
			if (variable > 0) {//previous code lines were variable
				variable = -1;//have to start new line in text
			}
		}

		//print text[] to .obj file
		if (variable == 1 || (cur->loc - textStart >= TEXTREC_LEN - 1 && variable == 0)) {//text line length>30 or variable line appeared
			sprintf(tmp, "%02X", cur->loc - textStart);//change line length to string
			strncpy(text + 7, tmp, 2);//save line length to text
			fprintf(fp_o, "%s\n", text);//print to .obj file
			if (firstExe == -1) {//if it is first text[] line
				firstExe = textStart;//save first executable address
			}
		}
		//reset text[] and start new line in .obj file
		if (cur->loc - textStart >= TEXTREC_LEN - 1 || variable == -1) {//text line length>30 or variable lines ended
			memset(text, '\0', strlen(text));//reset object code part of textline
			sprintf(text + 1, "%06X", cur->loc);//save start location 
			text[0] = 'T'; text[7] = -1; text[8] = -1;
			textStart = cur->loc;//reset start location of current text line
			if (variable == -1) {//reset variable flag
				variable = 0;
			}
		}

		//search opcode table for opcode
		opslot = searchHash(cur->opcode);

		//make object code and print in .lst
		if (cur->format == 1) {//format1
			//print to .lst
			fprintf(fp_l, "%-4d   %04X   %-10s%-10s%-15s%02X\n", lineNum, cur->loc, cur->label, cur->opcode, cur->operand, opslot->opcode);

			//save object code to text[] 
			sprintf(text + strlen(text), "%02X", opslot->opcode);
		}
		else if (cur->format == 2) {//format2
			reg1 = getRegNum(cur->operand);//get register number
			if (reg1 == -1) {//register not found
				printf("Error: Undefined Register\n");//error msg
				return -1;
			}
			if (cur->operand2[0] != '\0') {//2nd register(operand) exists
				reg2 = getRegNum(cur->operand2);//get 2nd register number
				if (reg2 == -1) {//register not found
					printf("Error: Undefined Register\n");//error msg
					return -1;
				}
				//create print format for operand
				strcpy(tmpOperand, cur->operand);
				strcpy(tmpOperand + strlen(tmpOperand), ", ");
				strcpy(tmpOperand + strlen(tmpOperand), cur->operand2);
			}
			else {//no 2nd register
				reg2 = 0;
				strcpy(tmpOperand, cur->operand);//create print format for operand
			}

			//print to .lst
			fprintf(fp_l, "%-4d   %04X   %-10s%-10s%-15s%02X%X%X\n", lineNum, cur->loc, cur->label, cur->opcode, tmpOperand, opslot->opcode, reg1, reg2);

			//save object code to text[] 
			sprintf(text + strlen(text), "%02X%X%X", opslot->opcode, reg1, reg2);
		}
		else if (cur->format == 3 || (cur->format == 4 && !isOpcode(BYTE, cur) && !isOpcode(WORD, cur))) {//format 3/4

			//set opcode + ni
			opcode_ni = opslot->opcode;//save opcode
			opcode_ni += cur->mode;//add ni

			//set x
			if (cur->operand2[0] == 'X') {//uses index register
				xbpe += 8;
			}

			//get targetAddr
			if (cur->operand[0] == '\0') {//no operand
				targetAddr = 0;
			}
			else if (isNumber(cur->operand)) {//operand is number
				targetAddr = atoi(cur->operand);
			}
			else {//operand is targetAddr
				targetAddr = getSymAddr(cur->operand);//search Symtable for targetAddr
				if (targetAddr == -1) {//targetAddr is not defined
					printf("Error: Undefined Symbol in Operand Field\n");//error
					return -1;
				}
			}

			//create print format for operand
			if (cur->mode == immediate) {
				tmpOperand[0] = '#';
				strcpy(tmpOperand + 1, cur->operand);
			}
			else if (cur->mode == indirect) {
				tmpOperand[0] = '@';
				strcpy(tmpOperand + 1, cur->operand);
			}
			else if (cur->operand2[0] != '\0') {
				if (cur->operand2[0] == 'X') {//x
					strcpy(tmpOperand, cur->operand);
					strcpy(tmpOperand + strlen(tmpOperand), ", X");
				}
				else {//error
					printf("Error: Invalid Operand\n");
					return -1;
				}
			}
			else {
				strcpy(tmpOperand, cur->operand);
			}

			if (cur->format == 3) {//format 3
				//decide addressing method & get disp

				if (cur->operand[0] == '\0' || (isNumber(cur->operand) && cur->mode == immediate && targetAddr <= 4095)) {//direct addressing
					disp = targetAddr;
				}
				else if (targetAddr - cur->next->loc >= -2048 && targetAddr - cur->next->loc <= 2047) {//PC relative
					disp = targetAddr - cur->next->loc;//disp = TA - PC
					xbpe += 2;//p = 1
				}
				else if (baseAddr != -1 && targetAddr - baseAddr >= 0 && targetAddr - baseAddr <= 4095) {//base relative
					disp = targetAddr - baseAddr;//disp = TA - base
					xbpe += 4;// b = 1
				}
				else {//targetAddr out of range
					printf("Error: Invalid Operand. Unable to Display with SIC/XE Architecture\n");
					return -1;
				}

				//print to .lst
				fprintf(fp_l, "%-4d   %04X   %-10s%-10s%-15s%02X%X%03X\n", lineNum, cur->loc, cur->label,cur->opcode, tmpOperand, opcode_ni, xbpe, disp & 0XFFF);
				//save object code to text[] 
				sprintf(text + strlen(text), "%02X%X%03X", opcode_ni, xbpe, disp & 0XFFF);
			}
			else if (cur->format == 4) {//format4 
				xbpe += 1;//e=1
				//create print format for opcode
				tmpOpcode[0] = '+';
				strcpy(tmpOpcode + 1, cur->opcode);
				//print to .lst
				fprintf(fp_l, "%-4d   %04X   %-10s%-10s%-15s%02X%X%05X\n", lineNum, cur->loc, cur->label, tmpOpcode, tmpOperand, opcode_ni, xbpe, targetAddr);

				//save object code to text[] 
				sprintf(text + strlen(text), "%02X%X%05X", opcode_ni, xbpe, targetAddr);
				//save modify code
				if (!(cur->mode == immediate && isNumber(cur->operand))) {//if not #[number]
					if (sizeof(modify) < strlen(modify) + 20) {//reallocate if size of modify is not enough
						modify = (char *)realloc(modify, (strlen(modify) + 100) * sizeof(char));
					}
					sprintf(modify + strlen(modify), "M%06X05\n", cur->loc + 1);//save modify code
				}
			}
		}
		if (isOpcode(RESW, cur) || isOpcode(RESB, cur)) {//variables
			//print to list file
			fprintf(fp_l, "%-4d   %04X   %-10s%-10s%-15s\n", lineNum, cur->loc,cur->label, cur->opcode, cur->operand);

		}
		else if (isOpcode(BYTE, cur)) {//opcode = 'BYTE' constant
			if (cur->operand[0] == 'C') {//character(ascii code)
				//print to list file
				fprintf(fp_l, "%-4d   %04X   %-10s%-10s%-15s", lineNum, cur->loc, cur->label, cur->opcode, cur->operand);

				for (int i = 2; cur->operand[i] != '\''; i++) {
					fprintf(fp_l, "%X", cur->operand[i]);
					sprintf(text + strlen(text), "%X", cur->operand[i]);
				}
				fprintf(fp_l, "\n");
			}
			else if (cur->operand[0] == 'X') {//hex
				for (int i = 2, j=0, k=0 ; ; i++) {//visit each char in cur->operand
					if (cur->operand[i] == '\'') {//last char
						if (k == 0) {//the value is 0
							tmpOperand[0] = '0';
							tmpOperand[1] = '\0';
						}
						break;
					}
					if (k == 0) {//0 in the front can be removed
						if (cur->operand[i] == '0') {
							continue;
						}
						else {
							k = 1;
						}
					}
					if (cur->operand[i] == ' ') {//skip space
						continue;
					}
					//check if the operand value is hex number
					if ((cur->operand[i] >= '0' && cur->operand[i] <= '9') || (cur->operand[i] >= 'A' && cur->operand[i] <= 'F')) {
						tmpOperand[j++] = cur->operand[i];//save it to tmpOperand
						tmpOperand[j] = '\0';//to make sure that the last letter is null char
					}
					else {//non-hex character exists in the operand
						printf("Error: Invalid Operand - Operand is not Pure Hex Number\n");
						return -1;
					}
				}
				if (strlen(tmpOperand) % 2 == 0) {//the length of operand is even number
					//print to list file
					fprintf(fp_l, "%-4d   %04X   %-10s%-10s%-15s%s\n", lineNum, cur->loc, cur->label, cur->opcode, cur->operand, tmpOperand);
					//save object code to text[] 
					sprintf(text + strlen(text), "%s", tmpOperand);
				}
				else {//the length of operand is odd number
					//print to list file
					fprintf(fp_l, "%-4d   %04X   %-10s%-10s%-15s0%s\n", lineNum, cur->loc, cur->label, cur->opcode, cur->operand, tmpOperand);
					//save object code to text[] 
					sprintf(text + strlen(text), "0%s", tmpOperand);
				}
				
			}
		
			
		}
		else if (isOpcode(WORD, cur)) {//opcode = 'WORD' constant
			//print to list file
			fprintf(fp_l, "%-4d   %04X   %-10s%-10s%-15s%06X\n", lineNum, cur->loc, cur->label, cur->opcode, cur->operand, atoi(cur->operand));
			//save object code to text[] 
			sprintf(text + strlen(text), "%06X", atoi(cur->operand));
		}

		
		
	}

	//print last line of .lst file
	fprintf(fp_l, "%-4d          %-10s%-10s%-15s\n", lineNum, cur->label, cur->opcode, cur->operand);

	if (text[9] != '\0') {//object code exist in text
		//print last part of .obj file
		sprintf(tmp, "%02X", cur->loc - textStart);//convert line length to string
		strncpy(text + 7, tmp, 2);//save line length to text[]
		fprintf(fp_o, "%s\n", text);//print text to .obj file
	}
	fprintf(fp_o, "%s", modify);//print modify
	fprintf(fp_o, "E%06X\n", firstExe);//save first executable instruction

	free(modify);//free modify
	return 0;
}

/* assemble the input file */
int assembleAsm(CmdTokens* tokens) {
	char filename[STRPAR_LEN];
	char* frontFilename;
	FILE* fp; //file to be read
	FILE *fp_l, *fp_o; //files to be created

	clearSymTable(); //free slots and initialize to 0
	clearAsmSrc(); //free nodes of assembly code linkedlist and initialize
	ProgramLen = 0; //initialize program length
	SymbolNum = 0;//initialize the number of symbols

	strcpy(filename, tokens->strpar); // copy filename

	fp = fopen(filename, "r");//file open
	if (fp == NULL) {//file error check
		printf("Error: File Open Error\n");
		return -1;
	}

	//check file extension
	frontFilename = strtok(filename, ".");
	if (strcmp(strtok(NULL, "."), "asm")) {
		printf("Error: Wrong File Type");
		fclose(fp);
		return -1;
	}

	//run pass1. if error, terminate
	if (pass1(fp)) {//error
		fclose(fp);
		return -1;
	}

	//open lst and obj files to write
	fp_l = fopen(strcat(frontFilename, ".lst"), "w");
	frontFilename = strtok(filename, ".");
	fp_o = fopen(strcat(frontFilename, ".obj"), "w");
	if (fp_l == NULL || fp_o == NULL) {//file error check
		printf("Error: File Open Error\n");
		fclose(fp);
		return -1;
	}

	//run pass2. if error, delete lst and obj files and terminate
	if (pass2(fp_l, fp_o)) {//error
		fclose(fp);
		fclose(fp_l);
		fclose(fp_o);
		//delete files
		frontFilename = strtok(filename, ".");
		remove(strcat(frontFilename, ".lst"));
		frontFilename = strtok(filename, ".");
		remove(strcat(frontFilename, ".obj"));
		return -1;
	}

	makeSymList();//make symbol list
	fclose(fp);
	fclose(fp_l);
	fclose(fp_o);
	return 0;
}


/*function used in qsort to compare two strings */
int cmpSort(const void* a, const void* b)
{
	return strcmp(*(char**)a, *(char**)b);
}

/* make list of symbols */
void makeSymList(void) {
	char** symArr = (char**)calloc(SymbolNum, sizeof(char*));
	int index = 0;
	SymSlot *cur;

	if (SymList == NULL) {//initialize
		SymList = (char*)malloc(30 * SymbolNum * sizeof(char));
	}
	else if (SymbolNum > sizeof(SymList) / sizeof(char)) {//space is not enough
		SymList = (char*)realloc(SymList, 30 * SymbolNum * sizeof(char));//reallocate
	}
	memset(SymList, 0, strlen(SymList) + 2);//reset
	//initialize symArr while traversing SymTable
	for (int i = 0; i < SYMTAB_LEN; i++) {//visit each index
		cur = SymTable[i];
		while (cur) {
			symArr[index++] = cur->symb;//save string to symArr
			cur = cur->next;
		}
	}
	//sort symArr
	qsort(symArr, SymbolNum, sizeof(char*), cmpSort);
	//print symArr to SymList
	for (int i = 0; i < SymbolNum; i++) {
		sprintf(SymList + strlen(SymList), "        %-8s%04X", symArr[i], getSymAddr(symArr[i]));
		if (i < SymbolNum - 1) {
			sprintf(SymList + strlen(SymList), "\n");
		}
	}
	free(symArr);
}


/* print symbol list */
void listSymbol(void) {
	if (SymList == NULL) {//symbol table not created
		return;
	}
	//print symbol list
	printf("%s\n", SymList);
	return;
}
