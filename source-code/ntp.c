#include <sys/time.h>
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
	//send ntp request
	struct ntp_header header = {
		.li_vn_mode = 0x23, //leap indicator 0, version number 4, mode client (3)
	};
	result = sendto(sock,&header,sizeof(struct ntp_header),0,address_info->ai_addr,address_info->ai_addrlen);
	//read ntp packet
	result = recvfrom(sock,&header,sizeof(struct ntp_header),0,NULL,NULL);
	if (result < 0){perror("result"); goto end;}
	struct ntp_timestamp timestamp;
	//reinterpret cast
	//header.transmit_timestamp = be64toh(header.transmit_timestamp);
	timestamp = *(struct ntp_timestamp *)&header.transmit_timestamp;
	//fill in return struct
	tv->tv_sec = ntohl(timestamp.secs)-2208988800;
	tv->tv_usec = 0;
	//cleaning
	end:
	close(sock);
	freeaddrinfo(address_info);
	return result;
}
int main(int argc, char **argv){
	struct timeval tv;
	if (get_utc("0.uk.pool.ntp.org",&tv) < 0) return 1;
	printf("The time now is %s\n",ctime(&tv.tv_sec));
	return 0;
}
