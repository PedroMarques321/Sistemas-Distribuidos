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
#include <arpa/inet.h>
#include "serialization.h"

/* Serializa todas as chaves presentes no array de strings keys para o
 * buffer keys_buf, que será alocado dentro da função. A serialização
 * deve ser feita de acordo com o seguinte formato:
 *    | int   | string | string | string |
 *    | nkeys | key1   | key2   | key3   |
 * Retorna o tamanho do buffer alocado ou -1 em caso de erro.
 */
int keyArray_to_buffer(char **keys, char **keys_buf) {
    if (keys == NULL || keys_buf == NULL) return -1;  // Verificação de erro

    int nkeys = 0;
    while (keys[nkeys] != NULL) {
        nkeys++;
    }

    int size = sizeof(int);  // Tamanho para armazenar o número de chaves

    for (int i = 0; i < nkeys; i++) {
        size += strlen(keys[i]) + 1;  // Adicionar tamanho da string + 1 byte
    }

    // Aloca memória para o buffer
    *keys_buf = (char *)malloc(size);
    if (*keys_buf == NULL) return -1;  // Erro

    char *curr = *keys_buf;

    // Copia o número de chaves para o buffer
    int nkeys_network = htonl(nkeys);
    memcpy(curr, &nkeys_network, sizeof(int));
    curr += sizeof(int);

    // Copia cada chave para o buffer
    for (int i = 0; i < nkeys; i++) {
        strcpy(curr, keys[i]);
        curr += strlen(keys[i]) + 1;
    }

    return size;
};

/* De-serializa a mensagem contida em keys_buf, colocando-a num array de
 * strings cujo espaco em memória deve ser reservado. A mensagem contida
 * em keys_buf deverá ter o seguinte formato:
 *    | int   | string | string | string |
 *    | nkeys | key1   | key2   | key3   |
 * Retorna o array de strings ou NULL em caso de erro.
 */
char** buffer_to_keyArray(char *keys_buf) {
    if (keys_buf == NULL) return NULL;  // Verificação de erro

    // Lê o número de chaves do buffer
    int nkeys_network;
    memcpy(&nkeys_network, keys_buf, sizeof(int));
    int nkeys = ntohl(nkeys_network);

    char **keys = (char **)malloc((nkeys + 1) * sizeof(char *));  // Alocar memória para o array de strings
    if (keys == NULL) return NULL;  // Erro

    char *curr = keys_buf + sizeof(int);

    // Copia cada chave para o array
    for (int i = 0; i < nkeys; i++) {
        keys[i] = strdup(curr);
        if (keys[i] == NULL) {
            // Erro, desalocar memória
            for (int j = 0; j < i; j++) {
                free(keys[j]);
            }
            free(keys);
            return NULL;
        }
        curr += strlen(keys[i]) + 1;
    }

    keys[nkeys] = NULL;

    return keys;
};
