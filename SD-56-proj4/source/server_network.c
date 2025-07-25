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

#include "inet-private.h"
#include "message-private.h"

#include <poll.h>

#include "server_network.h"
#include "server_network-private.h"
#include "server_skeleton.h"
#include "stats-server.h"
#include "server_zk_utils-private.h"

#include "client_network.h"
#include "client_stub-private.h"

#include <pthread.h>

#define BACKLOG 20 /* how many pending connections queue will hold */
#define NFDESC 500 // Número de sockets (uma para listening)
#define TIMEOUT 1000 // em milisegundos

int exit_loop = 0;
int threadId_contador = 1; // "privada"

int socketsfd_de_cliente[NFDESC] = {0}; // array de sockets dos clientes

void exit_server_and_thread_loops_graciously() {
    exit_loop = 1;
}


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // mutex partilhado por todas as threads (contexto global)

// tipo privado - só para este módulo
struct thread_parameters {
    unsigned int threadId;
    int socketfd_cliente;
    struct table_t *table;
    char **ext_nextServerAddress;
};

/* Função para preparar um socket de receção de pedidos de ligação
* num determinado porto.
* Retorna o descritor do socket ou -1 em caso de erro.
*/
int server_network_init(short port) {
    struct addrinfo hints, *res, *rp;
    int status, sfd;
    char sport[10];
    int yes = 1;
    
    sprintf(sport, "%d", port);
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; /*fill in host IP*/
    
    if ((status = getaddrinfo(NULL, sport, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return -1;
    }
    
    /* 
     * DO MANUAL DE getaddrinfo:
     * getaddrinfo() returns a list of address structures.
      Try each address until we successfully bind(2).
      If socket(2) (or bind(2)) fails, we (close the socket
      and) try the next address. */
    
    for (rp = res; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1)
            continue; /* failed */
           
        if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
            break; /* Success */

        close(sfd);
    }

    if (rp == NULL) {
        fprintf(stderr, "ERROR: could not bind!");
        return -1;
    }
    
    freeaddrinfo(res);   /* No longer needed */
    
    if (listen(sfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    };

    return sfd; //sfd e o descritor da socket
}



/**
 * Função que vai tratar dos pedidos do cliente, cada thread(cliente) vai correr esta função e 
 * permanece ativa enquanto o cliente estiver conectado, para garantir consistência dos dados,
 * utilizamos mutex para sincronizar o acesso às variáveis globais em operações críticas (operações que modificam a tabela e as estatísticas).
 * A thread é fechada quando o cliente se desconecta, quando há um erro de comunicação, ou quando o servidor termina.
 */
void * conn_thread_main(void *params) {

    struct thread_parameters *tpar;
    MessageT *message;
    char *buffer;
    size_t buf_size;
    char *nextServerAddress = NULL;  //thread local
    struct rtable_t *rtable_cli = NULL; // dados de ligação desta thread ao próximo servidor (simular cliente)
    
    tpar = (struct thread_parameters *) params; // id, socket de cliente e ponteiro para a tabela

    //printf("sockedfd_cliente: %d\n", tpar->socketfd_cliente);

    while (!exit_loop)
    {
        printf("begin read in thread %d\n", tpar->threadId);
        message = network_receive(tpar->socketfd_cliente);
        if(message == NULL) {

            //eliminar socket do registo
            for (int i = 0; i < NFDESC; i++) {
                if (socketsfd_de_cliente[i] == tpar->socketfd_cliente) {
                    socketsfd_de_cliente[i] = 0;
                    break;
                }
            }

            printf("mensagem inválida: fechar cliente (terminar thread)\n");
            close(tpar->socketfd_cliente);

            // terminar ligação cliente
            if (rtable_cli != NULL) {
                rtable_disconnect(rtable_cli);
                rtable_cli = NULL;
            }
            
            pthread_mutex_lock(&mutex);
            // SECÇÃO CRÍTICA
            stats_decrease_clients();  
            pthread_mutex_unlock(&mutex);
            
            pthread_exit((void*) 0);
            return NULL;
        }

        // verificar se o proximo servidor na cadeia foi atualizado
        pthread_mutex_lock(&mutex);        

        if (*(tpar->ext_nextServerAddress) == NULL) {
            // o próximo servidor foi removido
            if (rtable_cli != NULL) {
                rtable_disconnect(rtable_cli);
                rtable_cli = NULL;
            }
            if (nextServerAddress != NULL) {
                free(nextServerAddress);
                nextServerAddress = NULL;
            }
        }
        else if ((*(tpar->ext_nextServerAddress) != NULL && nextServerAddress == NULL) ) {
            // foi atualizado!
            #ifdef DEBUG_MESSAGES 
            printf("nextServerAddress antigo: NULL");
            printf("nextServerAddress atualizado para %s\n", nextServerAddress);
            #endif
            nextServerAddress = malloc(strlen(*(tpar->ext_nextServerAddress) + 1));
            strcpy(nextServerAddress, *(tpar->ext_nextServerAddress));
            rtable_cli = rtable_connect(nextServerAddress);
        } else if (strcmp(*(tpar->ext_nextServerAddress), nextServerAddress) != 0) {
            // foi atualizado!
            #ifdef DEBUG_MESSAGES 
            printf("nextServerAddress antigo: %s\n", nextServerAddress);
            printf("nextServerAddress atualizado para %s\n", nextServerAddress);
            #endif
            free(nextServerAddress);
            nextServerAddress = malloc(strlen(*(tpar->ext_nextServerAddress) + 1));
            strcpy(nextServerAddress, *(tpar->ext_nextServerAddress));
            rtable_disconnect(rtable_cli);
            rtable_cli = rtable_connect(nextServerAddress);
        }
        pthread_mutex_unlock(&mutex);

        // mensagem válida

        // processamento da mensagens com implicações em termos de concorrência
        if (message->opcode == MESSAGE_T__OPCODE__OP_PUT 
            || message->opcode == MESSAGE_T__OPCODE__OP_DEL
            || message->opcode == MESSAGE_T__OPCODE__OP_GET
            || message->opcode == MESSAGE_T__OPCODE__OP_GETKEYS
            || message->opcode == MESSAGE_T__OPCODE__OP_GETTABLE
            || message->opcode == MESSAGE_T__OPCODE__OP_SIZE) {
            
            pthread_mutex_lock(&mutex);
            // SECÇÃO CRÍTICA

            printf("operacao com OPCODE %d na thread %d\n", message->opcode, tpar->threadId);

            // enviar mensagem ao proximo servidor
            if (rtable_cli != NULL) {
                printf("enviar mensagem ao proximo servidor na thread %d\n", tpar->threadId);
                MessageT* msg_in = network_send_receive(rtable_cli, message);
                
                if (msg_in->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
                    printf("Erro: server enviou opcode ERROR do rtable_put\n");
                    message_destroy(msg_in);
                    
                    message->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                }
            }

            // se o servior seguinte tiver retornado mensagem com erro, não é necessário invocar e
            // segue em frente com a mensagem deste servidor com opcode OP_ERROR
            if (message->opcode != MESSAGE_T__OPCODE__OP_ERROR) {
                if (invoke(message, tpar->table) == -1) {
                    printf("Error in invoke!");
                }
            }
            pthread_mutex_unlock(&mutex);
        }
        else {
            // não é necessário mutex
            if (invoke(message, tpar->table) == -1) {
                printf("Error in invoke!");
            }
        }        

        // resposta
        buf_size = message_to_buffer((void *)&buffer, message);
        printf("envio de resposta nat thread %d\n", tpar->threadId);
        if (write_all(tpar->socketfd_cliente, buffer, buf_size) != buf_size) {
            perror("Erro no envio da resposta.");
            free(buffer);

            //eliminar socket do registo
            for (int i = 0; i < NFDESC; i++) {
                if (socketsfd_de_cliente[i] == tpar->socketfd_cliente) {
                    socketsfd_de_cliente[i] = 0;
                    break;
                }
            }

            printf("closing client socket por haver erro no envio da resposta\n");
            close(tpar->socketfd_cliente);

            pthread_mutex_lock(&mutex);
            // SECÇÃO CRÍTICA
            stats_decrease_clients();  
            pthread_mutex_unlock(&mutex);
            

            pthread_exit((void*) 0);
            return NULL;
        }
        free(buffer);
    }
    

    pthread_mutex_lock(&mutex);
    // SECÇÃO CRÍTICA
    stats_decrease_clients();  
    pthread_mutex_unlock(&mutex);

    return NULL;
}

/* A função network_main_loop() deve:
* - Aceitar uma conexão de um cliente;
* - Receber uma mensagem usando a função network_receive;
* - Entregar a mensagem de-serializada ao skeleton para ser processada
na tabela table;
* - Esperar a resposta do skeleton;
* - Enviar a resposta ao cliente usando a função network_send.
* A função não deve retornar, a menos que ocorra algum erro. Nesse
* caso retorna -1.
*/
int network_main_loop(int listening_socket, struct table_t *table) {

    socklen_t sin_size;
    struct sockaddr_storage their_addr; // connector's address information
    int socketfd_cliente = 0;
    struct thread_parameters* tpar;
    pthread_t thread;

    printf("INCIAR O CICLO DO SERVIDOR\n");
    stats_init(); // inicializar estatísticas

    while (!exit_loop) {
        
        sin_size = sizeof their_addr;
        socketfd_cliente = accept(listening_socket, (struct sockaddr *)&their_addr, &sin_size);

        if (socketfd_cliente == -1) {
            printf("Erro ao aceitar ligação de cliente\n");
            continue;
        }

        printf("Criar thread para o cliente com threadId =  %d\n", threadId_contador); 
        /* criação de nova thread */
        tpar = malloc(sizeof(struct thread_parameters));
        tpar->threadId = threadId_contador++;
        tpar->socketfd_cliente = socketfd_cliente;
        tpar->table = table;
        tpar->ext_nextServerAddress = &(g_zkinfo->nextServerAddress);
        if (pthread_create(&thread, NULL, &conn_thread_main, (void *) tpar) != 0){
            printf("\nThread não criada. Saindo graciosamente\n");
            exit_loop = 1;
            return -1;
        }

        // registar o socket do cliente
        for (int i = 0; i < NFDESC; i++) {
            if (socketsfd_de_cliente[i] == 0) {
                socketsfd_de_cliente[i] = socketfd_cliente;
                break;
            }
        }

        stats_increase_clients();  // não requer secção crítica pois só esta thread altera o valor
        // TODO: monitorizar defuncts
    }

    return -1;  // se retornar é porque houve erro
}

/* A função network_receive() deve:
* - Ler os bytes da rede, a partir do client_socket indicado;
* - De-serializar estes bytes e construir a mensagem com o pedido,
* reservando a memória necessária para a estrutura MessageT.
* Retorna a mensagem com o pedido ou NULL em caso de erro.
*/
MessageT *network_receive(int client_socket) {
    uint32_t read_size_netlong;
    uint32_t read_size;
    void *packed_message_buf;
    MessageT *msg;
    int result;
    
    // ler o número de bytes da mensagem
	result = read_all(client_socket, &read_size_netlong, sizeof(uint32_t));

	if(result != sizeof(uint32_t)) {
    	printf("Error reading size of message. %s %d\n", __FILE__, __LINE__);
		return NULL;
	}
    
    read_size = ntohl(read_size_netlong);
    packed_message_buf = malloc(read_size);

    // ler a mensagem
    result = read_all(client_socket, packed_message_buf, read_size);

    if (result != read_size) {
        perror("Error reading message: incorrect size.");
        return NULL;
    }

    msg = malloc(sizeof(MessageT));
    msg = message_t__unpack(NULL, read_size, (uint8_t*)packed_message_buf);
    
    return msg;
}

/* A função network_send() deve:
* - Serializar a mensagem de resposta contida em msg;
* - Enviar a mensagem serializada, através do client_socket.
* Retorna 0 (OK) ou -1 em caso de erro.
*/
int network_send(int client_socket, MessageT *msg) {
    void *message_out_buf;
    size_t result, out_size;

    if (msg == NULL) return -1;

    out_size = message_to_buffer(&message_out_buf, msg);  // format: [[uint32_t length][bytes (of length)]]

    result = write_all(client_socket, message_out_buf, out_size);

    if (result != out_size) {
        perror("Erro na escrita da mensage!");
        close(client_socket);
        return -1;
    }

    free(message_out_buf);

    return 0;
}

/* Liberta os recursos alocados por server_network_init(), nomeadamente
* fechando o socket passado como argumento.
* Retorna 0 (OK) ou -1 em caso de erro.
*/
int server_network_close(int socket) {
    return close(socket);
}

/**
 * Função para extrair o endereço IP de nós
 */
char* getIPv4()
{
    struct ifaddrs *ifaddr, *ifa;
    int s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;

        s=getnameinfo(ifa->ifa_addr,sizeof(struct sockaddr_in),host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

        //printf("Interface: %s\n", ifa->ifa_name);

        if( (strcmp(ifa->ifa_name,"eth0")==0 || strcmp(ifa->ifa_name,"enp0s3")==0 || strcmp(ifa->ifa_name, "wlp0s20f3")==0) && ifa->ifa_addr->sa_family==AF_INET)
        {
            if (s != 0)
            {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                exit(EXIT_FAILURE);
            }
            printf("\tInterface : <%s>\n",ifa->ifa_name );
            printf("\t  Address : <%s>\n", host);
            char* hostStr =  malloc((strlen(host)+1) * sizeof(char));
            strcpy(hostStr, host);
            return hostStr;
        }
    }
    freeifaddrs(ifaddr);
    printf("Não consegui detetar interface de rede desta máquina!");
    exit(EXIT_FAILURE);
}


/**
 * Função usada para enviar a tabela para os nós seguintes na cadeia
 * ATENCÃO: é bom que esta função só seja chamada quando este servidor deixou de ser o TAIL e há um novo tail (ver child_watcher em server_zk_utils.c)
 */
int send_table_to_next_server(struct table_t *table){

    printf("A enviar a tabela para o novo TAIL: %s | %s\n", g_zkinfo->nextNode, g_zkinfo->nextServerAddress);
    struct rtable_t *rtable_tail = NULL;
    int max_retries = 5;
    int retry_count = 0;

    // Try to connect multiple times
    while (retry_count < max_retries) {
        printf("conectar ao novo TAIL: %s | %s\n", g_zkinfo->nextNode, g_zkinfo->nextServerAddress);
        rtable_tail = rtable_connect(g_zkinfo->nextServerAddress);
        if (rtable_tail != NULL) {
            break;
        }
        printf("Tentativa %d de %d falhou. A tentar novamente em 2 segundos...\n", 
               retry_count + 1, max_retries);
        sleep(2);
        retry_count++;
    }

    if (rtable_tail == NULL) {
        printf("ERRO: Não foi possível conectar ao novo TAIL após %d tentativas\n", max_retries);
        return -1;  // Return error instead of exit
    }
    
    char ** keys = table_get_keys(table); // ultimo valor a NULL
    if (keys == NULL) {
        printf("Erro ao obter as chaves da tabela!\n");
        rtable_disconnect(rtable_tail);
        exit(EXIT_FAILURE); // radical! faz sentido continuar? logo se vê...
    }

    char **it = keys;  // iterador
    while (*it != NULL) {
        struct block_t *block = table_get(table, *it); // CÓPIA! pode ser destruido
        // fazer uma entry
        struct entry_t *entry = entry_create(strdup(*it), block);  // usar duplicado da string para depois se poder usar o entry_destroy
        // enviar com rtable_put
        if (rtable_put(rtable_tail, entry) == -1) {
            printf("Erro ao enviar a tabela para o TAIL!\n");
            table_free_keys(keys);
            rtable_disconnect(rtable_tail);
            exit(EXIT_FAILURE); // radical! faz sentido continuar? logo se vê...
        }
        entry_destroy(entry);
        it++;
    }

    printf("aquichegou %s %d\n", __FILE__, __LINE__);
    table_free_keys(keys);
    printf("aquichegou %s %d\n", __FILE__, __LINE__);
    rtable_disconnect(rtable_tail);
    printf("Tabela enviada com sucesso para o novo TAIL\n");
    return 0;
}




