#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
       #include <sys/select.h>
	   #include <stdio.h>
	   #include <stdlib.h>

int sockfd;
int Clienfd[1024] = {0};
int Clienid[1024] = {0};
int id = 0;
char *bufs[1024];

int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

void fatal()
{
	write(2, "Fatal error\n", 12);
	exit(1);
}

void Init(char *port)
{
	struct sockaddr_in servaddr; 

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1)
		fatal(); 
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(port)); 
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
			fatal(); 
	if (listen(sockfd, 10) != 0)
		fatal();
	Clienfd[sockfd] = sockfd;
}

void new()
{
	socklen_t len;
	int  connfd;
	struct sockaddr_in cli; 
	len = sizeof(cli);
	connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
	if (connfd < 0)
        fatal();
	char msg[100];
	Clienid[connfd] = id;
	sprintf(msg, "server: client %d just arrived\n", id++);
	for(int i = 0; i < 1024; ++i)
	{
		if (Clienfd[i] && Clienfd[i] != sockfd)
			send(Clienfd[i], msg, strlen(msg), 0);
	}
	Clienfd[connfd] = connfd;
}


int main(int ac, char **av) 
{
	if (ac != 2)
	{
		write(2, "Wrong number of argurments\n", 27);
		return 1;
	}
	Init(av[1]);
	while(1)
	{
		int max = 0;
		fd_set readfds;
		FD_ZERO(&readfds);
		for(int i = 0; i< 1024; ++i)
		{
			if (Clienfd[i])
				FD_SET(Clienfd[i], &readfds);
			if (max < Clienfd[i])
				max = Clienfd[i];
		}
		select(max +1, &readfds, NULL, NULL, NULL);
		for(int i = 0; i < 1024; ++i)
		{
			if (FD_ISSET(Clienfd[i], &readfds) && Clienfd[i] == sockfd)
				new();
			else if(FD_ISSET(Clienfd[i], &readfds))
			{
				char msg[100];
				char re[1000];
				int status = recv(Clienfd[i], re, sizeof(re), 0);
				if (status <= 0)
				{
					int fd = Clienfd[i];
					Clienfd[fd] = 0;
					free(bufs[fd]);
					bufs[fd] = NULL;
					sprintf(msg, "server: client %d just left\n", Clienid[fd]);
					for(int j = 0; j < 1024; ++j)
					{
						if (Clienfd[j] && Clienfd[j] != sockfd)
							send(Clienfd[j], msg, strlen(msg), 0);
					}
				}
				else
				{
					bufs[Clienid[i]] = str_join(bufs[Clienfd[i]], re);
					char *line;
					if (extract_message(&bufs[Clienid[i]], &line))
					{
						char envoe[1000];
						sprintf(envoe, "client %d: %s", Clienid[Clienfd[i]], line);
						for(int j = 0; j < 1024; ++j)
						{
							if (Clienfd[j] && Clienfd[j] != sockfd && Clienfd[j] != Clienfd[i])
								send(Clienfd[j], envoe, strlen(envoe), 0);
						}
						free(line);
					}
				}
			}
		}
	}
	close(sockfd);
	return 0;
}
