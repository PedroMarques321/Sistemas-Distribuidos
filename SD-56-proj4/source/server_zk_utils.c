#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "server_zk_utils-private.h"
#include "stats-server.h"
#include "client_stub-private.h"

static int is_zk_connected = 0;  // private

static struct zkinfo_t p_zkinfo = {0}; // private
struct zkinfo_t *g_zkinfo = &p_zkinfo; // public

extern pthread_mutex_t mutex; // mutex partilhado por todas as threads (contexto global)
extern int send_table_to_next_server(struct table_t *table); // extern porque está no server_network.c


/**
 * 
 */
void setNodePointers(struct String_vector *idList)
{
    int found = 0;

    printf("Node list before sort:\n");
    for (int i = 0; i < idList->count; i++)
    {
        printf("idList->data[%d]: %s\n", i, idList->data[i]);
    }
    
    sortNodeIds(idList); // garantir que a lista está ordenada
    
    printf("Node list after sort:\n");
    for (int i = 0; i < idList->count; i++)
    {
        printf("idList->data[%d]: %s\n", i, idList->data[i]);
    }

    for (int i = 0; i < idList->count; i++)
    {
        //printf("idList->data[%d]: %s\n", i, idList->data[i]);

        if (strcmp(idList->data[i], g_zkinfo->myNode) == 0)
        {
            // para garantir que a atualização da informação da posição do nó na lista é feita de forma atómica
            // porque atualização desta informação despoleta a re-conexão das threads de cliente ao longo da cadeia
            if (i == 0)
            {
                g_zkinfo->prevNode = NULL;
            }
            else
            {
                g_zkinfo->prevNode = idList->data[i - 1];
            }
            if (i == idList->count - 1)
            {
                g_zkinfo->nextNode = NULL;
                if (g_zkinfo->nextServerAddress != NULL) {
                    free(g_zkinfo->nextServerAddress);
                    g_zkinfo->nextServerAddress = NULL;
                }
            }
            else
            {
                g_zkinfo->nextNode = idList->data[i + 1];

                // get next server information (server ip:port)
                int buffer_len = 1000;
                char *buffer = malloc(1000);
                char *nextNodePath = malloc(strlen("/chain/") + strlen(g_zkinfo->nextNode) + 1);
                strcpy(nextNodePath, "/chain/");
                strcat(nextNodePath, g_zkinfo->nextNode);                

                if (ZOK != zoo_get(g_zkinfo->zh, nextNodePath, 0, buffer, &buffer_len, NULL))
                {
                    printf("Error getting metadata from %s\n", nextNodePath);
                    exit(EXIT_FAILURE);
                }
                if (g_zkinfo->nextServerAddress != NULL)
                    free(g_zkinfo->nextServerAddress);
                g_zkinfo->nextServerAddress = malloc(strlen(buffer));
                strcpy(g_zkinfo->nextServerAddress, buffer);
                free(buffer);
                free(nextNodePath);
            }
            found = 1;
            break;
        }
    }

    if (found == 0) {
        printf("Error: node id not found in list!\n");
        printf("%s\n", g_zkinfo->myNode);
        printf("\n");
        for (int i = 0; i < idList->count; i++)
        {
            printf("%s\n", idList->data[i]);
        }
        printf("\n");
        exit(EXIT_FAILURE);
    }
}


/**
 * 
 */
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


static void update_zk_chain_info(struct String_vector *children_list) {
    if (children_list->count == 0)
    {
        printf("ERROR: No children in /chain!\n");
        exit(EXIT_FAILURE);
    }

    setNodePointers(children_list);

    // para debug
    for (int i = 0; i < children_list->count; i++)
    {
        printf("Child %d: %s\n", i, children_list->data[i]);
    }

    printf("\n");
    printf("My node: %s\n", g_zkinfo->myNode);
    if (g_zkinfo->prevNode != NULL)
    {
        printf("Prev node: %s\n", g_zkinfo->prevNode);
    }
    else
    {
        printf("I am HEAD\n");
    }
    if (g_zkinfo->nextNode != NULL)
    {
        printf("Next node: %s\n", g_zkinfo->nextNode);
        printf("Next server: %s\n", g_zkinfo->nextServerAddress);
    }
    else
    {
        printf("I am TAIL\n");
    }

    if (g_zkinfo->nextNode != NULL && g_zkinfo->prevNode != NULL)
    {
        printf("I am MIDDLE\n");
    }
}


/**
 * Data Watcher function for this node
 * To be run if there is a change in /chain children
 * watcher_ctx vai ser o apontador para a table
 */
