CC=gcc
CFLAGS=-Wall
OBJS=20170001.o shell.o memory.o opcode.o assembler.o loader.o execute.o
TARGET=20170001.out

$(TARGET) : $(OBJS)
	$(CC) -o $(TARGET) $(OBJS)
.c .o : 
	$(CC) $(CFLAGS) -c $< -o $@
clean:
	rm -rf $(OBJS) $(TARGET)

$(OBJS) : 20170001.h
20170001.o shell.o : shell.h
20170001.o memory.o : memory.h
20170001.o opcode.o shell.o assembler.o : opcode.h
20170001.o assembler.o : assembler.h
20170001.o loader.o execute.o : loader.h
20170001.o loader.c execute.o : execute.h