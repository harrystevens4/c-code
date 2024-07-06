fancy-notify : fancy-notify.o args.o daemon-core.o
	gcc fancy-notify.o args.o daemon-core.o -o fancy-notify
	@echo "Complete!"
fancy-notify.o : ../../source-code/fancy-notify.c ../../source-code/daemon-core.h ../../source-code/args.h
	gcc -c ../../source-code/fancy-notify.c
daemon-core.o : ../../source-code/daemon-core.c ../../source-code/daemon-core.h
	gcc -c ../../source-code/daemon-core.c
args.o : ../../source-code/args.c ../../source-code/args.h
	gcc -c ../../source-code/args.c
clean :
	@echo "Cleaning..."
	@rm fancy-notify.o
	@rm args.o
	@rm daemon-core.o
	@echo "Finnished cleaning"
install : fancy-notify ../../source-code/simple-notification.py
	cp ../../source-code/simple-notification.py /usr/local/bin
	cp fancy-notify /usr/local/bin
	@if [ "$EUID" == "0" ]; then echo "installed!"; else echo "Run as root"; fi
