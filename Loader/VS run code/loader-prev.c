#pragma warning(disable : 4996)
#include "main.h"
#include "loader.h"
#include "execute.h"


/* create new external symbol slot */
ExSymSlot* newExSymSlot(void) {
	ExSymSlot *newSlot = (ExSymSlot*)malloc(sizeof(ExSymSlot));//allocate memory]
	//initialize
	newSlot->addr = 0;
	newSlot->len = 0;
	newSlot->csFlag = 0;
	newSlot->next = NULL;
	return newSlot;
}
/* hash function for external symbol table */
int ExSymTabFunc(char* symb) {
	unsigned long val = 5381;
	while (*symb != '\0') {//visit each character
		val = 33 * val + *symb;
		symb++;
	}
	return (int)(val % EXSYMTAB_LEN);
}
/* create new slot and insert it into ExSymTab */
void insertExSymTab(ExSymSlot *newSlot) {
	ExSymSlot *cur, *prev;
	int index;
	index = ExSymTabFunc(newSlot->symb); //get hash value
	cur = prev = ExSymTab[index]; //point to the first slot
	while (cur) { //get the last slot in the current index of hash table
		prev = cur;
		cur = cur->next;
	}
	//link new slot to appropriate place
	if (prev)
		prev->next = newSlot;
	else
		ExSymTab[index] = newSlot;
	return;
}

/* get external symbol
returns NULL if not found */
ExSymSlot* getExSym(char * symb) {
	ExSymSlot *cur;
	int index;
	index = ExSymTabFunc(symb);//get hash value
	cur = ExSymTab[index];//first slot of the index
	while (cur) {//visit each slot in the same index
		if (!strcmp(cur->symb, symb)) {//symbol found in the table
			return cur;
		}
		cur = cur->next;
	}
	return NULL;//symbol not found in the table
}

/* reset ExSymTab */
void clearExSymTab(void) {
	ExSymSlot *cur, *next;
	for (int i = 0; i < EXSYMTAB_LEN; i++) {
		cur = ExSymTab[i];
		while (cur) {
			next = cur->next;
			free(cur);
			cur = next;
		}
		ExSymTab[i] = 0;
	}
	return;
}
/* print load map with external symbol table */
void printLoadMap(void) {
	ExSymSlot *cur;
	printf("control symbol address length\nsection name\n-----------------------------------\n");
	for (int i = 0; i < EXSYMTAB_LEN; i++) {
		cur = ExSymTab[i];
		while (cur) {
			if (cur->csFlag == 1) {//control section
				printf("%-10s          %04X    %04X\n", cur->symb, cur->addr & 0XFFFF, cur->len & 0XFFFF);
			}
			else {//symbol name
				printf("          %6s    %04X\n", cur->symb, cur->addr & 0XFFFF);
			}
			cur = cur->next;
		}

	}
	printf("-----------------------------------\n                  total length %04X\n", ProgLen & 0XFFFF);
	return;
}
void copySymb(char* str1, char* str2) {
	int i = 0;
	while (str1[i] == ' ' || str1[i] == '\0') {
		str2[i] = str1[i];
		i++;
	}
	str2[i] = '\0';
	return;
}

