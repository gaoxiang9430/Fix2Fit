#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
	int a, b, i;
	a = atoi(argv[1]);
	for(i=0; i<=a;)
  		if(i>3)
  			break; 
  		else
  			i++;
			
	printf("%d\n", i);
	return 0;
}
