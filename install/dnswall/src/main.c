#include <unistd.h>
#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <linux/in.h>

struct __attribute__((packed)) dns_header {
	uint16_t transaction_id;
	uint8_t QR_OPCODE_AA_TC_RD;
	uint8_t RA_Z_AD_CD_RCODE;
	uint16_t question_count;
	uint16_t answer_count;
	uint16_t authority_rr_count;
	uint16_t additional_rr_count;
};

int set_promisc_mode(int socket, const char *interface, int state);
const char *protocol_name_from_number(int protocol);

int main(int argc, char **argv){
	if (argc < 2){
		fprintf(stderr,"please specify an interface\n");
		return 1;
	}
	const char *interface = argv[1];
	//====== setup socket ======
	int monitor_fd = socket(AF_PACKET,SOCK_DGRAM,htons(ETH_P_ALL));
	if (monitor_fd < 0){
		perror("socket");
		return 1;
	}
	//====== enable promiscuous mode ======
	int result = set_promisc_mode(monitor_fd,interface,1);
	if (result != 0){
		return 1;
	}
	//====== listen for incomming packets ======
	printf("listening...\n");
	while (1){
		//====== read the ip header version ======
		struct sockaddr_ll ll_addr = {0};
		socklen_t ll_addrlen = sizeof(ll_addr);
		//fetch version
		uint8_t ip_version = 0;
		//result will be actual size of packet
		long int packet_size = recvfrom(monitor_fd,&ip_version,sizeof(ip_version),MSG_PEEK | MSG_TRUNC,(struct sockaddr *)&ll_addr,&ll_addrlen);
		if (packet_size < 0){
			perror("recvfrom");
			close(monitor_fd);
			return 1;
		}

		ip_version >>= 4;
		//====== receive full packet ======
		char *packet = malloc(packet_size);
		long result = recvfrom(monitor_fd,packet,packet_size,0,NULL,NULL);
		if (result < 0){
			perror("recvfrom");
			close(monitor_fd);
			free(packet);
			return 1;
		}
		//discard packet if not ip
		int protocol = ntohs(ll_addr.sll_protocol);
		if (protocol != ETH_P_IP && protocol != ETH_P_IPV6){
			free(packet);
			continue;
		}
		printf("protocol: %d\n",ntohs(ll_addr.sll_protocol));
		if (ip_version != 4 && ip_version != 6){
			fprintf(stderr,"received packet with invalid version.\n");
			free(packet);
			continue;
		}
		printf("new packet of size %ld received\n",packet_size);
		printf("version: ipv%d\n",ip_version);
		//====== filter for udp ======
		switch (protocol){
		case ETH_P_IP:
			uint8_t protocol = *(packet+9);
			printf("==> IPv4 carying protocol %s\n",protocol_name_from_number(protocol));
			break;
		}
		//====== cleanup ======
		free(packet);
	}
	return 0;
}

int dns_send_response(void){
	//====== sending address ======
	struct sockaddr_in broadcast_address = {
		.sin_family = AF_INET,
		.sin_port = htons(53),
		.sin_addr = htonl(0xffffffff),
	};
	int send_fd = 0;
	//====== send responses back ======
	struct dns_header response = {
		.transaction_id = 0,
		.QR_OPCODE_AA_TC_RD = htobe16(0b1000010),
		.RA_Z_AD_CD_RCODE = htobe16(0b110110000),
		.question_count = 0,
		.answer_count = 0,
		.authority_rr_count = 0,
		.additional_rr_count = 0,
	};
	int result = sendto(send_fd,&response,sizeof(response),0,(struct sockaddr *)&broadcast_address,sizeof(broadcast_address));
	if (result < 0){
		perror("send");
		close(send_fd);
		return 1;
	}
}

int set_promisc_mode(int socket, const char *interface, int state){
	//====== get interface info ======
	int interface_index = if_nametoindex(interface);
	if (interface == 0){
		perror("if_nametoindex");
		return -1;
	}
	//====== configure group we want to join ======
	struct packet_mreq mreq = {0};
	mreq.mr_ifindex = interface_index;
	mreq.mr_type = PACKET_MR_PROMISC;
	//====== join promisc group ======
	int action = (state) ? PACKET_ADD_MEMBERSHIP : PACKET_DROP_MEMBERSHIP;
	int result = setsockopt(socket, SOL_PACKET, action, &mreq, sizeof(mreq));
	if (result != 0){
		perror("setsockopt");
		return -1;
	}
	return 0;
}

const char *protocol_name_from_number(int protocol){
	switch (protocol){
		case 0: return "IP";
		case 1: return "ICMP";
		case 2: return "IGMP";
		case 4: return "IPIP";
		case 6: return "TCP";
		case 8: return "EGP";
		case 12: return "PUP";
		case 17: return "UDP";
		case 22: return "IDP";
		case 29: return "TP";
		case 33: return "DCCP";
		case 41: return "IPV6";
		case 46: return "RSVP";
		case 47: return "GRE";
		case 50: return "ESP";
		case 51: return "AH";
		case 92: return "MTP";
		case 94: return "BEETPH";
		case 98: return "ENCAP";
		case 104: return "PIM";
		case 108: return "COMP";
		case 115: return "L2TP";
		case 132: return "SCTP";
		case 136: return "UDPLITE";
		case 137: return "MPLS";
		case 143: return "ETHERNET";
		case 144: return "AGGFRAG";
		case 255: return "RAW";
		case 256: return "SMC";
		case 262: return "MPTCP";
		default: return "N/A";
	}
}
