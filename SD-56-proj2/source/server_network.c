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
#include "server_skeleton.h"

#define BACKLOG 20 /* how many pending connections queue will hold */
#define NFDESC 500 // Número de sockets (uma para listening)
#define TIMEOUT 1000 // em milisegundos

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
        exit(1);//preciso de fechar socket neste caso?
    };

    return sfd; //sfd e o descritor da socket
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
    struct pollfd connections[NFDESC];  // array de ligações
    int i, count_cfd = 0, kfds, nlp;

    socklen_t sin_size;
    struct sockaddr_storage their_addr; // connector's address information
    int socket_de_cliente;
    char ipstr[INET6_ADDRSTRLEN];
    char ipstr_ref[NFDESC][INET6_ADDRSTRLEN]; // lista de endereços dos clientes

    MessageT *message; // do protobuf
    char *buffer;
    size_t buf_size=0;

    struct pollfd default_pollfd;
    default_pollfd.fd = -1;
    default_pollfd.events = POLLIN;
    default_pollfd.revents = 0;

    // preencher ligações com valores default
    for (i = 0; i < NFDESC; i++) 
        connections[i] = default_pollfd;

    // naprimeira posição do array, colocar a ligação de escuta
    connections[0].fd = listening_socket;
    count_cfd++;

    printf("INCIAR O CICLO DO SERVIDOR\n");

    // estrutura para medir o tempo
    struct timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;

    // enquanto houver ligações ativas
    while ((kfds = poll(connections, count_cfd, TIMEOUT)) >= 0) {

        if (kfds>0) { // então há fds para ler 
            nlp = 0; // número de ligações processadas

            for (i=0; i<NFDESC; i++) {
                if (nlp>=kfds) break;  // já foram processadas todas as ligações

                if (connections[i].revents & POLLIN) { // true if there is an event on this connection
                    nlp++; // incrementar o número de ligações processadas

                    if (i==0) {  // estamos no socket de escuta do servidor
                        //então é uma ligação de cliente e temos que adicionar mais um socket para comunicar com esse cliente
                        if (count_cfd<NFDESC) {  // ainda há espaço no array para fds
                            sin_size = sizeof their_addr;

                            // criar socket para comunicar com o cliente
                            socket_de_cliente = accept(listening_socket, (struct sockaddr *)&their_addr, &sin_size);                   
                            setsockopt(socket_de_cliente, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

                            if (socket_de_cliente == -1) {
                                perror("accept");
                                continue;
                            }

                            // procura posição vazia e adicionar fd
                            int j;
                            for (j=1;j<NFDESC;j++) {
                                if (connections[j].fd == -1) {
                                    connections[j] = default_pollfd;
                                    connections[j].fd = socket_de_cliente;
                                    break;
                                }
                            }

                            // DEBUG
                            if (j==NFDESC) {
                                fprintf(stderr, "ERRO!: não encontrou posição livre quando devia existir uma!\n");
                                exit(1);
                            }
                            
                            inet_ntop(their_addr.ss_family, &(((struct sockaddr_in*)&their_addr)->sin_addr), ipstr, sizeof ipstr);
                            
                            printf("server: got connection from client at %s\n", ipstr);
                            strcpy(ipstr_ref[i], ipstr);
                            count_cfd++;
                        }
                        continue;  // continua o loop para tratar das ligações dos clientes
                    }

                    // se chegou aqui, então é uma ligação de um cliente


                     //ler e processar mensagem do cliente
                    printf("begin read from %s\n", ipstr_ref[i]);
                    socket_de_cliente = connections[i].fd;
                    message = network_receive(socket_de_cliente);
                    if(message == NULL) {
                        printf("closing client socket por não ter enviado nada ou  mensagem inválida\n");
                        close(socket_de_cliente);
                        connections[i] = default_pollfd; // df novamente a -1
                        count_cfd--;
                        continue;
                    } else {/* processamento da mesnagem e envio da mesagem de resposta */

                        if (invoke(message, table) == -1) {
                            printf("Error in invoke!");
                        }

                        // send response message
                        //printf("aqui chega 0\n");
                        buf_size = message_to_buffer((void *)&buffer, message);
                        //printf("aqui chega 1\n");
                        printf("begin write\n");
                        if (write_all(socket_de_cliente, buffer, buf_size) != buf_size) {
                            perror("Erro no envio da resposta.");
                            free(buffer);
                            printf("closing client socket por haver erro no envio da resposta\n");
                            connections[i] = default_pollfd;
                            close(socket_de_cliente);
                            count_cfd--;
                            continue;
                        }
                        free(buffer);
                    }
                }
            }
        }

    }
    return -1;
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

