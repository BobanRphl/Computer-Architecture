#include <stdio.h>

main() 
{
   FILE *fp;
   char buff[255];
   fp=fopen("G:/CaD/lab2/memory.txt","r");
   //fscanf(fp,"%s",buff);
   //printf("a : %s", buff);
   fgets(buff,255,(FILE *)fp);
   printf("1 : %s", buff);
   fgets(buff,255,(FILE *)fp);
   printf("2 : %s", buff);
   fclose(fp);
}