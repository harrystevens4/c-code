all : systemd-controller
	cp systemd-controller /usr/local/sbin
systemd-controller : systemd-controller.o
	gcc systemd-controller.o -o systemd-controller -lncursesw
systemd-controller.o : ../../source-code/systemd-controller.c 
	gcc ../../source-code/systemd-controller.c -c -lncursesw
clean :
	if [ -f systemd-controller.o ]; then rm systemd-controller.o; fi
	if [ -f systemd-controller ]; then rm systemd-controller; fi
