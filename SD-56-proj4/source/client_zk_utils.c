#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "client_zk_utils-private.h"

static int is_zk_connected = 0;  // private

zhandle_t *zh;
char* headServerAddress = NULL;
char* tailServerAddress = NULL;


/**
 * @brief Get the head server address
 * 
 * @return char* 
 */
char* get_head_server_address() {
    return headServerAddress;
}


/**
 * @brief Get the tail server address
 * 
 * @return char* 
 */
char* get_tail_server_address() {
    return tailServerAddress;
}


void connection_watcher(zhandle_t *zzh, int type, int state, const char *path, void *context)
{
    if (type == ZOO_SESSION_EVENT)
    {
        if (state == ZOO_CONNECTED_STATE)
        {
            is_zk_connected = 1;
        }
        else
        {
            is_zk_connected = 0;
        }
    }
}


char* get_zk_node_serveraddress(char* nodeId) {
    char* serveraddress = NULL;    
    int buffer_len = 1000;
    char *buffer = malloc(1000);
    char *nodePath = malloc(strlen("/chain/") + strlen(nodeId) + 1);
    strcpy(nodePath, "/chain/");
    strcat(nodePath, nodeId);

    if (ZOK != zoo_get(zh, nodePath, 0, buffer, &buffer_len, NULL))
    {
        printf("Error getting metadata from %s\n", nodePath);
        exit(EXIT_FAILURE);
    }
    
    serveraddress = malloc(strlen(buffer));
    strcpy(serveraddress, buffer);
    free(buffer);
    free(nodePath);
    return serveraddress;
}


void set_head_tail_server_addresses(struct String_vector *children_list) {
    
    sortNodeIds(children_list);

    char* headNode = children_list->data[0];
    char* tailNode = children_list->data[children_list->count - 1];

    if (headServerAddress != NULL) free(headServerAddress);
    if (tailServerAddress != NULL) free(tailServerAddress);

    // get head and tail server information (server ip:port) from zk_nodes
    headServerAddress = get_zk_node_serveraddress(headNode);
    tailServerAddress = get_zk_node_serveraddress(tailNode);

    printf("Head server: %s\n", headServerAddress);
    printf("Tail server: %s\n", tailServerAddress);
}


static void child_watcher(zhandle_t *zh, int type, int state, const char *zpath, void *watcher_ctx)
{
    struct String_vector *children_list = (struct String_vector *)malloc(sizeof(struct String_vector)); // ver código do zoo_wget_children no github
    if (state == ZOO_CONNECTED_STATE)
    {
        if (type == ZOO_CHILD_EVENT)
        {
            /* Get the updated children and reset the watch */
            if (ZOK != zoo_wget_children(zh, "/chain", child_watcher, watcher_ctx, children_list))
            {
                fprintf(stderr, "Error setting watch at %s!\n", "/chain");
            }

            printf("Children of /chain updated!\n");

            if (children_list->count == 0)
            {
                printf("ERROR: No children in /chain!\n");
                exit(EXIT_FAILURE);
            }

            set_head_tail_server_addresses(children_list);
        }
    }
    free(children_list);
}


void client_zk_connect(char * zookeeperAddress) {
    char *watcher_ctx = "ZooKeeper Data Watcher";
    
    // setup connection to zookeeper
    zh = zookeeper_init(zookeeperAddress, connection_watcher, 2000, 0, 0, 0);

    if (zh == NULL)
    {
        fprintf(stderr, "Error connecting to ZooKeeper server[%d]!\n", errno);
        exit(EXIT_FAILURE);
    }
    sleep(5); /* Sleep a little for connection to complete */

    if (is_zk_connected == 0)
    {
        fprintf(stderr, "Error connecting to ZooKeeper server[%d]!\n", errno);
        exit(EXIT_FAILURE);
    }

    // obter a lista de children e registar watcher
    struct String_vector *children_list = (struct String_vector *)malloc(sizeof(struct String_vector)); // ver código do zoo_wget_children no github
    if (ZOK != zoo_wget_children(zh, "/chain", child_watcher, watcher_ctx, children_list))
    {
        fprintf(stderr, "Error setting watch at %s!\n", "/chain");
        exit(EXIT_FAILURE);
    }

    // determinar qual é o head e tail server
    // e atualizar as variáveis globais headServerAddress e tailServerAddress
    set_head_tail_server_addresses(children_list);

    free(children_list);
}
