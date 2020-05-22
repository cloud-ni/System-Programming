typedef struct SymSlot {//slot for symbol table
	char symb[10];//symbol
	int addr;//location(address) of the symbol 
	struct SymSlot *next;//points to next slot with same hash value
} SymSlot;
SymSlot* SymTable[SYMTAB_LEN]; //symbol table in hash table

typedef struct AsmSrcNode { //for linked list that stores source lines
	char line[ASMLINE_LEN]; //initial source instruction
	int loc;//location
	char label[ASMTOK_LEN];//label
	char opcode[ASMTOK_LEN]; //operation code - opcode or directive
	char operand[ASMTOK_LEN]; //first operand
	char operand2[10]; //second operand for 2nd register
	int format;//not known: -1,format2: 2, format3: 3, format4: 4
	enum { none, immediate, indirect, simple } mode;//addressing mode
	struct AsmSrcNode *next;//next node(instruction)
}AsmSrcNode;

char * SymList;

//AsmSrc linked list functions
AsmSrcNode* insertAsmSrc(char* line, char* label, char* opcode, char* operand, char* operand2);
void clearAsmSrc(void);

//symbol talbe functions
int SymTableFunc(char* sym);
void insertSymTable(char* symb, int addr);
int searchSymtable(char * label);
void clearSymTable(void);

//pass1
void splitAsmLine(char * line, char * label, char * opcode, char * operand, char * operand2);
int isOpcode(int dir, AsmSrcNode * cur);
int pass1(FILE * fp);

//pass2
int getRegNum(char * reg);
int getSymAddr(char * sym);
int isNumber(char * string);
int pass2(FILE * fp_l, FILE * fp_o);

//assemble
int assembleAsm(CmdTokens * tokens);

//print assemble
int cmpSort(const void * a, const void * b);
void makeSymList(void);
void listSymbol(void);