#include <stdio.h>
#include <stdlib.h>

char *compress(char *str) {
    char *start = str;
    char *c_first = str;
    char *c_last = str;
    char *c_write = str;
    int run_len = 0;
    while (*str) {
        printf("Test 1\n");
	++c_last;
        ++run_len;
        if (!(*c_last) || *c_last != *c_first || run_len == 9) { 
            // end of run
            *(c_write++) = *c_first;
            if (run_len > 1)
                *(c_write++) = '0' + run_len;
            // start next run
            run_len = 0; 
            c_first = c_last;
        }
        ++str;
    }
    *c_write = 0;
    return start;
}


int main() {
	char *string = "askjaskjansaaaaaa";
	printf("Test 0\n");
	printf("Original string is: %s\n", string);
	printf("%s\n", compress(string));
	return 0;
}
