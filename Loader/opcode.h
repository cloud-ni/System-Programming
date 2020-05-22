typedef struct HashSlot {
	int opcode;
	char mnem[10];
	int format; //format1: 1 format2: 2 format3/4:3
	struct HashSlot *next;
} HashSlot;

int hashFunc(char* mnem); //hash function to generate hash value
void addSlot(HashSlot* slot); //add new slot to the hash table
int createHash(void); //read opcode file and create hash table
void deleteHash(void); //free slots in the hash table
HashSlot* searchHash(char *mnem); //find mnemonic in the hash table
int getOpcode(CmdTokens *tokens); //get opcode of the mnemonic
void listOpcode(void); // list opcodes in the hash table(print table)