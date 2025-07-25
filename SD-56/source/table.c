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
#include "table.h"
#include "table-private.h"


struct table_t; /* A definir pelo grupo em table-private.h */

/* Função para criar e inicializar uma nova tabela hash, com n
 * linhas (n = módulo da função hash).
 * Retorna a tabela ou NULL em caso de erro.
 */
struct table_t *table_create(int n){
    if(n <= 0) return NULL;

    struct table_t* table = (struct table_t*)malloc(sizeof(struct table_t));

    table->nlists = n;
    table->lists = (struct list_t**)calloc(n, sizeof(struct list_t*));
    
    return table;
}

/* Função para adicionar um par chave-valor à tabela. Os dados de entrada
 * desta função deverão ser copiados, ou seja, a função vai criar uma nova
 * entry com *CÓPIAS* da key (string) e dos dados. Se a key já existir na
 * tabela, a função tem de substituir a entry existente na tabela pela
 * nova, fazendo a necessária gestão da memória.
 * Retorna 0 (ok) ou -1 em caso de erro.
 */
int table_put(struct table_t *t, char *key, struct block_t *value) {
    if(t == NULL || key == NULL || value == NULL) return -1;

    int h = hash_code(key, t->nlists);

    //Verificar se ja existe
    struct entry_t *entry = list_get(t->lists[h], key);
    struct block_t *copy_block = block_duplicate(value);
    char *copy_key = strdup(key);
    if (entry != NULL) {
        int result = entry_replace(entry, copy_key, copy_block);
        return result;
    } else {
        if (t->lists[h] == NULL) {
            t->lists[h] = list_create();
        }
        struct entry_t *new_entry = entry_create(copy_key, copy_block);
        if (new_entry == NULL) return -1;

        int result = list_add(t->lists[h], new_entry);
        if (result == -1) {
            entry_destroy(new_entry);
            return -1;
        }
    }
    
    return 0;
}

/* Função que procura na tabela uma entry com a chave key. 
 * Retorna uma *CÓPIA* dos dados (estrutura block_t) nessa entry ou 
 * NULL se não encontrar a entry ou em caso de erro.
 */
struct block_t *table_get(struct table_t *t, char *key) {
    if(t == NULL || key == NULL) return NULL;

    int h = hash_code(key, t->nlists);

    struct entry_t *found_entry = list_get(t->lists[h], key);

    if(found_entry == NULL) return NULL;

    struct block_t *found_block = block_duplicate(found_entry->value);
    return found_block;
}


/* Função que conta o número de entries na tabela passada como argumento.
 * Retorna o tamanho da tabela ou -1 em caso de erro.
 */
int table_size(struct table_t *t) {
    if(t->lists == NULL) return -1;

    int result = 0;
    for(int i = 0; i < t->nlists; i++) {
        if(t->lists[i] != NULL) result += list_size(t->lists[i]);
    }
    return result;
}

/* Função auxiliar que constrói um array de char* com a cópia de todas as keys na 
 * tabela, colocando o último elemento do array com o valor NULL e
 * reservando toda a memória necessária.
 * Retorna o array de strings ou NULL em caso de erro.
 */
char **table_get_keys(struct table_t *t) {
    if (t == NULL) return NULL;

    char **all_keys = (char**)malloc(sizeof(char*)*(t->nlists));
    int total_keys = 0;

    for (int i = 0; i < t->nlists; i++){
        char **list_keys = list_get_keys(t->lists[i]);
        if (list_keys == NULL) continue;

        // Contar nr de keys na lista
        int num_keys = 0;
        while (list_keys[num_keys] != NULL) num_keys++;

        char **new_all_keys = (char**)realloc(all_keys, sizeof(char*)*(total_keys + num_keys + 1));

        all_keys = new_all_keys;

        // Copiar as keys da lista para array all_keys
        for (int j = 0; j < num_keys; j++) {
            all_keys[total_keys++] = strdup(list_keys[j]);
            if (all_keys[total_keys - 1] == NULL) {
                for (j = 0; j < total_keys - 1; j++) {
                    free(all_keys[j]);
                }
                free(all_keys);
                return NULL;
            }
        }
        free(list_keys);
    }
    all_keys [total_keys] = NULL; // Ultimo elemento a NULL
    return all_keys;
}

/* Função auxiliar que liberta a memória ocupada pelo array de keys obtido pela 
 * função table_get_keys.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int table_free_keys(char **keys){
    if(keys == NULL) return -1;
    int free_keys = list_free_keys(keys);
    return free_keys;
}

/* Função que remove da lista a entry com a chave key, libertando a
 * memória ocupada pela entry.
 * Retorna 0 se encontrou e removeu a entry, 1 se não encontrou a entry,
 * ou -1 em caso de erro.
 */
int table_remove(struct table_t *t, char *key){
    if(t->lists == NULL || key == NULL) return -1;
    int h = hash_code(key, t->nlists);
    int result = list_remove(t->lists[h], key);
    return result;
}

/* Função que elimina uma tabela, libertando *toda* a memória utilizada
 * pela tabela.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int table_destroy(struct table_t *t){
    if(t == NULL || t->nlists <= 0) return -1;

    int result = 0;
    for(int i = 0; i < t->nlists; i++){
        result += list_destroy(t->lists[i]);
    }
    if(result != 0) return -1;

    return 0;
}


/* Função que calcula um hash code para uma dada string
 */
int hash_code(char *key, int n){
    unsigned long hash_value = 0;

    unsigned long prime = 31; // or other prime

    while (*key) {
        hash_value = (hash_value * prime) + *key;
        key++;
    }
    return (int)(hash_value % n);
}