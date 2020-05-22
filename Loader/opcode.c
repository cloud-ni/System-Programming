#include "20170001.h"
#include "opcode.h"

HashSlot * HashTable[20] = { 0 };

/* Create Hash Function with opcode */
int hashFunc(char* mnem) {
	int hashVal = 0;
	for (int i = 0; i < strlen(mnem); i++) {
		hashVal = 7 * hashVal + mnem[i] - 64;
	}
	return hashVal % 20;
}

/* add new slot to hash table*/
void addSlot(HashSlot* slot) {
	int hashVal = hashFunc(slot->mnem);
	HashSlot *cur = NULL;
	if (HashTable[hashVal] == 0) {//if none in the index
		HashTable[hashVal] = slot;
	}
	else {
		cur = HashTable[hashVal];
		while (cur->next) {// move to the last slot
			cur = cur->next;
		}
		cur->next = slot;
	}
}
/* Read the opcode file and create hash table */
int createHash(void) {
	int opcode, format1, format2 = 0;
	char mnem[10];
	HashSlot *newSlot = NULL;
	FILE * fp = fopen("opcode.txt", "r");
	if (fp == NULL) {
		printf("File open Error\n");
		return -1;
	}

	while (EOF != fscanf(fp, "%X %s %d/%d", &opcode, mnem, &format1, &format2)) {
		newSlot = (HashSlot*)malloc(sizeof(HashSlot));
		newSlot->opcode = opcode;
		strcpy(newSlot->mnem, mnem);
		newSlot->next = NULL;
		if (format1 == 3) {
			newSlot->format = 3;
		}
		else {
			newSlot->format = format1;
		}
		addSlot(newSlot);
	}
	fclose(fp);
	return 0;
}
/* delete hash table(free slots) */
void deleteHash(void) {
	HashSlot *cur = NULL;
	HashSlot *next = NULL;
	for (int i = 0; i < 20; i++) {
		cur = HashTable[i];
		HashTable[i] = NULL;
		while (cur) {
			next = cur->next;
			free(cur);
			cur = next;
		}
	}
	return;
}
/* search for mnemonic from opcode hash table */
HashSlot* searchHash(char *mnem) {
	int hashVal;
	HashSlot *cur = NULL;

	hashVal = hashFunc(mnem);
	cur = HashTable[hashVal];
	while (cur) {
		if (!strcmp(mnem, cur->mnem)) {//mnemonic found
			return cur;
		}
		cur = cur->next;
	}
	//if mnem is not in the table return -1
	return NULL;
}
/* function for user input "opcode" */
int getOpcode(CmdTokens *tokens) {
	HashSlot* slot = searchHash(tokens->strpar); //search mnemonic in the table

	//error if input mnemonic is not in the hash table
	if (slot == NULL) {
		printf("Error: The mnemonic is not valid\n");
		return -1;
	}

	printf("opcode is %02X\n", slot->opcode);
	return 0;
}
/* function for user input "listopcode"
prints opcode hash table */
void listOpcode(void) {
	HashSlot *cur = NULL;
	for (int i = 0; i < 20; i++) {
		printf("%-2d : ", i);
		cur = HashTable[i];
		while (cur) {
			if (cur != HashTable[i]) {
				printf(" -> ");
			}
			printf("[%s,%02X]", cur->mnem, cur->opcode);
			cur = cur->next;
		}
		printf("\n");
	}
}
