#ifndef _CRYPTO_H
#define _CRYPTO_H

int sha1(const char *message, size_t message_len, unsigned char *message_digest);
int sha256(const char *message, size_t message_len, unsigned char *message_digest);

#endif
