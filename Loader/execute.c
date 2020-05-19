#pragma warning(disable : 4996)
#include "main.h"
#include "execute.h"
#include "loader.h"

#define BPLIST_LEN 50
long* BpList;
int BpLen = 0;
typedef enum {A, X, L, B, S, T, F, PC=8, SW} reg;
long Reg[10];

typedef struct {
	int r1;
	int r2;
	long addr;
	long val;
} Operand;

void setBp(CmdTokens* tokens) {
	if (BpLen == 0) {
		BpList = (long*)malloc(BPLIST_LEN * sizeof(long));
	}
	else if (sizeof(BpList) / sizeof(long) <= BpLen + 1) {
		BpList = (long*)realloc(BpList, sizeof(BpList) + BPLIST_LEN * sizeof(long));
	}
	BpList[BpLen] = strtol(tokens->strpar, NULL, 16);
	printf("                 [ok] create breakpoint %04X\n", BpList[BpLen] & 0XFFFF );
	BpLen++;
}
int isBp(long addr) {
	int i;
	for (i = 0; i < BpLen; i++) {
		if (BpList[i] == addr) {
			return 1;
		}
	}
	return 0;
}
void clearBp(void) {
	if (BpList == NULL) {
		return;
	}
	free(BpList);
	BpList = NULL;
	printf("                 [ok] clear all breakpoints\n");
	return;
}
void printBp(void) {
	int i;
	printf("          breakpoint\n          -----------\n");
	for (i = 0; i < BpLen; i++) {
		printf("          %X\n", BpList[i]);
	}
	return;
}
/* returns format */
int getFormat(int* mnem, char* mem) {
	int flag = 1;
	for (int i = 0; i < 2 && flag; i++) {
		if (flag) {
			flag = 0;
		}
		switch (*mnem) {
		case(COMPR):case(CLEAR):case(TIXR):
			return 2;
		case(COMP):
		case(J):
		case(JEQ):
		case(JLT):
		case(JSUB):
		case(LDA):
		case(LDB):
		case(LDT):
		case(LDCH):
		case(RSUB):
		case(STA):
		case(STL):
		case(STX):
		case(STCH):
		case(TD): 
		case(RD): 
		case(WD):
			if ((mem[1] & 0X10) == 0X10)//check e bit from xbpe
				return 4;
			else
				return 3;
		default://by default menm is 8bit, but it may be 6bit(format3/4)
			(*mnem) = (*mnem) & 0XFC;//change to 6bit
			flag = 1; //go through switch once again with new mnemonic
		}

	}
	return -1;//error
}
/* calculate target address */
void getTA(int mnem, unsigned char* mem, Operand* op) {
	int ni, xbpe, addr, tmp;

	ni = mem[0] % 4;//get ni
	xbpe = mem[1] >> 4;//get xbpe

	//get specified address
	if ((xbpe & 1) == 1) {//format 4
		addr = (mem[1] & 0x0F) * 0X10000 + mem[2] * 0X100 + mem[3];
		//2's complement
		if (addr > 0x7FFFF) {
			addr = (0xFFFFF - addr) + 1;
			addr = (~addr) + 1;
		}
	}
	else {//format 3
		addr = (mem[1] & 0x0F) * 0X100 + mem[2];
		//2's complement
		if (addr > 0x7FF) {
			addr = (0xFFF - addr) + 1;
			addr = (~addr) + 1;
		}
	}
	
	//pc relative 
	if ((xbpe & 2) == 2){
		addr += Reg[PC];
	}
	//base relative
	if ((xbpe & 4) == 4) {
		addr += Reg[B];
	}
	//indexing
	if ((xbpe & 8) == 8) {
		addr += Reg[X];
	}
	
	//addressing mode
	switch (ni) {
	case(1)://immediate addressing
		op->val = addr;
		break;
	case(2)://indirect addressing
		if ((xbpe & 1) == 1) {//format 4
			op->addr = Mem[addr] * 0X1000000 + Mem[addr + 1] * 0X10000 + Mem[addr + 2] * 0X100 + Mem[addr + 3];
		}
		else {
			op->addr = Mem[addr] * 0X10000 + Mem[addr + 1] * 0X100 + Mem[addr + 2];
		}
		
		break;
	case(3)://simple addressing
		op->addr = addr;
		break;
	default:
		break;
	}
	return;
}
void storeMem(long val, long addr, int len) {
	for (int i = len-1; i >= 0; i--) {
		Mem[addr + i] = (char)(val % 0X100);
		val = val / 0X100;
	}
	return;
}
/* run instruction */
void exeInstruct(int mnem, Operand* op) {
	int r1, r2, i;
	long addr, val, m3, m6;
	//subsitution
	r1 = op->r1;
	r2 = op->r2;
	addr = op->addr;
	val = op->val;

	if (addr) {
		m3 = 0;
		//calculate target value 
		for (i = 0;; i++) {
			m3 += Mem[addr + i];
			if (i == 2)
				break;
			m3 = m3 << 8;
		}
		m6 = m3 << 8;
		//calculate floating point value
		for (i = 3;; i++) {
			m6 += Mem[addr + i];
			if (i == 5)
				break;
			m6 = m6 << 8;
		}
	}
	else if (val) {
		m3 = m6 = val;
	}
	else {
		m3 = m6 = 0;
	}

	switch (mnem) {
	case(CLEAR):
		Reg[r1] = 0;
		break;
	case(COMP):
		if (Reg[A] > m3) {
			Reg[SW] = '>';
		}
		else if (Reg[A] == m3) {
			Reg[SW] = '=';
		}
		else {
			Reg[SW] = '<';
		}
		break;
	case(COMPR):
		if (Reg[r1] > Reg[r2]) {
			Reg[SW] = '>';
		}
		else if (Reg[r1] == Reg[r2]) {
			Reg[SW] = '=';
		}
		else {
			Reg[SW] = '<';
		}
		break;
	case(J):
		Reg[PC] = addr;
		break;
	case(JEQ):
		if (Reg[SW] == '=') {
			Reg[PC] = addr;
		}
		break;
	case(JLT):
		if (Reg[SW] == '<') {
			Reg[PC] = addr;
		}
		break;
	case(JSUB):
		Reg[L] = Reg[PC];
		Reg[PC] = addr;
		break;
	case(LDA):
		Reg[A] = m3;
		break;
	case(LDB):
		Reg[B] = m3;
		break;
	case(LDT):
		Reg[T] = m3;
		break;
	case(LDCH):
		Reg[A] = Mem[addr];
		break;
	case(RD):
		break;
	case(RSUB):
		Reg[PC] = Reg[L];
		break;
	case(STA):
		storeMem(Reg[A], addr, 3);
		break;
	case(STL):
		storeMem(Reg[L], addr, 3);
		break;
	case(STX):
		storeMem(Reg[X], addr, 3);
		break;
	case(STCH):
		Mem[addr] = (char)Reg[A];
		break;
	case(TD):
		Reg[SW] = '<';
		break;
	case(TIXR):
		Reg[X]++;
		if (Reg[X] > Reg[r1]) {
			Reg[SW] = '>';
		}
		else if (Reg[X] == Reg[r1]) {
			Reg[SW] = '=';
		}
		else {
			Reg[SW] = '<';
		}
		break;
	case(WD):
		break;
	default:
		break;
	}

	return;
}
/* print registers */
void printReg(void) {

	printf("A  :  %06X   X  :  %06X\n", (unsigned int)(Reg[A] & 0XFFFFFF), (unsigned int)(Reg[X] & 0XFFFFFF));
	printf("L  :  %06X  PC  :  %06X\n", (unsigned int)(Reg[L] & 0XFFFFFF), (unsigned int)(Reg[PC] & 0XFFFFFF));
	printf("B  :  %06X   S  :  %06X\n", (unsigned int)(Reg[B] & 0XFFFFFF), (unsigned int)(Reg[S] & 0XFFFFFF));
	printf("T  :  %06X   \n", (unsigned int)(Reg[T] & 0XFFFFFF));
	if (Reg[PC] == ProgLen) {
		printf("               End Program\n");
	}
	else {
		printf("               Stop at checkpoint[%X]\n", Reg[PC]);
	}
	
	return;
}
/* initialize Operand */
void initOperand(Operand* operand) {
	operand->r1 = 0;
	operand->r2 = 0;
	operand->addr = 0;
	operand->val = 0;
	return;
}
/* run the program */
int runProgram(void) {
	int flag = 1, mnem, format;
	long cur;
	Operand operand;

	if (ExeAddr == ProgAddr) {//starting point
		memset(Reg, 0, 9 * sizeof(long));//reset registers
		Reg[L] = ProgLen;
	}
	
	//initialize registers
	Reg[PC] = ExeAddr;
	
	while (1) {

		cur = Reg[PC];//save processing address

		initOperand(&operand);//initialize operand
		mnem = Mem[cur];//get mnemonic
		format = getFormat(&mnem, Mem + cur);//get format
		Reg[PC] += format;//update program counter

		switch (format) {
		case(2):
			//get r1 and r2 from object code
			operand.r1 = Mem[cur + 1];
			operand.r2 = operand.r1 % 0X10;
			operand.r1 = operand.r1 / 0X10;
			break;
		case(3):
		case(4):
			getTA(mnem, Mem + cur, &operand);//get target address
			break;
		default:
			break;
		}
		
		exeInstruct(mnem, &operand);//run instruction
		
		if (Reg[PC] == ProgLen) {//reached last code in the program
			ExeAddr = ProgAddr;//reset execution address
			break;
		}
		if (isBp(Reg[PC])) {//reached breakpoint
			ExeAddr = Reg[PC];//save breakpoint as next execution address
			break;
		}

	}
	printReg();//print registers

	
	return 0;
}


void main(void) {
	CmdTokens* tokens = (CmdTokens*)malloc(sizeof(CmdTokens));
	CmdTokens* tokens1 = (CmdTokens*)malloc(sizeof(CmdTokens));
	CmdTokens* tokens2 = (CmdTokens*)malloc(sizeof(CmdTokens));
	ProgAddr = 0;
	ExeAddr = 0;
	strcpy(tokens->strpar, "copy.obj"); 
	tokens->param_num = 1;
	loadObj(tokens);
	

	strcpy(tokens2->strpar, "3");
	setBp(tokens2);
	strcpy(tokens2->strpar, "2A");
	setBp(tokens2);
	strcpy(tokens2->strpar, "1046");
	setBp(tokens2);

	printBp();

	runProgram();
	runProgram();
	runProgram();
	runProgram();
	tokens->par1 = 0X4000;
	setProaddr(tokens1);
	free(BpList);
	free(tokens); free(tokens1); free(tokens2);
	return;
}