#include <stdio.h>
#include <stdlib.h>

char *compress(char *str) {
	char *start1 = str;
	char *start2 = str+1;
	
	char *c_first1 = str;
	char *c_first2 = str+1;
	
	char *c_last1 = str;
	char *c_last2 = str+1;
	
	char *c_write1 = str;
	char *c_write2 = str+1;
	
	int run_len = 0;
	
	while (*str) {
		c_last1 = c_last1 + 2;
		c_last2 = c_last2 + 2;
		++run_len;
		printf("run_len: %d\n", run_len);
		
		if (!(*c_last2) || *c_last1 != *c_first1 || *c_last2 != *c_first2) { 
			// end of run
			*c_write1 = *c_first1;
			*c_write2 = *c_first2;
			
			c_write2 = c_write2 + 1;
			
			if (run_len > 1)
				*(c_write2++) = '0' + run_len;
				
			// start next run
			run_len = 0; 
			c_first1 = c_last1;
			c_first2 = c_last2;
		}
		str = str + 2;
	}
	*c_write2 = 0;
	return start1;
}


int main(int argc, char **argv) {
	if (argc != 2){
		printf("returning 1\n");
		return 1;
	}
	printf("%s\n", compress(argv[1]));
	return 0;
}
