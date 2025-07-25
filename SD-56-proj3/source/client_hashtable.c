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
#include <unistd.h>
#include "inet-private.h"
#include "client_stub-private.h"
#include "message-private.h"
#include "table.h"
#include "stats.h"

#define MAX_COMMAND_LENGTH 2048

void print_commands() {
    printf("\n");
    printf("COMANDOS DISPONÍVEIS:\n");
    printf("put <key> <data>\n");
    printf("get <key>\n");
    printf("del <key>\n");
    printf("size\n");
    printf("getkeys\n");
    printf("gettable\n");
    printf("stats\n");
    printf("quit\n");
}

int main(int argc, char **argv) {
    char *address_port;
    int arguments_ok = 1;
    int i;
    int iaddrn[5] = {0};
    if (argc != 2) arguments_ok = 0;

    if(arguments_ok){
        if (sscanf(argv[1], "%d.%d.%d.%d:%d", &iaddrn[0], &iaddrn[1], &iaddrn[2], &iaddrn[3],&iaddrn[4]) != 5) arguments_ok = 0;
    }

    if (!arguments_ok) {
        printf("USAGE: client_hashtable <ip_address:port>\n");
        return -1;
    }
    
    address_port = argv[1];
    struct rtable_t *rtable = rtable_connect(address_port);

    if (rtable == NULL) {
        printf("ERRO: rtable_connect\n");
        return -1;
    }

    // estamos ligados. vamos entrar no ciclo de comandos

    char command[MAX_COMMAND_LENGTH];
    char *cp;
    char *key;
    char *value;
    char **keys, **itk;
    struct block_t *block;
    struct entry_t *entry;
    char *str;
    struct entry_t **table, **ite;
    struct statistics_t *statistics;
    int opn, nclients, time;
    
    while (1) {

        memset(command, 0, MAX_COMMAND_LENGTH);
        print_commands();

        printf(">>> ");
        if (fgets(command, MAX_COMMAND_LENGTH, stdin) == NULL) {
            perror("Erro a ler o comando.\n");
            exit(1);
        }
        
        cp = strtok(command, " \n");
        if (cp == NULL) {
            print_commands();
            continue;
        }

        // Comando PUT
        if (strcmp(cp, "put") == 0) {
            key = strtok(NULL, " ");
            value = strtok(NULL, "\n");
            if (key == NULL || value == NULL) {
                printf("Faltam um ou mais argumentos no comando. Por favor tente de novo!\n");
                continue;
            }
            else {
                key = strdup(key);
                value = strdup(value);
                block = block_create(strlen(value), value);
                if (block == NULL) {
                    perror("PUT: Erro a criar block_t!\n");
                    free(key);
                    return -1;
                }
                entry = entry_create(key, block);
                if (entry == NULL) {
                    perror("PUT: Erro o a criar entry!\n");
                    if (block != NULL) block_destroy(block);
                    free(key);
                    return -1;
                }
                if (rtable_put(rtable, entry) == -1){
                    perror("Erro no rtable_put!\n");
                    continue;
                }
                printf("Comando executado com sucesso!\n");
                entry_destroy(entry);
            }
            continue;
        }

        

        // Comando GET
        if (strcmp(cp, "get") == 0) {
            key = strtok(NULL, "\n");
            if (key == NULL) {
                printf("Insira uma key no comando get. Por favor tente de novo!\n");                
                continue;
            } 
            else {
                block = rtable_get(rtable, key);
                if(block == NULL) {
                    printf("Erro no rtable_get \n");
                    continue;
                }
                printf("block size = %d\n", block->datasize);
                str = as_printable(block->data, block->datasize);
                printf("%s\n", str);
                free(str);
                block_destroy(block);
            }
            continue;
        }

        // Comando DEL
        if (strcmp(cp, "del") == 0) {
            int i;
            key = strtok(NULL, "\n");
            if (key == NULL) {
                printf("Insira uma key no comando del. Por favor tente de novo!\n");
                continue;
            } 
            else {
                i = rtable_del(rtable, key);
                if (i == -1) {
                    printf("Erro no rtable_del!\n");
                    continue;
                }
                printf("Sucesso, elemento removido da tabela!\n");
            }
            continue;
        }

        // Comando SIZE
        if (strcmp(cp, "size") == 0) {
            i = rtable_size(rtable);
            if (i == -1) printf("Error on rtable_size!\n");
            printf("Table size = %d\n", i);
            continue;
        }

        // Comando GETKEYS
        if (strcmp(cp, "getkeys") == 0) {
            keys = rtable_get_keys(rtable);
            if (keys == NULL) {
                printf("Erro no rtable_get_keys!\n");
                continue;
            }
            itk = keys;
            printf("Keys:\n");
            while (*itk != NULL) {
                printf("%s\n", *itk);
                itk++;
            }
            rtable_free_keys(keys);
            continue;
        }

        // Comando GETTABLE
        if (strcmp(cp, "gettable") == 0) {
            table = rtable_get_table(rtable);
            if (table == NULL) {
                printf("Erro no rtable_get_table!\n");
                continue;
            }
            
            ite = table;
            while (*ite != NULL) {
                str = as_printable((*ite)->value->data, (*ite)->value->datasize);
                printf("Key: %s\nConteúdo: %s\nTamanho: %d\n\n", (*ite)->key, str,(*ite)->value->datasize);
                ite++;
            }
            free(str);
            rtable_free_entries(table);
            continue;
        }

        // Comando STATS
        if(strcmp(cp, "stats") == 0) {
            statistics = rtable_stats(rtable);
            if (statistics == NULL) {
                printf("Erro no rtable_stats!\n");
                continue;
            }
            nclients = statistics->client_n;
            opn = statistics->op_n;
            time = statistics->time;
            printf("Número de clientes: %d\nNúmero de operações: %d\nTempo de execução total (em microsegundos): %d\n\n", nclients, opn, time);
            continue;
        }
        
        // Comando QUIT
        if (strcmp(cp, "quit") == 0) {
            return rtable_disconnect(rtable);
        }
        printf("Comando não reconhecido, por favor tente outra vez!\n");
    }
    exit(0);
}