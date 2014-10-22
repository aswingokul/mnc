//============================================================================
// Name        : Project1.cpp
// Author      : Aswin
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================
#include<sys/select.h>
#include<sys/time.h>
#include<unistd.h>
#include <iostream>
#include<cstdio>
#include<ifaddrs.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<cstdlib>
#include<cstring>
#include<list>
#define STDIN 0
#include<vector>
#include<algorithm>


using namespace std;

void displayObj(struct node obj);
string concatenate(struct node obj, string out);
void broadcast(vector<node> vec);
int portnum;
int sockfd, status;
struct sockaddr_in server_addr;

void help();
void creator();

struct node{
	string connection_id;
	string socket_id;
	string hostname;
	string ipaddress;
	string port;
	string connected;
};

void creator(){
	const char sl = '/';
	const char hash = '#';
	cout << "I have read and understood the course academic integrity policy located at	http:" <<sl<<sl <<"www.cse.buffalo.edu"<<sl<<"faculty" << sl <<"dimitrio"<<sl<<"courses"<<sl<<"cse4589_f14"<<sl<<"index.html"<<hash<<"integrity\n";

}

void help(){
	printf("Commands List:\n");
	printf("1. CREATOR -- Info about the program\n");
	printf("2. HELP -- man pages about the command\n");
	printf("3. MYIP -- Display the IP address of this process\n");
	printf("4. MYPORT -- T Display the port on which this process is listening for incoming connections\n");
	printf("5. REGISTER <server IP> <port_no> -- This command is used by the client to register itself with the server and to get the IP and listening port numbers of all the peers currently registered with the server\n");
	printf("6. CONNECT <destination> <port no> -- This command establishes a new TCP connection to the specified <destination> at the specified < port no>\n");
	printf("7. LIST -- Display a numbered list of all the connections this process is part of\n");
	printf("8. TERMINATE <connection id> -- This command will terminate the connection listed under the specified number when LIST is used to display all connections\n");
	printf("9. EXIT -- Close all connections and terminate the process\n");
	printf("10. UPLOAD <connection id> <file name> -- Upload a file to the host identified by the <connection id> \n");
	printf("11. DOWNLOAD <connection id> <file>  -- Download a file from the host identified by the <connection id>. Supports upto 3 parallel downloads \n");
	printf("12. STATISTICS -- Displays file transfer statistics\n ");

}

/*=======================================================================================
 * Description: Displays the ip address of the current machine. when MYIP command is given this will get
 * executed
 *
 *Used by both client and server
 * ========================================================================================
 */

void getipaddr(char *address){


	struct sockaddr_in si_me, si_other;


	int socketfd ; // The socket descriptor
	socketfd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if (socketfd == -1)  std::cout << "socket error " ;

	bzero(&si_me,sizeof(si_me));
	bzero(&si_other,sizeof(si_other));

	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(4567);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);


	//"connect" google's DNS server at 8.8.8.8 , port 4567
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(4567);
	si_other.sin_addr.s_addr = inet_addr("8.8.8.8");
	if(connect(socketfd,(struct sockaddr*)&si_other,sizeof si_other) == -1)
		cerr<< "connect error"<<endl;

	struct sockaddr_in my_addr;
	socklen_t len = sizeof my_addr;
	if(getsockname(socketfd,(struct sockaddr*)&my_addr,&len) == -1)
		cerr<< "getsockname error"<<endl;

	//print the local address of the socket that will be used when
	//sending datagrams to the "connected" peer.
	address = inet_ntoa(my_addr.sin_addr);
	cout<< inet_ntoa(my_addr.sin_addr);
	cout<<"\n";
}

/*==================================================================================
 * List command
 * Displays the connection list
 *
 * =================================================================================
 */
void displayConnections(string connections[]){
	char *tokens = new char[50];
	char* conn = new char[100];
	memset(tokens,'\0',sizeof(tokens));
	memset(conn,'\0',sizeof(conn));
	if(connections[0].empty()){
		cout<<"No connections found\n";
	}else{
		cout<<"Connection ID\t\t";
		cout<<"Host Name\t\t";
		cout<<"IP Address\t\t";
		cout<<"Port Number\t\t\n";

		for(size_t i = 0;i<sizeof(connections);i++){
			strcpy(conn,connections[i].c_str());
			tokens = strtok(conn,":");

			while(tokens!=NULL){
				cout<<tokens<<"\t\t";
				tokens = strtok(NULL,":");
			}
			cout<<"\n";
		}
	}
}

/*===================================================================================
 * Client Module
 * same as the server module
 *
 * ==================================================================================
 */
