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
#define MAXBUFLEN 100

int main(){
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes=0;
    char buf[MAXBUFLEN];
    char name_search[5];
    void *addr,*addr_c;
    char s[INET6_ADDRSTRLEN],t[INET6_ADDRSTRLEN];
    socklen_t addr_len;
    struct sockaddr_storage their_addr;
    struct client_key{
	char name[5];
	char key[6];
    };
    struct client_key client_table[12];
    FILE *fp;
    int k=0,j;
    char str[15];
    
    memset(&client_table, 0, sizeof client_table);
    memset(&hints, 0, sizeof hints);
    hints.ai_family=AF_UNSPEC;
    hints.ai_socktype=SOCK_DGRAM;
    
    if((fp=fopen("client2.txt","r"))==NULL){		//read the file
    printf("client1.txt cannot be opnened/n");
    exit(1);
  }
  while(!feof(fp)){		
    if(fgets(str,150,fp)!=NULL){
      int i;
      for(i=0;i<3;i++)
	client_table[k].name[i]=str[i];
      if(str[3]==' '){
	client_table[k].name[3]='\0';
      }
      else{
	client_table[k].name[4]='\0';
	client_table[k].name[3]=str[3];
      }
      for(i=5;i<10;i++)
	client_table[k].key[i-5]=str[i];
      client_table[k].key[5]='\0';
      k++;
      
    }
    if(k==12)
	break;
  }
  fclose(fp);
/*The searching loop*/
  while(1){
  printf("Please Enter Your Search(USC,UCLA etc):");
  scanf("%s", name_search);
  if(name_search!=NULL){
    for(k=0;k<12;k++){
     if(strcmp(client_table[k].name,name_search)==0)
	break;
    }
    if(k<12){
	printf("The client 2 has received a request with search word %s, which maps to %s.\n",name_search, client_table[k].key);
        strcpy(buf,"GET ");
        strcat(buf,client_table[k].key);
    }
    else{
	printf("input error!\n");
	exit(1);
    }
  }
  
    if((rv=getaddrinfo("localhost", SERVER1_PORT, &hints, &servinfo))!=0){	//get information of server1
	fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
	return 1;
    }
    for(p=servinfo; p!=NULL; p=p->ai_next){
	if((sockfd=socket(p->ai_family, p->ai_socktype, p->ai_protocol))==-1){
	   perror("talker: socket");
	   continue;
	}
	break;
    }
    if(p==NULL){
	fprintf(stderr, "talker: failed to bind socket\n");
	return 2;
    }
    if((numbytes=sendto(sockfd, buf, strlen(buf), 0, p->ai_addr, p->ai_addrlen))==-1){	//send request to server1
	perror("talker: sendto");
	exit(1);
    }
    struct sockaddr_in *ipv4=(struct sockaddr_in *)p->ai_addr;
    addr=&(ipv4->sin_addr);
    inet_ntop(p->ai_family, addr, s, sizeof s);
    printf("The Client 2 sends the request GET %s to the Server1 with port number "SERVER1_PORT" and IP address %s.\n",client_table[k].key,s);
    struct sockaddr_in addrMy;
    memset(&addrMy,0,sizeof(addrMy));
    int unsigned leng = sizeof(addrMy);
    int getsock_check = getsockname(sockfd,(struct sockaddr *)&addrMy,&leng);
    if(getsock_check==-1){
	perror("getsockname");
	exit(1);
    }
    struct sockaddr_in *ipv4_c=&addrMy;
    struct hostent *addr_c = gethostbyname("localhost");
    struct in_addr **addr_list=(struct in_addr **)addr_c->h_addr_list;
    printf("The Client2's port number is %d and IP address is ",ipv4_c->sin_port);
    for(j=0;addr_list[j]!=NULL;j++){
	printf("%s.\n",inet_ntoa(*addr_list[j]));
    }

    while(1){
	if((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1, 0, (struct sockaddr *)&their_addr, &addr_len))==-1){	//receive the value from server1
	   perror("recvfrom");
	   exit(1);
	} 
        if(numbytes>0){
	   buf[numbytes]='\0';
	   printf("The Client 2 received the value %s from the server 1 with port number "SERVER1_PORT" and IP address %s.\n",buf,s);
	   printf("The Client2's port number is %d and IP address is ",ipv4_c->sin_port);
	   for(j=0;addr_list[j]!=NULL;j++)
		printf("%s.\n",inet_ntoa(*addr_list[j]));
	   break;
	}
    }
    freeaddrinfo(servinfo);
    close(sockfd);
    }
    return 0;
}
