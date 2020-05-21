#pragma warning(disable : 4996)
#include "main.h"
#include "execute.h"
#include "loader.h"

#define BPLIST_LEN 50
long* BpList;
int BpLen = 0;

/* set new breakpoint */
int setBp(CmdTokens* tokens) {
	long addr;
	addr = strtol(tokens->strpar, NULL, 16);//get breakpoint address
	if (addr < ProgAddr || addr >= ProgAddr + ProgLen || addr < 0 || addr >= MEMORY_SIZE) {//bp out of boundary
		printf("Error: The breakpoint is out of the program address range. Currant Program range is [0x%X, 0x%X]\n", ProgAddr, ProgAddr + ProgLen - 1);
		return -1;
	}
	if (BpLen == 0) {//allocate memory 
		BpList = (long*)malloc(BPLIST_LEN * sizeof(long));
	}
	else if (sizeof(BpList) / sizeof(long) <= BpLen + 1) {//re allocate memory
		BpList = (long*)realloc(BpList, sizeof(BpList) + BPLIST_LEN * sizeof(long));
	}
	BpList[BpLen] = addr;//save breakbpoint
	printf("        [ok] create breakpoint %04X\n", BpList[BpLen] & 0XFFFF );
	BpLen++;
	return 0;
}
/* check if addr is breakpoint */
int isBp(long addr) {
	int i;
	for (i = 0; i < BpLen; i++) {//go through each index
		if (BpList[i] == addr) {//find addr in breakpoing list
			return 1;
		}
	}
	return 0;
}
/* clear breakpoint */
void clearBp(void) {
	if (BpList == NULL) {
		return;
	}
	free(BpList);
	BpList = NULL;
	printf("        [ok] clear all breakpoints\n");
	return;
}
/* print breakpoint */
void printBp(void) {
	int i;
	printf("        breakpoint\n        -----------\n");
	for (i = 0; i < BpLen; i++) {//move through BpList
		printf("        %X\n", BpList[i]);
	}
	return;
}
/* returns format */
int getFormat(int* mnem, char* mem) {
	int flag = 1;
	for (int i = 0; i < 2 && flag; i++) {
		if (flag) {//reset flag
			flag = 0;
		}
		switch (*mnem) {
		case FIX:
		case FLOAT:
		case HIO:
		case NORM:
		case SIO:
		case TIO:
			return 1;//format 1
		case ADDR:
		case(CLEAR):
		case(COMPR):
		case DIVR:
		case MULR:
		case RMO:
		case SHIFTL:		case SHIFTR:
		case SUBR:		case SVC:
		case(TIXR):
			return 2;//format 2
		case ADD:
		case ADDF:
		case AND:
		case(COMP):
		case COMPF:
		case DIV:
		case DIVF:
		case(J):
		case(JEQ):
		case JGT:
		case(JLT):
		case(JSUB):
		case(LDA):
		case(LDB):
		case(LDCH):		case LDF:		case LDL:		case LDS:		case(LDT):		case LDX:		case LPS:		case MUL:		case MULF:		case OR:		case(RD):
		case(RSUB):		case SSK:		case(STA):		case(STB):		case(STCH):		case STF:		case STI:		case(STL):		case STS:		case STSW:		case STT:		case(STX):		case SUB:
		case SUBF:		case(TD):		case TIX:		case(WD):
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
	int ni, xbpe, addr;

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
/* store value in a register to memory */
void storeMem(long val, long addr, int len) {
	//store each byte starting from the last byte
	for (int i = len-1; i >= 0; i--) { 
		Mem[addr + i] = (char)(val % 0X100);
		val = val / 0X100;
	}
	return;
}
/* run instruction */
int exeInstruct(int mnem, Operand* op) {
	int r1, r2, i;
	long addr, val, m3, m6;
	//subsitution
	r1 = op->r1;
	r2 = op->r2;
	addr = op->addr;
	val = op->val;

	//calculate target value
	if (addr) {
		m3 = 0;
		//calculate target value(3byte)
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

	//execute instruction
	switch (mnem) {
	case ADD:
		Reg[A] += m3;
		break;
	case ADDF:
		Reg[F] += m6;
		break;
	case ADDR:
		Reg[r2] += Reg[r1];
		break;
	case AND:
		Reg[A] &= m3;
		break;
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
	case COMPF:
		if (Reg[F] > m6) {
			Reg[SW] = '>';
		}
		else if (Reg[F] == m6) {
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
	case DIV:
		if (!m3) {
			printf("Error during execution: Unable to divide by 0\n");
			return -1;
		}
		Reg[A] /= m3;
		break;
	case DIVF:
		if (!m6) {
			printf("Error during execution: Unable to divide by 0\n");
			return -1;
		}
		Reg[F] /= m6;
		break;
	case DIVR:
		if (!Reg[r1]) {
			printf("Error during execution: Unable to divide by 0\n");
			return -1;
		}
		Reg[r2] /= Reg[r1];
		break;
	case FIX:
		Reg[A] = Reg[F];
		break;
	case FLOAT:
		Reg[F] = Reg[A];
		break;
	case HIO:
		break;
	case(J):
		Reg[PC] = addr;
		break;
	case(JEQ):
		if (Reg[SW] == '=') {
			Reg[PC] = addr;
		}
		break;
	case JGT:
		if (Reg[SW] == '>')
			Reg[PC] = addr;
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
	case(LDCH):
		Reg[A] = Mem[addr];
		break;
	case LDF:
		Reg[F] = m6;
		break;
	case LDL:
		Reg[L] = m3;
		break;
	case LDS:
		Reg[S] = m3;
		break;
	case(LDT):
		Reg[T] = m3;
		break;
	case LDX:
		Reg[X] = m3;
		break;
	case LPS:
		break;
	case MUL:
		Reg[A] *= m3;
		break;
	case MULF:
		Reg[F] *= m6;
		break;
	case MULR:
		Reg[r2] *= Reg[r1];
		break;
	case NORM:
		break;
	case OR:
		Reg[A] |= m3;
		break;
	case(RD):
		break;
	case RMO:
		Reg[r2] = Reg[r1];
		break;
	case(RSUB):
		Reg[PC] = Reg[L];
		break;
	case SHIFTL:
		Reg[r1] = (Reg[r1] << (r2 + 1)) | (Reg[r1] >> (8 - (r2 + 1)));
		break;
	case SHIFTR:
		Reg[r1] = (Reg[r1] >> (r2 + 1)) | ((Reg[r1] / 0X800)*(0XFFF << (24 - (r2 + 1)))) ;
		break;
	case SIO:
		break;
	case SSK:
		break;
	case(STA):
		storeMem(Reg[A], addr, 3);
		break;
	case(STB):
		storeMem(Reg[B], addr, 3);
		break;
	case(STCH):
		Mem[addr] = (char)Reg[A];
		break;
	case STF:
		storeMem(Reg[F], addr, 6);
		break;
	case STI:
		break;
	case(STL):
		storeMem(Reg[L], addr, 3);
		break;
	case STS:
		storeMem(Reg[S], addr, 3);
		break;
	case STSW:
		storeMem(Reg[SW], addr, 3);
		break;
	case STT:
		storeMem(Reg[T], addr, 3);
		break;
	case(STX):
		storeMem(Reg[X], addr, 3);
		break;
	case SUB:
		Reg[A] -= m3;
		break;
	case SUBF:
		Reg[F] -= m6;
		break;
	case SUBR:
		Reg[r2] -= Reg[r1];
		break;
	case SVC:
		break;
	case(TD):
		Reg[SW] = '<';
		break;
	case TIO:
		break;
	case TIX:
		Reg[X]++;
		if (Reg[X] > m3) {
			Reg[SW] = '>';
		}
		else if (Reg[X] == m3) {
			Reg[SW] = '=';
		}
		else {
			Reg[SW] = '<';
		}
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

	return 0;
}
/* print registers */
void printReg(void) {

	printf("A  :  %06X   X  :  %06X\n", (unsigned int)(Reg[A] & 0XFFFFFF), (unsigned int)(Reg[X] & 0XFFFFFF));
	printf("L  :  %06X  PC  :  %06X\n", (unsigned int)(Reg[L] & 0XFFFFFF), (unsigned int)(Reg[PC] & 0XFFFFFF));
	printf("B  :  %06X   S  :  %06X\n", (unsigned int)(Reg[B] & 0XFFFFFF), (unsigned int)(Reg[S] & 0XFFFFFF));
	printf("T  :  %06X   \n", (unsigned int)(Reg[T] & 0XFFFFFF));
	
	if (Reg[PC] == ExeProgLen) {
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

	
	if (ExeAddr == ExeProgAddr) {//execution started
		ExeProgLen = ProgLen;
		Reg[L] = ProgLen;
		if (ExeProgAddr != ProgAddr) {//set ExeAddr as new ProgAddr
			ExeProgAddr = ProgAddr;
			ExeAddr = ProgAddr;
		}
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
		//run instruction
		if (exeInstruct(mnem, &operand)) {
			return -1;
		}

		if (Reg[PC] == ExeProgLen) {//reached last code in the program
			printReg();//print registers
			memset(Reg, 0, 10 * sizeof(long));//reset registers
			Reg[L] = ProgLen;
			ExeAddr = ProgAddr;//reset execution address
			ExeProgAddr = ProgAddr;
			ExeProgLen = ProgLen;
			break;
		}
		if (isBp(Reg[PC])) {//reached breakpoint
			printReg();//print registers
			ExeAddr = Reg[PC];//save breakpoint as next execution address
			break;
		}

	}
	
	return 0;
}


void main(void) {
	CmdTokens* tokens = (CmdTokens*)malloc(sizeof(CmdTokens));
	CmdTokens* tokens1 = (CmdTokens*)malloc(sizeof(CmdTokens));
	CmdTokens* tokens2 = (CmdTokens*)malloc(sizeof(CmdTokens));
	
	//initialize
	ProgAddr = 0;
	ExeAddr = 0;
	ExeProgAddr = 0;


	///////proga, progb, progc
	tokens->par1 = 0X4000;
	setProaddr(tokens);

	strcpy(tokens->strpar, "proga.obj"); 
	strcpy(tokens->strpar1, "progb.obj");
	strcpy(tokens->strpar2, "progc.obj");
	tokens->param_num = 3;
	loadObj(tokens);
	
	for (int i = ProgAddr; i <= ProgAddr + ProgLen; i++) {
		if (i % 0x10 == 0) {
			printf("\n");
			printf("%04X ", i);
		}
		if (i % 4 == 0) {
			printf(" ");
		}
		printf("%02X.", Mem[i]);
		
	}
	
	tokens->par1 = 0X4000;
	setProaddr(tokens);
	runProgram();
	
	

	/////////copy.obj
	tokens->par1 = 0;
	setProaddr(tokens);

	strcpy(tokens->strpar, "copy.obj");
	tokens->param_num = 1;
	loadObj(tokens);

	for (int i = ProgAddr; i <= ProgAddr + ProgLen; i++) {

		if (i >= 0x30 && i < 0x1030) {
			continue;
		}
		if (i % 0x10 == 0) {
			printf("\n");
			printf("%04X ", i);
		}
		if (i % 4 == 0) {
			printf(" ");
		}
		printf("%02X.", Mem[i]);

	}
	
	strcpy(tokens2->strpar, "3");
	setBp(tokens2);
	strcpy(tokens2->strpar, "2a");
	setBp(tokens2);
	strcpy(tokens2->strpar, "1046");
	setBp(tokens2);
	printBp();

	runProgram();
	runProgram();
	runProgram();
	runProgram();

	
	//free 
	free(BpList);
	free(tokens); free(tokens1); free(tokens2);
	return;
}