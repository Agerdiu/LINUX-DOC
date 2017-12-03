#include <stdio.h>  
#include <string.h> 
int main()
{  
    char szTest[1000] = {0};  
    int len = 0;  
    char* sub = "[PT]";

    FILE *fp = fopen("/var/log/kern.log", "r");  
    if(NULL == fp)  
    {  
        printf("failed to open log\n");  
        return 1;  
    }  
  
    while(!feof(fp))  
    {  
        memset(szTest, 0, sizeof(szTest));  
        fgets(szTest, sizeof(szTest) - 1, fp);
         // 包含了\n  
        if(strstr(szTest,sub)) printf("%s", szTest);  
    }  
  
    fclose(fp);  
  
    printf("\n");  
  
    return 0;  
}  
