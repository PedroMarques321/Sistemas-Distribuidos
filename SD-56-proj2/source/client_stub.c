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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "client_stub-private.h"
#include "inet-private.h"
#include "client_network.h"
#include "message-private.h"
#include "block.h"
#include "entry.h"


/* Função para estabelecer uma associação entre o cliente e o servidor,
* em que address_port é uma string no formato <hostname>:<port>.
* Retorna a estrutura rtable preenchida, ou NULL em caso de erro.
*/
struct rtable_t *rtable_connect(char *address_port) {
    int iarr[5];
    char hostname[50] = {0};
    int port;
    struct rtable_t *rtable;

    if (sscanf(address_port, "%d.%d.%d.%d:%d", &iarr[0], &iarr[1], &iarr[2], &iarr[3], &iarr[4]) != 5) {
        fprintf(stderr, "Erro: formato deve ser 'IP_address:port' mas foi escrito: %s\n", address_port);
        return NULL;
    }
    sprintf(hostname, "%d.%d.%d.%d", iarr[0], iarr[1], iarr[2], iarr[3]);
    port = iarr[4];

    rtable = malloc(sizeof(struct rtable_t));
    
    rtable->server_address = strdup(hostname);
    rtable->server_port = port;
    //tentar mais que uma vez pois pode falhar à primeira
    if(network_connect(rtable) == -1) {
        printf("Erro: network_connect!\n");
        sleep(2);
        if(network_connect(rtable) == -1) {
            printf("Erro: segunda tentativa, network_connect!\n");
            sleep(2);
            if(network_connect(rtable) == -1) {
                printf("Erro: terceira tentativa, network_connect!\n");
                return NULL;
            }
        }
    }

    return rtable;
}

/* Termina a associação entre o cliente e o servidor, fechando a
* ligação com o servidor e libertando toda a memória local. * Retorna 0 se tudo correr bem, ou -1 em caso de erro. */ 
int rtable_disconnect(struct rtable_t *rtable) {
    free(rtable->server_address);
    if(network_close(rtable) != 0) {
        printf("client_stub: Erro a fechar socket em rtable_disconnect!\n");
        return -1;
    }
    free(rtable);
    return 0;
}

/* Função para adicionar uma entrada na tabela.
* Se a key já existe, vai substituir essa entrada pelos novos dados.
* Retorna 0 (OK, em adição/substituição), ou -1 (erro).
*/
int rtable_put(struct rtable_t *rtable, struct entry_t *entry) {
    if (rtable == NULL || entry == NULL) return -1;

    MessageT *msg_in, *msg_out;
    msg_out = message_init();

    // Definir o tipo de operação
    msg_out->opcode = MESSAGE_T__OPCODE__OP_PUT;
    msg_out->c_type = MESSAGE_T__C_TYPE__CT_ENTRY;
 
    // Duplicar a entrada para evitar alterações posteriores
    struct entry_t *tmp_entry = entry_duplicate(entry);

    // Alocar memória para a entrada protobuf
    EntryT *pb_entry = malloc(sizeof(EntryT));
    entry_t__init(pb_entry);

    // Copiar as referencias dos dados da entrada para a entrada protobuf
    pb_entry->key = tmp_entry->key;
    pb_entry->value.len = tmp_entry->value->datasize;
    pb_entry->value.data = tmp_entry->value->data;
    msg_out->entry = pb_entry;
    
    // Libertar memória
    free(tmp_entry->value); // deixamos o data
    free(tmp_entry);

    // Enviar mensagem e libertar memória
    msg_in = network_send_receive(rtable, msg_out);
    message_destroy(msg_out);

    if (msg_in == NULL) {
        printf("Erro: network_send_receive, não enviou resposta válida\n");
        return -1;
    }

    if (msg_in->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
        printf("Erro: server enviou opcode ERROR do rtable_put\n");
        message_destroy(msg_in);
        return -1;
    }

    if (msg_in->opcode != MESSAGE_T__OPCODE__OP_PUT + 1) {
        printf("Erro: resposta com opcode inesperado\n");
        printf("OP_CODE recebido: %d", msg_in->opcode);
        message_destroy(msg_in);
        return -1;
    }

    printf("rtable_put: Sucesso\n");
    message_destroy(msg_in);
    return 0;
}

/* Retorna a entrada da tabela com chave key, ou NULL caso não exista
* ou se ocorrer algum erro.
*/
struct block_t *rtable_get(struct rtable_t *rtable, char *key) {
    if (rtable == NULL || key == NULL) return NULL;

    MessageT *msg_in, *msg_out;
    msg_out = message_init();

