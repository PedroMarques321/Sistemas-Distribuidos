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
#include "list-private.h"

/* Função que cria e inicializa uma nova lista (estrutura list_t a
 * ser definida pelo grupo no ficheiro list-private.h).
 * Retorna a lista ou NULL em caso de erro.
 */
struct list_t *list_create() {
    struct list_t* l = (struct list_t*)malloc(sizeof(struct list_t));
    if (l == NULL) return NULL;
    l->head = NULL;
    l->tail = NULL;
    l->size = 0;
    return l;
}

/* Função que adiciona à lista a entry passada como argumento.
 * A entry é inserida de forma ordenada, tendo por base a comparação
 * de entries feita pela função entry_compare do módulo entry e
 * considerando que a entry menor deve ficar na cabeça da lista.
 * Se já existir uma entry igual (com a mesma chave), a entry
 * já existente na lista será substituída pela nova entry,
 * sendo libertada a memória ocupada pela entry antiga.
 * Retorna 0 se a entry ainda não existia, 1 se já existia e foi
 * substituída, ou -1 em caso de erro.
 */
int list_add(struct list_t *l, struct entry_t *entry) {
    if (l == NULL || entry == NULL) return -1;

    //Se for a primeira entry, lista vazia 
    if (l->tail == NULL) {
        if (l->head != NULL) return -1;
        l->head = node_create(entry);
        l->tail = l->head;
        l->size = 1;
        return 0;
    }

    // encontrar a posição correta para inserir a entry
    struct node_t* it0 = NULL;
    struct node_t* it = l->head;
    while (it != l->tail) {
        if (entry_compare(it->entry, entry) == 0) break;
        if (entry_compare(it->entry, entry) == 1) break;
        it0 = it; // guarda it anterior
        it = it->next;
    }
    
    // é igual a entry atual -> substituir a entry (não o conteúdo da entry)
    if (entry_compare(it->entry, entry) == 0) {
        entry_destroy(it->entry);
        it->entry = entry;
        return 1;
    }

    // it é o tail e é inferior a entry -> insere seguir a ele
    if (it == l->tail && entry_compare(it->entry, entry) == -1) {
        l->tail->next = node_create(entry);
        l->tail = l->tail->next;
        l->size++;
        return 0;
    }

    // it é o head e nesse caso fica antes a ele
    if (it == l->head) {
        struct node_t* new_node = node_create(entry);
        new_node->next = l->head;
        l->head = new_node;
        l->size++;
        return 0;
    }

    // it está no meio da lista e é maior que a entry -> inserir antes
    struct node_t* new_node = node_create(entry);
    new_node->next = it;
    it0->next = new_node;
    l->size++;
    return 0;
}

/* Função que conta o número de entries na lista passada como argumento.
 * Retorna o tamanho da lista ou -1 em caso de erro.
 */
int list_size(struct list_t *l) {
    if (l == NULL) return -1;
    return l->size;
}

/* Função que obtém da lista a entry com a chave key.
 * Retorna a referência da entry na lista ou NULL se não encontrar a
 * entry ou em caso de erro.
*/
struct entry_t *list_get(struct list_t *l, char *key) {
    if (l == NULL || key == NULL) return NULL;

    struct node_t* it_list = l->head;
    
    while (it_list != NULL) {
        if (strcmp(it_list->entry->key, key) == 0) return it_list->entry;
        it_list = it_list->next;
    }
    return NULL;
}

/* Função auxiliar que constrói um array de char* com a cópia de todas as keys na 
 * lista, colocando o último elemento do array com o valor NULL e
 * reservando toda a memória necessária.
 * Retorna o array de strings ou NULL em caso de erro.
 */
char **list_get_keys(struct list_t *l) {
    if (l == NULL || l->size == 0) return NULL;

    char** keylist = malloc(sizeof(char*)*(l->size+1));
    char** it_keylist = keylist;
    struct node_t* it_list = l->head;

    while (it_list != NULL) {
        *it_keylist = malloc(strlen(it_list->entry->key)+1);
        strcpy(*it_keylist, it_list->entry->key);
        it_keylist++;
        it_list = it_list->next;
    }
    *it_keylist = NULL;
    return keylist;
}

/* Função auxiliar que liberta a memória ocupada pelo array de keys obtido pela 
 * função list_get_keys.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int list_free_keys(char **keys) {
    if (keys == NULL) return -1;
    char** it = keys; //devemos usar o list_get_keys ou podemos apenas fazer it = keys?
    while (*it != NULL) {
        free(*it);
        it++;
    }
    free(keys);
    return 0;
}

/* Função que elimina da lista a entry com a chave key, libertando a
 * memória ocupada pela entry.
 * Retorna 0 se encontrou e removeu a entry, 1 se não encontrou a entry,
 * ou -1 em caso de erro.
 */
int list_remove(struct list_t *l, char *key) {
    if (l == NULL || key == NULL) return -1;
    struct node_t* it = l->head;
    struct node_t* itlast = NULL;
    
    while (it != NULL) {
        if (strcmp(it->entry->key, key) == 0) break;
        itlast = it;
        it = it->next;
    }

    if (it == NULL) return 1;

    if (itlast != NULL) itlast->next = it->next;
    if (it == l->head) l->head = it->next;
    if (it == l->tail) l->tail = itlast;

    l->size--;

    node_destroy(it);
    return 0;
}

/* Função que elimina uma lista, libertando *toda* a memória utilizada
 * pela lista (incluindo todas as suas entradas).
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int list_destroy(struct list_t *l) {
    if (l == NULL) return -1;
    struct node_t* node = l->head;
    struct node_t* tmp;

    while (node != NULL) {
        tmp = node;
        node = node->next;
        node_destroy(tmp);
    }
    free(l);
    return 0;
}

/*
 * Imprime lista para stdout
 */
void list_print(struct list_t* list) {
    const char* indent = "    ";
    int i = 0;
    struct node_t* node = list->head;
    while (node != NULL) {
        printf("NODE: %d\n", i);
        printf("%sKEY: %s\n", indent, node->entry->key);
        printf("%sDATA_SIZE: %d\n", indent, node->entry->value->datasize);
        node = node->next;
        i++;
    }
}

/*
 * Inicializa uma estrutura node_t com apontadores a NULL
 */
void node_initialize(struct node_t* node) {
    node->entry = NULL;
    node->next = NULL;
}

/*
 * Função que cria um novo node_t contendo a entry que lhe é dada
 */
struct node_t *node_create(struct entry_t* entry) {
    if (entry == NULL) return NULL;

    struct node_t* new_node = (struct node_t*)malloc(sizeof(struct node_t));
    node_initialize(new_node);
    new_node->entry = entry;
    return new_node;
}

/*
 * Destroi um node_t libertando *toda* a memória correspondente
 */
void node_destroy(struct node_t *node) {
    if (node == NULL) return;
    entry_destroy(node->entry);
    free(node);
}