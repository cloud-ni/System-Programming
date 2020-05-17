#define EXSYMTAB_LEN 10
#define OBJLINE_LEN 999
#define EXSYM_NUM 500

typedef struct ExSymSlot {//node in external symbol table 
	char symb[10];//symbol name or name of control section
	long addr;//address
	long len;//length of control section
	int csFlag; //flag for control section
	struct ExSymSlot *next;
} ExSymSlot;
ExSymSlot* ExSymTab[EXSYMTAB_LEN]; //external symbol table

long CsAddr;//control section address
long ProgAddr;//program load address
long ProgLen;//total length of loaded program
long ExeAddr;//execution address
long toHex(char* str, int len);

int loaderPass2(FILE ** fp);

int loadObj(CmdTokens * tokens);

void setProaddr(CmdTokens * tokens);

ExSymSlot * newExSymSlot(void);

int ExSymTabFunc(char * symb);

void insertExSymTab(ExSymSlot * newSlot);

ExSymSlot * getExSym(char * symb);

void clearExSymTab(void);

void printLoadMap(void);

void copySymb(char * str1, char * str2);

int loaderPass1(FILE ** fp);
