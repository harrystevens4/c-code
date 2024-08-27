shutdown-menu : shutdown-menu.o
	gcc shutdown-menu.o -o shutdown-menu `pkg-config --cflags --libs gtk4` 
shutdown-menu.o : ../../source-code/shutdown-menu.c
	gcc -c ../../source-code/shutdown-menu.c `pkg-config --cflags --libs gtk4`
install : shutdown-menu
	@./install
