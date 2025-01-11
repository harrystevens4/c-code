#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>

#define CHECK_ERR(val,func)\
do{\
	if (val < 0){\
		fprintf(stderr,"%s() - ",__FUNCTION__);\
		perror(func);\
		return val;\
	}\
}while(0)

int get_baud(int baud);
int open_tty(const char *filename);
int close_tty(int fd);
int setup_tty(int fd,int baud);

int main(char *argv, int argc){
	char *filename = "/dev/tty2";
	int tty_fd = open_tty(filename);
	if (tty_fd < 0){
		fprintf(stderr,"Couldnt open terminal device.\n");
		return 1;
	}
	int result = setup_tty(tty_fd,baud);

	//open stdin and stdout in non blocking mode
	
	for (;;){
		//read from stdin
		//write to tty

		//read from tty
		//write to stdout
	}
	close_tty(tty_fd);
}
int open_tty(const char *filename){
	int tty_fd = open(filename,O_RDWR);
	CHECK_ERR(tty_fd,"open");
	return tty_fd;
}
int close_tty(int fd){
	int result = close(fd);
	CHECK_ERR(result,"close");
}
int setup_tty(int fd,int baud){
	speed_t speed = get_baud(baud); //lookup table
	if (speed < 0){
		fprintf(stderr,"Invalaid baud rate\n")
		return -1;
	}

	struct termios options;
	int result = tcgetattr(fd,&options);
	CHECK_ERR(result,"tcgetattr");
	
	//set baud rate
	cfsetispeed(&options,speed);
	cfsetospeed(&options,speed);
	
	//apply changes
	result = tcsetattr(fd,0,&options);
	CHECK_ERR(result,"tcsetattr");
}
int get_baud(int baud) //https://stackoverflow.com/questions/47311500/how-to-efficiently-convert-baudrate-from-int-to-speed-t
{
    switch (baud) {
    case 9600:
        return B9600;
    case 19200:
        return B19200;
    case 38400:
        return B38400;
    case 57600:
        return B57600;
    case 115200:
        return B115200;
    case 230400:
        return B230400;
    case 460800:
        return B460800;
    case 500000:
        return B500000;
    case 576000:
        return B576000;
    case 921600:
        return B921600;
    case 1000000:
        return B1000000;
    case 1152000:
        return B1152000;
    case 1500000:
        return B1500000;
    case 2000000:
        return B2000000;
    case 2500000:
        return B2500000;
    case 3000000:
        return B3000000;
    case 3500000:
        return B3500000;
    case 4000000:
        return B4000000;
    default:
        return -1;
    }
}
