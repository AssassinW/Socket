#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>

#define SERVER1_PORT "21552"
#define SERVER2_PORT "22552"
#define MAXBUFLEN 100

void *get_in_addr(struct sockaddr *sa)
{
  if(sa->sa_family==AF_INET){
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main()
{
  int sockfd,sockfd2;
  struct addrinfo hints,hints2, *servinfo,*servinfo2, *p;
  int rv;
  int numbytes;
  struct sockaddr_storage their_addr;
  char buf[MAXBUFLEN],sendbuf[MAXBUFLEN];
  socklen_t addr_len;
  char s[INET6_ADDRSTRLEN],s2[INET6_ADDRSTRLEN],t[INET6_ADDRSTRLEN];
  void *addr;
  struct key_server{
    char key[6];
    char value[8];
  };
  char str[15];
  struct key_server server_table[12];
  FILE *fp;
  int request=0;
  int k=0;
  char cmp[6];
  int client_num=0,client;

  memset(&hints, 0, sizeof(hints));
  memset(&hints2, 0, sizeof hints2);
  memset(server_table,0,sizeof(server_table));
  memset(cmp,0,sizeof cmp);
  if((fp=fopen("server1.txt","r"))==NULL){
    printf("server1.txt cannot be opnened/n");
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

  hints.ai_family=AF_INET;
  hints.ai_socktype=SOCK_DGRAM;
  hints.ai_flags=AI_PASSIVE;

  hints2.ai_family=AF_INET;
  hints2.ai_socktype=SOCK_STREAM;
  
  if((rv=getaddrinfo("localhost", SERVER1_PORT, &hints, &servinfo))!=0){
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }
  for(p=servinfo; p!= NULL;p=p->ai_next){
    if((sockfd=socket(p->ai_family,p->ai_socktype,p->ai_protocol))==-1){
	perror("listener: socket");
	continue;
    }
    if(bind(sockfd, p->ai_addr, p->ai_addrlen)==-1){
	close(sockfd);
	perror("listener: bind");
	continue;
    }
    struct sockaddr_in *ipv4=(struct sockaddr_in *)p->ai_addr;
    addr=&(ipv4->sin_addr);
    inet_ntop(p->ai_family, addr, s, sizeof s);
    printf("The Server 1 has UDP port number " SERVER1_PORT " and IP address %s.\n", s);
    break;	
  }
  if(p==NULL){
    fprintf(stderr, "listener: failed to bind socket\n");
    return 2;
  }
  
  freeaddrinfo(servinfo);
  
  addr_len=sizeof their_addr;
  while(1){
     client_num++;
     client=(client_num%2)?1:2;
     numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1, 0, (struct sockaddr *)&their_addr, &addr_len);
     if(numbytes==-1){
	perror("recvfrom");
	exit(1);
     }
     buf[numbytes]='\0';
     struct sockaddr_in *sin;
     sin=(struct sockaddr_in *)&their_addr;
     inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr),s, sizeof s); 
     printf("The Server 1 has received a request with key %s from client %d with port number %d and IP address %s.\n",buf,client,sin->sin_port,s );
     int i;
     char *t1=&buf[4];
     char *t2=&sendbuf[5];
     for(i=0; i<12; i++){
	   if(strcmp(server_table[i].key,t1)==0){
		strcpy(sendbuf,"POST ");
		strcat(sendbuf,server_table[i].value);
		struct sockaddr *client_addr;
  		client_addr=(struct sockaddr*)&their_addr;
		request=0;
		break;
	   }
	   if(i==11)
		request=1;
     }
     if(request==1){
	if ((rv = getaddrinfo("localhost", SERVER2_PORT, &hints2, &servinfo2)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo2; p != NULL; p = p->ai_next) {
		if ((sockfd2 = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd2, p->ai_addr, p->ai_addrlen) == -1) {
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
	
	if(send(sockfd2,buf, sizeof buf,0)==-1)
	   perror("send");
	printf("The Server 1 sends the request %s to the Server2.\n",buf);

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

	if ((numbytes = recv(sockfd2, sendbuf, MAXBUFLEN-1, 0)) == -1) {
	    perror("recv");
	    exit(1);
	}

	sendbuf[numbytes] = '\0';
	printf("The Server 1 received the value %s from the Server 2\n",sendbuf);
	close(sockfd2);
        printf("The Server 1 closed the TCP connection with Server 2.\n");

	

	for(i=0;i<12;i++){
	   if(strcmp(server_table[i].key,cmp)==0){
		strcpy(server_table[i].key,t1);
		strcpy(server_table[i].value,t2);
		break;
	   }
	   i++;    									
	}
     }
     struct sockaddr *client_addr;
     client_addr=(struct sockaddr*)&their_addr;
     if((numbytes=sendto(sockfd, sendbuf, strlen(sendbuf), 0, client_addr, addr_len))==-1){
	perror("talker: sendto");
	exit(1);
     }
     printf("The Server 1 sends the reply %s to Client %d with port number %d and IP address %s.\n",sendbuf,client,sin->sin_port,s);
  }
  
  close(sockfd);
}
