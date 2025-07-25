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

#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "server_network.h"
#include "server_network-private.h"
#include "server_skeleton.h"
#include "server_zk_utils-private.h"


int main(int argc, char **argv){ 

    short port;
    int nlists;
    int socket_de_escuta;
    char* zookeeperAddress;

    if(argc != 4){
        perror("Usage: ./server_hashtable <port> <nlists> <zookeeper address: ip:port>");
        return -1;
    }

    port = atoi(argv[1]);
    nlists = atoi(argv[2]);
    zookeeperAddress = argv[3];

    //TODO: admitir localhost
    int a, b, c, d, zkport;
    if (sscanf(argv[3], "%d.%d.%d.%d:%d", &a, &b, &c, &d, &zkport) != 5) {
        perror("Invalid zookeeper address format");
        return -1;
    }

    // inicializar a tabela
    struct table_t *table = server_skeleton_init(nlists);

    /* inicialização da camada de rede */
	if ((socket_de_escuta = server_network_init(port)) < 0){
	       	return -1;
	}

    /* inicialização do zookeeper */
    g_zkinfo->zookeeperAddress = zookeeperAddress;
    g_zkinfo->ip_address = getIPv4();
    g_zkinfo->port = argv[1];
    server_zk_connect(table);

	return network_main_loop(socket_de_escuta, table);
}