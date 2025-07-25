#ifndef _TABLE_PRIVATE_H
#define _TABLE_PRIVATE_H /* Módulo TABLE_PRIVATE */

#include "list.h"

/* Esta estrutura define o par {chave, valor} para a tabela
 */
struct table_t {
	struct list_t **lists;
	int nlists;
};

int hash_code(char *key, int n);

#endif
