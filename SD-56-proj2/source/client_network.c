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

#include "client_network.h"
#include "inet-private.h"
#include "client_stub-private.h"
#include "message-private.h"

/* Esta função deve:
* - Obter o endereço do servidor (struct sockaddr_in) com base na
* informação guardada na estrutura rtable;
* - Estabelecer a ligação com o servidor;
* - Guardar toda a informação necessária (e.g., descritor do socket)
* na estrutura rtable;
* - Retornar 0 (OK) ou -1 (erro).
*/
int network_connect(struct rtable_t *rtable) {
    struct addrinfo hints, *res, *p;
    int status;
    char ipstr[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    char portStr[10];
    sprintf(portStr, "%d", rtable->server_port);
    if ((status = getaddrinfo(rtable->server_address, portStr, &hints, &res)) != 0) {
        fprintf(stderr, "erro getaddrinfo: %s\n", gai_strerror(status));
        return -1;
    }

    // dar loop em todos os resultados e conectar ao primeiro possível
    for (p = res; p != NULL; p = p->ai_next) {
        // se der erro a criar socket, continuamos
        if((rtable->sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client_network: erro na socket!\n");
            continue;
        }
        // se der erro a dar connect, continuamos
        if (connect(rtable->sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(rtable->sockfd);
            perror("client_network: erro no connect!\n");
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client_network: conexão falhada!\n");
        return -1;
    }

    inet_ntop(p->ai_family, &(((struct sockaddr_in*)p->ai_addr)->sin_addr), ipstr, sizeof(ipstr));

    struct timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    setsockopt(rtable->sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    freeaddrinfo(res);

    return 0;
}

/* Esta função deve:
* - Obter o descritor da ligação (socket) da estrutura rtable_t;
* - Serializar a mensagem contida em msg;
* - Enviar a mensagem serializada para o servidor;
* - Esperar a resposta do servidor;
* - De-serializar a mensagem de resposta;
* - Tratar de forma apropriada erros de comunicação;
* - Retornar a mensagem de-serializada ou NULL em caso de erro.
*/
MessageT *network_send_receive(struct rtable_t *rtable, MessageT *msg) {
    void *message_out_buf; //aqui tinhas char em vez de void
    void *packed_message_buf;
    size_t result, out_size;

    uint32_t read_size_netlong;
    uint32_t read_size;

    MessageT *msg_res;
    //aqui davas cast de message_out_buf para void*
    out_size = message_to_buffer(&message_out_buf, msg);

    if (out_size <= 0) {
        perror("Error: network_send_receive, erro na serialização!\n");
        exit(1);
    }

    if (write_all(rtable->sockfd, message_out_buf, out_size) != out_size) {
        perror("Error: network_send_receive, erro no write_all!\n");
        exit(1);
    }

    free(message_out_buf);

    result = read_all(rtable->sockfd, (void *) &read_size_netlong, sizeof(uint32_t)); // ler só o tamanho da mensagem (inteiro de 4 bytes)

    if (result != sizeof(uint32_t)) {
        // fechar a conexão
        printf("Erro na conexão, vamos tentar novamente dentro de um momento!\n");
        if(close(rtable->sockfd) != 0) {
            perror("Error: erro a fechar a socket!\n");
            exit(1);
        }

        sleep(5);

        printf("A tentar reconectar...\n");

        if (network_connect(rtable) == -1) {
            perror("Erro a reconectar, tente mais tarde.\n");
            exit(1);
        }

        out_size = message_to_buffer(&message_out_buf, msg);

        if (out_size <= 0) {
            perror("Error: network_send_receive, erro na serialização!\n");
            exit(1);
        }

        if (write_all(rtable->sockfd, message_out_buf, out_size) != out_size) {
            perror("Error: network_send_receive, erro no write_all!\n");
            exit(1);
        }

        free(message_out_buf);

        //processar a resposta
        result = read_all(rtable->sockfd, (void *) &read_size_netlong, sizeof(uint32_t));

        if (result != sizeof(uint32_t)) {
            printf("Error: erro a ler o tamanho da mensagem: %s %d\n", __FILE__, __LINE__); // que faz o FILE e o LINE?
            return NULL;
        }
    }

    // se chegar até aqui, conseguiu enviar mensagem e ler tamanho da resposta

    read_size = ntohl(read_size_netlong);
    packed_message_buf = malloc(read_size);

    result = read_all(rtable->sockfd, packed_message_buf, read_size);

    if (result != read_size) {
        perror("Erro a ler a mensagem, tamanho incorreto!\n");
        return NULL;
    }

    msg_res = malloc(sizeof(MessageT));
    msg_res = message_t__unpack(NULL, read_size, (uint8_t *)packed_message_buf);

    if (msg_res == NULL) {
        perror("Error: erro a deserializar mensagem!\n");
        free(packed_message_buf);
        return NULL;
    }

    free(packed_message_buf);
    return msg_res;
}

/* Fecha a ligação estabelecida por network_connect().
* Retorna 0 (OK) ou -1 (erro).
*/
int network_close(struct rtable_t *rtable) {
    return close(rtable->sockfd);
}