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
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include "server_skeleton.h"
#include "stats-server.h"


/* Inicia o skeleton da tabela. * O main() do servidor deve chamar esta função antes de poder usar a * função invoke(). O parâmetro n_lists define o número de listas a
* serem usadas pela tabela mantida no servidor.
* Retorna a tabela criada ou NULL em caso de erro. */ 
struct table_t *server_skeleton_init(int n_lists)  {
    return table_create(n_lists);
}

/* Liberta toda a memória ocupada pela tabela e todos os recursos
* e outros recursos usados pelo skeleton.
* Retorna 0 (OK) ou -1 em caso de erro. */
int server_skeleton_destroy(struct table_t *table) {
    return table_destroy(table);
}

/**
 * put
 * PEDIDO:   OP_PUT     CT_ENTRY  <entry>
 * RESPOSTA: OP_PUT+1   CT_NONE | OP_ERROR CT_NONE
 */
void op_put(MessageT *msg, struct table_t *table) {
    int result;
    struct block_t *block;

    if (msg == NULL || table == NULL)
    {
        perror("Mensagem a null");
        msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
        msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
        return;
    }

    // Output no server da key, para debug
    #ifdef OUTPUT_INFO
    printf("OP_PUT: key = %s\n", msg->entry->key);
    #endif

    block = block_create(msg->entry->value.len, msg->entry->value.data);
    result = table_put(table, msg->entry->key, block); // nota: copia dados
    free(block); // libertar bloco sem destruir dados

    if (result == -1)
    {   
        msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
        msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
        return;
    }

    msg->opcode++;
    msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
}

/**
 * get
 * PEDIDO:   OP_GET     CT_KEY <key>
 * RESPOSTA: OP_GET+1   CT_VALUE <value> | OP_ERROR CT_NONE
 */
void op_get(MessageT *msg, struct table_t *table) {
    struct block_t *block;

    if (msg == NULL || table == NULL)
    {
        perror("Mensagem a null");
        msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
        msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
        return;
    }


    // Output no server da key, para debug
    #ifdef OUTPUT_INFO
    printf("OP_GET: key = %s\n", msg->key);
    #endif

    block = table_get(table, msg->key);  // copia

    if (block == NULL)
    {
        msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
        msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
        return;
    }

    msg->opcode++;
    msg->c_type = MESSAGE_T__C_TYPE__CT_VALUE;
    msg->value.len = block->datasize;
    msg->value.data = block->data;
}

/**
 * del
 * PEDIDO:   OP_DEL     CT_KEY <key>
 * RESPOSTA: OP_DEL+1   CT_NONE | OP_ERROR CT_NONE
 */
void op_del(MessageT *msg, struct table_t *table) {
    int result;

    if (msg == NULL || table == NULL)
    {
        perror("Mensagem a null");
        msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
        msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
        return;
    }

    // Output no server da key, para debug
    #ifdef OUTPUT_INFO
    printf("OP_DEL: key = %s\n", msg->key);
    #endif

    result =  table_remove(table, msg->key);

    if (result == -1)
    {
        msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
        msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
        return;
    }

    msg->opcode++;
    msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
}

/**
 * size
 * PEDIDO:   OP_SIZE    CT_NONE
 * RESPOSTA: OP_SIZE+1 CT_RESULT <size> | OP_ERROR CT_NONE
 */
void op_size(MessageT *msg, struct table_t *table) {
        int size;

        if (msg == NULL || table == NULL)
        {
            perror("Mensagem a null em op_size");
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return;
        }
        
        size = table_size(table);
        printf("Table size is: %d\n", size);  // debug
        msg->opcode++;
        msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
        msg->result = size;
}


/**
 * getkeys
 * PEDIDO:   OP_GETKEYS CT_NONE
 * RESPOSTA: OP_GETKEYS+1 CT_KEYS <keys>
 */
void op_getkeys(MessageT *msg, struct table_t *table) {
    char **keys;

    if (msg == NULL || table == NULL)
    {
        perror("Mensagem ou table a NULL em op_getkeys");
        msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
        msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
        return;
    }

    keys = table_get_keys(table);
    
    if (keys == NULL)
    {
        msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
        msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
        return;
    }    

    int size = table_size(table);

    msg->opcode++;
    msg->c_type = MESSAGE_T__C_TYPE__CT_KEYS;
    msg->keys = malloc(sizeof(char*) * size);
    for (int i = 0; i<size; i++)
    {
        msg->keys[i] = keys[i]; // só os apoontadores
    }
    msg->n_keys = size; // o NULL não pode ir devido ao sistema de empacotamento do protobuf
    free(keys); // libertar array de keys
}


