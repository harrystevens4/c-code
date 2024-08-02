mail-manager : mail-manager.o args.o daemon-toolkit.o
	gcc mail-manager.o args.o daemon-toolkit.o -o mail-manager
daemon-toolkit.o : ../../source-code/daemon-toolkit.c ../../source-code/daemon-toolkit.h
	gcc -c ../../source-code/daemon-toolkit.c
args.o : ../../source-code/args.c ../../source-code/args.h
	gcc -c ../../source-code/args.c
mail-manager.o : ../../source-code/mail-manager.c
	gcc -c ../../source-code/mail-manager.c
view-mail : view-mail.o daemon-toolkit.o
	gcc view-mail.o daemon-toolkit.o -lncursesw -o view-mail
view-mail.o : ../../source-code/view-mail.c
	gcc -c ../../source-code/view-mail.c
install : mail-manager view-mail
	@./install
