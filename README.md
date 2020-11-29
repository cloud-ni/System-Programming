# CSE4100 System Programming Project #
  - Implemented Shell, Assembler, Linking Loader for SIC/XE Machine

## Execution

	To execute the program, type the command below.
	$ make
	$ ./20170001.out

	To clean up the directory after the execution, type the command below.
	$ make clean

## Commands

	sicsim> h[elp]                      -> show valid commands
	sicsim> d[ir]                       -> print files in current directory
	sicsim> q[uit]                      -> terminate sicsim
	sicsim> hi[story]                   -> show command history
	sicsim> du[mp][start, end]          -> print memory
	sicsim> e[dit] address, value       -> edit memory of address with value
	sicsim> f[ill] start, end, value    -> fill memory [start, end] with value 
	sicsim> reset                       -> reset memory
	sicsim> opcode mnemonic             -> print opcode of the mnemonic
	sicsim> opcodelist                  -> print list of opcodes
	sicsim> assemble filename           -> assemble file
	sicsim> type filename               -> print file in console
	sicsim> symbol                      -> print symbol table
	sicsim> progaddr address            -> set starting address to run or load a program
	sicsim> loader [filename1] [filename2] [filename3]  -> link and load a program
	sicsim> bp [address]                -> set breakpoint
	sicsim> bp                          -> print breakpoint
	sicsim> bp [clear]                  -> clear breakpoint

	*The parameters should be hex numbers. 
	*mnemonic and filename are string inputs

## Created by

	강서현 20170001
