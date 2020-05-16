#include "main.h"
#include "execute.h"
#include "loader.h"

#define BPLIST_LEN 50
long* BpList;
int BpLen = 0;
typedef enum {A, X, L, B, S, T, F, PC=8, SW} reg;
long Reg[9];

void setBp(CmdTokens* tokens) {
	if (BpLen == 0) {
		BpList = (long*)malloc(BPLIST_LEN * sizeof(long));
	}
	else if (sizeof(BpList) / sizeof(long) <= BpLen + 1) {
		BpList = (long*)realloc(BpList, sizeof(BpList) + BPLIST_LEN * sizeof(long));
	}
	BpList[BpLen] = strtol(tokens->strpar, NULL, 16);
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
/* get format */
int getFormat(int* mnem, char* mem) {
	int flag;
	for (int i = 0; i < 2 && flag;i++) {
		if (flag) {
			flag = 0;
		}
		switch (*mnem) {
		case(TD): case(RD): case(WD):
			return 1;
		case(CLEAR):case(TIXR):
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
			if ((mem[2] & 0X1) == 0)
				return 3;
			else
				return 4;
		default:
			flag = 1;
			(*mnem) = (*mnem) & 0XFC;
		}

	}
	return -1;
}
/* calculate target address */
long getTA(int mnem, char* mem) {
	int ni, xbpe, addr, TA;
	ni = mem[1] % 4;//get ni
	xbpe = mem[2];//get xbpe
	//get specified address
	if ((xbpe & 1) == 0) {//format 3
		addr = mem[3] * 0X100 + mem[4] * 0X10 + mem[5];
	}
	else {//format 4
		addr = mem[3] * 0X1000 + mem[4] * 0X100 + mem[5] * 0X10 + mem[6];
	}
	if (xbpe & 8 == 8) {//indexing

	}
	if (xbpe & 4 == 4) {//base relative

	}
	if (xbpe & 2 == 2) {//pc relative 

	}
	switch (ni) {//addressing mode
	case(1)://immediate addressing

		break;
	case(2)://indirect addressing
		break;
	case(3)://simple addressing
		break;
	default:

	}
	return TA;
}
/* run instruction */
void exeInstruct(int mnem, int r1, int r2, long TA) {
	switch (mnem) {
	case(CLEAR):
		break;
	case(COMP):
		break;
	case(J):
		break;
	case(JEQ):
		break;
	case(JLT):
		break;
	case(JSUB):
		break;
	case(LDA):
		break;
	case(LDB):
		break;
	case(LDT):
		break;
	case(LDCH):
		break;
	case(RSUB):
		break;
	case(STA):
		break;
	case(STL):
		break;
	case(STX):
		break;
	case(STCH):
		break;
	case(TD):
		break;
	case(TIXR):
		break;
	case(WD):
		break;
	default:
	}
	return;
}
/* print registers */
void printReg(void) {
	printf("A  :  %06X   X  :  %06X\n", Reg[A] & 0XFFFFFF, Reg[X] & 0XFFFFFF);
	printf("L  :  %06X  PC  :  %06X\n", Reg[L] & 0XFFFFFF, Reg[PC] & 0XFFFFFF);
	printf("B  :  %06X   S  :  %06X\n", Reg[B] & 0XFFFFFF, Reg[S] & 0XFFFFFF);
	return;
}
/* run the program */
int runProgram(void) {
	int flag = 1, mnem, format;
	int r1, r2;
	long TA;
	memset(Reg, 0, 9 * sizeof(long));//reset registers
	//initialize registers
	Reg[PC] = ExeAddr;
	Reg[L] = ProgLen;
	
	while (1) {
		if (Reg[PC] >= ProgAddr + ProgLen) {//reached last code in the program
			ExeAddr = ProgAddr;//reset execution address
			break;
		}
		if (isBp(Reg[PC])) {//reached breakpoint
			ExeAddr = Reg[PC];//save breakpoint as next execution address
			break;
		}
		r1 = 0; r2 = 0; TA = 0;//initialize
		mnem = Mem[Reg[PC]] * 0X10 + Mem[Reg[PC] + 1];//get mnemonic
		format = getFormat(&mnem, Mem + Reg[PC]);//get format
		switch (format) {
		case(2):
			//get r1 and r2 from object code
			r1 = Mem[Reg[PC] + 1];
			r2 = r1 % 4;
			r1 = r1 / 4;
			break;
		case(3):
		case(4):
			TA = getTA(mnem, Mem + Reg[PC]);//get target address
			break;
		default:
			break;
		}
		Reg[PC] += format;//update program counter
		
		exeInstruct(mnem, r1, r2, TA);//run instruction
		
	}
	printReg();
	return 0;
}