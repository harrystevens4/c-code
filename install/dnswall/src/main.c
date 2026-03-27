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

struct dns_response {
	uint16_t transaction_id;
	struct sockaddr *dest_addr;
	socklen_t dest_addrlen;
};

#define MAX_TRANSACTION_IDS 50
//ring buffer
uint16_t recent_transaction_ids[MAX_TRANSACTION_IDS] = {0};
int recent_transaction_ids_head = 0;

int set_promisc_mode(int socket, const char *interface, int state);
const char *protocol_name_from_number(int protocol);
int dns_send_response(int send_fd, struct dns_response *response);
void add_recent_transaction_id(uint16_t id);
int is_transaction_id_recent(uint16_t id);

int main(int argc, char **argv){
	if (argc < 2){
		fprintf(stderr,"please specify an interface\n");
		return 1;
	}
	const char *interface = argv[1];
	//====== setup sockets ======
	int monitor_fd = socket(AF_PACKET,SOCK_DGRAM,htons(ETH_P_ALL));
	if (monitor_fd < 0){
		perror("socket");
		return 1;
	}
	int ipv4_send_fd = socket(AF_INET,SOCK_DGRAM,0);
	if (ipv4_send_fd < 0){
		perror("socket");
		close(monitor_fd);
		return 1;
	}
	int ipv6_send_fd = socket(AF_INET6,SOCK_DGRAM,0);
	if (ipv6_send_fd < 0){
		perror("socket");
		close(monitor_fd);
		close(ipv4_send_fd);
		return 1;
	}
	//====== enable promiscuous mode ======
	int result = set_promisc_mode(monitor_fd,interface,1);
	if (result != 0){
		close(ipv4_send_fd);
		close(ipv6_send_fd);
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
			close(ipv4_send_fd);
			close(ipv6_send_fd);
			return 1;
		}

		ip_version >>= 4;
		//====== receive full packet ======
		char *packet = malloc(packet_size);
		long result = recvfrom(monitor_fd,packet,packet_size,0,NULL,NULL);
		if (result < 0){
			perror("recvfrom");
			close(monitor_fd);
			close(ipv4_send_fd);
			close(ipv6_send_fd);
			free(packet);
			return 1;
		}
		//discard packet if not ip
		int protocol = ntohs(ll_addr.sll_protocol);
		if (protocol != ETH_P_IP && protocol != ETH_P_IPV6){
			free(packet);
			continue;
		}
		if (ip_version != 4 && ip_version != 6){
			fprintf(stderr,"received packet with invalid version.\n");
			free(packet);
			continue;
		}
		printf("\n[%ld octets, ipv%d]\n",packet_size,ip_version);
		//====== extract protocol specific information ======
		struct sockaddr_storage src_addr = {0};
		socklen_t src_addrlen = 0;
		struct sockaddr_storage dest_addr = {0};
		socklen_t dest_addrlen = 0;
		char *ip_payload = NULL;
		size_t ip_payload_size = 0;
		uint8_t transport_protocol = 0;
		//different for IPv4 and IPv6
		switch (protocol){
		case ETH_P_IP:
			//protocol
			transport_protocol = *(packet+9);
			printf("==> IPv4 carying protocol %s\n",protocol_name_from_number(transport_protocol));
			//sender and recipient
			memcpy(&((struct sockaddr_in *)&src_addr)->sin_addr,packet+12,sizeof(struct in_addr));
			src_addrlen = sizeof(struct sockaddr_in);
			memcpy(&((struct sockaddr_in *)&dest_addr)->sin_addr,packet+16,sizeof(struct in_addr));
			dest_addrlen = sizeof(struct sockaddr_in);
			src_addr.ss_family = AF_INET;
			dest_addr.ss_family = AF_INET;
			char addr_buffer_src[64] = {0};
			char addr_buffer_dest[64] = {0};
			printf("==> from [%s] to [%s]\n",
				inet_ntop(AF_INET,
					&((struct sockaddr_in *)&src_addr)->sin_addr,
					addr_buffer_src,sizeof(addr_buffer_src)),
				inet_ntop(AF_INET,
					&((struct sockaddr_in *)&dest_addr)->sin_addr,
					addr_buffer_dest,sizeof(addr_buffer_dest))
			);
			//packet length
			size_t header_length = *(uint8_t *)packet & 0x0f; //last 4 bits;
			header_length = (header_length*32)/8; //IHL specifies number of 32 bit words
			uint16_t total_length = ntohs(*(uint16_t *)(packet+2));
			ip_payload = packet+header_length;
			ip_payload_size = total_length-header_length;
			printf("==> IP packet payload size [%lu] octets\n",ip_payload_size);
			break;
		case ETH_P_IPV6:
			transport_protocol = *(packet+6);
			printf("==> IPv6 carying protocol %s\n",protocol_name_from_number(transport_protocol));
			//sender and recipient
			memcpy(&((struct sockaddr_in6 *)&src_addr)->sin6_addr,packet+12,sizeof(struct in6_addr));
			src_addrlen = sizeof(struct sockaddr_in6);
			memcpy(&((struct sockaddr_in6 *)&dest_addr)->sin6_addr,packet+16,sizeof(struct in6_addr));
			dest_addrlen = sizeof(struct sockaddr_in6);
			src_addr.ss_family = AF_INET6;
			dest_addr.ss_family = AF_INET6;
			printf("==> From [%s] To [%s]\n",
				inet_ntop(AF_INET6,
					&((struct sockaddr_in6 *)&src_addr)->sin6_addr,
					addr_buffer_src,sizeof(addr_buffer_src)),
				inet_ntop(AF_INET6,
					&((struct sockaddr_in6 *)&dest_addr)->sin6_addr,
					addr_buffer_dest,sizeof(addr_buffer_dest))
			);
			//packet length
			ip_payload = packet+40;
			ip_payload_size = ntohs(*(uint16_t *)(packet+4));
			printf("==> IP packet payload size [%lu] octets\n",ip_payload_size);
			break;
		}
		//====== process udp packets ======
		//ignore all non udp packets
		if (transport_protocol != IPPROTO_UDP){
			free(packet);
			continue;
		}
		//extract port numbers
		uint16_t src_port = ntohs(*(uint16_t *)ip_payload);
		uint16_t dest_port = ntohs(*(uint16_t *)(ip_payload+2));
		printf("==> Source port [%u]\n",src_port);
		printf("==> Destination port [%u]\n",dest_port);
		switch (src_addr.ss_family){
		case AF_INET:
			struct sockaddr_in *src_addr_in = (struct sockaddr_in *)&src_addr;
			struct sockaddr_in *dest_addr_in = (struct sockaddr_in *)&src_addr;
			src_addr_in->sin_port = htons(src_port);
			dest_addr_in->sin_port = htons(dest_port);
			break;
		case AF_INET6:
			struct sockaddr_in6 *src_addr_in6 = (struct sockaddr_in6 *)&src_addr;
			struct sockaddr_in6 *dest_addr_in6 = (struct sockaddr_in6 *)&src_addr;
			src_addr_in6->sin6_port = htons(src_port);
			dest_addr_in6->sin6_port = htons(dest_port);
			break;
		}
		char *udp_payload = ip_payload + 8;
		size_t udp_payload_size = ntohs(*(uint16_t *)(ip_payload+4)) - 8;
		printf("==> Payload size [%lu]\n",udp_payload_size);
		//====== process dns requests ======
		//discard all non port 53 traffic
		if (dest_port != 53){
			free(packet);
			continue;
		}
		//read dns headers
		struct dns_header *dns_header = (struct dns_header *)udp_payload;
		struct dns_response response = {0};
		//ignore responses we send
		if (is_transaction_id_recent(dns_header->transaction_id)){
			free(packet);
			continue;
		}
		//construct response
		printf("\033[32m<====> DNS packet detected <====>\033[39m\n");
		printf("==> Transaction id [%u]\n",ntohs(dns_header->transaction_id));
		response.transaction_id = dns_header->transaction_id;
		response.dest_addr = (struct sockaddr *)&src_addr;
		response.dest_addrlen = src_addrlen;
		int sending_socket = (dest_addr.ss_family == AF_INET) ? ipv4_send_fd : ipv6_send_fd;
		result = dns_send_response(sending_socket,&response);
		if (result < 0){
			fprintf(stderr,"Failed to send dns response\n");
		}
		//record that the request has been responded to
		add_recent_transaction_id(dns_header->transaction_id);
		char addr_string_buffer[64] = {0};
		switch (response.dest_addr->sa_family){
		case AF_INET:
			inet_ntop(AF_INET,&((struct sockaddr_in *)(response.dest_addr))->sin_addr,addr_string_buffer,sizeof(addr_string_buffer));
			break;
		case AF_INET6:
			inet_ntop(AF_INET6,&((struct sockaddr_in6 *)(response.dest_addr))->sin6_addr,addr_string_buffer,sizeof(addr_string_buffer));
			break;
		}
		printf("<== Sent response to [%s:%u], [%ld] bytes\n",addr_string_buffer,src_port,result);
		//====== cleanup ======
		free(packet);
	}
	return 0;
}

void add_recent_transaction_id(uint16_t id){
	recent_transaction_ids[recent_transaction_ids_head] = id;
	recent_transaction_ids_head++;
	recent_transaction_ids_head %= MAX_TRANSACTION_IDS;
}

int is_transaction_id_recent(uint16_t id){
	for (int i = 0; i < MAX_TRANSACTION_IDS; i++){
		if (recent_transaction_ids[i] == id) return 1;
	}
	return 0;
}

int dns_send_response(int send_fd, struct dns_response *response_info){
	//====== sending address ======
	struct sockaddr *dest_addr = response_info->dest_addr;
	socklen_t dest_addrlen = response_info->dest_addrlen;
	//====== send responses back ======
	struct dns_header response = {
		.transaction_id = response_info->transaction_id,
		.QR_OPCODE_AA_TC_RD = htobe16(0b1000010),
		.RA_Z_AD_CD_RCODE = htobe16(0b110110000),
		.question_count = 0,
		.answer_count = 0,
		.authority_rr_count = 0,
		.additional_rr_count = 0,
	};
	int result = sendto(send_fd,&response,sizeof(response),0,dest_addr,dest_addrlen);
	if (result < 0){
		perror("sendto");
		return -1;
	}
	return result;
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
