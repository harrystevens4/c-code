#include <stdio.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <ctype.h>
#include <sys/stat.h>
#include <string.h>
#include <pwd.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <netdb.h>
#include <errno.h>

#define PERROR(func) (fprintf(stderr,"[%s:%d] %s: %s\n",__FUNCTION__,__LINE__,func,strerror(errno)))

//======================== structs ========================
struct dns_question {
	int type;
	int class;
	char **labels;
	size_t label_count;
};
struct dns_request {
	int question_count;
	int answer_count;
	int authority_rr_count;
	int additional_rr_count;
	struct dns_question *questions;
};
struct domain_config {
	char *name;
	int block;
	char *redirect;
};
struct global_config {
	size_t domain_configs_count;
	struct domain_config *domain_configs;
};
struct __attribute__((packed)) dns_header {
	uint16_t transaction_id;
	uint8_t QR_OPCODE_AA_TC_RD;
	uint8_t RA_Z_AD_CD_RCODE;
	uint16_t question_count;
	uint16_t answer_count;
	uint16_t authority_rr_count;
	uint16_t additional_rr_count;
};

//======================== globals ========================
//default config used if none can be loaded
struct global_config config = {
	.domain_configs_count = 0,
	.domain_configs = NULL,
};

//======================== prototypes ========================
int parse_config(char *file,struct global_config *config);
char *rtrim(char *input,char delim);
int (*_getaddrinfo)(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);
char **read_file_lines(FILE *file,size_t *line_count);
size_t split_into(char *input, char ***ret_array, size_t pointer_count, char delim);
void remove_after(char *line, char delim);
char *trim_whitespace(char *input);
int read_labels(int sockfd, char ***labels_p, size_t *label_count_p, size_t offset);
int read_request(int sockfd, struct dns_request *request);
void free_request(struct dns_request *request);
long int recvfrom_index(int sockfd, void *buffer, size_t len, size_t offset);
char *dup_or_null(char *str);
void free_config(struct global_config *config);
int string_to_bool(char *str);
char *bool_to_string(int b);

