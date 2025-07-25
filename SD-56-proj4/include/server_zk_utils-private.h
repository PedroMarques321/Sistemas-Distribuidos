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

#ifndef _SERVER_ZK_UTILS_PRIVATE_H
#define _SERVER_ZK_UTILS_PRIVATE_H

#include "table-private.h"
#include "common_zk_utils-private.h"

struct zkinfo_t {
    zhandle_t* zh;
    char* zookeeperAddress;     // ip:port
    char* ip_address;           // this server ip
    char* port;                 // this server port

    char* myNode;               // zookeeper id of this node
    char* nextNode;             // zookeeper id of next node (NULL if TAIL)
    char* prevNode;             // zookeeper id of previous node (NULL if HEAD)
    char* nextServerAddress;    // next server ip:port
};

extern struct zkinfo_t *g_zkinfo;

/**
 * @brief Connects to zookeeper server
 */
int server_zk_connect(struct table_t *table);

#endif