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

#ifndef _MESSAGE_PRIVATE_H
#define _MESSAGE_PRIVATE_H

#include "htmessages.pb-c.h"

/**
 * Envia buffer de qualquer dimensão pela rede, assegurando que só retorna quando o buffer for totalmente enviado
 */
int write_all(int sock, void *buf, int len);

/**
 * Recebe buffer de qualquer dimensão pela rede, assegurando que só retorna quando o buffer for totalmente recebido
 * ou ocorrer um erro.
 */
int read_all(int sock, void *buf, int len);

struct MessageT *message_init();

void message_destroy(MessageT *msg);

/**
 * Aloca memória para uma referência de um buffer, serializa conteúdo da mensagem com o 
 * formato [[uint32_t length][bytes (of length)]] para que o leitor saiba quanto ler para receber a mensagem completa.
 */
size_t message_to_buffer(void **buffer_ref, MessageT *msg);

char *as_printable(const void *str, const int len);
#endif