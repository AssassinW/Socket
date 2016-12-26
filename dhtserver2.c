/*	dhtserver2	*/

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

#define SERVER2_PORT "22552"  
#define SERVER3_PORT "23552"

#define MAXBUFLEN 100
#define BACKLOG 10	 // how many pending connections queue will hold

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
	int sockfd, new_fd,sockfd2;  // listen on sock_fd, new connection on new_fd, sockfd2 for connection to server3
	struct addrinfo hints,hints2, *servinfo,*servinfo2, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN],s2[INET6_ADDRSTRLEN],t[INET6_ADDRSTRLEN];
	int rv;
	char recvbuf[MAXBUFLEN],sendbuf[MAXBUFLEN];
	void *addr,*addr2;
	int numbytes;

        struct key_server{		//structure used to store the key-value pair
    	       char key[6];
               char value[8];
        };
   	char str[15];
  	struct key_server server_table[12];	//local table
  	FILE *fp;
  	int request=0,k=0;

	memset(&hints, 0, sizeof hints);
	memset(&hints2, 0, sizeof hints2);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP
	
	hints2.ai_family = AF_UNSPEC;
	hints2.ai_socktype = SOCK_STREAM;

	memset(&server_table,0,sizeof(server_table));
  	if((fp=fopen("server2.txt","r"))==NULL){	//read the file
    		printf("server2.txt cannot be opnened/n");
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

	if ((rv = getaddrinfo("localhost", SERVER2_PORT, &hints, &servinfo)) != 0) {	//get information of local host
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,	//create a TCP socket
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {	//bind the socket
			close(sockfd);
			perror("server: bind");
			continue;
		}
		struct sockaddr_in *ipv4=(struct sockaddr_in *)p->ai_addr;
    		addr=&(ipv4->sin_addr);
    		inet_ntop(p->ai_family, addr, s, sizeof s);
    		printf("The Server 2 has TCP port number " SERVER2_PORT " and IP address %s.\n", s);
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

		inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr),s, sizeof s);
 
		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
			if ((numbytes = recv(new_fd, recvbuf, MAXBUFLEN-1, 0)) == -1) {		//receive the message from server1
	    		   perror("recv");
	    	           exit(1);
			}
			recvbuf[numbytes] = '\0';
			struct sockaddr_in *sin;
     			sin=(struct sockaddr_in *)&their_addr;
     			inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr),s, sizeof s); 
     			printf("The Server 2 has received a request with key %s from server 1 with port number %d and IP address %s\n",recvbuf,sin->sin_port,s );
     			int i;
			char *t1=&recvbuf[4];
			char *t2=&sendbuf[5];
     			for(i=0; i<12; i++){
			   if(strcmp(server_table[i].key,t1)==0){  //search the local table for match
				strcpy(sendbuf,"POST ");
				strcat(sendbuf,server_table[i].value);
				request=0;
				break;
			   }
			   else if(i==11)
				request=1;		//the local table doesn't have matched information
			}
			if(request==1){
			   if ((rv = getaddrinfo("localhost", SERVER3_PORT, &hints2, &servinfo2)) != 0) { //get the host information of server3
				fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
				return 1;
			   }

	// loop through all the results and connect to the first we can
			   for(p = servinfo2; p != NULL; p = p->ai_next) {
				if ((sockfd2 = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
					perror("client: socket");
					continue;
				}

				if (connect(sockfd2, p->ai_addr, p->ai_addrlen) == -1) {	//connect to server3
					close(sockfd2);
					perror("client: connect");
					continue;
				}

				break;
			   }

			   if (p == NULL) {
				fprintf(stderr, "client: failed to connect\n");
				return 2;
			   }
			   inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),s2, sizeof s2);
			   freeaddrinfo(servinfo2);
	
			   if(send(sockfd2,recvbuf, sizeof recvbuf,0)==-1)		//pass the message to server3
	   			perror("send");
	         	   printf("The Server 2 sends the request %s to the Server3.\n",recvbuf);

			   struct sockaddr_in addrMy;
    			   memset(&addrMy,0,sizeof(addrMy));
    			   int unsigned leng = sizeof(addrMy);
    			   int getsock_check = getsockname(sockfd2,(struct sockaddr *)&addrMy,&leng);
    			   if(getsock_check==-1){
				perror("getsockname");
				exit(1);
    			   }
    			   struct sockaddr_in *ipv4_c=&addrMy;
    			   void * addr_c=&(ipv4_c->sin_addr);
    			   inet_ntop(AF_INET, addr_c, t, sizeof t);
        		   printf("The TCP port number is %d and the IP address is %s.\n",ipv4_c->sin_port,t);

			   if ((numbytes = recv(sockfd2, sendbuf, MAXBUFLEN-1, 0)) == -1) {	//receive message from server3
	    			perror("recv");
	    			exit(1);
			   }

			   sendbuf[numbytes] = '\0';
			   printf("The Server 2 received the value %s from the Server 3 with port number "SERVER3_PORT" and IP address %s.\n",sendbuf,s2);
			   close(sockfd2);
			   printf("The Server 2 closed the TCP connection with Server 3.\n");

			   for(i=0;i<12;i++){
	   			if(server_table[i].key==NULL){			//save the key-value pair in local table for future use
					strcpy(server_table[i].key,t1);
					strcpy(server_table[i].value,t2);
					break;
	   			}
	   			i++;    									
			   }

			   
			}
			if (send(new_fd, sendbuf, sizeof sendbuf, 0) == -1)	//send the message to server1
				perror("send");
			printf("The Server 2 sends the reply %s to Server 1 with port number %d and IP address %s.\n",sendbuf,sin->sin_port,s);
			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}