void clientModule(const char *listenport){
	int status;
	struct addrinfo host_info;       // The struct that getaddrinfo() fills up with data.
	struct addrinfo *host_info_list; // Pointer to the to the linked list of host_info's.
	char *ipadd;
	ipadd = (char *)listenport;

	char* input;
	char *token;
	vector<string> token_items;

	fd_set master;
	fd_set read_fds;

	int listener, fdmax;
	int conn_sockfd;
	int socketfd ; // The socket descripter
	struct sockaddr_storage remoteaddr;
	socklen_t addrlen = sizeof(remoteaddr);
	char ipstr[INET_ADDRSTRLEN];

	char incoming_data[2048];
	ssize_t bytes_recvd;
	ssize_t bytes_sent;

	int peer_conn_no = 1;


	char hostname[100];
	memset(hostname,0,sizeof(hostname));

	vector<string> peerDetails;
	vector<string> peers;

	vector<node> connection;

	int serv_connected = 0;
	int peer_connected = 0;
	string terminated = string("0");
	string disconnected = string("-1");




	FD_ZERO(&master);
	FD_ZERO(&read_fds);

	// The MAN page of getaddrinfo() states "All  the other fields in the structure pointed
	// to by hints must contain either 0 or a null pointer, as appropriate." When a struct
	// is created in c++, it will	1 be given a block of memory. This memory is not nessesary
	// empty. Therefor we use the memset function to make sure all fields are NULL.
	memset(&host_info, 0, sizeof host_info);



	host_info.ai_family = AF_UNSPEC;     // IP version not specified. Can be both.
	host_info.ai_socktype = SOCK_STREAM; // Use SOCK_STREAM for TCP or SOCK_DGRAM for UDP.
	host_info.ai_flags = AI_PASSIVE;     // IP Wildcard

	// Now fill up the linked list of host_info structs with google's address information.
	status = getaddrinfo(NULL, listenport, &host_info, &host_info_list);
	// getaddrinfo returns 0 on succes, or some other value when an error occured.
	// (translated into human readable text by the gai_gai_strerror function).
	if (status != 0)  std::cout << "getaddrinfo error" << gai_strerror(status) ;

	/*
	 * Create a socket for the server to listen
	 */
	listener = socket(host_info_list->ai_family, host_info_list->ai_socktype,
			host_info_list->ai_protocol);
	if (listener == -1){
		cout << "Socket Error";
		exit(0);
	}

	// we use to make the setsockopt() function to make sure the port is not in use
	// by a previous execution of our code. (see man page for more information)
	int yes = 1;
	status = setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
	status = bind(listener, host_info_list->ai_addr, host_info_list->ai_addrlen);
	if (status == -1){
		std::cout << "bind error: " <<gai_strerror(status)<< std::endl ;
		close(listener);
		//return;
	}

	/*
	 * Listen()ing to the socket for any incoming requests
	 */

	status =  listen(listener, 10);
	if (status == -1) {
		std::cout << "listen error" << std::endl ;
		return;
	}


	FD_SET(listener,&master);
	FD_SET(STDIN, &master);
	fdmax = listener;

	while(1){
		read_fds = master;
		cout<<"{PA1}>>";
		fflush(stdout);
		status =(select(fdmax+1, &read_fds, NULL, NULL, NULL));
		if (status==-1){
			cerr<<"select error: "<<gai_strerror(status);
		}
		for(int i = 0; i<fdmax+1; i++){
			if(FD_ISSET(i,&read_fds)){
				if(i == STDIN){
					input = new char[100];
					//cout<<"i: "<<i<<endl;
					memset(input,0,sizeof(input));
					gets(input);
					input[strlen(input)] = '\0';
					//cout<<"input: "<<input<<endl;
					token = new char[100];
					memset(token,0,sizeof(token));
					token = strtok(input," ");
					//cout<<"token: "<<token<<endl;
					token_items.clear();
					while(token != NULL){
						token_items.push_back(token);
						//cout<< "token inserted into vector: "<<token<<"\n";
						token = strtok(NULL," ");
					}
					peer_connected = 0;
					//cout<<"token_items[0]: "<<token_items[0]<<"\n";
					if(strcasecmp((char *)token_items[0].c_str(),"CREATOR")==0){
						creator();
						memset(&input,0,sizeof(input));
						break;
					}else if(strcasecmp((char *)token_items[0].c_str(),"HELP")==0){
						help();
						memset(&input,0,sizeof(input));
						break;
					}else if(strcasecmp((char *)token_items[0].c_str(),"LIST")==0){
						cout<<"Conn ID\t Host Name\t IP Address\t Port No\n";

						for(size_t j = 0; j<connection.size(); j++){
							if(connection[j].connection_id == terminated || connection[j].connection_id == disconnected){
								continue;
							}else{
								cout<<connection[j].connection_id<<"\t";
								cout<<connection[j].hostname<<"\t";
								cout<<connection[j].ipaddress<<"\t";
								cout<<connection[j].port<<"\n";
							}
						}
						memset(&input,0,sizeof(input));
						break;
					}else if(strcasecmp((char *)token_items[0].c_str(),"MYPORT")==0){
						cout<< listenport<<"\n";
						memset(&input,0,sizeof(input));
						break;
					}else if(strcasecmp((char *)token_items[0].c_str(),"MYIP")==0){
						getipaddr(ipadd);
						cout<<"{PA1}>>";
						fflush(stdout);
						break;
					}else if(strcasecmp((char *)token_items[0].c_str(),"UPLOAD")==0){
						memset(&input,0,sizeof(input));
						break;
					}else if(strcasecmp((char *)token_items[0].c_str(),"DOWNLOAD")==0){
						memset(&input,0,sizeof(input));
						break;
					}else if(strcasecmp((char *)token_items[0].c_str(),"REGISTER")==0){
						if((!(token_items[1].empty())) && (!(token_items[2].empty()))){
						//	cout << "token_items[1]: " << token_items[1]<<endl;
						//	cout << "token_items[2]: " << token_items[2]<<endl;

							/*========================================================
							 *Sends the register request to server
							 * =======================================================
							 */

							gethostname(hostname, 1023);
						//	cout<<"Hostname: "<<hostname<<endl;

							int status_ip = strcmp("128.205.36.32",token_items[1].c_str());
							//cout<<"status_ip: "<< status_ip<<"\n";
							int status_hn = strcmp("dokken.cse.buffalo.edu",token_items[1].c_str());
							//cout<<"status_hn: "<< status_hn<<"\n";

							if((status_hn == 0 || status_ip == 0) && serv_connected == 1){

								cout<<"No duplicate connections allowed\n";
								break;
							}
							memset(&host_info, 0, sizeof host_info);

							//std::cout << "Setting up the structs..."  << std::endl;

							host_info.ai_family = AF_UNSPEC;     // IP version not specified. Can be both.
							host_info.ai_socktype = SOCK_STREAM; // Use SOCK_STREAM for TCP or SOCK_DGRAM for UDP.

							// Now fill up the linked list of host_info structs with google's address information.
							status = getaddrinfo(token_items[1].c_str(), token_items[2].c_str(), &host_info, &host_info_list);
							// getaddrinfo returns 0 on succes, or some other value when an error occured.
							// (translated into human readable text by the gai_gai_strerror function).
							if (status != 0)  std::cout << "getaddrinfo error" << gai_strerror(status) ;


							//std::cout << "Creating a socket..."  << std::endl;

							socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype,
									host_info_list->ai_protocol);
							if (socketfd == -1)  std::cout << "socket error: "<< gai_strerror(socketfd) ;
							FD_SET(socketfd,&master);
							if(socketfd > fdmax)
								fdmax = socketfd;
							serv_connected = 1;

							//std::cout << "Connect()ing..."  << std::endl;
							status = connect(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
							if (status == -1){
								std::cout << "connect error: "<< gai_strerror(status);
								return;
							}


							//std::cout << "send()ing message through socketfd: "<<socketfd<< std::endl;

							int len;

							strcat(hostname,":");
							strcat(hostname,listenport);
							//strcat(hostname," ");
							len = strlen(hostname);
							bytes_sent = send(socketfd, hostname, len, 0);
							//std::cout<<"outgoing data: "<<hostname<<endl;
							//std::cout << "Bytes sent: "  << bytes_sent<<std::endl;

							char sockstr[10];
							sprintf(sockstr,"%d",socketfd);

							struct node servobj;
							servobj.socket_id = string(sockstr);
							servobj.connection_id = string("1");
							servobj.hostname = string("dokken.cse.buffalo.edu");
							servobj.ipaddress = string("128.205.36.32");
							servobj.port = string(token_items[2]);
							servobj.connected = string("1");
							connection.push_back(servobj);

//							cout<<"Added into the connection vector<node>\n ";
//							cout<< "server socket id: "<<connection[0].socket_id<<"\n";
//							cout<< "server connection_id: "<<connection[0].connection_id<<"\n";
//							cout<< "server hostname: "<<connection[0].hostname<<"\n";
//							cout<< "server ipaddress: "<<connection[0].ipaddress<<"\n";
//							cout<< "server port: "<<connection[0].port<<"\n";
//							cout<< "server connected: "<<connection[0].connected<<"\n";


							freeaddrinfo(host_info_list);
							/*==================================================================================
							 *Ending of Register
							 * ===================================================================================
							 */


						}else{
							cerr<<"Bad command!! - Refer help"<<endl;
						}
						break;
					}else if(strcasecmp((char *)token_items[0].c_str(),"CONNECT")==0){
						if((!(token_items[1].empty())) && (!(token_items[2].empty()))){
//							cout << "token_items[1]" << token_items[1]<<endl;
	//						cout << "token_items[2]" << token_items[2]<<endl;

							/*========================================================
							 *Sends the connect request to peers
							 * =======================================================
							 */
							int flag = 0;
							struct node peer;
							int status_self_hn, status_self_ip;
							gethostname(hostname, 1023);
							cout<<"Hostname: "<<hostname<<endl;
							//printf("Hostname: %s\n", hostname);
	//						struct hostent* h;
		//					h = gethostbyname(hostname);
							//printf("h_name: %s\n", h->h_name);
							status_self_hn = strcmp(hostname,token_items[1].c_str());
							//cout<< "status_self_hn: "<< status_self_hn<<"\n";
							status_self_ip = strcmp(ipadd,token_items[1].c_str());
							//cout<<"status_self_ip: "<< status_self_ip<<"\n";
							if(status_self_hn == 0 || status_self_ip == 0){
								cout<<"No self connections allowed\n";
								break;
							}
							memset(&host_info, 0, sizeof host_info);

							//std::cout << "Setting up the structs..."  << std::endl;

							host_info.ai_family = AF_UNSPEC;     // IP version not specified. Can be both.
							host_info.ai_socktype = SOCK_STREAM; // Use SOCK_STREAM for TCP or SOCK_DGRAM for UDP.

							// Now fill up the linked list of host_info structs with google's address information.
							status = getaddrinfo(token_items[1].c_str(), token_items[2].c_str(), &host_info, &host_info_list);
							// getaddrinfo returns 0 on succes, or some other value when an error occured.
							// (translated into human readable text by the gai_gai_strerror function).
							if (status != 0)  std::cout << "getaddrinfo error" << gai_strerror(status) ;


							//std::cout << "Creating a socket..."  << std::endl;

							conn_sockfd = socket(host_info_list->ai_family, host_info_list->ai_socktype,
									host_info_list->ai_protocol);
							if (conn_sockfd == -1)  std::cout << "socket error: "<< gai_strerror(conn_sockfd) ;
							FD_SET(conn_sockfd,&master);
							if(conn_sockfd > fdmax)
								fdmax = conn_sockfd;
							//FD_SET(socketfd,&read_fds);


							//std::cout << "Connect()ing..."  << std::endl;
							status = connect(conn_sockfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
							if (status == -1){
								std::cout << "connect error: "<< gai_strerror(conn_sockfd);
								break;
							}


							//std::cout << "send()ing message through socketfd: "<<conn_sockfd<< std::endl;

							int len;
							//ssize_t bytes_sent;
							strcat(hostname,":");
							strcat(hostname,listenport);
							strcat(hostname," ");
							len = strlen(hostname);
							bytes_sent = send(conn_sockfd, hostname, len, 0);
//							std::cout<<"outgoing data: "<<hostname<<endl;
	//						std::cout << "Bytes sent: "  << bytes_sent<<std::endl;

							for(int j= 0; j<connection.size(); j++){
								if((connection[j].ipaddress == token_items[1] ||
										connection[j].hostname == token_items[1])){
									if(connection[j].socket_id > terminated){
										peer_connected = 1;
										break;
									}
								}
							}


							//Adding to the connlist
							if(peer_connected == 1){
								cout<<"No duplicate connections allowed\n";
								break;
							}
							char clsock_str[10];
							int status_hn,status_ip,status_port;
							sprintf(clsock_str,"%d",conn_sockfd);
							peer_conn_no++;
							char sPeer_conn_no[10];
							sprintf(sPeer_conn_no,"%d",peer_conn_no);

							for(size_t i=0; i<peerDetails.size();i+=3){
//								cout<<"Before entering the big ass loop\n";
								cout<<"peerDetails["<<i<<"]:" <<peerDetails[i]<<"\n";
								cout<<"peerDetails["<<i+1<<"]:" <<peerDetails[i+1]<<"\n";
								cout<<"peerDetails["<<i+2<<"]:" <<peerDetails[i+2]<<"\n";
								cout<<" token_items[1]: "<< token_items[1]<<"\n";
								cout<<" token_items[2]: "<< token_items[2]<<"\n";
								status_hn = strcmp(peerDetails[i].c_str(),token_items[1].c_str());
							//	status_hn = (peerDetails[i] == token_items[1]?0:-1);
								status_ip = strcmp(peerDetails[i+1].c_str(),token_items[1].c_str());
								//status_ip = (peerDetails[i+2]==token_items[1]?0:-1);
								status_port = strcmp(peerDetails[i+2].c_str(),token_items[2].c_str());
								//status_port = (peerDetails[i+1] == token_items[2]?0:-1);
//								cout<<"status_hn: "<< status_hn<<"\n";
//								cout<<"status_ip: "<< status_ip <<"\n";
//								cout<<"status_port: "<< status_port <<"\n";
								if((status_hn ==0 || status_ip == 0) && (status_port == 0)){
//									cout<<"Entering after that big ass if condition\n";
									//strcat(conndet,":");
									peer.socket_id = clsock_str;
									peer.hostname = peerDetails[i];
									peer.ipaddress = peerDetails[i+1];
									peer.port = peerDetails[i+2];
									peer.connection_id = string(sPeer_conn_no);
									peer.connected = string("1");

									connection.push_back(peer);
									flag = 1;
									break;
								}
							}
							if(flag == 0){
								cout<<"No specified connection found\n";
							}


							freeaddrinfo(host_info_list);
							/*==================================================================================
							 *Ending of Connect
							 * ===================================================================================
							 */

						}else{
							cerr<<"Bad command!! - Refer help"<<endl;
						}
						break;
					}else if(strcasecmp((char *)token_items[0].c_str(),"TERMINATE")==0){
						//char out_buff[25] = "Terminate\0";
						int sock;
//						cout<<"Inside Terminate\n";
//						cout<<"----------------------\n";
						if(token_items[1] == connection[0].connection_id){
							cout<<"Terminate is not applicable for server\n";
							break;
						}
						for(size_t j=0; j < connection.size();j++){
							if(connection[j].connection_id == token_items[1]){
								sock = atoi(connection[j].socket_id.c_str());
								close(sock);
								FD_CLR(sock,&master);
								connection[j].connection_id = string("0");
								connection[j].socket_id = string("0");
								displayObj(connection[j]);
								break;
							}
						}
						break;
					}else if(strcasecmp((char *)token_items[0].c_str(),"EXIT")==0){
						int serv_sock;
						string serv_conn_id = string("1");
						if(connection[0].connection_id == serv_conn_id){
							serv_sock = atoi(connection[0].socket_id.c_str());
							close(serv_sock);
							FD_CLR(serv_sock,&master);
						}
						exit(0);
					}else if(strcasecmp((char *)token_items[0].c_str(),"STATISTICS")==0){
						break;
					}
				}else if(i == listener){
					char *localtoken = new char[2048];
					char *conn_ok = "connection successful!\0";
					char peer_sock[10];
					int rec_conn_sockfd;
					vector<string> local_vec;
					struct node connected_peer;
					char* smalltok = new char[50];
					char* sPeer_conn_no = new char[10];

					memset(&remoteaddr,0,addrlen);
					rec_conn_sockfd = accept(listener, (struct sockaddr*)&remoteaddr,&addrlen);
					if(rec_conn_sockfd<0){
						cerr<<"accept error: "<<gai_strerror(rec_conn_sockfd);
					}else{
						FD_SET(rec_conn_sockfd,&master);
						if(rec_conn_sockfd > fdmax){
							fdmax = rec_conn_sockfd;
						}
						cout<<"Accepting a new connection from a peer"<<endl;
						/*===============================================================
						 *Accepting connection from new peers
						 * ==============================================================
						 */
						cout<<"Accepting in the new socket fd "<<i<<endl;
						bytes_recvd = recv(rec_conn_sockfd, incoming_data,sizeof(incoming_data), 0);
//						cout << "bytes recieved: "<< bytes_recvd <<endl;
						incoming_data[bytes_recvd] = '\0';
//						cout << "incoming_data: "<< incoming_data<<endl;

						memset(&remoteaddr,0,addrlen);
						getpeername(rec_conn_sockfd,(struct sockaddr*) &remoteaddr,&addrlen);
						struct sockaddr_in *remoteip = (struct sockaddr_in *)&remoteaddr;
//						int remoteport = ntohs(remoteip->sin_port);
						inet_ntop(AF_INET,&remoteip->sin_addr,ipstr,sizeof(ipstr));
//						cout << "remote ip address: " << ipstr <<endl;
//						cout << "remote outgoing port: " << remoteport << endl;

						/*-----------------------------------------------------
						 * New Code using structures
						 * ----------------------------------------------------
						 */
//						cout<<"adding client conn details to connlist"<<endl;
						strcpy(localtoken,incoming_data);
						smalltok = strtok(localtoken,":");
						local_vec.clear();
						while(smalltok!=NULL){
							local_vec.push_back(smalltok);
							smalltok = strtok(NULL,":");
						}
						sprintf(peer_sock,"%d",rec_conn_sockfd);
						peer_conn_no++;
						sprintf(sPeer_conn_no,"%d",peer_conn_no);
						connected_peer.hostname = string(local_vec[0]);
						connected_peer.port = string(local_vec[1]);
						connected_peer.ipaddress = string(ipstr);
						connected_peer.socket_id = string(peer_sock);
						connected_peer.connection_id = string(sPeer_conn_no);
						connected_peer.connected = string("1");

						connection.push_back(connected_peer);
						bytes_sent = send(rec_conn_sockfd,conn_ok,strlen(conn_ok),0);
						cout<<"bytes_sent: "<<bytes_sent<<"\n";
						fflush(stdout);
//						cout<<"Listener in client module ends"<<endl;
					}
				}
				/*================================================================
				 * Final else part in the client module
				 * ==============================================================
				 */
				else{
					char *intokens = new char[500];
					char *conntokens = new char[100];
					char tmpconntokens[100];
					char tmp[100];
//					cout<<"Final else part in the client module"<<endl;
//					cout<<"Accepting in the new socket fd "<<i<<endl;
					bytes_recvd = recv(i, incoming_data,1000, 0);
					incoming_data[bytes_recvd] = '\0';
					if(bytes_recvd <= 0){
						if(bytes_recvd == 0){
							cout<<"Connection from socket "<<i<<"terminated\n";
							char sSock[10];
							sprintf(sSock,"%d",i);
//							cout<<"Inside Terminate of the listening client\n";
//							cout<<"------------------------------------------\n";
							string stSock = string(sSock);
							for(size_t j= 0; j<connection.size();j++){
								if(connection[j].socket_id == stSock){
									connection[j].connection_id = string("0");
									connection[j].socket_id = string("0");
									displayObj(connection[j]);
									close(i);
									FD_CLR(i,&master);
									break;
								}
							}
						}
						else
							cerr<<"recv error: "<< endl;
						close(i);
						FD_CLR(i,&master);
					}else{
						vector<string>::iterator it_peers;
						vector<string>::iterator it_peerDetails;
//						cout<<"We got some data!\n";
//						cout << "incoming_data: "<<incoming_data <<endl;
						if(strcmp(incoming_data,"connection successful!")==0){
							cout<<incoming_data<<"\n";
						}else{
							strcpy(tmp,incoming_data);
//							cout<<"tmp: "<<tmp<<"\n";
							intokens = strtok(tmp,";");
//							cout<<"intokens: "<<intokens<<"\n";
//							cout<<"Entering the tokenizer loop for peers\n";
							peers.clear();
							while(intokens!=NULL){
								//cout<<"intokens: "<<intokens<<"\n";
								peers.push_back(intokens);
								//cout<<"After push_back\n";
								intokens = strtok(NULL,";");

							}
							char *connection = new char[100];
							strcpy(connection,"0");
							strcat(connection,":");
//							cout<<"Entering the tokenizer loop for peerDetails\n";
							peerDetails.clear();
							for(size_t i = 0; i<peers.size(); i++){
								if(peers[i].empty()){
									continue;
								}else{
									strcpy(tmpconntokens,peers[i].c_str());
									conntokens = strtok(tmpconntokens,":");
									while(conntokens!=NULL){
//										cout<<"conntokens: "<<conntokens<<"\n";
										peerDetails.push_back(conntokens);
										conntokens = strtok(NULL,":");
									}
								}
							}

//							cout<<"printing peers vector\n";
							for(size_t i= 0; i<peers.size();i++){
								cout<<"peers["<<i<<"]: "<<peers[i]<<"\n";
							}
						}
					}
				}
				fflush(stdout);
			}
		}
	}
}
/*============================================================================================
 * End of Client Module
 * ===========================================================================================
 */


/*==========================================================
 * General Functional module for creating server listening
 * port
 *
 * Used by Server
 * ========================================================
 */
void serverModule(const char *serverport){
	int status;
	struct addrinfo host_info;       // The struct that getaddrinfo() fills up with data.
	struct addrinfo *host_info_list; // Pointer to the to the linked list of host_info's.
	char *ipadd;
	ipadd = (char *)serverport;
	int server_peer_no=0;
//	char *claddr = (char *)malloc(500);
	char* input;
	char *token;
	vector<string> token_items;
	fd_set master;
	fd_set read_fds;
	int listener, fdmax;
	int newfd;
	struct sockaddr_storage remoteaddr;
	socklen_t addrlen = sizeof(remoteaddr);
	char ipstr[INET_ADDRSTRLEN];
//	string connlist[10];
//	string dupconnlist[10];
	char incoming_data[2048];
	ssize_t bytes_recvd;
//	ssize_t bytes_sent;
//	char hostname[250] = "timberlake.cse.buffalo.edu";
//	int outlen;
	vector<node> server_connection;
	FD_ZERO(&master);
	FD_ZERO(&read_fds);

	// The MAN page of getaddrinfo() states "All  the other fields in the structure pointed
	// to by hints must contain either 0 or a null pointer, as appropriate." When a struct
	// is created in c++, it will be given a block of memory. This memory is not nessesary
	// empty. Therefor we use the memset function to make sure all fields are NULL.
	memset(&host_info, 0, sizeof host_info);


	host_info.ai_family = AF_UNSPEC;     // IP version not specified. Can be both.
	host_info.ai_socktype = SOCK_STREAM; // Use SOCK_STREAM for TCP or SOCK_DGRAM for UDP.
	host_info.ai_flags = AI_PASSIVE;     // IP Wildcard

	// Now fill up the linked list of host_info structs with google's address information.
	status = getaddrinfo(NULL, serverport, &host_info, &host_info_list);
	// getaddrinfo returns 0 on succes, or some other value when an error occured.
	// (translated into human readable text by the gai_gai_strerror function).
	if (status != 0)  std::cout << "getaddrinfo error" << gai_strerror(status) ;

	/*
	 * Create a socket for the server to listen
	 */
	listener = socket(host_info_list->ai_family, host_info_list->ai_socktype,
			host_info_list->ai_protocol);
	if (listener == -1)  std::cout << "socket error " ;

	// we use to make the setsockopt() function to make sure the port is not in use
	// by a previous execution of our code. (see man page for more information)
	int yes = 1;
	status = setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
	status = bind(listener, host_info_list->ai_addr, host_info_list->ai_addrlen);
	if (status == -1) {
		std::cout << "bind error: " <<gai_strerror(status)<< std::endl ;
		close(listener);
	}

	/*
	 * Listen()ing to the socket for any incoming requests
	 */
	//std::cout << "Listen()ing for connections..."  << std::endl;
	status =  listen(listener, 100);
	if (status == -1)  std::cout << "listen error" << std::endl ;

	//memset(connlist,0,sizeof(connlist));
	FD_SET(listener,&master);
	FD_SET(STDIN, &master);
	fdmax = listener;

	while(1){
		read_fds = master;
		cout<<"{PA1}$>";
		fflush(stdout);
		status =(select(fdmax+1, &read_fds, NULL, NULL, NULL));
		if (status==-1){
			cerr<<"select error: "<<gai_strerror(status);
		}
		for(int i = 0; i<=fdmax; i++){
			if(FD_ISSET(i,&read_fds)){
				if(i == STDIN){
					input = new char[256];
					//cout<<"i: "<<i<<endl;
					memset(input,0,sizeof(input));

					gets(input);
//					if(strcmp(input,NULL)==0){
//						break;
//					}
					input[strlen(input)] = '\0';
//					cout<<"input: "<<input<<endl;
					token = new char[100];
					memset(token,0,sizeof(token));
					token = strtok(input," ");
//					cout<<"token: "<<token<<endl;
					token_items.clear();
					while(token != NULL){
						token_items.push_back(token);
						token = strtok(NULL," ");
					}
//					cout<<"token_items[0]: "<<token_items[0]<<"\n";

					if(strcasecmp((char *)token_items[0].c_str(),"CREATOR")==0){
						creator();
						memset(&input,0,sizeof(input));
						break;
					}else if(strcasecmp((char *)token_items[0].c_str(),"HELP")==0){
						help();
						memset(&input,0,sizeof(input));
						break;
					}else if(strcasecmp((char *)token_items[0].c_str(),"LIST")==0){
						cout<<"Conn ID\t Host Name\t IP Address\t Port No\n";
						string terminated = string("0");
						string disconnected = string("-1");
						for(size_t j = 0; j<server_connection.size(); j++){
							if(server_connection[j].connection_id == terminated ||
									server_connection[j].connection_id == disconnected){
								continue;
							}else{
								cout<<server_connection[j].connection_id<<"\t";
								cout<<server_connection[j].hostname<<"\t";
								cout<<server_connection[j].ipaddress<<"\t";
								cout<<server_connection[j].port<<"\n";
							}
						}
						memset(&input,0,sizeof(input));
						break;
					}else if(strcasecmp((char *)token_items[0].c_str(),"MYPORT")==0){
						cout<< serverport;
						memset(&input,0,sizeof(input));
						break;
					}else if(strcasecmp((char *)token_items[0].c_str(),"MYIP")==0){
						getipaddr(ipadd);
						break;
					}else if(strcasecmp((char *)token_items[0].c_str(),"UPLOAD")==0){
						memset(&input,0,sizeof(input));
						break;
					}else if(strcasecmp((char *)token_items[0].c_str(),"DOWNLOAD")==0){
						memset(&input,0,sizeof(input));
						break;
					}else if(strcasecmp((char *)token_items[0].c_str(),"REGISTER")==0){
						cerr<<"Bad command!! - Refer help"<<endl;
						break;
					}else if(strcasecmp((char *)token_items[0].c_str(),"CONNECT")==0){
						cerr<<"Bad command!! - Refer help"<<endl;
						break;
					}else if(strcasecmp((char *)token_items[0].c_str(),"TERMINATE")==0){
						cerr<<"Bad command!! - Refer help"<<endl;
						break;
					}else if(strcasecmp((char *)token_items[0].c_str(),"EXIT")==0){
						exit(0);
					}else if(strcasecmp((char *)token_items[0].c_str(),"STATISTICS")==0){
						break;
					}
				}
				/*==========================================================================
				 * Server's Listener code
				 * =========================================================================
				 */
				else if(i == listener){
//					cout<<"Entering Server's Listener code\n";
					addrlen = sizeof(remoteaddr);
					memset(&remoteaddr,'\0',addrlen);
//					cout<<"Going to accept\n";
					newfd = accept(listener, (struct sockaddr*)&remoteaddr,&addrlen);
//					cout<<"Accepted newfd: "<<newfd<<"\n";
					if(newfd<0){
						cerr<<"accept error: "<<gai_strerror(newfd);
					}else{

						struct node peerobj;
						char localtoken[2048];
//						char connection[100];
						char sServ_peer_no[10];
						vector<string> local_vector;
						char* smalltok = new char[100];
						char *out = new char[1000];
						memset(out,'\0',sizeof(out));
						char str[10];
						sprintf(str,"%d",newfd);
						FD_SET(newfd,&master);
						//FD_SET(newfd,&read_fds);
						if(newfd > fdmax){
							fdmax = newfd;
						}
						memset(&remoteaddr,0,addrlen);
						getpeername(newfd,(struct sockaddr*) &remoteaddr,&addrlen);
						struct sockaddr_in *remoteip = (struct sockaddr_in *)&remoteaddr;
//						int remoteport = ntohs(remoteip->sin_port);
						inet_ntop(AF_INET,&remoteip->sin_addr,ipstr,sizeof(ipstr));
//						cout << "remote ip address: " << ipstr <<endl;
//						cout << "remote outgoing port: " << remoteport << endl;

//						cout<<"New connection in the "<<listener<<"  socket"<<endl;
//						cout<<"Accepting in the new socket fd "<<newfd<<endl;
						memset(incoming_data,'\0',sizeof(incoming_data));
						bytes_recvd = recv(newfd, incoming_data,sizeof(incoming_data), 0);
//						cout << "bytes recieved: "<< bytes_recvd <<endl;
						incoming_data[bytes_recvd] = '\0';
//						cout << "1. incoming_data: "<< incoming_data<<endl;

						/*-----------------------------------------------------
						 * New Code using structures
						 * ----------------------------------------------------
						 */
						memset(localtoken,'\0',sizeof(localtoken));
						strcpy(localtoken,incoming_data);
						local_vector.clear();
						smalltok = strtok(localtoken,":");
						while(smalltok!=NULL){
							local_vector.push_back(smalltok);
							smalltok = strtok(NULL,":");
						}
						peerobj.hostname = string(local_vector[0]);
						peerobj.port = string(local_vector[1]);
						peerobj.socket_id = string(str);
						peerobj.ipaddress = string(ipstr);
						server_peer_no++;
//						cout<<"server_peer_no: "<< server_peer_no<<"\n";
						sprintf(sServ_peer_no,"%d",server_peer_no);
						peerobj.connection_id = string(sServ_peer_no);
						peerobj.connected = string("1");
						//displayObj(peerobj);

						server_connection.push_back(peerobj);

//						cout<<"Added to the server connection vector\n";
//
//						for(size_t i = 0; i<server_connection.size(); i++){
////							cout<<"server_connection["<<i<<"]:\n ";
////							cout<<"----------------------\n";
//							displayObj(server_connection[i]);
//						}

						broadcast(server_connection);

						/*-----------------------------------------------------
						 * End of New Code using structures
						 * ----------------------------------------------------
						 */

					}
				}
				/*==========================================================================
				 * Server's Listener code ends
				 * =========================================================================
				 */

				/*==========================================================================
				 * Server's final else part
				 * =========================================================================
				 */

				else{
//					cout<<"Executing the final else part of the server module"<<endl;
//					cout<<"Accepting in the new socket fd "<<i<<endl;
					bytes_recvd = recv(i,incoming_data,1000,0);
					incoming_data[bytes_recvd] = '\0';
					if(bytes_recvd<=0){
						char sConnSock[10];
						sprintf(sConnSock,"%d",i);
						if(bytes_recvd == 0){
							for(size_t j=0; j < server_connection.size();j++){
								if(server_connection[j].socket_id == string(sConnSock)){
									server_connection[j].connection_id = string("-1");
									server_connection[j].connected = string("0");
									server_connection[j].socket_id = string("0");
									close(i);
									FD_CLR(i,&master);
									break;
								}
							}
							broadcast(server_connection);
//							cout<<"socket "<< i<<" hung up"<<endl;
						}
						else
							cerr<<"recv error: "<< endl;
					}else{
						cout << "incoming_data: "<<incoming_data <<endl;
					}
				}
			}
		}
	}
	/*==========================================================================
	 * Server's final else part ends
	 * =========================================================================
	 */
}
/*=====================================================================================
 * End of Server Module
 * ====================================================================================
 */


/*======================================================================================
 *Display Obj
 *Display objects
 *A utility funciton to display objects
 *
 *======================================================================================
 */
void displayObj(struct node obj){
	cout<<obj.socket_id<<"\n";
	cout<<obj.hostname<<"\n";
	cout<<obj.ipaddress<<"\n";
	cout<<obj.port<<"\n";
	cout<<obj.connection_id<<"\n";
}

/*=====================================================================================
 *Broadcast:
 *Function to broadcast the server connection list to all the clients
 *input is the vector of type node
 * ====================================================================================
 */
void broadcast(vector<node> vec){
	int sock;
	string out_going;
	ssize_t bytes_sent;
//	cout<<"----------------------\n";
//	cout<<"Inside broadcast\n";
//	cout<<"vec[0].hostname: "<< vec[0].hostname<<"\n";
//	out_going = string(vec[0].hostname);
//	out_going = out_going+":";
//	out_going = out_going+string(vec[0].ipaddress);
//	out_going = out_going+":";
//	out_going = out_going+string(vec[0].port);
//	out_going = out_going+";";
//	cout<<"out_going: "<<out_going<<"\n";

	for(size_t i = 0; i<vec.size(); i++){
		if(vec[i].connection_id == string("-1"))
			continue;
		else
			out_going = concatenate(vec[i], out_going);
//		cout<<"Inside the for loop\n";
//		cout<<"out_going: "<<out_going<<"\n";
	}

//	cout<<"Outgoing Message: "<<out_going<<"\n";

	for(size_t i = 0; i<vec.size();i++){
		sock = atoi(vec[i].socket_id.c_str());
		char *out_buff = new char[2048];
		strcpy(out_buff,out_going.c_str());
		//cout<<"final outgoing msg: "<<out_buff<<"\n";
		bytes_sent = send(sock,out_buff,strlen(out_buff),0);
		//cout<<"Sent to socket id: "<<sock<<"\n";
		//cout<<"Bytes Sent: "<<bytes_sent<<"\n";
	}
//	cout<<"Exiting broadcast\n";
//	cout<<"----------------------\n";
}

/*====================================================================================
 *Utility function to concatenate the structure elements into a string
 *
 *====================================================================================
 */
string concatenate(struct node obj, string out){

	out = out+obj.hostname;
	out = out+":";
	out = out+obj.ipaddress;
	out = out+":";
	out = out+obj.port;
	out = out+";";
	return out;
}
/*=======================================================================
 *
 * MAIN ()
 *
 * =======================================================================
 */
int main(int argc, char* argv[]) {
	if(argc != 3){
		fprintf(stderr,"Usage: <program name> <s/c flag> <port no>");
	}


	if(strcmp(argv[1], "s") == 0){
		serverModule(argv[2]);

	}else if(strcmp(argv[1],"c") == 0){

		clientModule(argv[2]);
	}else{
		cout<<"Usage: <program name> <s/c flag> <port no>\n";
	}

	return 0;

}

