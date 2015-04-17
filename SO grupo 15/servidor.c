#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>


#define BUFFER_SERV 1024

struct listaContagem {
	int cont;
	int myId;
	char campo[256];
	struct listaContagem *lowerLvl;
	struct listaContagem *next;
};

struct listaContagem *topLvl = NULL;



int execArg(char *instruct[], int sizeArg);

int checkNivel(char *instruct[], int *nivel, int sizeArg)
{
	int func = 2;	
	
	sscanf(instruct[sizeArg-1], "%d", &func);
	
	if(func == 0)
	{
		if((*nivel) == 0) return 1;
		if((*nivel) > 2) return 1;
	}
	
	if((*nivel) == 0) 
	{
		(*nivel) = sizeArg-1;
	}
	if( (*nivel) != sizeArg-1 && func == 1 ) return 1;

	return 0;
}


/*
*Le do pipe e envia para a funçao execarg
*/
int main()
{
	int pipeRead, sizeArg = 0, i = 0, lixo, nivel = 0;
	char str[BUFFER_SERV], **instruct;

	mkfifo("fifo", 0666);

	pipeRead = open("fifo", O_RDONLY);

	str[0] = ' ';

	while(1)
	{
		lixo = read(pipeRead, str, BUFFER_SERV); 
		if (lixo <= 0)
		{
			close(pipeRead);	
            		pipeRead = open("fifo", O_RDONLY);
			continue;
        	}

		sscanf(str, "%d", &sizeArg);
		instruct = malloc(sizeof(char *) * sizeArg);

		while(i < sizeArg)
		{
			instruct[i] = malloc(sizeof(char) * BUFFER_SERV);
			lixo = read(pipeRead, instruct[i], BUFFER_SERV);
			i++;
		}
		
		if (checkNivel(instruct, &nivel, sizeArg) == 0) execArg(instruct, sizeArg);

		memset(str, '\0', BUFFER_SERV); lixo = 0; 
		i = 0;
		while(i<sizeArg){ free(instruct[i]); i++;}
		free(instruct);
		instruct = NULL; i = 0;	sizeArg = 0; 
	}

	return 0;
}



struct listaContagem *getTopLvl(char instruct[BUFFER_SERV]);

int topLvlProcess(struct listaContagem Topo, int reDo, int sizeReload);

/*
*Funçao que cria e/ou comunica para os processos filhos a informaçao das queries
*/
int execArg(char *instruct[], int sizeArg)
{
	int fpid = 0, fpipe, logfile, i = 0, j = 0, lixo, status = 15;
	char rndstr[BUFFER_SERV], topName[BUFFER_SERV];
	struct listaContagem *apontaTopo = NULL, *reboot = NULL;
	
	apontaTopo = getTopLvl(instruct[0]);

	if(apontaTopo->myId == 0)
	{	
		mkfifo(instruct[0], 0666);
		
		fpid = fork();		
		if(fpid == 0)
		{
			topLvlProcess((*apontaTopo), 0, 0);
			exit(0);
		}
		
		apontaTopo->myId = fpid;
	}
	else
	{
		if(waitpid(apontaTopo->myId, &status, WNOHANG) != 0)
		{
			fpid = fork();		
			if(fpid == 0)
			{
				topLvlProcess((*apontaTopo), 1, sizeArg);
				exit(1);
			}
		
			apontaTopo->myId = fpid;
		}
	}
	
	if(topLvl != NULL)
	{
		logfile = open("log.txt", O_RDONLY, 0666);
		lixo = read(logfile, rndstr, BUFFER_SERV);
		close(logfile);

		sscanf(rndstr, "%d %s", &j, topName);
		if(waitpid(j, &status, WNOHANG) != 0)
		{
			reboot = getTopLvl(topName);
			fpid = fork();		
			if(fpid == 0)
			{
				topLvlProcess((*reboot), 1, sizeArg);
				exit(1);
			}
		
			reboot->myId = fpid;
		}		
	}
	

	logfile = open("log.txt", O_RDWR | O_TRUNC | O_CREAT, 0666);
	
	memset(rndstr, '\0', BUFFER_SERV);
	sprintf(rndstr, "%d", apontaTopo->myId);
	j = strlen(rndstr);
	lixo = write(logfile, rndstr, j);
	lixo = write(logfile, " ", 1);

	while(i < sizeArg)
	{
		memset(rndstr, '\0', BUFFER_SERV);
		sprintf(rndstr, "%s", instruct[i]);
		j = strlen(rndstr);
		lixo = write(logfile, rndstr, j);
		lixo = write(logfile, " ", 1);
		i++;
	}
	lixo = write(logfile, "\n", 1);
	close(logfile);
	lixo ++;


	i = 1;
	fpipe = open(apontaTopo->campo, O_WRONLY);
	sprintf(rndstr, "%d number", sizeArg); 
	lixo = write(fpipe, rndstr, BUFFER_SERV);
	while(i < sizeArg)
	{
		lixo = write(fpipe, instruct[i], BUFFER_SERV);
		i++;
	}	
	
	close(fpipe);
	return 0;
}

