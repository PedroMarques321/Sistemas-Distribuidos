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

#include "stats.h"

struct statistics_t statistics;

/**
 *  Função que inicializa as estatísticas do servidor
 */
void stats_init() {
    statistics.client_n = 0;
    statistics.op_n = 0;
    statistics.time = 0;
}

/**
 *  Função que devolve uma cópia das estatísticas do servidor
 */
struct statistics_t stats_get() {
    struct statistics_t statistics_copy = statistics;
    return statistics_copy;
}

/**
 *  Função que incrementa o número de operações
 */
void stats_increase_op() {
    statistics.op_n++;
}

/**
 *  Função que incrementa o número de clientes
 */
void stats_increase_clients() {
    statistics.client_n++;
}

/**
 *  Função que decrementa o número de clientes
 */
void stats_decrease_clients() {
    statistics.client_n--;
}

/**
 *  Função que incrementa o tempo de execução
 */
void stats_increase_time(int time) {
    statistics.time += time;
}