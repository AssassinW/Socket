/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define SERVER3_PORT "23552"  
#define MAXBUFLEN 100
#define BACKLOG 10	 // how many pending connections queue will hold

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // server2's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;
	char buf[MAXBUFLEN];
	
	struct key_server{
    	       char key[6];
               char value[8];
        };
   	char str[15];
  	struct key_server server_table[12];
  	FILE *fp;
	int numbytes,k=0;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	memset(&server_table,0,sizeof(server_table));
  	if((fp=fopen("server3.txt","r"))==NULL){		//read the file
    		printf("server3.txt cannot be opnened/n");
    		exit(1);
  	}
  	while(!feof(fp)){
    	   if(fgets(str,15,fp)!=NULL){
      		int i;
      		for(i=0;i<5;i++)
		   server_table[k].key[i]=str[i];
      		server_table[k].key[5]='\0';
      		for(i=6;i<13;i++)
		   server_table[k].value[i-6]=str[i];
      		server_table[k].value[7]='\0';
      		k++;
      
    	   }
    	   if(k==4)
		break;
  	}
  	fclose(fp);

	if ((rv = getaddrinfo("localhost", SERVER3_PORT, &hints, &servinfo)) != 0) {	//get information of local host
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) { 	//bind the socket
			close(sockfd);
			perror("server: bind");
			continue;
		}
		struct sockaddr_in *ipv4=(struct sockaddr_in *)p->ai_addr;
    		void *addr=&(ipv4->sin_addr);
    		inet_ntop(p->ai_family, addr, s, sizeof s);
    		printf("The Server 3 has TCP port number " SERVER3_PORT " and IP address %s.\n", s);
		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}


	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
			if ((numbytes = recv(new_fd, buf, MAXBUFLEN-1, 0)) == -1) {
	    		   perror("recv");
	    	           exit(1);
			}
			buf[numbytes] = '\0';
			struct sockaddr_in *sin;
     			sin=(struct sockaddr_in *)&their_addr;
     			inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr),s, sizeof s); 
     			printf("The Server 3 has received a request with key %s from Server 2 with port number %d and IP address %s\n",buf,sin->sin_port,s );
     			int i;
			char *t=&buf[4];
     			for(i=0; i<12; i++){
			   if(strcmp(server_table[i].key,t)==0){		//search the local table
				strcpy(buf,"POST ");
				strcat(buf,server_table[i].value);
				break;
			   }
			}
			if (send(new_fd, buf, sizeof buf, 0) == -1)		//send the value back to server2
				perror("send");
			printf("The Server 3 sends the value %s to Server 2 with port number %d and IP address %s.\n",buf,sin->sin_port,s);
			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}

