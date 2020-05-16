#define EXSYMTAB_LEN 10
#define OBJLINE_LEN 999
#define EXSYM_NUM 500

long CsAddr;//control section address
long ProgAddr;//program load address
long ProgLen;//total length of loaded program
long ExeAddr = 0;//execution address
long toHex(char* str, int len);