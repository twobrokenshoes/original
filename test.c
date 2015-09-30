#include<stdio.h>

void unknow(int argc,...)
{
	int i = 1;
	void *p = (void *)(&argc);
	for( ;i <= argc; i++ )
	{
		p = p + sizeof(int);
		printf("%s\n",*(char **)p);
	}
}

int main()
{
	unknow(5,"hello world","well","just do it","only for test","error");
	return 0;
}
