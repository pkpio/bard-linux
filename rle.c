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
		
		// end of run
		if (!(*c_last2) || run_len == 255 || *c_last1 != *c_first1 
			|| *c_last2 != *c_first2) { 
			
			// No repition. Input as output.
			if(run_len < 2){
				*c_write1 = *c_first1;
				*c_write2 = *c_first2;
			}
			
			// repeated twice. Input as output.
			if(run_len == 2){
				// Write the input
				*c_write1 = *c_first1;
				*c_write2 = *c_first2;
				
				c_write1 = c_write1 + 2;
				c_write2 = c_write2 + 2;
				
				// Write the input again. No encoding.
				*c_write1 = *c_first1;
				*c_write2 = *c_first2;				
			}
			
			// run_len >= 3. We also add a special char.
			else if(run_len > 2){
				// Skip 2 bytes. We shall write the special char
				// here at the end.
				c_write1 = c_write1 + 2;
				c_write2 = c_write2 + 2;
				
				// Write run_len
				*c_write1 = '0' + run_len;
				
				c_write1 = c_write1 + 1;
				c_write2 = c_write2 + 1;
				
				// Write the repeated bytes
				*c_write1 = *c_first1;
				*c_write2 = *c_first2;
				
				// Now go back and write the special char
				*(c_write1-3) = 'r';
				*(c_write2-3) = 'r';			
				
			}
			
			// set write pointers for next write
			c_write1 = c_write1 + 2;
			c_write2 = c_write2 + 2;
				
			// start next run
			run_len = 0; 
			c_first1 = c_last1;
			c_first2 = c_last2;
		}
		
		if(!*(str+1))
			break;
		str = str + 2;
	}
	*c_write1 = 0;
	return start1;
}


int main(int argc, char **argv) {
	char * buffer = 0;
	long length;
	FILE * f = fopen ("input", "rb");

	if (f)
	{
		fseek (f, 0, SEEK_END);
		length = ftell (f);
		fseek (f, 0, SEEK_SET);
		buffer = malloc (length);
		if (buffer){
			fread (buffer, 1, length, f);
		}
		fclose (f);
	}

	if (buffer)
		printf("\n%s\n", compress(buffer));
	
	return 0;
}
