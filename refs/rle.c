#include <stdio.h>
#include <stdlib.h>

typedef unsigned u8;
typedef unsigned u16;

char *compress(char *str, long length, int *rled_len) {
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
	*rled_len = 0; // RLE data length
	
	while (count != length-1) {
		count = count + 2;		
				
		c_last1 = c_last1 + 2;
		c_last2 = c_last2 + 2;
		
		++run_len;
		
		// end of run
		if (run_len == 255 || *c_last1 != *c_first1 
			|| *c_last2 != *c_first2) {
			
			
			/*
			 * ESC char case: When ever ESC char occurs. We will 
			 * replace it with a value next to it. This is to ensure
			 * that the length of the encoded data never exceeds the
			 * input length what ever be the case
			 */
			if(*c_first1 == 'r' && *c_first2 == 'r'){
				*c_first1 = 'r'-1;
				*c_first2 = 'r'-1;
			}
			
			// No repition. Input as output.
			if(run_len < 2){
				*c_write1 = *c_first1;
				*c_write2 = *c_first2;
				
				// 2 bytes written to output
				*rled_len = *rled_len + 2;
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
				
				// 4 bytes written to output
				*rled_len = *rled_len + 4;				
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
				
				// 5 bytes written to output
				*rled_len = *rled_len + 5;
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
	*c_write1 = '\0';
	return start1;
}


int main(int argc, char **argv) {
	char * buffer = 0;
	long length;
	int rled_len = 0;
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
		buffer = compress(buffer, length, &rled_len);
		
		printf("Encoded length : %d\n", rled_len);
	
		fwrite(buffer, 1, rled_len, f_out);
		buffer = buffer + 1;
		fclose(f_out);
	}
	
	return 0;
}
