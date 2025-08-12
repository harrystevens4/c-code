#include <sys/time.h>
#include <getopt.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <netdb.h>

struct __attribute__((packed)) ntp_timestamp{
	uint32_t secs;
	uint32_t fractional_secs;
};
struct __attribute__((packed)) ntp_header {
	uint8_t li_vn_mode;
	uint8_t stratum;
	uint8_t poll;
	uint8_t precision;
	uint32_t root_delay;
	uint32_t root_dispersion;
	uint32_t reference_id;
	uint64_t reference_timestamp;
	uint64_t origin_timstamp;
	uint64_t receive_timestamp;
	uint64_t transmit_timestamp;
};

static uint32_t nsecs_from_fractional_secs(uint32_t fractional_secs){
	uint32_t total = 0;
	for (int i = 0; i < 32; i++){
		uint8_t bit = (fractional_secs >> (31-i)) & 1;
		if (bit) total += 1000000/(2<<i);
	}
	return total;
}
int get_utc(char *timeserver,struct timeval *tv){
	int result = 0;
	//get address info
	struct addrinfo hints = {
		.ai_family = AF_UNSPEC,
		.ai_socktype = SOCK_DGRAM,
	};
	struct addrinfo *address_info;
	result = getaddrinfo(timeserver,"123",&hints,&address_info);
	if (result < 0){
		fprintf(stderr,"getaddrinfo: %s\n",gai_strerror(result));
		return -1;
	};
	//open socket
	int sock = socket(AF_INET,SOCK_DGRAM,0);
	if (sock < 0){perror("socket"); goto end;}
	struct timeval timeout = {
		.tv_sec = 1,
		.tv_usec = 0,
	};
	if (setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(struct timeval)) < 0){
		perror("setsockopt");
		result = -1;
		goto end;
	};
	//send ntp request
	struct ntp_header header = {
		.li_vn_mode = 0x23, //leap indicator 0, version number 4, mode client (3)
	};
	result = sendto(sock,&header,sizeof(struct ntp_header),0,address_info->ai_addr,address_info->ai_addrlen);
	if (result < (long long int)sizeof(struct ntp_header)){
		fprintf(stderr,"sendto timeout\n");
		result = -1;
		goto end;
	}
	//read ntp packet
	result = recvfrom(sock,&header,sizeof(struct ntp_header),0,NULL,NULL);
	if (result < (long long int)sizeof(struct ntp_header)){
		fprintf(stderr,"recvfrom timeout\n");
		result = -1;
		goto end;
	}
	struct ntp_timestamp timestamp;
	//reinterpret cast
	//header.transmit_timestamp = be64toh(header.transmit_timestamp);
	timestamp = *(struct ntp_timestamp *)&header.transmit_timestamp;
	//fill in return struct
	tv->tv_sec = ntohl(timestamp.secs)-2208988800;
	tv->tv_usec = nsecs_from_fractional_secs(ntohl(timestamp.fractional_secs));
	//cleaning
	end:
	close(sock);
	freeaddrinfo(address_info);
	return result;
}
static void print_help(){
	printf("usage:\n");
	printf("	ntp [options]\n");
	printf("options:\n");
	printf("	-h, --help : display this text\n");
	printf("	-t, --timestamp : print unix timestamp\n");
	printf("	-c, --ctime : print in ctime format (default)\n");
	printf("	-f <format string>, --format=<format string> : print with custom format string\n");
	printf("	-s <timeserver>, --server=<timeserver> : use selected timeserver");
}
int main(int argc, char **argv){
	//defaults
	char *time_server = "pool.ntp.org";
	struct timeval tv;
	//====== get command line options ======
	const struct option long_options[] = {
		{"help",no_argument,0,'h'},
		{"server",required_argument,0,'s'},
		{"format",required_argument,0,'f'},
		{"timestamp",no_argument,0,'t'},
	};
	for (;;){
		int index;
		int result = getopt_long(argc,argv,"htc:f:s:",long_options,&index);
		if (result == -1) break;

		//process arguments
		switch (result){
			case 'h':
			print_help();
			return 0;
			
			case 's':
			time_server = optarg;
			break;

			case 'f':
			if (get_utc(time_server,&tv) < 0) return 1;
			char buffer[1024] = {0}; //probably big enough
			strftime(buffer,sizeof(buffer),optarg,localtime(&tv.tv_sec));
			printf("%s\n",buffer);
			return 0;

			case 't':
			if (get_utc(time_server,&tv) < 0) return 1;
			printf("%llu\n",(long long unsigned int)tv.tv_sec);
			return 0;

			case 'c':
			break; //ctime is the default option
			
			default:
			print_help();
			return 1;
		}
	}
	//====== default action ======
	if (get_utc(time_server,&tv) < 0) return 1;
	//ctime provides \n
	printf("%s reports it is now %s",time_server,ctime(&tv.tv_sec));
	return 0;
}
