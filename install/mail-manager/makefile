mail-manager : mail-manager.o args.o daemon-toolkit.o
	gcc mail-manager.o args.o daemon-toolkit.o -o mail-manager
daemon-toolkit.o : ../../source-code/daemon-toolkit.c ../../source-code/daemon-toolkit.h
	gcc -c ../../source-code/daemon-toolkit.c
args.o : ../../source-code/args.c ../../source-code/args.h
	gcc -c ../../source-code/args.c
mail-manager.o : ../../source-code/mail-manager.c
	gcc -c ../../source-code/mail-manager.c
install : mail-manager
	@./install
