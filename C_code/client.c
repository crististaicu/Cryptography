#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/bn.h>
#include <openssl/bio.h>
#include "prime_generator.h"
#include "rsa.c"
#include "all5.h"
#include "maj5.h"

char * read_word(FILE *f) {
	char word[10000];
	int i = 0, j;
	char c;
	do {
		c = fgetc(f);
	} while (c == ' ' || c == '\n' || c == '\r');

	do {
		word[i] = c;
		i++;
		c = fgetc(f);
	} while (c != EOF && c != ' ' && c != '\n' && c != '\r');
	char *res = (char *) malloc((i + 1) * sizeof(char));
	for (j = 0; j < i; j++) {
		res[j] = word[j];
	}
	res[i] = 0;
	return res;
}

void get_rsa_serv_pub_key(BIGNUM** s_puk, BIGNUM** n) {
	FILE *filepointer;
	filepointer = fopen("C_code/client_folder/server_rsa_public_key.txt", "r");

	char* s_puk_str = read_word(filepointer);
	char* n_str = read_word(filepointer);
	BN_dec2bn(s_puk, s_puk_str);
	BN_dec2bn(n, n_str);
}

char * convert_to_binary(char *x) {
	char *res = (char*) malloc(10000 * sizeof(char));
	int i,c,power;
	int j = 0;
	for (i = 0; x[i] != '\0'; i++) {
		c = x[i];
		for (power = 7; power + 1; power--)
			if (c >= (1 << power)) {
				c -= (1 << power);
				res[j++] = '1';
			} else
				res[j++] = '0';
	}
	res[j] = 0;
	return res;
}

	void get_rsa_client_priv_key(BIGNUM** s_prk, BIGNUM** n) {
		FILE *filepointer;
		filepointer = fopen("C_code/client_folder/client_rsa_private_key.txt",
				"r");
		char* s_prk_str = read_word(filepointer);
		char* n_str = read_word(filepointer);
		BN_dec2bn(s_prk, s_prk_str);
		BN_dec2bn(n, n_str);
	}

	char * binary_to_hex(char * bin_array) {
		int len = strlen(bin_array);
		int i, j = 0;
		char *res = (char*) malloc((len / 4 + 1) * sizeof(char));
		for (i = 0; i < len; i += 4) {
			int n = (bin_array[i] - '0') * 8 + (bin_array[i + 1] - '0') * 4
					+ (bin_array[i + 2] - '0') * 2 + (bin_array[i + 3] - '0');
			if (n < 10) {
				res[j++] = '0' + n;
			} else {
				res[j++] = 'A' + (n - 10);
			}
		}
		res[len / 4] = 0;
		return res;
	}

	int main(int argc, char ** argv) {
		int sc_fifo_fd, cs_fifo_fd;
		BIGNUM* s_puk = BN_new();
		BIGNUM* c_prk = BN_new();
		BIGNUM* n = BN_new();
		/* Mandatory arguments */
		if (!argv[1] || !argv[2] || !argv[3] || !argv[4]) {
			fprintf(stderr,
					"client [server->client fifo] [client->server fifo] [password file] [username]\n");
			exit(1);
		}
		/* Create connection with the server */
		fprintf(stderr, "Create connection...\n");
		sc_fifo_fd = open_channel(argv[1]);
		cs_fifo_fd = open_channel(argv[2]);

		write_msg(cs_fifo_fd, (const u_int8_t *) CONNECTION_STRING,
				strlen(CONNECTION_STRING));

		/* Read OK */
		if (read_string(sc_fifo_fd, OK_STRING) < 0) {
			fprintf(stderr, "Communication error\n");
			goto next;
		}

		/* Server authentication */
//	write_msg(cs_fifo_fd,(const u_int8_t *)"A",1);
// GET public rsa key of S, (s_puk,n), from "client_folder/server_rsa_public_key.txt"
		get_rsa_serv_pub_key(&n, &s_puk);
		printf("Server public key: (%s %s)\n", BN_bn2hex(s_puk), BN_bn2hex(n));
		/* ... */
		// CREATE a random number r
		primitive_p = initialize("1011011");
		initialize_rand(38362178);
		BIGNUM *rand;
		rand = generate_random_no(5);
//	BIGNUM *rand = BN_new();
//	BN_add_word(rand, 5);
		printf("Generated random number: %s\n", BN_bn2hex(rand));

		// ENCRYPT r using (s_puk,n) -> c = r^s_puk mod n
		BIGNUM *enc_r = enc_dec(rand, s_puk, n);
		//TODO change here
		char *r_enc_str = BN_bn2hex(enc_r);
		printf("Encrypted random number: %s\n", r_enc_str);
		// WRITE c to S
		write_msg(cs_fifo_fd, r_enc_str, strlen(r_enc_str) + 1);

		// READ r' from C
		u_int8_t * buff;
		int len = read_msg(sc_fifo_fd, &buff);
		char *buf_str = (char *) malloc((len + 1) * sizeof(char));
		int i;
		for (i = 0; i < len; i++) {
			buf_str[i] = (char) buff[i];
		}
		print_buff(buff, len);
		BIGNUM* dec_r = BN_new();
		BN_hex2bn(&dec_r, buf_str);
		printf("Received decrypted random number: %s, %d characters\n",
				BN_bn2hex(dec_r), len);
		// CHECK if r = r'
		if (strcmp(BN_bn2hex(dec_r), BN_bn2hex(rand)) == 0) {
			printf("SERVER AUTHENTICATION SUCCESFULL!!\n");
		} else {
			printf("Server auth. failed!\n");
			exit(1);
		}

		/* Client authentication */
		// SEND client_name to S
		char* username = argv[4];
		printf("client is sending its username: %s\n", username);
		write_msg(cs_fifo_fd, username, strlen(username) + 1);

		// GET private rsa key of C, (s_prk,n) from "client_folder/client_rsa_private_key.txt"
		get_rsa_client_priv_key(&n, &c_prk);
		printf("Client private key: (%s %s)\n", BN_bn2hex(c_prk), BN_bn2hex(n));
		// READ c from S
		len = read_msg(sc_fifo_fd, &buff);
		buf_str = (char *) malloc((len + 1) * sizeof(char));
		for (i = 0; i < len; i++) {
			buf_str[i] = (char) buff[i];
		}
		print_buff(buff, len);
		enc_r = BN_new();
		BN_hex2bn(&enc_r, buf_str);
		printf("Received encrypted random number: %s, %d characters\n",
				BN_bn2hex(enc_r), len);

		// DECRYPT c using (c_prk,n) -> r' = c^c_prk mod n
		BIGNUM *r = BN_new();
		r = enc_dec(enc_r, c_prk, n);
		char *r_str = BN_bn2hex(r);
		printf("Decrypted random number, r'=%s\n", r_str);
		// WRITE r' to S
		write_msg(cs_fifo_fd, r_str, strlen(r_str) + 1);

		/* Negotiation of the cipher suite */
		/* Read cipher suite  and send to server*/
		FILE *filepointer;
		filepointer = fopen("C_code/client_folder/client_cipher_suite.txt",
				"r");
		char cipher;
		fscanf(filepointer, "%c", &cipher);
		printf("Send cipher suite to the server: %c\n", cipher);
		write_msg(cs_fifo_fd, &cipher, 1);
		/* Negotiation of the private key */
		len = read_msg(sc_fifo_fd, &buff);
		buf_str = (char *) malloc((len + 1) * sizeof(char));
		for (i = 0; i < len; i++) {
			buf_str[i] = (char) buff[i];
		}
		print_buff(buff, len);
		BIGNUM *enc_key = BN_new();
		BN_hex2bn(&enc_key, buf_str);
		printf("The encrypted key is %s\n", BN_bn2hex(enc_key));
		BIGNUM *key = BN_new();
		key = enc_dec(enc_key, c_prk, n);
		printf("The decrypted key is %s\n", BN_bn2hex(key));

		/* Encrypt communication */
		/* Read message */
		filepointer = fopen("C_code/client_folder/client_message.txt", "r");
		char *msg = (char *) malloc(10000 * sizeof(char));
		char *encrypted_message = (char *) malloc(10000 * sizeof(char));
		msg[0] = 0;
		char *buff_msg = (char *) malloc(10000 * sizeof(char));
		while (fgets(buff_msg, 10000, filepointer) != NULL){
			strcat(msg, buff_msg);
//			printf("%s---\n", buff_msg);
		}
		char *message = convert_to_binary(msg);
		printf("Message to be send: %s\n", message);
//	if (cipher == 'A') {
//		/* BUNNY */
//		polynom m = initialize(message);
//		polynom k = initialize(hex_to_binary(BN_bn2hex(key)));
//		polynom c = BunnyTn(m, k);
//		for (i = 0; i < c.size; i++) {
//			encrypted_message[i] = c.p[c.size - i - 1] + '0';
//		}
//		encrypted_message[c.size] = 0;
		if (cipher == 'A' || cipher == 'B') {
			/* BUNNY CBC */
			polynom c = cipher_block_chaining_enc(message,
					hex_to_binary("000000"), hex_to_binary(BN_bn2hex(key)));
			for (i = 0; i < c.size; i++) {
				encrypted_message[i] = c.p[c.size - i - 1] + '0';
			}
			encrypted_message[c.size] = 0;
			polynom msg_poly = initialize(message);
			char diff = 24 - (msg_poly.size % 24);
			if (diff != 24) {
			msg_poly = shift_left(msg_poly, diff);
			}
			for (i = 0; i < msg_poly.size; i++) {
				message[i] = msg_poly.p[msg_poly.size - i - 1] + '0';
			}
			message[msg_poly.size] = 0;
			printf("The padded message: %s\n", message);
		} else if (cipher == 'C' || cipher == 'D') {
			/* ALL5 */
			char *res = ALL5ENC(hex_to_binary(BN_bn2hex(key)), message);
			for (i = 0; i <= strlen(res); i++) {
				encrypted_message[i] = res[i];
			}
		} else if (cipher == 'E' || cipher == 'F') {
			/* MAJ5 */
			printf("KEY=%s\n", hex_to_binary(BN_bn2hex(key)));
			char *res = MAJ5ENC(hex_to_binary(BN_bn2hex(key)), message);
			for (i = 0; i <= strlen(res); i++) {
				encrypted_message[i] = res[i];
			}
		}
//	} else if (cipher == 'E') {
//		/* A5/1 */
//		char *res = A51ENC(hex_to_binary(BN_bn2hex(key)), message);
//		for (i = 0; i <= strlen(res); i++) {
//			encrypted_message[i] = res[i];
//		}
//	}

		printf("Encrypted message is %s\n", encrypted_message);
//	write_msg(cs_fifo_fd, encrypted_message, strlen(encrypted_message) + 1);
		/*Compute hash and send it*/
		char *hashed_msg = SPONGEBUNNY(message);
		strcat(encrypted_message, hashed_msg);
		printf("Hashed message: %s\n", hashed_msg);
		encrypted_message = binary_to_hex(encrypted_message);
		printf("Hexed message: %s\n", encrypted_message);
		write_msg(cs_fifo_fd, encrypted_message, strlen(encrypted_message) + 1);
		/* Disconnection */
		/* ... */

		next:
		/* Close connection with the server */
		fprintf(stderr, "Closing connection...\n");

		/* Expect BYE */
		if (read_string(sc_fifo_fd, CLOSE_CONNECTION_STRING) < 0) {
			fprintf(stderr, "Communication error\n");
			goto next;
		}

		close_channel(cs_fifo_fd);
		close_channel(sc_fifo_fd);
		exit(0);
	}
