#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BITS sizeof(int) * 8 

typedef struct ObjRef {
	unsigned int size;
	unsigned char data[1];
	int brokenHeart;
	struct ObjRef* forwardPointer;
} *ObjRef;

int main(int argc, char *argv[]) {
	/* char str[] = "Ein Wort, das hier nicht hingehört: Mist!";
  	 char replace[] = "M***";
 	  char *ptr;

	   ptr = strstr(str, "Mist");
	   memcpy(ptr, replace, strlen(replace));
	   printf("%s\n",str);
	*/

	char *heap = malloc(4);
	char *temp;
	temp = heap + 8;
	printf("heap = %p\n", heap);
	printf("temp = %p\n", temp);

    

    return 0;
}
