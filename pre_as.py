# GAS preprocessor
# This is intended to rewrite MingWin's cc1plus output for as to optimize (mostly) for
# time.  Most of these optimizations also optimize for space.
# Kenneth Boyd, zaimoni@zaimoni.com

# The general strategy is to read in the entire *.s file, then rewrite it, then write out
# the file as a *.z file.  We assume that all functions end with a ret ASM command.

import sys

def ASPreprocess():
	"ASPreprocess(): preprocesses MingWin cc1plus output for as"
	if 2<=len(sys.argv):
# read in line from processed
		TargetFile = open(sys.argv[1],'rb')
		LineList = TargetFile.readlines()
		TargetFile.close()

# blot .p2align directives: Win__ doesn't care
		Idx = len(LineList)
		while 0<Idx:
			Idx = Idx-1
			if LineList[Idx][:9]=="\t.p2align":
				LineList[Idx:Idx+2] = LineList[Idx+1:Idx+2]

# locate functions, then preprocess each function:
# find ret, then find corresponding .globl, then add 3
		Idx = len(LineList)
		while 0<Idx:
			Idx = Idx-1
			if LineList[Idx][:]=="\tret\r\n":
				Idx2 = Idx
				while 0<Idx2:
					Idx2 = Idx2-1
					if LineList[Idx2][:6]==".globl":
						Idx2 = Idx2+3
#						TODO: rest of preprocessing: last instruction Idx, first instruction Idx2
###########	safe
#	movl %ebp,%esp
#	popl %ebp
# to
#	leave

						Idx3 = Idx-2
						while Idx2<Idx3:
							if LineList[Idx3:Idx3+2]==["\tmovl %ebp,%esp\r\n" , "\tpopl %ebp\r\n"]:
								LineList[Idx3:Idx3+2] = ["\tleave\r\n"]
								Idx = Idx-1
							Idx3 = Idx3-1
###########
#	addl $16,%esp
#	addl $-12,%esp
# to
#   addl $4,%esp
						Idx3 = Idx-2
						while Idx2<Idx3:
							if 		LineList[Idx3][:7]!="\taddl $":
								Idx3 = Idx3-2
							elif LineList[Idx3+1][:7]=="\taddl $" and LineList[Idx3][-7:]==LineList[Idx3+1][-7:] and LineList[Idx3][-7:-5]==",%":
								NewAddend = int(LineList[Idx3][7:-7])+int(LineList[Idx3+1][7:-7])
								if 0==NewAddend:
									LineList[Idx3:Idx3+3]=LineList[Idx3+2:Idx3+3]
									Idx = Idx-2
								else:
									LineList[Idx3:Idx3+2]=["\taddl $"+str(NewAddend)+LineList[Idx3][-7:]]
									Idx = Idx-1
							else:
								Idx3 = Idx3-1

###########
#	call _memmove
## C/C++: needs 3 arguments pushed onto the stack.
##  look for
#	movl __, %___
#	push %___			where %___ is %eax, %ecx, or %edx [these are not preserved]
##  If %___ is not used before call _memmove, it is safe to rewrite
#	movl __, %___
#	push %___
##	as
#	pushl __
##	stop the backtracking for: labels, jump instructions, 3 pushl, instructions manipulating %esp
#						Idx3 = Idx-2
#						while Idx2<Idx3:
#							if LineList[Idx3][:14]=="\tcall _memmove":
#								Idx4 = Idx3-1
#								PushCount = 0
#								while 3>PushCount and LineList[Idx4][:1]=="\t" and LineList[Idx4][1:2]!="j" and LineList[Idx4][1:5]!="call" and Idx2<Idx4:
#									if LineList[Idx4][1:7]=="pushl ":
#										PushCount = PushCount-1
## this if-condition is ugly.  See if this is a good use for Python in
#										if (LineList[Idx4][-6:-2]=="%eax" or LineList[Idx3][-6:-2]=="%ecx" or LineList[Idx3][-6:-2]=="%edx")	\
#											and LineList[Idx4-1][:6]=="\tmovl " and LineList[Idx4-1][-6:-2]==LineList[Idx4][-6:-2] and LineList[Idx4-1][-7:-6]==",":
## This should check for reuse of the target before call _memmove...but WHY should memmove be called
## with src==dest, src==size, or dest==size?
#											LineList[Idx4-1:Idx4+1] = ["\tpushl "+LineList[Idx4-1][6:-7]+"\r\n"]
#									Idx4 = Idx4-1																						
#								Idx3 = Idx4
#							else:
#								Idx3 = Idx3-1

#####
#	call _ReleaseMutex@4 (only memory.s need apply)
## C/C++: needs 1 argument pushed onto the stack.
#	movl _RAMBlock,%eax
#	addl $-12,%esp
#	pushl %eax
#	call _ReleaseMutex@4
## as
#	addl $-12,%esp
#	pushl _RAMBlock
#	call _ReleaseMutex@4
#						Idx3 = Idx-2
#						while Idx2<Idx3:
#							if LineList[Idx3][:21]=="\tcall _ReleaseMutex@4" and LineList[Idx3-1][:11]=="\tpushl %eax" and LineList[Idx3-2][:15]=="\taddl $-12,%esp" and LineList[Idx3-3][:6]=="\tmovl " and LineList[Idx3-3][-6:-2]=="%eax":
#								LineList[Idx3-3:Idx3] = ["\taddl $-12,%esp\r\n", "\tpushl "+LineList[Idx3-3][6:-7]+"\r\n"]
#							else:
#								Idx3 = Idx3-1

######
#	tactical optimization: interferes with GNU scheduler
#	this one works better as a sweep-forward
##
#	movl (%edx),%eax
#	pushl %eax
#	movl 4(%edx),%eax
## as
#	pushl (%edx)
#	movl 4(%edx),%eax
#						Idx3 = Idx2+1
#						while Idx-2>Idx3:
#							if LineList[Idx3][:4]=="\tmov" and LineList[Idx3][-7:-2]==",%eax" and LineList[Idx3-1][:-2]=="\tpushl %eax" and LineList[Idx3-2][:6]=="\tmovl " and LineList[Idx3-2][-7:-2]==",%eax":
#								LineList[Idx3-2:Idx3] = ["\tpushl "+LineList[Idx3-2][6:-7]+"\r\n"]
#							else:
#								Idx3 = Idx3+1

						Idx = Idx2-4
						Idx2 = 0


# write LineList to TmpFile, then close
		TargetFile = open(sys.argv[1],'wb')
		TargetFile.writelines(LineList)
		TargetFile.close()

if __name__ == '__main__':
	ASPreprocess()
