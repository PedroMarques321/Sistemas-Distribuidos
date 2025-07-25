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

/**
 * Envia buffer de qualquer dimensão pela rede, assegurando que só retorna quando o buffer for totalmente enviado
 * ou se ocorrer um erro
 */
int write_all(int sock, void *buf, int len) {
    int bufsize = len;
	int result;
    char* buf2 = buf;

	while(len > 0) {
		result = write(sock, buf2, len);

		if(result<0) {
			perror("Erro no write_all");
			return result;
		}

		buf2 += result;
		len -= result;
	}

	return bufsize;
}

/**
 * Recebe buffer de qualquer dimensão pela rede, assegurando que só retorna quando o buffer for totalmente recebido
 * ou se ocorrer um erro.
 */
int read_all(int sock, void *buf, int len) {
    int bufsize = len;
	int result;
    char* buf2 = buf;

	while(len > 0) {
		result = read(sock, buf2, len);

		if(result == 0) 
			return result;

		if(result<0) {
			perror("Erro no read_all");
			return result;
		}

		buf2 += result;
		len -= result;
	}

	return bufsize;
}

/**
 * Inicializa e retorna uma estrutura MessageT
 */
struct MessageT * message_init() {
	MessageT *msg = malloc(sizeof(MessageT));
    message_t__init(msg);
    return msg;
}


void message_destroy(MessageT *msg) {
    message_t__free_unpacked(msg, NULL);
}

/// @brief Receives a buffer reference and allocates it. Serializes message content with format 
/// [[uint32_t length][bytes (of length)]] so that the reader knows how much to read to get the complete message.
/// @param buffer_ref will be allocated in this function
/// @param msg 
/// @return allocated size
size_t message_to_buffer(void **buffer_ref, MessageT *msg) {
    size_t packed_message_size;
    void * packed_message_buf;

    if(msg == NULL) return 0;
    //printf("aqui chega 00\n");
    packed_message_size = message_t__get_packed_size(msg);
    packed_message_buf = malloc(packed_message_size);
    //printf("aqui chega 11\n");
    message_t__pack(msg, packed_message_buf);
    //printf("aqui chega 22\n");

    void *mbuff;
    void *it = NULL;
    
    size_t size = sizeof(uint32_t) + packed_message_size;
    mbuff = malloc(size);
    it = mbuff;
    
    // put size in buffer
    *((uint32_t *)it) = htonl(packed_message_size);
    it = it + sizeof(uint32_t);

    // put data in buffer
    memcpy(it, packed_message_buf, packed_message_size);

    *buffer_ref = mbuff;
    return size;
}


char *as_printable(const void *str, const int len) {
    int i;
    char *out = calloc(sizeof(char), len+1);
    memcpy(out, str, len);
    for (i=0; i<len; i++) {
        if (out[i]<20 || out[i]>126) out[i] = '.';
    }
    return out;
}
