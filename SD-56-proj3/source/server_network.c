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

#include <pthread.h>

#define BACKLOG 20 /* how many pending connections queue will hold */
#define NFDESC 500 // Número de sockets (uma para listening)
#define TIMEOUT 1000 // em milisegundos

int exit_loop = 0;
int threadId_contador = 1; // "privada"

void exit_server_and_thread_loops_graciously() {
    exit_loop = 1;
}


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // mutex partilhado por todas as threads (contexto global)


// tipo privado - só para este módulo
struct thread_parameters {
    unsigned int threadId;
    int socketfd_cliente;
    struct table_t *table;
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
    
    tpar = (struct thread_parameters *) params; // id, socket de cliente e ponteiro para a tabela

    while (!exit_loop)
    {
        printf("begin read in thread %d\n", tpar->threadId);
        message = network_receive(tpar->socketfd_cliente);
        if(message == NULL) {
            printf("mensagem inválida: fechar cliente (terminar thread)\n");
            close(tpar->socketfd_cliente);
            
            pthread_mutex_lock(&mutex);
            // SECÇÃO CRÍTICA
            stats_decrease_clients();  
            pthread_mutex_unlock(&mutex);
            
            pthread_exit((void*) 0);
            return NULL;
        }

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
            if (invoke(message, tpar->table) == -1) {
                printf("Error in invoke!");
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
        if (pthread_create(&thread, NULL, &conn_thread_main, (void *) tpar) != 0){
            printf("\nThread não criada. Saindo graciosamente\n");
            exit_loop = 1;
            return -1;
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

