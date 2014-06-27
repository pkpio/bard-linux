#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#define DEFAULT_DEVICE	"/dev/bard0"
#define BUFFER_SIZE 200

void send_cmd(int fd, int cmd)
{
	int retval = 0;
	int i = 0;
	char buffer[BUFFER_SIZE];
	
	for(i=0;i<BUFFER_SIZE;i++){
		buffer[i] = 0x6f;
	}
	
	retval = write(fd, buffer, BUFFER_SIZE);
	if (retval < 0)
		fprintf(stderr, "an error occured: %d\n", retval);
}

int main (){	
	int fd;
	int cmd = 0xff;
	char *dev = DEFAULT_DEVICE;
	
	fd = open(dev, O_RDWR);
	if (fd == -1)
	{
		perror("open");
		exit(1);
	}
	
	fprintf(stderr, "The device is: %s\n", DEFAULT_DEVICE);

	send_cmd(fd, cmd);
	close(fd);
}
