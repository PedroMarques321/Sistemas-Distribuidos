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
#include <entry.h>

/* Função que cria uma entry, reservando a memória necessária e
 * inicializando-a com a string e o bloco de dados de entrada.
 * Retorna a nova entry ou NULL em caso de erro.
 */
struct entry_t *entry_create(char *key, struct block_t *value) {
    struct entry_t *e;

    if (value == NULL || key == NULL) return NULL;

    e = (struct entry_t*)malloc(sizeof(struct entry_t));

    e->key = key;
    e->value = value;

    return e;
}

/* Função que compara duas entries e retorna a ordem das mesmas, sendo esta
 * ordem definida pela ordem das suas chaves.
 * Retorna 0 se as chaves forem iguais, -1 se e1 < e2,
 * 1 se e1 > e2 ou -2 em caso de erro.
 */
int entry_compare(struct entry_t *e1, struct entry_t *e2) {
    if (e1 == NULL || e2 == NULL){
        return -2;
        if (strlen(e1->key) == 0 && strlen(e2->key) == 0) {
            return 0;
        } 
    }
    int r = strcmp(e1->key, e2->key); 
    if (r==0) return 0;
    return r < 0 ? -1 : 1;
}

/* Função que duplica uma entry, reservando a memória necessária para a
 * nova estrutura.
 * Retorna a nova entry ou NULL em caso de erro.
 */
struct entry_t *entry_duplicate(struct entry_t *e) {
    struct entry_t *duplicate;
    char *key;

    if (e == NULL) return NULL;

    duplicate = (struct entry_t*) malloc(sizeof(struct entry_t));

    if (duplicate == NULL) return NULL;

    key = (char*)malloc(strlen(e->key) + 1);

    if (key == NULL) {
        free(duplicate);
        return NULL;
    }

    strcpy(key, e->key);

    duplicate->key = key;

    duplicate->value = block_duplicate(e->value);

    return duplicate;
}

/* Função que substitui o conteúdo de uma entry, usando a nova chave e
 * o novo valor passados como argumentos, e eliminando a memória ocupada
 * pelos conteúdos antigos da mesma.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int entry_replace(struct entry_t *e, char *new_key, struct block_t *new_value) {
    if (e == NULL || new_key == NULL || new_value == NULL) return -1;

    free(e->key);
    block_destroy(e->value);

    e->key = new_key;
    e->value = new_value;

    return 0;
}

/* Função que elimina uma entry, libertando a memória por ela ocupada.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int entry_destroy(struct entry_t *e) {
    if (e == NULL) return -1;
    if (e->value == NULL) return -1;
    if (e->key == NULL) return -1;
    
    free(e->key);
    block_destroy(e->value);
    free(e);
    
    return 0;
}

