battery-display : battery-display.o
	gcc -o battery-display `pkg-config --cflags --libs gtk4` battery-display.o
battery-display.o : ../../source-code/battery-display.c
	gcc -c `pkg-config --cflags --libs gtk4` ../../source-code/battery-display.c
