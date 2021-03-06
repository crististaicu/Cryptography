#include "common.h"

void print_buff(u_int8_t * buff, size_t buff_size)
{
	int i = 0, j;

	while( i < buff_size ) {
		j = 0;
		while( ((j + i) < buff_size) && (j < 16) ) {
			fprintf(stderr,"%02X ",buff[j]);
			j++;
		}

		while( j < 16 ) {
			fprintf(stderr,"-- ");
			j++;
		}
		fprintf(stderr," *** ");

		j = 0;
		while( ((j + i) < buff_size) && (j < 16) ) {
			fprintf(stderr,"%c",isprint(buff[j]) ? buff[j] : '.');
			j++;
		}

		while( j < 16 ) {
			fputc('.',stderr);
			j++;
		}

    		i += j;
		fputc('\n',stderr);
	}
}

int write_OK(int channel_fd)
{
	return (write_msg(channel_fd,(const u_int8_t *)OK_STRING,strlen(OK_STRING)));
}

int write_BYE(int channel_fd)
{
	return (write_msg(channel_fd,(const u_int8_t *)CLOSE_CONNECTION_STRING,strlen(CLOSE_CONNECTION_STRING)));
}

int open_channel(const char * pathname)
{
	int fd;

	/* Open the FIFO in pathname */
	if( (fd = open(pathname,O_RDWR)) < 0 ) {
		perror("open()");
		exit(1);
	}

	return (fd);
}

void close_channel(int channel_fd)
{
	/* Close the file descriptor */
	close(channel_fd);
}

ssize_t read_msg(int channel_fd, u_int8_t ** msg)
{
	ssize_t read_len;
	u_int16_t msg_size;
	static u_int8_t msg_buff[MSG_SIZE_MAX];

	/* The first 16 bits represents the message size */
	if( read(channel_fd,(void *)&msg_size,sizeof(msg_size)) < 0 ) {
		perror("read()");
		exit(1);
	}

	fprintf(stderr,"Reading size:\n");
	print_buff((u_int8_t *)(&msg_size),sizeof(msg_size));

	/* Verify that the size of the message is no more than MSG_SIZE_MAX bytes */
	if( msg_size >= MSG_SIZE_MAX ) {
		fprintf(stderr,"Message too large\n");
		return (-1);
	}

	/* Read msg_size bytes from the channel */
	if( (read_len = read(channel_fd,(void *)msg_buff,msg_size)) < 0 ) {
		perror("read()");
		exit(1);
	}

	/* Assign the buffer to msg */
	(*msg) = msg_buff;

	fprintf(stderr,"Reading message:\n");
	print_buff((u_int8_t *)(*msg),msg_size);

	return (read_len);
}

ssize_t write_msg(int channel_fd, const u_int8_t * msg, u_int16_t msg_size)
{
	/* Send the size */
	if( write(channel_fd,(const void *)&msg_size,sizeof(msg_size)) < 0 ) {
		perror("write()");
		exit(1);
	}

	fprintf(stderr,"Writing size:\n");
	print_buff((u_int8_t *)&msg_size,sizeof(msg_size));

	/* Send the message */
	if( write(channel_fd,(const void *)msg,msg_size) < 0 ) {
		perror("write()");
		exit(1);
	}

	fprintf(stderr,"Writing message:\n");
	print_buff((u_int8_t *)msg,msg_size);

	return (msg_size);
}

int read_string(int channel_fd, const char * str)
{
	size_t read_len;
	u_int8_t * msg;

  read_len = read_msg(channel_fd,&msg);
	if(	(read_len != strlen(str) ) ||
   		 memcmp(msg,str,strlen(str)) )
		return (-1);

	return (0);
}

const char* quads[] = { "0000", "0001", "0010", "0011", "0100", "0101", "0110",
		"0111", "1000", "1001", "1010", "1011", "1100", "1101", "1110", "1111" };

const char * hex_to_bin_quad(unsigned char c) {
	if (c >= '0' && c <= '9')
		return quads[c - '0'];
	if (c >= 'A' && c <= 'F')
		return quads[10 + c - 'A'];
	if (c >= 'a' && c <= 'f')
		return quads[10 + c - 'a'];
	return -1;
}

char * hex_to_binary(char* hex_array) {
	int len = strlen(hex_array);
	char *res = (char*) malloc((len * 4 + 1) * sizeof(char));
	int i = 0;
	for (i = 0; i < len; i++) {
		char *quad = hex_to_bin_quad(hex_array[i]);
		int j;
		for (j = 0; j < 4; j++) {
			res[i * 4 + j] = quad[j];
		}
	}
	res[len * 4] = 0;
	return res;
}
