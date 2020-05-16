#include "main.h"
#include "loader.h"


typedef struct ExSymSlot {//node in external symbol table 
	char symb[10];//symbol name or name of control section
	long addr;//address
	long len;//length of control section
	int csFlag; //flag for control section
	struct ExSymSlot *next;
} ExSymSlot;
ExSymSlot* ExSymTab[EXSYMTAB_LEN]; //external symbol table

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
				printf("%-10s          %04X    %04X\n",cur->symb, cur->addr & 0XFFFF, cur->len & 0XFFFF);
			}
			else {//symbol name
				printf("          %6s    %04X\n", cur->symb, cur->addr & 0XFFFF);
			}
			cur = cur->next;
		}
		
	}
	printf("-----------------------------------\n                    total length %04X\n", TotalLen & 0XFFFF);
	return;
}
void copySymb(char* str1, char* str2) {
	int i = 0;
	while(str1[i] == ' ' || str1[i] == '\0') {
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
	int labelIdx;
	ExSymSlot* symSlot;
	CsAddr = ProgAddr;//initialize address of control section
	ProgLen = 0;//initialize total length
	for(int i=0; i<3; i++){//read each file
		while (!feof(*fp)) {//go through multiple control sections in a file
			
			fgets(line, OBJLINE_LEN, *fp);//read header line
			if (line[strlen(line) - 1] == '\n') {
				line[strlen(line) - 1] = '\0';
			}

			if (line[0] == '.')//check if it is comment
				continue;

			strp = strtok(line, " ");//get control section name
			symSlot = getExSym(strp + 1);//find symbol from ExSymTab
			if (symSlot) {//found cs name from ExSymTab
				printf("Error: duplicate external symbol");
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
			symSlot->addr = strtol(strp, NULL, 16);//save CS address
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
					while (strp[6]!='\0' || labelIdx == 1) {//while next symbol exists or it is first symbol
						//search ExSymTab for symbol name
						symSlot = getExSym(strp + labelIdx);
						if (symSlot) {//found 
							printf("Error: duplicate external symbol");
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
		fp++;
	}
	return 0;
}
/* converting string to hex integer 
len: the length of digits */
long toHex(char* str, int len) {
	int i, digit;
	long res = 0;
	for (i = len-1; i >= 0; i--) {//go through each index from the smallest digit
		digit = len - i - 1;
		if (str[i] >= '0' && str[i] <= '9') {
			res += (str[i]-'0')*(1 << (digit * 4));
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
	char buf[50];
	long addr, val;
	int i;
	ExSymSlot* symSlot;
	long symList[EXSYM_NUM];
	CsAddr = ProgAddr;
	
	for (int i = 0; i < 3; i++) {//read each file
		while (!feof(*fp)) {//go through multiple control sections in a file

			fgets(line, OBJLINE_LEN, *fp);//read header line
			if (line[strlen(line) - 1] == '\n') {
				line[strlen(line) - 1] = '\0';
			}

			if (line[0] == '.')//check if it is comment
				continue;
			
			strp = strtok(line, " ");
			strp = strtok(NULL, " ");//get control section address and length
			CsLth = strtol(strp + 6, NULL, 16);//remember CS length

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
							printf("Error: Undefined External Symbol\n");
							return -1;
						}
						//save address in the reference number
						symList[(int)toHex(strp, 2)] = symSlot->addr;
						strp = strtok(NULL, " ");//get next token
					}
				}
				else if (line[0] == 'T') {//text record
					strp = line + 9;//get starting point of object code
					addr = CsAddr + toHex(line + 1, 6);//get real starting address of the line
					for (i = 0; i < toHex(line + 7, 2); i++) {//load each byte of object code
						Mem[addr + i] = (char)toHex(strp + i, 2);
					}

				}
				else if (line[0] == 'M') {//modification record
					addr = CsAddr + toHex(line + 1, 6);//get real starting address of modify code
					//get the current value in the address
					val = Mem[addr] * 0X100 + Mem[addr + 1] * 0X10 + Mem[addr + 2];
					if (line[9] == '+') {//add referenced address
						val += symList[toHex(line + 10, 2)];
					}
					else if (line[9] == '-') {//subtract referenced address
						val -= symList[toHex(line + 10, 2)];
					}
					for (i = 0; i < 3; i++) {//load each byte of new address
						Mem[addr + i] = (val >> ((2 - i) * 4)) & 0XF;
					}
				}
			}
			CsAddr += CsLth;//set starting address for next control section
		}
		fp++;
	}
	return 0;
}
int loadAsm(CmdTokens* tokens) {
	FILE* fp[3] = { NULL, NULL, NULL };
	//initialize
	clearExSymTab();

	//open files
	if (tokens->strpar) {
		fp[0] = fopen(tokens->strpar, "r");
	}
	if (tokens->strpar1) {
		fp[1] = fopen(tokens->strpar1, "r");
	}
	if (tokens->strpar2) {
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

	ExeAddr = ProgAddr;//set execution address

	//close files
	for (int i = 0; i < 3; i++) {
		fclose(fp[i]);
	}
}
/* sets program address */
void setProaddr(CmdTokens* tokens) {
	ProgAddr = tokens->par1;
	return;
}