//======================= definitions ========================
int main(int argc, char **argv){
	int return_status = 0;
	//====== load config files ======
	//try the global config
	parse_config("/etc/dnsmonitor/dnsmonitor.conf",&config);
	//if no config is loaded a default empty config is used
	printf("configuration:\n");
	for (size_t i = 0; i < config.domain_configs_count; i++){
		printf("(%s) block: %s, redirect: %s\n",config.domain_configs[i].name,bool_to_string(config.domain_configs[i].block),config.domain_configs[i].redirect);
	}
	//====== prepare to serve ======
	//get local ip
	struct ifaddrs *ifaddrs;
	int result = getifaddrs(&ifaddrs);
	if (result < 0){
		PERROR("getifaddrs");
		free_config(&config);
		return 1;
	}
	struct sockaddr addr;
	socklen_t addrlen = 0;
	//open a socket
	int sockfd = socket(AF_INET,SOCK_DGRAM,0);
	if (sockfd < 0){
		PERROR("socket");
		free_config(&config);
		return 1;
	}
	//find address we can bind to
	for (struct ifaddrs *next = ifaddrs; next != NULL; next = next->ifa_next){
		int family = next->ifa_addr->sa_family;
		if (family == AF_INET){
			memcpy(&addr,next->ifa_addr,sizeof(struct sockaddr));
			addrlen = sizeof(struct sockaddr_in);
			//set port
			((struct sockaddr_in *)&addr)->sin_port = htons(53);
			//print
			char addrbuf[INET_ADDRSTRLEN];
			if (inet_ntop(AF_INET,&((struct sockaddr_in *)&addr)->sin_addr,addrbuf,sizeof(addrbuf)) == NULL){
				PERROR("inet_ntop");
				free_config(&config);
				close(sockfd);
				return 1;
			}
			printf("interface: %s, address: %s\n",next->ifa_name,addrbuf);
			//try to bind
			result = bind(sockfd,&addr,addrlen);
			if (result != 0){
				PERROR("bind");
				continue;
			}
			break;
		}
	}
	if (addrlen == 0){
		fprintf(stderr,"Could not find non loopback address to bind to");
		free_config(&config);
		close(sockfd);
		return 1;
	}
	freeifaddrs(ifaddrs);
	//====== serve ======
	for (;;){
		struct sockaddr client_addr;
		socklen_t client_addrlen = sizeof(struct sockaddr);
		struct dns_header header;
		//====== peek at the header ======
		result = recvfrom(sockfd,&header,sizeof(header),MSG_PEEK,&client_addr,&client_addrlen);
		if (result < 0){
			PERROR("recvfrom");
			free_config(&config);
			close(sockfd);
			return 1;
		}
		struct dns_request request;
		size_t dns_msg_size = read_request(sockfd,&request);
		break;
	}
	//cleanup
	free_config(&config);
	close(sockfd);
	return return_status;
}
int parse_config(char *file,struct global_config *config){
	//open
	FILE *config_file = fopen(file,"r");
	if (config_file == NULL) return -1;
	size_t line_count = 0;
	//read lines
	char **lines = read_file_lines(config_file,&line_count);
	if (lines == NULL){
		fclose(config_file);
		return -1;
	}
	size_t config_count = 0;
	for (size_t i = 0; i < line_count; i++){
		char *domain, *block, *redirect;
		//split
		char **fields[] = {&domain,&block,&redirect};
		size_t field_count = sizeof(fields)/sizeof(char **);
		//remove comments
		remove_after(lines[i],'#');
		//if line isnt empty
		if (strlen(trim_whitespace(lines[i])) > 0){
			//no need to free after as pointers are part of lines
			size_t result = split_into(lines[i],fields,field_count,',');
			if (result < field_count){
				//incorrect number of arguments on line
				fprintf(stderr,"Could not parse line %lu of %s\n",i+1,file);
				fprintf(stderr,"---> %s <---\n",lines[i]);
			}else{
				//fill in info
				config_count++;
				config->domain_configs = realloc(config->domain_configs,config_count*sizeof(struct domain_config));
				config->domain_configs_count = config_count;
				config->domain_configs[config_count-1].name = strdup(trim_whitespace(domain));
				config->domain_configs[config_count-1].block = string_to_bool(block);
				config->domain_configs[config_count-1].redirect = dup_or_null(trim_whitespace(redirect));
			}
		}
		free(lines[i]);
	}
	free(lines);
	return 0;
}
void free_config(struct global_config *config){
	for (size_t i = 0; i < config->domain_configs_count; i++){
		free(config->domain_configs[i].name);
		free(config->domain_configs[i].redirect);
	}
	free(config->domain_configs);
}
//essentialy python's .rstrip() method.
char *rtrim(char *input,char delim){
	for (; input[strlen(input)-1] == delim;) input[strlen(input)-1] = '\0';
	return input;
}
char **read_file_lines(FILE *file,size_t *line_count){
	//get filesize
	int fd = fileno(file);
	struct stat statbuf;
	if (fstat(fd,&statbuf) < 0) return NULL;
	//allocate and read
	char *contents = malloc(statbuf.st_size);
	if (fread(contents,1,statbuf.st_size,file) < (size_t)statbuf.st_size){
		PERROR("fread");
		free(contents);
		return NULL;
	}
	//split into lines
	char **lines = NULL;
	char *line_buffer = malloc(statbuf.st_size);
	size_t line_length = 0;
	size_t current_line_count = 0;
	for (long i = 0; i < statbuf.st_size; i++){
		if (contents[i] == '\n'){
			current_line_count++;
			lines = realloc(lines,current_line_count*sizeof(char *));
			lines[current_line_count-1] = malloc(line_length+1);
			memcpy(lines[current_line_count-1],line_buffer,line_length);
			lines[current_line_count-1][line_length] = '\0';
			line_length = 0;
		}else{
			line_buffer[line_length] = contents[i];
			line_length++;
		}	
	}
	*line_count = current_line_count;
	free(contents);
	free(line_buffer);
	return lines;
}
//pass array of references to pointers, and the number of pointers, and it will split the string into them. No need to free any of the returned pointers either
size_t split_into(char *input, char ***ret_array, size_t pointer_count, char delim){
	size_t element_index = 0;
	char *element_start = input;
	size_t input_size = strlen(input)+1;
	for (size_t i = 0; i < input_size; i++){
		if (input[i] == delim || input[i] == '\0'){
			if (element_index < pointer_count){
				*ret_array[element_index] = element_start;
				input[i] = '\0';
				element_start = input+i+1;
			}
			element_index++;
		}
	}
	return element_index;
}
//anything after delim is removed from the string
void remove_after(char *line, char delim){
	for (size_t i = 0; i < strlen(line); i++){
		if (line[i] == delim){
			line[i] = '\0';
			return;
		}
	}
}
char *trim_whitespace(char *input){
	for (; isspace(input[0]);) input++;
	for (; strlen(input) > 0 && isspace(input[strlen(input)-1]);) input[strlen(input)-1] = '\0';
	return input;
}
int string_to_bool(char *str){
	char *trimmed = trim_whitespace(str);
	if (strcmp(trimmed,"true") == 0) return 1;
	else return 0;
}
char *dup_or_null(char *str){
	if (strcmp(str,"None") == 0) return NULL;
	return strdup(str);
}
char *bool_to_string(int b){
	if (b) return "true";
	else return "false"; 
}
long int recvfrom_index(int sockfd, void *buffer, size_t len, size_t offset){
	char *datagram = malloc(len+offset);
	int result = recvfrom(sockfd,datagram,len+offset,MSG_PEEK,NULL,0);
	if (result < 0) return result;
	memcpy(buffer,datagram+offset,len);
	free(datagram);
	return result;
}
int read_request(int sockfd, struct dns_request *request){
	struct sockaddr client_addr;
	socklen_t client_addrlen = sizeof(struct sockaddr);
	struct dns_header header;
	//====== peek at the header ======
	size_t dns_msg_size = 0;
	int result = recvfrom(sockfd,&header,sizeof(header),MSG_PEEK,&client_addr,&client_addrlen);
	if (result < 0){
		PERROR("recvfrom");
		return -1;
	}
	dns_msg_size += sizeof(header);
	request->question_count = ntohs(header.question_count);
	request->answer_count = ntohs(header.answer_count);
	request->authority_rr_count = ntohs(header.authority_rr_count);
	request->additional_rr_count = ntohs(header.additional_rr_count);
	printf("questions: %d, answers: %d, authority rrs: %d, aditional rrs: %d\n",request->question_count,request->answer_count,request->authority_rr_count,request->additional_rr_count);
	//====== read questions ======
	//allocate arrays
	struct dns_question *questions = malloc(sizeof(struct dns_question *)*request->question_count);
	request->questions = questions;
	for (int i = 0; i < request->question_count; i++){

		//read the domain
		char **labels;
		size_t label_count;
		int result = read_labels(sockfd,&labels,&label_count,dns_msg_size);
		if (result < 0){
			free_request(request);
			return -1;
		};
		dns_msg_size += result;
		//read the type and class
		struct {short a; short b;} ab;
		result = recvfrom_index(sockfd,&ab,sizeof(ab),dns_msg_size);
		if (result < 0){
			PERROR("recvfrom");
			free_request(request);
			return -1;
		}
		questions[i].type = ntohs(ab.a);
		questions[i].class = ntohs(ab.b);
		questions[i].labels = labels;
		questions[i].label_count = label_count;
		printf("name = ");
		for (size_t i = 0; i < label_count; i++){
			printf("%s",labels[i]);
			if (i != label_count-1) printf(".");
		}
		printf(", type = %d, class = %d\n",questions[i].type,questions[i].class);
		dns_msg_size += 4;
	}
	//====== read answers ======
	//allocate arrays
	//struct dns_question *answers = malloc(sizeof(struct dns_question *)*request->question_count);
	for (int i = 0; i < request->answer_count; i++){

	}
	return 0;
}
void free_request(struct dns_request *request){
	int question_count = request->question_count;
	//====== free up data structs ======
	for (;question_count > 0; question_count--){
		for (size_t i = 0; i < request->questions[question_count-1].label_count; i++){ 
			free(request->questions[question_count-1].labels[i]);
		}
		free(request->questions[question_count-1].labels);
	}
	free(request->questions);
}
int read_labels(int sockfd, char ***labels_p, size_t *label_count_p, size_t offset){
	//fill in details
	size_t label_count = 0;
	char **labels = NULL;
	*labels_p = NULL;
	size_t size = 0;
	//read the label
	for (;;){
		//get the section size
		uint8_t section_size = 0;
		int result = recvfrom_index(sockfd,&section_size,sizeof(uint8_t),offset+size);
		if (result < 0){
			PERROR("recvfrom");
			return -1;
		}
		size += 1 + section_size;
		if (section_size == 0) break;
		label_count++;
		labels = realloc(labels,sizeof(char *)*label_count);
		labels[label_count-1] = calloc(1,section_size+1);
		result = recvfrom_index(sockfd,labels[label_count-1],section_size,offset+size-section_size);
		if (result < 0){
			PERROR("recvfrom");
			return -1;
		}
	}
	*labels_p = labels;
	*label_count_p = label_count;
	return size;
}