struct listaContagem *getTopLvl(char instruct[BUFFER_SERV])
{
	int n = 1;
	struct listaContagem **aux, **last = NULL;

	aux = &topLvl; last = &topLvl;
	
	if((*aux) == NULL)
	{
		(*aux) = malloc(sizeof(struct listaContagem));
		(*aux)->cont = 0; (*aux)->myId = 0;
		strcpy((*aux)->campo, instruct);
		(*aux)->lowerLvl = NULL; (*aux)->next = NULL;
		return (*aux);
	}
	
	while((*aux) != NULL && n != 0)
	{
		n = strcmp(instruct, (*aux)->campo);
		last = aux;
		aux = &((*aux)->next);
	}
	
	if(n != 0)
	{
		(*last)->next = malloc(sizeof(struct listaContagem));
		last = &((*last)->next);
		(*last)->cont = 0; (*last)->myId = 0;
		strcpy((*last)->campo, instruct);
		(*last)->lowerLvl = NULL; (*last)->next = NULL;
	}
 
	return (*last);
}



int reloadData(struct listaContagem *myList, int sizeReload);

int incrementFunc(struct listaContagem *myList, char *comando[], int sizeArg);

int agregarFunc(struct listaContagem *myList, char *comando[], int sizeArg);

/*
*Funçao de topo dos processos filhos, recebe informação através de um pipe de nome e envia para a funçao respectiva
*/
int topLvlProcess(struct listaContagem Topo, int reDo, int sizeReload)
{
	int fpipe, n = 0, sizeArg = 0, i = 0, func = 0;
	char str[BUFFER_SERV], **comando;
	struct listaContagem *myList;
	
	myList = malloc(sizeof(struct listaContagem));
	myList->cont = Topo.cont; myList->myId = Topo.myId;
	strcpy(myList->campo, Topo.campo);
	myList->lowerLvl = NULL; myList->next = NULL;

	if(reDo == 1) reloadData(myList, sizeReload);

	fpipe = open(myList->campo, O_RDONLY); 

	while(1)
	{
		n = read(fpipe, str, BUFFER_SERV);
		if (n <= 0)
		{
			close(fpipe);	
            		fpipe = open(myList->campo, O_RDONLY);
			continue;
        	}
		
		sscanf(str, "%d", &sizeArg); sizeArg--;
		comando = malloc(sizeof(char *) * sizeArg);

		while(i < sizeArg)
		{
			comando[i] = malloc(sizeof(char) * BUFFER_SERV);
			n = read(fpipe, comando[i], BUFFER_SERV);
			i++;
		}
		
		func = 0;
		sscanf(comando[i-1], "%d", &func);
		if(func == 1) incrementFunc(myList, comando, sizeArg);
		else agregarFunc(myList, comando, sizeArg);

		memset(str, '\0', BUFFER_SERV); n = 0;	
		i = 0;
		while(i<sizeArg){ free(comando[i]); i++;}
		free(comando);
		comando = NULL; i = 0;	sizeArg = 0; 
	}

	return 0;
}


