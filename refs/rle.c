#include <stdio.h>
#include <stdlib.h>

typedef unsigned u8;

char *compress(char *str, long length) {
	long count = 0;
	
	char *start1 = str;
	char *start2 = str+1;
	
	char *c_first1 = str;
	char *c_first2 = str+1;
	
	char *c_last1 = str;
	char *c_last2 = str+1;
	
	char *c_write1 = str;
	char *c_write2 = str+1;
	
	u8 run_len = 0;
	
	while (count != length) {
		count = count + 2;
		
		c_last1 = c_last1 + 2;
		c_last2 = c_last2 + 2;
		++run_len;
		
		// end of run
		if (run_len == 255 || *c_last1 != *c_first1 
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
				*c_write1 = run_len;
				
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
		
		str = str + 2;
	}
	*c_write1 = 'e';
	return start1;
}


int main(int argc, char **argv) {
	char * buffer = 0;
	long length;
	FILE * f_in = fopen ("input", "rb");
	FILE * f_out = fopen("output", "wb");

	if (f_in)
	{
		fseek (f_in, 0, SEEK_END);
		length = ftell (f_in);
		fseek (f_in, 0, SEEK_SET);
		buffer = malloc (length);
		if (buffer){
			fread (buffer, 1, length, f_in);
		}
		fclose (f_in);
	} else{
		printf("File reading failed\n");
	}

	if (buffer && f_out){
		buffer = compress(buffer, length);
		
		while(*buffer != 'e'){
			fwrite(buffer, 1, 1, f_out);
			buffer = buffer + 1;
		}
		fclose(f_out);
	}
	
	return 0;
}
