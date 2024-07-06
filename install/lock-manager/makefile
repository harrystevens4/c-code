all : lock-manager.o daemon-core.o args.o 
	echo "Compiling..."
	gcc lock-manager.o daemon-core.o args.o -o lock-manager
	echo "Installing..."
	cp lock-manager /usr/local/sbin
	echo "Installed!"
lock-manager.o : ../../source-code/args.h ../../source-code/daemon-core.h ../../source-code/daemon-core.c
	gcc -c ../../source-code/lock-manager.c
daemon-core.o : ../../source-code/daemon-core.h ../../source-code/daemon-core.c
	gcc -c ../../source-code/daemon-core.c
args.o : ../../source-code/args.h ../../source-code/args.c
	gcc -c ../../source-code/args.c