/**
 * gettable
 * PEDIDO:   OP_GETTABLE CT_NONE
 * RESPOSTA: OP_GETTABLE+1 CT_TABLE <entries>
 */
void op_gettable(MessageT *msg, struct table_t *table) {
    char **keys;
    int tsize;
    struct block_t *block;

    if (msg == NULL || table == NULL)
    {
        perror("Mensagem ou table a NULL em op_gettable");
        msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
        msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
        return;
    }

    keys = table_get_keys(table); //todas as keys na table (tsize + 1)
    tsize = table_size(table);
    msg->n_entries = tsize;
    msg->entries = malloc(sizeof(EntryT*) * tsize); 

    for (int i = 0; i < tsize; i++) {
        msg->entries[i] = malloc(sizeof(EntryT));
        entry_t__init(msg->entries[i]);
        msg->entries[i]->key = keys[i];
        block = table_get(table, keys[i]); // cópia => dados seguros
        msg->entries[i]->value.len = block->datasize;
        msg->entries[i]->value.data = block->data;
        free(block); // libertar bloco sem destruir dados
    }

    msg->opcode++;
    msg->c_type = MESSAGE_T__C_TYPE__CT_TABLE;
}

/**
 * stats
 * PEDIDO: OP_STATS CT_NONE
 * RESPOSTA: OP_STATS+1 CT_STATS <stats>
 */
void op_stats(MessageT *msg) {
    if (msg == NULL)
    {
        perror("Mensagem a NULL em op_stats");
        msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
        msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
    }
    
    struct statistics_t stats = stats_get();

    msg->opcode++;
    msg->c_type = MESSAGE_T__C_TYPE__CT_STATS;
    msg->clients = stats.client_n;
    msg->operations = stats.op_n;
    msg->time = stats.time;
}

/**
 * Função responsável por executar uma operação e medir o tempo de execução, corremos a operação que queremos e medimos o tempo de execução nesta função
 * void (*fun_ptr)(MessageT *msg, struct table_t *table) - ponteiro para a função a ser executada que tem como parâmetros uma MessageT e uma table_t
 * Para evitar código repetido
 */
void exec_op(void (*fun_ptr)(MessageT *msg, struct table_t *table), MessageT *msg, struct table_t *table) {
    struct timeval time0;
    struct timeval time1;
    gettimeofday(&time0, NULL);
    fun_ptr(msg, table);
    gettimeofday(&time1, NULL);
    stats_increase_time((time1.tv_sec*1000000 + time1.tv_usec) - (time0.tv_sec*1000000 + time0.tv_usec));
    stats_increase_op();
}


/* Executa na tabela table a operação indicada pelo opcode contido em msg
* e utiliza a mesma estrutura MessageT para devolver o resultado. * Retorna 0 (OK) ou -1 em caso de erro. */ 
int invoke(MessageT *msg, struct table_t *table) {


    // NÃO ESQUECER!!!!! SÓ PARA TESTES!!!! COMENTAR ANTES DE ENTREGAR!!!!
    //useconds_t usecs = 1000000;  // 1 segundo
    //usleep(usecs);

    //printf("INVOKE OPCODE: %d\n", msg->opcode);
    
    if (msg == NULL)
    {
        perror("Mensagem a null no invoke");
        return -1;
    }

    switch (msg->opcode)
    {
        case MESSAGE_T__OPCODE__OP_PUT:
            exec_op(op_put, msg, table);
            return 0;
            break;

        case MESSAGE_T__OPCODE__OP_GET:
            exec_op(op_get, msg, table);
            return 0;
            break;
        
        case MESSAGE_T__OPCODE__OP_DEL:
            exec_op(op_del, msg, table);
            return 0;
            break;

        case MESSAGE_T__OPCODE__OP_SIZE:
            exec_op(op_size, msg, table);
            return 0;
            break;
        
        case MESSAGE_T__OPCODE__OP_GETKEYS:
            exec_op(op_getkeys, msg, table);
            return 0;
            break;
        
        case MESSAGE_T__OPCODE__OP_GETTABLE:
            exec_op(op_gettable, msg, table);
            return 0;
            break;
        
        case MESSAGE_T__OPCODE__OP_STATS:
            op_stats(msg);
            return 0;
            break;

        default:
            return -1;
    }

    return -1;
}

