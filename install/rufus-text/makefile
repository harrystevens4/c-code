rufus-text : rufus-text.o
	gcc rufus-text.o -o rufus-text -lncursesw
rufus-text.o : ../../source-code/rufus-text.c
	gcc -c ../../source-code/rufus-text.c
install : rufus-text
	@./install
