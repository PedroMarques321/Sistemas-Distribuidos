/* ____  _     _                                        
* / ___|(_)___| |_ ___ _ __ ___   __ _ ___              
* \___ \| / __| __/ _ \ '_ ` _ \ / _` / __|             
*  ___) | \__ \ ||  __/ | | | | | (_| \__ \             
* |____/|_|___/\__\___|_| |_| |_|\__,_|___/ _           
* |  _ \(_)___| |_ _ __(_) |__  _   _/_/ __| | ___  ___ 
* | | | | / __| __| '__| | '_ \| | | | |/ _` |/ _ \/ __|
* | |_| | \__ \ |_| |  | | |_) | |_| | | (_| | (_) \__ \
* |____/|_|___/\__|_|  |_|_.__/_\__,_|_|\__,_|\___/|___/
* |___ \ / _ \___ \| || |   / /___ \| ___|              
*   __) | | | |__) | || |_ / /  __) |___ \              
*  / __/| |_| / __/|__   _/ /  / __/ ___) |             
* |_____|\___/_____|  |_|/_/  |_____|____/
*            
* Grupo 56
* Pedro Marques nº48674
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "block.h"

/* Função que cria um novo bloco de dados block_t e que inicializa 
 * os dados de acordo com os argumentos recebidos, sem necessidade de
 * reservar memória para os dados.	
 * Retorna a nova estrutura ou NULL em caso de erro.
 */
struct block_t *block_create(int size, void *data) {
    struct block_t *b;

    if (data == NULL || size <= 0) return NULL;

    b = (struct block_t*)malloc(sizeof(struct block_t));

    b->datasize = size;
    b->data = data;

    return b;
}

/* Função que duplica uma estrutura block_t, reservando a memória
 * necessária para a nova estrutura.
 * Retorna a nova estrutura ou NULL em caso de erro.
 */
struct block_t *block_duplicate(struct block_t *b) {
    if (b == NULL || b->datasize <= 0 || b->data == NULL) return NULL;

    struct block_t *duplicate = (struct block_t*)malloc(sizeof(struct block_t));

    duplicate->datasize = b->datasize;
    duplicate->data = malloc(b->datasize);
    memcpy(duplicate->data, b->data, b->datasize);

    return duplicate;
}

/* Função que substitui o conteúdo de um bloco de dados block_t.
 * Deve assegurar que liberta o espaço ocupado pelo conteúdo antigo.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int block_replace(struct block_t *b, int new_size, void *new_data) {
    if (b == NULL || new_data == NULL || new_size <= 0) return -1;

    free(b->data);

    b->datasize = new_size;
    b->data = new_data;

    return 0;
}

/* Função que elimina um bloco de dados, apontado pelo parâmetro b,
 * libertando toda a memória por ele ocupada.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int block_destroy(struct block_t *b) {
    if (b == NULL) return -1;

    if (b->data != NULL) free(b->data);
    
    free(b);
    return 0;
}



