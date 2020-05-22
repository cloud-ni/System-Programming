#include "20170001.h"
#include "shell.h"
#include "opcode.h"

// head, tail, and length of history linked list
HistNode * histHead = NULL;
HistNode * histTail = NULL;
int histLen = 0;

/* print available commands */
void helpCmd(void) {
	printf(
		"h[elp] \n"
		"d[ir] \n"
		"q[uit] \n"
		"hi[story] \n"
		"du[mp][start, end] \n"
		"e[dit] address, value \n"
		"f[ill] start, end, value \n"
		"reset \n"
		"opcode mnemonic \n"
		"opcodelist \n"
		"assemble filename \n"
		"type filename \n"
		"symbol \n"
		"progaddr address\n"
		"loader filename1 [filename2] [filename3]\n"
		"bp [address]\n"
		"bp clear\n"
		"run\n"
	);
	return;
}

/* print files and folder in current directory */
void dirCmd(void) {
	DIR *dirp = NULL;
	struct dirent *file;
	struct stat sbuf;
	short flag = 0;

	//create file stream to read files, folder in current directory
	if ((dirp = opendir(".")) == NULL) {
		perror("Cannot read the directory\n");
		return;
	}

	//read files and folders in current directory in order
	while ((file = readdir(dirp)) != NULL) {
		if (file->d_ino == 0) //exclude deleted file 
			continue;
		stat(file->d_name, &sbuf); //get file status
		if (sbuf.st_mode & S_IFDIR) { //if directory, add '/' 
			printf("%15s/", file->d_name); //filename print
		}
		else if (sbuf.st_mode & S_IXUSR) { //if exe file, add '*' 
			printf("%15s*", file->d_name); //filename print
		}
		else {
			printf("%16s", file->d_name); //filename print
		}
		flag++;
		if (flag % 4 == 0) {
			printf("\n");
		}
	}
	if (flag == 0 || flag % 4)
		printf("\n");
	closedir(dirp); //close directory filename stream
	return;
}
/*terminate shell*/
void quitCmd(void) {
	deleteHash();//free Hash table slots
	freeHistory();//free resources
	exit(0);//terminate
	return;
}
/*create history */
void addHistory(char *str) {
	HistNode * cur = (HistNode*)malloc(sizeof(HistNode));
	if (cur == NULL) {
		printf("Memory Allocation Error \n");
		return;
	}
	histLen++; //increment history length 
	strcpy(cur->instruct, str); //store command 
	cur->next = NULL;
	if (histHead == NULL) {//no history yet
		histHead = cur;
		histTail = cur;
	}
	else {
		histTail->next = cur;
		histTail = cur;
	}
	return;

}
/* print history*/
void historyCmd(void) {
	HistNode* cur = histHead;
	int count = 1;
	
	if (histHead == NULL)// no history to print
		return;
	while (cur) {
		printf("%-3d  %s\n", count++, cur->instruct);
		cur = cur->next;
	}
	return;

}
/*free memory of history */
void freeHistory(void) {
	HistNode * cur, *next;
	histLen = 0;
	cur = histHead;
	while (cur) {
		next = cur->next;
		free(cur);
		cur = next;
	}
	histHead = NULL;
	histTail = NULL;
	return;
}

/* print filename */
int typeCmd(CmdTokens *tokens) {
	DIR *dirp = NULL;
	struct dirent *file;
	struct stat sbuf;
	FILE* fp = NULL;
	int isDir = 0;
	int found = 0;
	char ch;
	//create stream to read current dir
	if ((dirp = opendir(".")) == NULL) {
		perror("Cannot read the directory\n");
		return -1;
	}

	//go through each file or folder of current directory
	while ((file = readdir(dirp)) != NULL) {
		if (file->d_ino == 0) //exclude deleted files
			continue;
		if (!strcmp(file->d_name, tokens->strpar)) {//target file found
			stat(file->d_name, &sbuf); //get file status
			if (sbuf.st_mode & S_IFDIR) {//it is directory
				isDir = 1;
			}
			else {//correct file found
				isDir = 0;
				found = 1;
				break;
			}
		}
	}
	if (isDir) {
		printf("The File is Directory\n");
		return -1;
	}
	else if (found == 0) {
		printf("The File Does Not Exist\n");
		return -1;
	}
	fp = fopen(tokens->strpar, "r");//open file stream
	if (fp == NULL) {//file open error
		printf("Error: File Open Error\n");
		return -1;
	}
	while ((ch = fgetc(fp)) != EOF) {//get each char
		putchar(ch);//put char
	}
	
	fclose(fp);
	closedir(dirp); //close directory file stream
	return 0;
}