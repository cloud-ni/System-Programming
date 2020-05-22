//history linked list node
typedef struct HistNode {
	char instruct[INSTRUCT_LEN];
	struct HistNode * next;
} HistNode;

void helpCmd(void); //help - print lists of valid commands
void dirCmd(void); //dir - show files in the current directory
void quitCmd(void); //quit shell
void addHistory(char *str); //add current instruction to history
void historyCmd(void); //print history
void freeHistory(void); //free linked list used for history
int typeCmd(CmdTokens *tokens);