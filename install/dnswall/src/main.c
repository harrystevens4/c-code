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
#include <time.h>

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
	struct sockaddr *src_addr;
	socklen_t src_addrlen;
	int interface;
	char destination_mac[8];
	int destination_mac_len;
};

struct __attribute__((packed)) udp_header {
	uint16_t source_port;
	uint16_t destination_port;
	uint16_t length;
	uint16_t checksum;
};

struct __attribute__((packed)) ipv4_header {
	uint8_t version_IHL;
	uint8_t DSCP_ECN;
	uint16_t total_length;
	uint16_t identification;
	uint16_t flags_fragment_offset;
	uint8_t time_to_live;
	uint8_t protocol;
	uint16_t header_checksum;
	uint32_t source_address;
	uint32_t destination_address;
};

struct ipv4_info {
	uint8_t protocol;
	struct in_addr src_addr;
	struct in_addr dest_addr;
	int interface;
	char destination_mac[8];
	int destination_mac_len;
};

struct udp_info {
	struct sockaddr *src_addr;
	struct sockaddr *dest_addr;
	int interface;
	char destination_mac[8];
	int destination_mac_len;
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
int udp_send(int raw_sock, void *data, size_t len, struct udp_info *udp_info);
int ipv4_send(int raw_sock, void *data, size_t len, struct ipv4_info *ipv4_info);

int main(int argc, char **argv){
	srandom(time(NULL));
	//====== arguments ======
	if (argc < 2){
		fprintf(stderr,"please specify an interface\n");
		return 1;
	}
	const char *interface = argv[1];
	int interface_index = if_nametoindex(interface);
	if (interface == 0){
		perror("if_nametoindex");
		return -1;
	}
	//====== setup sockets ======
	int raw_sock = socket(AF_PACKET,SOCK_DGRAM,htons(ETH_P_ALL));
	if (raw_sock < 0){
		perror("socket");
		return 1;
	}
	//enable broadcast
	const int one = 1;
	int result = setsockopt(raw_sock,SOL_SOCKET,SO_BROADCAST,&one,sizeof(one));
	if (result < 0){
		perror("setsockopt(SO_BROADCAST)");
		close(raw_sock);
		return 1;
	}
	//====== enable promiscuous mode ======
	result = set_promisc_mode(raw_sock,interface,1);
	if (result != 0){
		close(raw_sock);
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
		long int packet_size = recvfrom(raw_sock,&ip_version,sizeof(ip_version),MSG_PEEK | MSG_TRUNC,(struct sockaddr *)&ll_addr,&ll_addrlen);
		if (packet_size < 0){
			perror("recvfrom");
			close(raw_sock);
			return 1;
		}

		ip_version >>= 4;
		//====== receive full packet ======
		char *packet = malloc(packet_size);
		long result = recvfrom(raw_sock,packet,packet_size,0,NULL,NULL);
		if (result < 0){
			perror("recvfrom");
			close(raw_sock);
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
			struct sockaddr_in *dest_addr_in = (struct sockaddr_in *)&dest_addr;
			src_addr_in->sin_port = htons(src_port);
			dest_addr_in->sin_port = htons(dest_port);
			break;
		case AF_INET6:
			struct sockaddr_in6 *src_addr_in6 = (struct sockaddr_in6 *)&src_addr;
			struct sockaddr_in6 *dest_addr_in6 = (struct sockaddr_in6 *)&dest_addr;
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
			goto cleanup;
		}
		//read dns headers
		struct dns_header *dns_header = (struct dns_header *)udp_payload;
		struct dns_response response = {0};
		//ignore responses we send
		if (is_transaction_id_recent(dns_header->transaction_id)){
			goto cleanup;
		}
		//construct response
		printf("\033[32m<====> DNS packet detected <====>\033[39m\n");
		printf("==> Transaction id [%u]\n",ntohs(dns_header->transaction_id));
		response.transaction_id = dns_header->transaction_id;
		response.dest_addr = (struct sockaddr *)&src_addr;
		response.dest_addrlen = src_addrlen;
		response.src_addr = (struct sockaddr *)&dest_addr;
		response.src_addrlen = dest_addrlen;
		response.interface = interface_index;
		response.destination_mac_len = ll_addr.sll_halen;
		memcpy(&response.destination_mac,&ll_addr.sll_addr,ll_addr.sll_halen);
		result = dns_send_response(raw_sock,&response);
		if (result < 0){
			fprintf(stderr,"Failed to send dns response\n");
			goto cleanup;
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
		cleanup:
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

int dns_send_response(int raw_socket, struct dns_response *response_info){
	struct sockaddr *dest_addr = response_info->dest_addr;
	//struct sockaddr_in broadcast_addr = {
	//	.sin_family = AF_INET,
	//	.sin_addr = 0xffffffff,
	//	.sin_port = ((struct sockaddr_in *)response_info->dest_addr)->sin_port,
	//};
	//struct sockaddr *dest_addr = (struct sockaddr *)&broadcast_addr;
	struct sockaddr *src_addr = response_info->src_addr;
	//====== send responses back ======
	struct dns_header response = {
		.transaction_id = response_info->transaction_id,
		.QR_OPCODE_AA_TC_RD = 0b10000101,
		.RA_Z_AD_CD_RCODE = 0b10110000,
		.question_count = 0,
		.answer_count = 0,
		.authority_rr_count = 0,
		.additional_rr_count = 0,
	};
	struct udp_info udp_info = {
		.src_addr = response_info->src_addr,
		.dest_addr = response_info->dest_addr,
		.interface = response_info->interface,
		.destination_mac_len = response_info->destination_mac_len,
	};
	memcpy(&udp_info.destination_mac,&response_info->destination_mac,response_info->destination_mac_len);
	return udp_send(raw_socket,&response,sizeof(response),&udp_info);
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

uint16_t udpv4_calculate_checksum(void *data, size_t len, struct udp_info *udp_info){
	uint32_t total = 0;
	//====== get info for pseudo IPv4 header ======
	struct in_addr *src_addr = &((struct sockaddr_in *)udp_info->src_addr)->sin_addr;
	struct in_addr *dest_addr = &((struct sockaddr_in *)udp_info->dest_addr)->sin_addr;
	total +=  (*(uint32_t *)src_addr >> 16) & 0xffff;
	total +=  (*(uint32_t *)src_addr) & 0xffff;
	total +=  (*(uint32_t *)dest_addr >> 16) & 0xffff;
	total +=  (*(uint32_t *)dest_addr) & 0xffff;
	total += htons(IPPROTO_UDP);
	struct udp_header *udp_header = data;
	total += udp_header->length;
	//====== sum the rest of the packet ======
	for (int i = 0; i < len; i+=2){
		total += ((uint16_t *)data)[i/2];
	}
	if (len % 2 != 0){
		total += (uint16_t)((uint8_t *)data)[len-1] << 8;
	}
	total = (total >> 16) + (total & 0xffff); //add top 16 bits to lower 16 bits
	total += (total >> 16); //carry
	uint16_t checksum = ~(uint16_t)total;
	if (checksum == 0) checksum = 0xffff;
	return checksum;
}

int udp_send(int raw_sock, void *data, size_t len, struct udp_info *udp_info){
	//casting for convenience
	struct sockaddr_in *src_in = (struct sockaddr_in *)udp_info->src_addr;
	struct sockaddr_in *dest_in = (struct sockaddr_in *)udp_info->dest_addr;
	struct sockaddr_in *src_in6 = (struct sockaddr_in *)udp_info->src_addr;
	struct sockaddr_in *dest_in6 = (struct sockaddr_in *)udp_info->dest_addr;
	//====== construct udp packet ======
	size_t packet_len = sizeof(struct udp_header)+len;
	struct __attribute__((packed)) {
		struct udp_header header;
		char data[];
	} *packet = malloc(packet_len);
	//fill in details
	switch (udp_info->src_addr->sa_family){
	case AF_INET:
		//header
		packet->header.source_port = src_in->sin_port;
		packet->header.destination_port = dest_in->sin_port;
		packet->header.length = htons(packet_len);
		//copy in the data
		memcpy(&packet->data,data,len);
		//checksum
		packet->header.checksum = 0;
		packet->header.checksum = udpv4_calculate_checksum(packet,packet_len,udp_info);
		//====== send the ipv4 packet ======
		struct ipv4_info ipv4_info = {
			.src_addr = src_in->sin_addr,
			.dest_addr = dest_in->sin_addr,
			.protocol = IPPROTO_UDP,
			.interface = udp_info->interface,
			.destination_mac_len = udp_info->destination_mac_len,
		};
		memcpy(&ipv4_info.destination_mac,&udp_info->destination_mac,udp_info->destination_mac_len);
		int result = ipv4_send(raw_sock,packet,packet_len,&ipv4_info);
		//cleanup
		free(packet);
		return result;
	}
	free(packet);
	fprintf(stderr,"protocol not supported");
	return -1;

}

int ipv4_send(int raw_sock, void *data, size_t len, struct ipv4_info *ipv4_info){
	//====== prep packet ======
	//we shall ignore options
	size_t packet_len = len + sizeof(struct ipv4_header);
	struct __attribute__((packed)) {
		struct ipv4_header header;
		char data[];
	} *packet = malloc(packet_len);
	memset(packet,0,packet_len);
	//====== fill in details ======
	//header
	packet->header.version_IHL = (4 << 4) | 5; //no options so IHL is always 5
	packet->header.DSCP_ECN = 0;
	packet->header.total_length = htons(packet_len);
	packet->header.identification = random();
	packet->header.flags_fragment_offset = htons(1 << 14);
	packet->header.time_to_live = 5;
	packet->header.protocol = ipv4_info->protocol;
	memcpy(&packet->header.source_address,&ipv4_info->src_addr,sizeof(ipv4_info->src_addr));
	memcpy(&packet->header.destination_address,&ipv4_info->dest_addr,sizeof(ipv4_info->dest_addr));
	//data
	memcpy(&packet->data,data,len);
	//calculate checksum
	uint32_t total = 0;
	for (int i = 0; i < 10; i++){
		total += ((uint16_t *)packet)[i];
	}
	total = (total >> 16) + (total & 0xffff); //add top 16 bits to lower 16 bits
	total += (total >> 16); //carry
	packet->header.header_checksum = ~total;
	//====== send the packet ======
	struct sockaddr_ll addr = {
		.sll_family = AF_PACKET,
		.sll_protocol = htons(ETH_P_IP),
		.sll_ifindex = ipv4_info->interface,
		.sll_halen = ipv4_info->destination_mac_len,
	};
	memcpy(&addr.sll_addr,&ipv4_info->destination_mac,ipv4_info->destination_mac_len);
	long result = sendto(raw_sock,packet,packet_len,0,(struct sockaddr *)&addr,sizeof(addr));
	if (result < 0){
		perror("sendto");
	}
	//cleanup
	free(packet);
	return result;
}