    // Definir o tipo de operação
    msg_out->opcode = MESSAGE_T__OPCODE__OP_GET;
    msg_out->c_type = MESSAGE_T__C_TYPE__CT_KEY;

    // Alocar memória e copiar key para msg_out
    msg_out->key = malloc(strlen(key) + 1);
    strcpy(msg_out->key, key);

    msg_in = network_send_receive(rtable, msg_out);
    message_destroy(msg_out);

    if (msg_in == NULL) {
        printf("Erro: network_send_receive, não enviou resposta válida!\n");
        return NULL;
    }

    if (msg_in->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
        printf("Erro: server enviou opcode ERROR do rtable_get!\n");
        message_destroy(msg_in);
        return NULL;
    }

    if (msg_in->opcode != MESSAGE_T__OPCODE__OP_GET + 1) {
        printf("Erro: resposta com opcode inesperado!\n");
        printf("OP_CODE recebido: %d", msg_in->opcode);
        message_destroy(msg_in);
        return NULL;
    }

    printf("rtable_get: Sucesso\n");
    struct block_t *block = block_create(msg_in->value.len, msg_in->value.data);
    struct block_t *block_dup = block_duplicate(block);
    free(block);
    message_destroy(msg_in);
    return block_dup;
}

/* Função para remover um elemento da tabela. Vai libertar
* toda a memoria alocada na respetiva operação rtable_put().
* Retorna 0 (OK), ou -1 (chave não encontrada ou erro).
*/
int rtable_del(struct rtable_t *rtable, char *key) {
    if (rtable == NULL || key == NULL) return -1;

    MessageT *msg_in, *msg_out;
    msg_out = message_init();

    // Definir o tipo de operação
    msg_out->opcode = MESSAGE_T__OPCODE__OP_DEL;
    msg_out->c_type = MESSAGE_T__C_TYPE__CT_KEY;

    // Alocar memória para chave a apagar
    msg_out->key = malloc(strlen(key) + 1);
    strcpy(msg_out->key, key);

    // Enviar o pedido para o servidor e libertar memória usada
    msg_in = network_send_receive(rtable, msg_out);
    message_destroy(msg_out);

    if (msg_in == NULL) {
        printf("Erro: network_send_receive, não enviou resposta válida!\n");
        return -1;
    }

    if (msg_in->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
        printf("Erro: server enviou opcode ERROR do rtable_del!\n");
        message_destroy(msg_in);
        return -1;
    }

    if (msg_in->opcode != MESSAGE_T__OPCODE__OP_DEL + 1) {
        printf("Erro: resposta com opcode inesperado!\n");
        printf("OP_CODE recebido: %d", msg_in->opcode);
        message_destroy(msg_in);
        return -1;
    }

    printf("rtable_del: Sucesso!\n");
    message_destroy(msg_in);
    return 0;
}

/* Retorna o número de elementos contidos na tabela ou -1 em caso de erro.
*/
int rtable_size(struct rtable_t *rtable) {
    if (rtable == NULL) return -1;

    MessageT *msg_in, *msg_out;
    msg_out = message_init();

    // Definir o tipo de operação
    msg_out->opcode = MESSAGE_T__OPCODE__OP_SIZE;
    msg_out->c_type = MESSAGE_T__C_TYPE__CT_NONE;

    // Enviar o pedido para o servidor e libertar memória
    msg_in = network_send_receive(rtable, msg_out);
    message_destroy(msg_out);

    if (msg_in == NULL) {
        printf("Erro: network_send_receive, não enviou resposta válida\n");
        return -1;
    }

    if (msg_in->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
        printf("Erro: server enviou opcode ERROR do rtable_size\n");
        message_destroy(msg_in);
        return -1;
    }

    if (msg_in->opcode != MESSAGE_T__OPCODE__OP_SIZE + 1) {
        printf("Erro: resposta com opcode inesperado\n");
        printf("OP_CODE recebido: %d", msg_in->opcode);
        message_destroy(msg_in);
        return -1;
    }

    printf("rtable_size: Sucesso\n");
    int size = msg_in->result;
    return size;
}