/*
*Funçao que caso o processo filho tenha saido, apos o reboot rele os dados de volta para a estrutura de dados.
*/
int reloadData(struct listaContagem *myList, int sizeReload)
{
	int filePointer = 0, lixo, i = 0, encon = 1;
	char fileName[200], fileBuffer[BUFFER_SERV], **inserts, *auxStr;
	struct listaContagem **aux = NULL, **last = NULL;
	unsigned inc = 0;

	inserts = malloc(sizeof(char *)*sizeReload);
	while(i < sizeReload){ inserts[i] = malloc(sizeof(char) * BUFFER_SERV); i++; }

	sprintf(fileName, "%s.txt", myList->campo);
	filePointer = open(fileName, O_RDONLY, 0666);

	i = 0;
	while(read(filePointer, fileBuffer, BUFFER_SERV) > 0)
	{
		auxStr = strtok (fileBuffer,":");
	  	while(auxStr != NULL)
	 	{
			strcpy(inserts[i], auxStr);
			i++;
	    		auxStr = strtok (NULL, ":");
		}
		
		sscanf(inserts[i], "%u", &inc);

		aux = &(myList->lowerLvl);
		i = 0;
		for(i = 0; i < (sizeReload-2); i++)
		{
			while((*aux)  != NULL && encon != 0)
			{
				encon = strcmp(inserts[i], (*aux)->campo);
				last = aux;
				aux = &((*aux)->next);
			}
			if(encon == 0)
			{ 
				(*last)->cont += inc; aux = last; 
				aux = &((*aux)->lowerLvl);
			}
			else
			{
				(*aux) = malloc(sizeof(struct listaContagem));
				(*aux)->cont = inc; (*aux)->myId = 0;
				strcpy((*aux)->campo, inserts[i]);
				(*aux)->lowerLvl = NULL; (*aux)->next = NULL;
			
			}

			aux = &((*aux)->lowerLvl); encon = 1;
		}
		i = 0;
	}

	close(filePointer);
}



void createNewLvl(struct listaContagem *myList, char *comando[], int sizeArg, int inc);

void writeFich(struct listaContagem *myList, char *comando[], int sizeArg, int encon, int inc);

/*
*Funçao uqe incrementa no ficheiro e estrutura de dados
*/
int incrementFunc(struct listaContagem *myList, char *comando[], int sizeArg)
{
	struct listaContagem **aux = NULL, **last = NULL;
	int lixo, encon = 1, i = 0, fich, n = 0;
	unsigned inc = 0;
	char savFich[BUFFER_SERV];

	sscanf(comando[sizeArg-1], "%d %u", &lixo, &inc);

	myList->cont += inc;
	aux = &(myList->lowerLvl);	

	if((*aux) == NULL)
	{
		sprintf(savFich, "%s.txt", myList->campo);
		fich = open(savFich, O_WRONLY | O_CREAT | O_TRUNC, 0666);	
	
		sprintf(savFich, "%s", myList->campo);
		
		while(i < (sizeArg-1))
		{
			sprintf(savFich, "%s:%s", savFich, comando[i]);
			i++;
		}
		sprintf(savFich, "%s:%d", savFich, inc);
		n = strlen(savFich);		
		memset((savFich+n), ' ', (BUFFER_SERV-n));
		savFich[1023] = '\n';

		lixo = write(fich, savFich, BUFFER_SERV);

		close(fich);

		createNewLvl(myList, comando, sizeArg, inc);

		return 0;
	}	

	for(i = 0; i < (sizeArg-1); i++)
	{
		while((*aux)  != NULL && encon != 0)
		{
			encon = strcmp(comando[i], (*aux)->campo);
			last = aux;
			aux = &((*aux)->next);
		}
		if(encon == 0)
		{ 
			(*last)->cont += inc; aux = last; 
			if( (i+1) == (sizeArg-1) ){ writeFich(myList, comando, sizeArg, encon, (*last)->cont); return 0; }
		}
		else
		{
			(*aux) = malloc(sizeof(struct listaContagem));
			(*aux)->cont = inc; (*aux)->myId = 0;
			strcpy((*aux)->campo, comando[i]);
			(*aux)->lowerLvl = NULL; (*aux)->next = NULL;
			
		}

		aux = &((*aux)->lowerLvl); encon = 1;
	}

	writeFich(myList, comando, sizeArg, encon, inc);
	
	return 0;
}

void createNewLvl(struct listaContagem *myList, char *comando[], int sizeArg, int inc)
{
	int i = 0;
	struct listaContagem **aux = NULL;
	
	aux = &(myList->lowerLvl); 

	for(i = 0; i < (sizeArg-1); i++)
	{		
		(*aux) = malloc(sizeof(struct listaContagem));
		(*aux)->cont = inc; (*aux)->myId = 0;
		strcpy((*aux)->campo, comando[i]);
		(*aux)->lowerLvl = NULL; (*aux)->next = NULL;
		aux = &((*aux) ->lowerLvl);
	}
}

