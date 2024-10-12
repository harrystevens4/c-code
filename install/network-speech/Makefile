network-speech : network-speech.o libspeechd.o
	gcc network-speech.o libspeechd.o `pkg-config --libs glib-2.0` -o network-speech
network-speech.o : ../../source-code/network-speech.c
	gcc -c ../../source-code/network-speech.c
libspeechd.o : ../../lib/external/speechd/libspeechd.c
	gcc `pkg-config --cflags glib-2.0` -c ../../lib/external/speechd/libspeechd.c
install : network-speech
	@./install
