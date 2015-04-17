#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFF_SIZE 1024



int connectPipe()
{
	int abrePipe = 0;

	abrePipe = open("fifo", O_WRONLY);

	return abrePipe;
}



int incrementar(char *nome[], unsigned valor)
{
	char buffer[BUFF_SIZE];
	int i  = 0, nNomes = 0, lixo = 0;
	int pipeServ = 0;

	if( (pipeServ = connectPipe()) <= 0 ) return 1;  

	while(nome[nNomes] != NULL)
	{
		nNomes++;
	}
	
	nNomes++;
	sprintf(buffer, "%d number", nNomes);	
	lixo = write(pipeServ, buffer, BUFF_SIZE);	
	
	while(nome[i] != NULL)
	{
		lixo = write(pipeServ, nome[i], BUFF_SIZE);
		i++;
	}

	sprintf(buffer, "1 %d useless", valor);	
	lixo = write(pipeServ, buffer, BUFF_SIZE);
	
	close(pipeServ);
	return 0;
	lixo++;
}



int agregar(char *prefixo[], unsigned nivel, char *path)
{
	char buffer[BUFF_SIZE];
	int i  = 0, nNomes = 0, lixo = 0;
	int pipeServ = 0;

	if( (pipeServ = connectPipe()) <= 0 ) return 1; 

	while(prefixo[nNomes] != NULL)
	{
		nNomes++;
	}

	nNomes++;
	sprintf(buffer, "%d number", nNomes);	
	lixo = write(pipeServ, buffer, BUFF_SIZE);		
	
	while(prefixo[i] != NULL)
	{
		lixo = write(pipeServ, prefixo[i], BUFF_SIZE);
		i++;
	}

	sprintf(buffer, "0 %d %s", nivel, path);	
	lixo = write(pipeServ, buffer, BUFF_SIZE);

	close(pipeServ);
	return 0;
	lixo++;
}