/* Retorna um array de char* com a cópia de todas as keys da tabela,
* colocando um último elemento do array a NULL.
* Retorna NULL em caso de erro.
*/
char **rtable_get_keys(struct rtable_t *rtable) {
    if(rtable == NULL) return NULL;

    MessageT *msg_in, *msg_out;
    msg_out = message_init();

    // Definir o tipo de operação
    msg_out->opcode = MESSAGE_T__OPCODE__OP_GETKEYS;
    msg_out->c_type = MESSAGE_T__C_TYPE__CT_NONE;

    // Enviar o pedido para o servidor e libertar memória usada
    msg_in = network_send_receive(rtable, msg_out);
    message_destroy(msg_out);

    if (msg_in == NULL) {
        printf("Erro: network_send_receive, não enviou resposta válida");
        return NULL;
    }

    if (msg_in->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
        printf("Erro: server enviou opcode ERROR do rtable_get_keys");
        message_destroy(msg_in);
        return NULL;
    }

    if (msg_in->opcode != MESSAGE_T__OPCODE__OP_GETKEYS + 1) {
        printf("Erro: resposta com opcode inesperado");
        printf("OP_CODE recebido: %d", msg_in->opcode);
        message_destroy(msg_in);
        return NULL;
    }

    // Alocar memória para todas as keys
    char** keys = calloc(msg_in->n_keys + 1, sizeof(char*));
    int n;
    for (n = 0; n < msg_in->n_keys; n++) keys[n] = strdup(msg_in->keys[n]);
    message_destroy(msg_in);
    printf("rtable_get_keys: Sucesso");
    return keys;
}

/* Liberta a memória alocada por rtable_get_keys().
*/
void rtable_free_keys(char **keys) {
    if (keys == NULL) {
        printf("Erro: rtable_free_keys, keys a NULL");
        return;
    }

    // Percorrer array keys e libertar cada key
    int n = 0;
    while (keys[n] != NULL) {
        free(keys[n]);
        n++;
    }
    // Por fim libertar o array em si
    free(keys);
    
    printf("rtable_free_keys: Sucesso");
}

/* Retorna um array de entry_t* com todo o conteúdo da tabela, colocando
* um último elemento do array a NULL. Retorna NULL em caso de erro.
*/
struct entry_t **rtable_get_table(struct rtable_t *rtable) {
    if(rtable == NULL) return NULL;

    MessageT *msg_in, *msg_out;
    msg_out = message_init();

    // Definir o tipo de operação
    msg_out->opcode = MESSAGE_T__OPCODE__OP_GETTABLE;
    msg_out->c_type = MESSAGE_T__C_TYPE__CT_NONE;

    // Enviar o pedido para o servidor e libertar memória usada
    msg_in = network_send_receive(rtable, msg_out);
    message_destroy(msg_out);

    if (msg_in == NULL) {
        printf("Erro: network_send_receive, não enviou resposta válida");
        return NULL;
    }

    if (msg_in->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
        printf("Erro: server enviou opcode ERROR do rtable_get_keys");
        message_destroy(msg_in);
        return NULL;
    }

    if (msg_in->opcode != MESSAGE_T__OPCODE__OP_GETTABLE + 1) {
        printf("Erro: resposta com opcode inesperado");
        printf("OP_CODE recebido: %d", msg_in->opcode);
        message_destroy(msg_in);
        return NULL;
    }

    if(msg_in->n_entries <= 0) {
        printf("Erro: não existem entries!");
        return NULL;
    }

    struct entry_t **entries = calloc(msg_in->n_entries + 1, sizeof(struct entry_t*));
    int n;
    int i;
    for (n = 0; n < msg_in->n_entries; n++) {
        // Criar estrutura entry_t temporária para cópia de dados
        struct entry_t tmp_entry;
        tmp_entry.key = msg_in->entries[n]->key;
        tmp_entry.value = block_create(msg_in->entries[n]->value.len,msg_in->entries[n]->value.data);  // não duplica

        // Usar entry_duplicate para obter uma cópia da entry_t temporária e colocá-la no arr a devolver
        entries[n] = entry_duplicate(&tmp_entry);
        free(tmp_entry.value); // deixamos o data

        // Se der erro, destruir memória alocada
        // Se for o primeiro, só é necessário dar free do array e do msg_in
        if (entries[n] == NULL) {
            // Se tivermos mais entries, então limpamos as entries do início do array ao fim
            for (i = 0; i < n; i++) {
                entry_destroy(entries[i]);
            }
            free(entries);
            message_destroy(msg_in);
            return NULL;
        }
    }
    message_destroy(msg_in);
    printf("rtable_get_table: Sucesso\n");
    return entries;
}

/* Liberta a memória alocada por rtable_get_table().
*/
void rtable_free_entries(struct entry_t **entries) {
    if (entries == NULL) {
        printf("Erro: rtable_free_entries, entries a NULL");
        return;
    }

    // Percorrer array entries e libertar cada entry
    int n = 0;
    while (entries[n] != NULL) {
        entry_destroy(entries[n]);
        n++;
    }
    // Por fim libertar o array em si
    free(entries);
    printf("rtable_free_entries: Sucesso\n");
}
