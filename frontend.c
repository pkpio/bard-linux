#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#define DEFAULT_DEVICE	"/dev/bard0"

void send_cmd(int fd, int cmd)
{
	int retval = 0;
	int i = 0;
	char buffer[16384];
	
	for(i=0;i<16383;i++){
		buffer[i] = 0x6f;
	}
	
	retval = write(fd, buffer, 16384);
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
