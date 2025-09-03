#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

/* structure of serialized data
uint8_t frequency_table_size
uint8_t final_byte_size
char frequency_table[frequency_table_size]
raw_data

*/

struct char_frequency {
	char ch;
	int frequency;
};
struct frequency_table {
	int entry_count;
	struct char_frequency table[];
};
struct huffman_tree_node {
	struct huffman_tree_node *left;
	struct huffman_tree_node *right;
	char ch;
};
int extract_bit(char *data,size_t index){
	return data[index/8] >> (7-(index%8));
}
void print_huffman_tree(struct huffman_tree_node *tree,int depth){
	static char buffer[1024] = {0};
	if (tree == NULL) return;
	if (tree->left == NULL && tree->right == NULL){
		printf("%c: %s\n",tree->ch,buffer);
		return;
	}
	buffer[depth+1] = '\0';
	buffer[depth] = '0';
	print_huffman_tree(tree->left,depth+1);
	buffer[depth] = '1';
	print_huffman_tree(tree->right,depth+1);
}
int decode_huffman_data(struct huffman_tree_node *tree,char *data,int byte_offset,char *byte_return){
	//====== traverse untill leaf node found ======
	struct huffman_tree_node *current_node = tree;
	int i = 0;
	for (;current_node->left != NULL || current_node->right != NULL;){
		//something has gone wrong
		if (current_node == NULL) return '\0';
		//extract the next bit
		int bit = extract_bit(data,i);
		if (bit) current_node = current_node->right;
		else current_node = current_node->left;
		i++;
	}
	*byte_return = current_node->ch;
	return i;
}
static int char_frequency_cmp(const void *a, const void *b){
	const struct char_frequency *freq_a = a;
	const struct char_frequency *freq_b = b;
	return freq_b->frequency - freq_a->frequency;
}
struct huffman_tree_node *build_huffman_tree(char frequency_table[],int frequency_table_size){
	//====== recursively build the tree ======
	if (frequency_table_size == 1){
		//====== leaf node ======
		printf("adding root node for %c\n",frequency_table[0]);
		struct huffman_tree_node *node = malloc(sizeof(struct huffman_tree_node));
		memset(node,0,sizeof(struct huffman_tree_node));
		node->ch = frequency_table[0];
		return node;
	}else{
		//====== branch node ======
		struct huffman_tree_node *node = malloc(sizeof(struct huffman_tree_node));
		memset(node,0,sizeof(struct huffman_tree_node));
		//split list down the middle and use each side as each side of the node
		int midpoint = (frequency_table_size-1)/2;
		struct huffman_tree_node *left = build_huffman_tree(frequency_table,midpoint+1);
		struct huffman_tree_node *right = build_huffman_tree(frequency_table+midpoint+1,frequency_table_size-midpoint-1);
		node->left = left; node->right = right;
		return node;
	}
}
void free_huffman_tree(struct huffman_tree_node *root){
	if (root == NULL) return;
	free_huffman_tree(root->left);
	free_huffman_tree(root->right);
	free(root);
}
int get_bit_sequence(struct huffman_tree_node *root,char byte,uint8_t *buffer,int depth){
	if (root == NULL) return 0;
	if (root->left == NULL && root->right == NULL){
	//====== leaf node found ======
		if (root->ch == byte) return depth;
		else return 0;
	}else{
	//====== else keep traversing ======
		int result = 0;
		buffer[depth] = 0;
		if (result = get_bit_sequence(root->left,byte,buffer,depth+1)) return result;
		buffer[depth] = 1;
		if (result = get_bit_sequence(root->right,byte,buffer,depth+1)) return result;
		return 0;
	}
}

//sets output to a malloc'd pointer
size_t hfmn_compress(const char data[],size_t len,char **output){
	//====== count frequencies ======
	//use the char as an index
	struct char_frequency frequency_table[UCHAR_MAX+1] = {0};
	for (int i = 0; i < UCHAR_MAX+1; i++) frequency_table[i].ch = i;
	for (size_t i = 0; i < len; i++){
		frequency_table[(int)data[i]].frequency++;
	}
	//sort in prep to build tree
	qsort(frequency_table,UCHAR_MAX+1,sizeof(struct char_frequency),char_frequency_cmp);
	//for (int i = 0; i < UCHAR_MAX+1; i++) printf("%c%d",frequency_table[i].ch,frequency_table[i].frequency); printf("\n");
	//exclude chars that dont show up
	int frequency_table_size = UCHAR_MAX+1;
	for (int i = 0; i < UCHAR_MAX+1; i++){
		if (frequency_table[i].frequency == 0){
			frequency_table_size = i;
			break;
		}
	}
	//====== write header to output ======
	size_t header_size = (frequency_table_size*sizeof(char))+sizeof(uint8_t)+sizeof(uint8_t);
	char *output_buffer = malloc(header_size);
	for (int i = 0; i < frequency_table_size; i++) (output_buffer+2)[i] = frequency_table[i].ch;
	//create huffman tree
	struct huffman_tree_node *tree = build_huffman_tree(output_buffer,frequency_table_size);
	print_huffman_tree(tree,0);
	//====== encode data ======
	char *data_buffer = NULL;
	size_t data_size = 0;
	int byte_offset = 0;
	for (size_t i = 0; i < len; i++){
		char byte = data[i];
		uint8_t bit_sequence[UCHAR_MAX+1];
		memset(bit_sequence,0,UCHAR_MAX+1);
		int bit_sequence_length = get_bit_sequence(tree,byte,bit_sequence,0);
		//write the bits to the buffer
		data_buffer = realloc(data_buffer,data_size+(bit_sequence_length/8)+1);
		for (int i = 0; i < bit_sequence_length; i++){
			if (byte_offset == 0){
				data_size++;
				data_buffer[data_size-1] = 0;
			}
			data_buffer[data_size-1] |= bit_sequence[i] << (7-byte_offset);
			byte_offset++; byte_offset %= 8;
		}
	}
	//====== write the data ======
	output_buffer = realloc(output_buffer,header_size+data_size);
	memcpy(output_buffer+header_size,data_buffer,data_size);
	((uint8_t *)output_buffer)[0] = frequency_table_size;
	((uint8_t *)output_buffer)[0] = (byte_offset == 0) ? 8 : byte_offset;
	//====== return ======
	*output = output_buffer;
	free(data_buffer);
	free_huffman_tree(tree);
	return header_size+data_size;
}
size_t hfmn_decompress(char data[],size_t len,char **output){
	//====== read the frequency table ======
	int frequency_table_size = ((uint8_t *)data)[0];
	int final_byte_size = ((uint8_t *)data)[1];
	struct huffman_tree_node *tree = build_huffman_tree(data+2,frequency_table_size);
	//====== decode the raw data ======
	char *decoded_data;
	size_t decoded_data_size = 0;
	int byte_offset = 0;
	size_t data_offset = 0;
	for (;decoded_data_size < len-2-frequency_table_size;){
		if (decoded_data_size >= len-2-frequency_table_size && byte_offset >= final_byte_size) break;
		char byte = 0;
		int bits_read = decode_huffman_data(tree,data+2+data_offset,byte_offset,&byte);
		byte_offset += bits_read;
		data_offset += byte_offset/8;
		byte_offset %= 8;
		decoded_data_size++;
		decoded_data = realloc(decoded_data,decoded_data_size);
		decoded_data[decoded_data_size-1] = byte;
	}
	//====== return ======
	*output = decoded_data; 
	free_huffman_tree(tree);
	return decoded_data_size;
}
