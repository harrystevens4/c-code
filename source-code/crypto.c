#include <unistd.h>
#include <sys/socket.h>
#include <linux/if_alg.h>

//using the kernel's built in crypto api
//https://docs.kernel.org/crypto/userspace-if.html
int sha1(const char *message, size_t message_len, unsigned char *message_digest){
	//====== open a crypto socket ======
	//open a crypto connection for which we can later accept to create new sockets from
	int crypto_socket_provider = socket(AF_ALG,SOCK_SEQPACKET,0);
	if (crypto_socket_provider < 0) return -1;
	struct sockaddr_alg alg_info = {
		.salg_family = AF_ALG,
		.salg_type = "hash",
		.salg_name = "sha1",
	};
	int result = bind(crypto_socket_provider,(struct sockaddr *)&alg_info,sizeof(alg_info));
	if (result < 0) return -1;
	//accept a new socket that can be used to sha1 digest
	int crypto_socket = accept(crypto_socket_provider,NULL,0);
	if (crypto_socket < 0) return -1;
	//====== send data to be hashed ======
	result = send(crypto_socket,message,message_len,0);
	if (result < 0) return -1;
	//====== receive digest ======
	result = recv(crypto_socket,message_digest,20,0);
	if (result < 0) return -1;
	//====== close the socket ======
	close(crypto_socket);
	close(crypto_socket_provider);
	return 0;
}

int sha256(const char *message, size_t message_len, unsigned char *message_digest){
	//====== open a crypto socket ======
	//open a crypto connection for which we can later accept to create new sockets from
	int crypto_socket_provider = socket(AF_ALG,SOCK_SEQPACKET,0);
	if (crypto_socket_provider < 0) return -1;
	struct sockaddr_alg alg_info = {
		.salg_family = AF_ALG,
		.salg_type = "hash",
		.salg_name = "sha256",
	};
	int result = bind(crypto_socket_provider,(struct sockaddr *)&alg_info,sizeof(alg_info));
	if (result < 0) return -1;
	//accept a new socket that can be used to sha1 digest
	int crypto_socket = accept(crypto_socket_provider,NULL,0);
	if (crypto_socket < 0) return -1;
	//====== send data to be hashed ======
	result = send(crypto_socket,message,message_len,0);
	if (result < 0) return -1;
	//====== receive digest ======
	result = recv(crypto_socket,message_digest,32,0);
	if (result < 0) return -1;
	//====== close the socket ======
	close(crypto_socket);
	close(crypto_socket_provider);
	return 0;
}
