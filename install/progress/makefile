progress : progress.o shell-toolkit.o args.o
	gcc progress.o shell-toolkit.o args.o -o progress
progress.o : ../../source-code/progress.c ../../source-code/shell-toolkit.h 
	gcc -c ../../source-code/progress.c
shell-toolkit.o : ../../source-code/shell-toolkit.h ../../source-code/shell-toolkit.c
	gcc -c ../../source-code/shell-toolkit.c
args.o : ../../source-code/args.c ../../source-code/args.h
	gcc -c ../../source-code/args.c
install : progress
	@./install | ./progress

