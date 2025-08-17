#include <stdio.h>
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
struct domain_config {
	char *name;
	int block;
	char *redirect;
};
struct global_config {
	size_t domain_configs_count;
	struct domain_config *domain_configs;
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
char *dup_or_null(char *str);
void free_config(struct global_config *config);
int string_to_bool(char *str);
char *bool_to_string(int b);

//======================= definitions ========================
int main(int argc, char **argv){
	//====== load config files ======
	struct passwd *pw = getpwuid(getuid());
	char config_location[2048];
	//try the users local config dir
	snprintf(config_location,sizeof(config_location),"%s/.config/dnsmonitor/dnsmonitor.conf",rtrim(pw->pw_dir,'/'));
	int result = parse_config(config_location,&config);
	if (result < 0){
		//try the global config
		parse_config("/etc/dnsmonitor/dnsmonitor.conf",&config);
	}
	//if no config is loaded a default empty config is used
	printf("configuration:\n");
	for (size_t i = 0; i < config.domain_configs_count; i++){
		printf("(%s) block: %s, redirect: %s\n",config.domain_configs[i].name,bool_to_string(config.domain_configs[i].block),config.domain_configs[i].redirect);
	}
	//====== serve ======
	
	//cleanup
	free_config(&config);
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
