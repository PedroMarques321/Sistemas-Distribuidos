#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "block.h"

/**************************************************************/
void pee(const char *msg)
{
	perror(msg);
	exit(0);
}

/**************************************************************/
int testCreate() {
	int result, data_size;
	struct block_t *b;
	char *data_str = strdup("1234567890abc");

	/* Ignore \0, because we consider arbitrary contents */
	data_size = strlen(data_str); 

	printf("Módulo block -> testCreate: ");
	fflush(stdout);

	assert(block_create(-1, data_str) == NULL);
	result = (block_create(-1, data_str) == NULL);

    assert(block_create(0, data_str) == NULL);
	result = result && (block_create(0, data_str) == NULL);

	assert(block_create(data_size, NULL) == NULL);
	result = result && (block_create(data_size, NULL) == NULL);

	if ((b = block_create(data_size, data_str)) == NULL)
		pee("  block_create retornou NULL - O teste não pode prosseguir");

	result = result && (b->data == data_str)
                        && (b->datasize == data_size)
		        && (memcmp(b->data, data_str, data_size) == 0);
	block_destroy(b);

	printf("%s\n",result?"passou":"não passou");
	return result;
}

/**************************************************************/
int testDestroy(){
	char *data_str = strdup("1234567890abc");
	int result, data_size = strlen(data_str);
	struct block_t *b;

	printf("Módulo block -> testDestroy: ");
	fflush(stdout);

	assert(block_destroy(NULL) == -1);
	result = (block_destroy(NULL) == -1);

	if ((b = block_create(data_size, data_str)) == NULL)
		pee("  block_create retornou NULL - O teste não pode prosseguir");
		
	result = result && (block_destroy(b) == 0);

	printf("%s\n",result?"passou":"não passou");
	return result;
}

/**************************************************************/
int testDuplicate() {
	char *data_str = strdup("1234567890abc");
	int result, data_size = strlen(data_str);
	struct block_t *b;
	struct block_t *b2;

	printf("Módulo block -> testDuplicate: ");
	fflush(stdout);

	assert(block_duplicate(NULL) == NULL);
	result = (block_duplicate(NULL) == NULL);

	b = (struct block_t *) malloc(sizeof(struct block_t));

	b->data = malloc(1);

	b->datasize = -1;
	assert(block_duplicate(b) == NULL);
	result = result && (block_duplicate(b) == NULL);

	b->datasize = 0;
	assert(block_duplicate(b) == NULL);
	result = result && (block_duplicate(b) == NULL);

	b->datasize = 1;
	free(b->data);
	b->data = NULL;
	assert(block_duplicate(b) == NULL);
	result = result && (block_duplicate(b) == NULL);
	
	free(b);

	if ((b = block_create(data_size, data_str)) == NULL)
		pee("  block_create retornou NULL - O teste não pode prosseguir");
	
	b2 = block_duplicate(b);
	
	result = result && (b2 != b)
                        && (b->data != b2->data)
                        && (b->datasize == b2->datasize)
                        && (memcmp(b->data, b2->data, b->datasize) == 0);

	block_destroy(b);
	block_destroy(b2);

	printf("%s\n",result?"passou":"não passou");
	return result;
}

/**************************************************************/
int main() {
	int score = 0;

	printf("\nIniciando o teste do módulo block \n");

	score += testCreate();

	score += testDestroy();

	score += testDuplicate();
	
	printf("teste block (score): %d/3\n", score);

	if (score == 3)
        	return 0;
	else
        	return -1;
}
