#ifndef _LIST_PRIVATE_H
#define _LIST_PRIVATE_H /* Módulo LIST_PRIVATE */

#include "entry.h"

struct node_t {
    struct entry_t *entry;
    struct node_t *next;
};

struct list_t {
    int size;
    struct node_t *head;
    struct node_t *tail;
};

/*
 * Imprime lista para stdout
 */
void list_print(struct list_t* list);

/*
 * Inicializa uma estrutura node_t com apontadores a NULL
 */
void node_initialize(struct node_t* node);

/*
 * Função que cria um novo node_t contendo a entry que lhe é dada
 */
struct node_t *node_create(struct entry_t* entry);

/*
 * Destroi um node_t libertando *toda* a memória correspondente
 */
void node_destroy(struct node_t *node);

#endif