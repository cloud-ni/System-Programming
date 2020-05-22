#include "20170001.h"
#include "memory.h"

int MemLastAddr = -1; //save the last address of the memory printed by dump command

/*print dump memory*/
/*returns 1 when there is an error*/
int dumpMem(CmdTokens* tokens) {
	int i;
	int param_num = tokens->param_num;
	int start = tokens->par1;
	int end = tokens->par2;

	//error check & get start, end
	if (param_num == 0) { // zero parameter
		MemLastAddr++;//next address
		if (MemLastAddr >= MEMORY_SIZE)
			MemLastAddr = 0;
		start = MemLastAddr;
		//if next address + 160 is greater than or equal to 0xFFFFF
		//print until the last one
		if (MemLastAddr + 159 > MEMORY_SIZE - 1) {
			end = MEMORY_SIZE - 1;
		}
		else {
			end = MemLastAddr + 159;
		}
	}
	else if (param_num == 1) { //1 parameter
		// memory range error
		if (start < 0 || start > MEMORY_SIZE - 1) {
			printf("Error: The address is out of memory range\n");
			return -1;
		}
		if (start + 159 > MEMORY_SIZE - 1) {
			end = MEMORY_SIZE - 1;
		}
		else {
			end = start + 159;
		}
	}
	else if (param_num == 2) { //2 parameters
		// memory range error
		if (start < 0 || start > MEMORY_SIZE - 1 || end < 0 || end > MEMORY_SIZE - 1) {
			printf("Error: The address is out of memory range\n");
			return -1;
		}
		if (start > end) {
			printf("Error: Wrong address range(start < end)\n");
			return -1;
		}
	}


	for (i = (start / 16) * 16; i <= (end / 16) * 16 + 15; i++) {
		//print address
		if (i % 16 == 0) {
			printf("%05X ", (i / 16) * 16);
		}
		//print empty spaces if it is out of range
		if (i < start || i > end) {
			printf("   ");
		}
		else //print memory in hex
			printf("%02X ", Mem[i]);

		//print the ASCII part
		if (i % 16 == 15) {
			printf("; ");
			//print ASCII 
			for (int j = i - 15; j <= i; j++) {
				if (j >= start && j <= end && Mem[j] >= 0x20 && Mem[j] <= 0x7E) { //valid range
					printf("%c", Mem[j]);
				}
				else {//invalid range
					printf(".");
				}
			}
			printf("\n"); //next line
		}
	}
	MemLastAddr = end;
	return 0;
}

int editMem(CmdTokens *tokens) {
	int addr = tokens->par1;
	int val = tokens->par2;

	//check if the input is valid or not
	if (addr < 0 || addr >  MEMORY_SIZE - 1) {
		printf("Error: The address is out of memory range\n");
		return -1;
	}
	if (val < 0 || val > 0xFF) {
		printf("Error: The input value is not valid\n");
		return -1;
	}
	Mem[addr] = (unsigned char)val;
	return 0;
}
int fillMem(CmdTokens * tokens) {
	int start = tokens->par1;
	int end = tokens->par2;
	int val = tokens->par3;

	//memory range error
	if (start < 0 || start > MEMORY_SIZE - 1 || end < 0 || end > MEMORY_SIZE - 1) {
		printf("Error: The address is out of memory range\n");
		return -1;
	}
	if (start > end) {
		printf("Error: Wrong address range(start < end)\n");
		return -1;
	}
	//check value
	if (val < 0 || val > 0xFF) {
		printf("Error: The input value is not valid\n");
		return -1;
	}

	for (int i = start; i <= end; i++) {
		Mem[i] = (unsigned char)val;
	}
	return 0;
}
void resetMem(void) {
	for (int i = 0; i < MEMORY_SIZE; i++)
		Mem[i] = 0;
	return;
}