static void child_watcher(zhandle_t *zh, int type, int state, const char *zpath, void *watcher_ctx)
{
    printf("CHILD WATCHER\n");
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

            // enviar um size para todos os clientes para desbloquear o read e se porem à espera da atualização da cadeia
            for (int i = 0; i < stats_get_clients(); i++)
            {
            }

            pthread_mutex_lock(&mutex);   // obter o mutex
            #ifdef DEBUG_MESSAGES
            printf("a atualizar informação de cadeia...\n");
            #endif
            int IwasTAIL = g_zkinfo->nextNode == NULL ? 1 : 0;
            update_zk_chain_info(children_list);

            // se eu era TAIL e deixei de ser
            if (IwasTAIL == 1 && g_zkinfo->nextNode != NULL) {
                // função que se liga o TAIL e faz put de toda a tabela
                send_table_to_next_server((struct table_t *)watcher_ctx);
            }
            pthread_mutex_unlock(&mutex);  // libertar o mutex
        }
    }
    free(children_list);
}


/**
 * para correr quando o servidor inicia
 */
int server_zk_connect(struct table_t *table) {

    char *watcher_ctx = (char*)table;
    
    // setup connection to zookeeper
    g_zkinfo->zh = zookeeper_init(g_zkinfo->zookeeperAddress, connection_watcher, 2000, 0, 0, 0);

    if (g_zkinfo->zh == NULL)
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


    // CREATE ROOT NODE IF NEEDED: /chain
    if (ZNONODE == zoo_exists(g_zkinfo->zh, "/chain", 0, NULL))
    {
        fprintf(stderr, "/chain does not exist! Creating...\n");
        fflush(stderr);
        if (ZOK != zoo_create(g_zkinfo->zh, "/chain", "chain node", 11 /*10+1*/, &ZOO_OPEN_ACL_UNSAFE, ZOO_PERSISTENT, NULL, 0 /*no new name because no sequence*/))
        {
            fprintf(stderr, "Error creating znode /chain !\n");
            exit(EXIT_FAILURE);
        }
        printf("/chain created!\n");
        fflush(stdout);
    }

    int chain_concluded = 0;
    for (int i = 0; i < 5; i++)
    {
        sleep(3);
        if (ZNONODE != zoo_exists(g_zkinfo->zh, "/chain", 0, NULL))
        {
            chain_concluded = 1;
            break;
        }
    }
    if (!chain_concluded)
    {
        fprintf(stderr, "Error creating znode /chain verif.!\n");
        exit(EXIT_FAILURE);
    }

    // ADD THIS SERVER TO THE CHAIN (CREAT CHILD NODE)
    int slen = 128;
    char* zkServerNodePath = malloc(slen * sizeof(char));
    printf("Creating child node /chain/node...\n");
    printf("ip_address: %s\n", g_zkinfo->ip_address);
    printf("port: %s\n", g_zkinfo->port);
 
    fflush(stdout);
    char *this_ip_port = malloc((strlen(g_zkinfo->ip_address) + 1 + strlen(g_zkinfo->port) + 1) * sizeof(char));
    strcpy(this_ip_port, g_zkinfo->ip_address);
    strcat(this_ip_port, ":");
    strcat(this_ip_port, g_zkinfo->port); 

    // we will not free [this_ip_port] because we do not know how it is used in zoo_create
    int z_result = zoo_create(g_zkinfo->zh, "/chain/node", this_ip_port, strlen(this_ip_port)+1, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL_SEQUENTIAL, zkServerNodePath, slen);
    if (ZOK != z_result)
    {
        fprintf(stderr, "Error creating znode /chain/node -> %s !\n", zkServerNodePath);
        if (z_result == ZNODEEXISTS)
            fprintf(stderr, "ZNODEEXISTS\n");
        if (z_result == ZNOCHILDRENFOREPHEMERALS)
            fprintf(stderr, "ZNOCHILDRENFOREPHEMERALS\n");
        exit(EXIT_FAILURE);
    }
    printf("Ephemeral Sequencial ZNode created! ZNode path: %s\n", zkServerNodePath);

    int node_concluded = 0;
    for (int i = 0; i < 5; i++)
    {
        sleep(5);
        if (ZNONODE != zoo_exists(g_zkinfo->zh, zkServerNodePath, 0, NULL))
        {
            node_concluded = 1;
            break;
        }
    }
    if (!node_concluded)
    {
        fprintf(stderr, "Error creating znode %s !\n", zkServerNodePath);
        exit(EXIT_FAILURE);
    }

    // /chain/node0000000000
    char* tmpStrPtr = zkServerNodePath + strlen("/chain/");
    g_zkinfo->myNode = malloc((strlen(tmpStrPtr) + 1) * sizeof(char));
    strcpy(g_zkinfo->myNode, tmpStrPtr);
    free(zkServerNodePath);

    sleep(2); /* Sleep a little for children_list to complete */
    struct String_vector *children_list = (struct String_vector *)malloc(sizeof(struct String_vector)); // ver código do zoo_wget_children no github
    if (ZOK != zoo_wget_children(g_zkinfo->zh, "/chain", child_watcher, watcher_ctx, children_list))
    {
        fprintf(stderr, "Error setting watch at %s!\n", "/chain");
        exit(EXIT_FAILURE);
    }
    update_zk_chain_info(children_list);
    free(children_list);

    return 0;
}