void writeFich(struct listaContagem *myList, char *comando[], int sizeArg, int encon, int inc)
{
	char savFich[BUFFER_SERV], *auxStr;
	int fich = 0, i = 0, lixo = 0, n = 0, nEqual = 0, filePos = 0;

	if(encon != 0)
	{
		sprintf(savFich, "%s.txt", myList->campo);
		fich = open(savFich, O_RDWR | O_APPEND);	
	
		sprintf(savFich, "%s", myList->campo);
		
		while(i < (sizeArg-1))
		{
			sprintf(savFich, "%s:%s", savFich, comando[i]);
			i++;
		}
		sprintf(savFich, "%s:%d", savFich, inc);
		n = strlen(savFich);		
		memset((savFich+n), ' ', (BUFFER_SERV-n));
		savFich[1023] = '\n';

		lixo = write(fich, savFich, BUFFER_SERV);

		close(fich);
	}
	else
	{	
		sprintf(savFich, "%s.txt", myList->campo);
		fich = open(savFich, O_RDWR);	
		
		lseek(fich, 0, SEEK_SET);
		
		i = 0;
		lixo = read(fich, savFich, BUFFER_SERV);
		auxStr = strtok (savFich,":");
  		while(auxStr != NULL && i<(sizeArg-1))
 		{
			n = strcmp(auxStr, comando[i]);
			if(n == 0) nEqual++;
			n = strcmp(myList->campo, auxStr);
			i++;			
			if(n == 0 && i == 1) i = 0;
			
    			auxStr = strtok (NULL, ":");
		}
		
		while( nEqual < (sizeArg-1) && read(fich, savFich, BUFFER_SERV) != -1 )
		{
			filePos++;			
			nEqual = 0;
			i = 0;

			auxStr = strtok (savFich,":");
  			while(auxStr != NULL && i<sizeArg)
 			{
				n = strcmp(auxStr, comando[i]);
				if(n == 0) nEqual++;
				n = strcmp(myList->campo, auxStr);
				i++;			
				if(n == 0 && i == 1) i =0;
			
	    			auxStr = strtok (NULL, ":");
			}
		}		
		
		lseek(fich, (BUFFER_SERV*filePos), SEEK_SET);
		
		i = 0;
		sprintf(savFich, "%s", myList->campo);
		
		while(i < (sizeArg-1))
		{
			sprintf(savFich, "%s:%s", savFich, comando[i]);
			i++;
		}
		sprintf(savFich, "%s:%d", savFich, inc);
		n = strlen(savFich);		
		memset((savFich+n), ' ', (BUFFER_SERV-n));
		savFich[1023] = '\n';

		lixo = write(fich, savFich, BUFFER_SERV);

		close(fich);
	}

	lixo++;
}



/*
*Funçao que agrega os dados atraves da estrutura em memoria
*/
int agregarFunc(struct listaContagem *myList, char *comando[], int sizeArg)
{
	int i = 0, filePointer, lixo = 0, strSize = 0, nivel = 0;
	char fileName[BUFFER_SERV], fileBuffer[BUFFER_SERV], topStr[BUFFER_SERV];
	struct listaContagem *aux = NULL, *last = NULL;

	sscanf(comando[sizeArg-1], "%d %d %s", &lixo, &nivel, fileName);

	filePointer = open(fileName, O_WRONLY | O_CREAT | O_TRUNC, 0666);

	if(nivel > 2) return 1;

	sprintf(topStr, "%s", myList->campo);
	if(sizeArg != 1)
	{
		aux = myList->lowerLvl;
		while(i < (sizeArg-1) && aux != NULL)
		{
			lixo = strcmp(aux->campo, comando[i]);
			if(lixo == 0)
			{
				last = aux;
				aux = aux->lowerLvl;
				sprintf(topStr, "%s:%s", topStr, comando[i]);
				i++;
			}
			else
				aux = aux->next;			
		}
	}
	else last = myList;
	if(last == NULL) return 1;


	sprintf(fileBuffer, "%s:%d\n", topStr, last->cont);
	strSize = strlen(fileBuffer); 
	lixo = write(filePointer, fileBuffer, strSize);

	if(nivel > 0)
	{	
		last = last->lowerLvl;
		while(last != NULL)
		{
			sprintf(fileBuffer, "%s:%s:%d\n", topStr, last->campo, last->cont);
			strSize = strlen(fileBuffer);
			lixo = write(filePointer, fileBuffer, strSize);

			if(nivel == 2)
			{	
				aux = last->lowerLvl;
				while(aux != NULL)
				{
					sprintf(fileBuffer, "%s:%s:%s:%d\n", topStr, last->campo, aux->campo, aux->cont);
					strSize = strlen(fileBuffer);
					lixo = write(filePointer, fileBuffer, strSize);

					aux = aux->next;
				}
			}
			last = last->next;
		}	
	}

	close(filePointer);
	return 0;
}








