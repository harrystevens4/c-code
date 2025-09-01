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

static int char_frequency_cmp(const void *a, const void *b){
	const struct char_frequency *freq_a = a;
	const struct char_frequency *freq_b = b;
	return freq_a->frequency - freq_b->frequency;
}

struct huffman_tree_node *build_huffman_tree(char frequency_table[],int frequency_table_size){
	//====== recursively build the tree ======
	if (frequency_table_size == 1){
		//====== leaf node ======
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
bool get_bit_sequence(struct huffman_tree_node *root,char byte,uint8_t *buffer,int *count){
	if (root == NULL) return false;
	if (root->left == NULL && root->right == NULL){
	//====== leaf node found ======
	if (root->ch == byte) return true;
		else return false;
	}else{
	//====== else keep traversing ======
		*count++;
		buffer[(*count)-1] = 0;
		if (get_bit_sequence(root->left,byte,buffer,count)) return true;
		buffer[(*count)-1] = 1;
		if (get_bit_sequence(root->right,byte,buffer,count)) return true;
		*count--;
		return false;
	}
}

//sets output to a malloc'd pointer
size_t hfmn_compress(const unsigned char data[],size_t len,char **output){
	//====== count frequencies ======
	//use the char as an index
	struct char_frequency frequency_table[UCHAR_MAX+1] = {0};
	for (int i = 0; i < UCHAR_MAX+1; i++) frequency_table[i].ch = i;
	for (size_t i = 0; i < len; i++){
		frequency_table[data[i]].frequency++;
	}
	//sort in prep to build tree
	qsort(frequency_table,UCHAR_MAX+1,sizeof(struct char_frequency),char_frequency_cmp);
	//exclude chars that dont show up
	int frequency_table_size = UCHAR_MAX+1;
	for (int i = 0; i < UCHAR_MAX+1; i++) if (frequency_table[i].frequency == 0) frequency_table_size = i;
	//====== write header to output ======
	size_t header_size = (frequency_table_size*sizeof(char))+sizeof(uint8_t)+sizeof(uint8_t);
	char *output_buffer = malloc(header_size);
	for (int i = 0; i < frequency_table_size; i++) (output_buffer+2)[i] = frequency_table[i].ch;
	//create huffman tree
	struct huffman_tree_node *tree = build_huffman_tree(output_buffer,frequency_table_size);
	//====== encode data ======
	char *data_buffer = NULL;
	size_t data_size = 0;
	int byte_offset = 0;
	for (size_t i = 0; i < len; i++){
		char byte = data[i];
		uint8_t bit_sequence[UCHAR_MAX+1];
		memset(bit_sequence,0,UCHAR_MAX+1);
		int bit_sequence_length = 0;
		get_bit_sequence(tree,byte,bit_sequence,&bit_sequence_length);
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
	memcpy(output_buffer+(frequency_table_size*sizeof(char))+sizeof(uint8_t),data_buffer,data_size);
	((uint8_t *)output_buffer)[0] = frequency_table_size;
	((uint8_t *)output_buffer)[0] = (byte_offset == 0) ? 8 : byte_offset;
	//====== return ======
	*output = output_buffer;
	free(data_buffer);
	free_huffman_tree(tree);
	return 0;
}
size_t hfmn_decompress(char data[],size_t len,char **output){
	char *output_buffer = malloc(10);
	memcpy(output_buffer,"hi",3);
	*output = output_buffer; 
	return 3;
}