/* Assign addresses to all external symbols */
int loaderPass1(FILE** fp) {
	char line[OBJLINE_LEN];//buffer to read a line from object code
	long CsLth;//length of control section
	char *strp;
	char addr[10];
	char progName[20];
	int labelIdx;
	ExSymSlot* symSlot;
	CsAddr = ProgAddr;//initialize address of control section
	ProgLen = 0;//initialize total length
	clearExSymTab();

	for (int i = 0; i < 3 && (*fp); i++) {//read each file
		while (!feof(*fp)) {//go through multiple control sections in a file

			fgets(line, OBJLINE_LEN, *fp);//read header line
			if (line[strlen(line) - 1] == '\n') {
				line[strlen(line) - 1] = '\0';
			}

			if (line[0] == '.')//check if it is comment
				continue;

			strp = strtok(line, " ");//get control section name
			strcpy(progName, strp + 1);//save program name
			symSlot = getExSym(strp + 1);//find symbol from ExSymTab
			if (symSlot) {//found cs name from ExSymTab
				printf("Error from pass1: Duplicate program name '%s'\n", symSlot->symb);
				return -1;
			}
			//not found, create new ExSym slot
			symSlot = newExSymSlot();
			//set ExSymSlot
			strcpy(symSlot->symb, strp + 1);
			symSlot->csFlag = 1;
			strp = strtok(NULL, " ");//get control section address and length
			symSlot->len = strtol(strp + 6, NULL, 16);//save CS length
			CsLth = symSlot->len;//remember CS length
			strp[6] = '\0';
			symSlot->addr = CsAddr + strtol(strp, NULL, 16);//save CS address
			insertExSymTab(symSlot);//insert the slot into ExSymTab

			while (1) {
				fgets(line, OBJLINE_LEN, *fp); //read next line
				if (line[strlen(line) - 1] == '\n') {
					line[strlen(line) - 1] = '\0';
				}

				if (line[0] == 'E') {//end record
					break;
				}

				if (line[0] == 'D') {//define record
					strp = strtok(line, " ");//get first token
					labelIdx = 1;//symbol name starts from index 1
					while (strp[6] != '\0' || labelIdx == 1) {//while next symbol exists or it is first symbol
						//search ExSymTab for symbol name
						symSlot = getExSym(strp + labelIdx);
						if (symSlot) {//found 
							printf("Error from pass1: Duplicate external symbol '%s' in program '%s'\n", symSlot->symb, progName);
							return -1;
						}
						//not found, create new ExSym slot
						symSlot = newExSymSlot();
						strcpy(symSlot->symb, strp + labelIdx);//save symbol name
						//get next token
						strp = strtok(NULL, " ");
						//extract address
						strncpy(addr, strp, 6);
						addr[6] = '\0';
						symSlot->addr = CsAddr + strtol(addr, NULL, 16);//save address
						insertExSymTab(symSlot);//insert into ExSymTab

						if (labelIdx == 1) {
							labelIdx = 6;//symbol name starts from index 6
						}
					}
				}
			}
			CsAddr += CsLth;//set starting address for next control section
			ProgLen += CsLth;
		}
		fseek((*fp), 0, SEEK_SET);//move file pointer to the front
		fp++;
	}
	return 0;
}
/* converting string to hex integer
len: the length of digits */
long toHex(char* str, int len) {
	int i, digit;
	long res = 0;
	for (i = len - 1; i >= 0; i--) {//go through each index from the smallest digit
		digit = len - i - 1;
		if (str[i] >= '0' && str[i] <= '9') {
			res += (str[i] - '0')*(1 << (digit * 4));
		}
		else if (str[i] >= 'A' && str[i] <= 'F') {
			res += (str[i] - 'A' + 10)*(1 << (digit * 4));
		}
		else if (str[i] >= 'a' && str[i] <= 'f') {
			res += (str[i] - 'a' + 10)*(1 << (digit * 4));
		}
		else {
			return -1;
		}
	}
	return res;
}
/* Performs loading, relocation, linking */
int loaderPass2(FILE** fp) {
	char line[OBJLINE_LEN];//buffer to read a line from object code
	long CsLth;//lenth of control section
	char *strp;
	long addr, val, len;
	char progName[20];
	ExSymSlot* symSlot;
	long symList[EXSYM_NUM];
	CsAddr = ProgAddr;

	for (int i = 0; i < 3 && (*fp); i++) {//read each file
		while (!feof(*fp)) {//go through multiple control sections in a file

			memset(symList, 0, EXSYM_NUM * sizeof(long));//initialize 

			fgets(line, OBJLINE_LEN, *fp);//read header line
			if (line[strlen(line) - 1] == '\n') {
				line[strlen(line) - 1] = '\0';
			}

			if (line[0] == '.')//check if it is comment
				continue;

			strp = strtok(line, " ");
			strcpy(progName, strp + 1);
			symSlot = getExSym(strp + 1);//search ExSymTab for program address
			if (!symSlot) {//symbol not found
				printf("Error from pass2: Undefined Program '%s'\n", strp + 1);
				memset(Mem + ProgAddr, 0, sizeof(Mem[0])*ProgLen);//unload program
				return -1;
			}

			symList[1] = symSlot->addr;//save address in symbol list
			CsLth = symSlot->len;//save control section length

			while (1) {
				fgets(line, OBJLINE_LEN, *fp); //read next line
				if (line[strlen(line) - 1] == '\n') {
					line[strlen(line) - 1] = '\0';
				}

				if (line[0] == 'E')//end record
					break;
				else if (line[0] == 'R') {//refer record
					strp = strtok(line + 1, " ");//get first token
					while (strp) {
						symSlot = getExSym(strp + 2);//search ExSymTab for symbol name
						if (!symSlot) {//symbol not found
							printf("Error from pass2: Undefined External Symbol '%s' in program '%s'\n", strp + 2, progName);
							memset(Mem + ProgAddr, 0, sizeof(Mem[0])*ProgLen);//unload program
							return -1;
						}
						//save address in the reference number
						symList[atoi(strp)] = symSlot->addr;
						strp = strtok(NULL, " ");//get next token
					}
				}
				else if (line[0] == 'T') {//text record
					strp = line + 9;//get starting point of object code
					addr = CsAddr + toHex(line + 1, 6);//get real starting address of the line
					len = toHex(line + 7, 2);
					for (i = 0; i < len; i++) {//load each byte of object code
						if (addr + i >= MEMORY_SIZE) {//memory bound error
							printf("Error from pass2: No more memory can be loaded because the memory address exceeded FFFFF. Address range: [0, FFFFF]\n");
							return -1;
						}
						Mem[addr + i] = (char)toHex(strp + 2 * i, 2);
					}

				}
				else if (line[0] == 'M') {//modification record
					addr = CsAddr + toHex(line + 1, 6);//get real starting address of modify code
					if (addr < 0 || addr + 2 >= MEMORY_SIZE) {//memory bound error;
						printf("Error from pass2: Modfication code address is out of memory bound. Address range: [0, FFFFF]\n");
						return -1;
					}
					//get the current value in the address
					val = Mem[addr] * 0X10000 + Mem[addr + 1] * 0X100 + Mem[addr + 2];
					if (line[9] == '+') {//add referenced address
						val += symList[atoi(line + 10)];
					}
					else if (line[9] == '-') {//subtract referenced address
						val -= symList[atoi(line + 10)];
					}
					for (i = 2; i >= 0; i--) {//load each byte of new address
						Mem[addr + i] = val & 0XFF;
						val = val >> 8;
					}
				}
			}
			CsAddr += CsLth;//set starting address for next control section
		}
		fseek((*fp), 0, SEEK_SET);//move file pointer to the front
		fp++;
	}
	return 0;
}
/* linking and loading object code */
int loadObj(CmdTokens* tokens) {
	FILE* fp[4] = { NULL, NULL, NULL, NULL };
	//initialize
	clearExSymTab();

	//open files
	if (tokens->param_num >= 1) {
		fp[0] = fopen(tokens->strpar, "r");
	}
	if (tokens->param_num >= 2) {
		fp[1] = fopen(tokens->strpar1, "r");
	}
	if (tokens->param_num >= 3) {
		fp[2] = fopen(tokens->strpar2, "r");
	}

	//run pass1
	if (loaderPass1(fp)) {
		return -1;
	}

	//run pass2
	if (loaderPass2(fp)) {
		return -1;
	}

	//close files
	for (int i = 0; i < tokens->param_num; i++) {
		fclose(fp[i]);
	}

	memset(Reg, 0, 10 * sizeof(long));//reset registers
	Reg[L] = ProgLen;
	ExeAddr = ProgAddr;//set execution address

	return 0;
}
/* sets program address */
int setProaddr(CmdTokens* tokens) {
	if (tokens->par1 < 0 || tokens->par1 >= 0x100000) {//input bound error
		printf("Address Out of Memory Bound. Address range: [0, FFFFF]\n");
		return -1;
	}
	ProgAddr = tokens->par1;
	return 0;
}