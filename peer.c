// COE768 - Design Project 
// November 27th, 2022
// Leandra Budau
// 500873032

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
// Structure of PDU
struct PDU {
	char type;
	char data[100];
};
// Definitions
#define BUFSIZE sizeof(struct PDU)
#define ERROR 	'E'
#define REGISTER 'R'
#define ONLINE	'O'
#define QUIT	'Q'
#define SEARCH	'S'
#define ACKNOWLEDGEMENT	'A'
#define DEREG	'T'
#define CONTENT 'C'
#define DOWNLOAD 'D'

// Universal variables
int	s, n, type, new_tcp_socket;	
char peer_name[10];

// Used to print off all the selection options each iteration
void printOptions(){
	printf("\nPlease select one of the following actions to complete: \n");
	printf("'R' - register new content\n");
	printf("'O' - get list of content\n");
	printf("'D' - download new content\n");
	printf("'S' - search for content\n");
	printf("'T' - deregister new content\n");
	printf("'Q' - quit program\n");
}

// Used to register new content 
void registerContent(char content[], char peer[]){
	// Check and make sure the file you're offering exists on the computer
	if (access(content, F_OK) != 0){
		printf("This file does not exist");
		return;
	}
	// Make tcp connection
	int 	tcp_socket, new_sd, client_len, port, alen, message_read_size;
	struct	sockaddr_in server, client;
	struct 	PDU msg, r_msg;
	char 	tcp_address[5], tcp_port[6], send_data[100];
	
	if ((tcp_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Can't create a socket\n");
		exit(1);
	}

	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(0);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(tcp_socket, (struct sockaddr *)&server, sizeof(server)) == -1){
		fprintf(stderr, "Can't bind name to socket\n");
		exit(1);
	}
	alen = sizeof(struct sockaddr_in);
	getsockname(tcp_socket, (struct sockaddr*)&server, &alen);

	// Save the address and port number of the tcp socket
	snprintf(tcp_address, sizeof(tcp_address), "%d", server.sin_addr.s_addr);
	snprintf(tcp_port, sizeof(tcp_port), "%d", htons(server.sin_port));

	// Make R type message to send to the index
	memset(&msg, '\0', sizeof(msg));
	msg.type = REGISTER;

	strcpy(send_data, peer); 
	strcat(send_data, ";");
	strcat(send_data, content); 
	strcat(send_data, ";");
	strcat(send_data, tcp_address);
	strcat(send_data, ";");
	strcat(send_data, tcp_port);
	strcpy(msg.data, send_data);

	// Send message to index
	write(s, &msg, sizeof(msg));

	// Reads response from index
	memset(&r_msg, '\0', sizeof(r_msg));
	read(s, &r_msg, sizeof(r_msg));
	
	// If response from index is error, don't register content 
	if (r_msg.type == ERROR){
		//handle an error
		printf("The data cant be registered");
	}
	// If response from index is acknowledgement, register content
	else if (r_msg.type == ACKNOWLEDGEMENT){
		// Clear variables 
		memset(&msg, '\0', sizeof(msg));
		memset(&r_msg, '\0', sizeof(r_msg));
		// Split into child and parent to host content
		listen(tcp_socket, 5);
		switch (fork()) {
      		case 0: 
				while (1){
					// Set up new branch
					int client_length = sizeof(client);
					new_tcp_socket = accept(tcp_socket, (struct sockaddr *)&client, &client_length);
					// Listen to message from other peer
					read(new_tcp_socket, &r_msg, sizeof(msg));
					// If message from peer is type 'D', start download process
					if (r_msg.type == DOWNLOAD){
						// Open up the file 
						FILE *file;
						file = fopen(content, "rb");
						if (!file){
							printf("This file doesn't exist\n");
						}
						// If the file exists, iterativly send messages to peer with content of file
						else {
							msg.type = CONTENT;
							while (fgets(msg.data, sizeof(msg.data), file)>0){
								write(new_tcp_socket, &msg, sizeof(msg));
							}
						}
						// Close the file 
						fclose(file);
						close(new_tcp_socket);
						break;
					}
					// If the message from the other peer isn't 'D' type
					else {
						break;
					}
				}
				break;
      		case -1:
        		fprintf(stderr, "fork: error\n");
				break;
    	}
	}
}
// Used to download content from another peer
void downloadContent(char content_name[], char address[]){
	//variables
	int d_socket, data, content_counter=0;
	struct sockaddr_in server;
	struct PDU msg, r_msg;
	struct hostent *host;
	char *serverHost = "0";

	//create TCP socket
	if ((d_socket = socket(AF_INET, SOCK_STREAM, 0))<0){
		fprintf(stderr, "Can't create a TCP socket\n");
		exit(1);
	}

	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET; 
	server.sin_port = htons(atoi(address));
	printf("address: %d\n", atoi(address));
	if (host = gethostbyname(serverHost)){
		bcopy(host->h_addr, (char *)&server.sin_addr, host->h_length);
	}
	else if (inet_aton(serverHost, (struct in_addr *) &server.sin_addr)){
		fprintf(stderr, "Can't get server's address\n");
	}

	if (connect(d_socket, (struct sockaddr *)&server, sizeof(server)) == -1){
		fprintf(stderr, "cant't connect to server: %s\n", host->h_name);
	}

	// Send download request to peer
	msg.type = DOWNLOAD; 
	strcpy(msg.data, peer_name);
	strcat(msg.data, ";");
	strcat(msg.data, content_name);
	write(d_socket, &msg, sizeof(msg));

	// Open a new file to store downloaded content
	FILE *file;
	file = fopen("tester", "w+"); //change this after testing
	//Iterativly get data from the peer and add it to the new file
	while ((data = read(d_socket, &r_msg, sizeof(r_msg)))>0){
		if(r_msg.type == CONTENT){
			fprintf(file, "%s", r_msg.data);
			content_counter += 1;
		}
		else if (r_msg.type == ERROR){
			printf("error message recieved");
		}
	}		
	fclose(file);
	// If content was downloaded, register it
	if (content_counter > 0){
		registerContent(content_name, peer_name);
	}
}
// Main part of the program
int main(int argc, char **argv)
{	
	char	*host = "localhost";
	int	port = 3000;
	char	now[100];	
	struct hostent *phe;	
	struct sockaddr_in sin;	

	int	socket_read_size;
	char	socket_read_buf[BUFSIZE];
	int 	c_read_size;
	char 	c_read_buf[100];
	struct PDU msg, r_msg;
	FILE*	fp;
	char	file_name[100], data_send[100];
	int	error_found = 0;
	char	test_file_name[100] = "", choice[1], content_name[10], address[80]="placeholder address", input[10], input2[10];
	char	temp_port[10], temp_addr[10], temp_content_name[10], temp_peer_name[10];

	// Set up the UDP socket
	switch (argc) {
		case 1:
			break;
		case 2:
			host = argv[1];
		case 3:
			host = argv[1];
			port = atoi(argv[2]);
			break;
		default:
			fprintf(stderr, "usage: UDP File Download [host [port]]\n");
			exit(1);
	}

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
                                                                                        
	if ( phe = gethostbyname(host) ){
		memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
	}
	else if ( (sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE )
		fprintf(stderr, "Can't get host entry \n");
                                                                                
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0)
		fprintf(stderr, "Can't create socket \n");
	
	if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		fprintf(stderr, "Can't connect to %s\n", host);
	}

	// Get the name of the peer on start up 
	memset(&peer_name, '\0', sizeof(peer_name));
	printf("Enter Peer Name:\n");
	c_read_size = read(0, peer_name, BUFSIZE); 
	if (c_read_size >= 10) { 
		printf("ERROR\n");
		return -1;
	}
	peer_name[strlen(peer_name)-1] = '\0';

	// Main loop of the program
	while(1) {
		// Reset variables
		memset(&input, '\0', sizeof(input));
		memset(&msg, '\0', sizeof(msg));		
		
		// Print Options
		printOptions();
		read(0, choice, 2);
		printf("your choice is: %s\n", choice);

		//Start of the options
		if (choice[0] == 'O') { //ONLINE CONTENT 
			//Write request to index
			msg.type = ONLINE;
			strcpy(msg.data, "online content");
			write(s, &msg, BUFSIZE);
			//Recieve message
			read(s, &r_msg, BUFSIZE);
			printf("Available Content: ");
			printf("%s\n", r_msg.data);
		}
		else if (choice[0] == 'R') { //REGISTER CONTENT
			//send R-type message to index
			msg.type = REGISTER;
			printf("Please enter the name of the content you'd like to register: ");
			scanf("%9s", input);
			registerContent(input, peer_name);
		}
		else if (choice[0] == 'D') { //DOWNLOAD CONTENT
			printf("Please enter the name of the content you'd like to download: ");
			scanf("%9s", input);
			printf("Please enter the name of the peer you'd like to download from: ");
			scanf("%9s", input2);
			//send S-type to index
			msg.type = SEARCH;
			strcpy(msg.data, input2);
			strcat(msg.data, ";");
			strcat(msg.data, input);
			write(s, &msg, BUFSIZE);
			// Recieve message from index
			read(s, &r_msg, BUFSIZE);
			sscanf(r_msg.data, "%[a-zA-Z0-9];%[a-zA-Z0-9]%[0-9];%[0-9]", temp_content_name, temp_peer_name, temp_addr, temp_port);
			// Download the content
			downloadContent(temp_content_name, temp_port);
		}
		else if (choice[0] == 'T'){ //DEREGISTER CONTENT
			printf("Please enter the name of the content you'd like to deregister: ");
			scanf("%s", input);
			if (strcmp(input, "ntent")==0){
				strcpy(input, "content");
			}
			// Send deregister message to index
			msg.type = DEREG;
			printf("peer name: %s\n", peer_name);
			strcpy(msg.data, peer_name);
			strcat(msg.data, ";");
			strcat(msg.data, input);
			write(s, &msg, BUFSIZE);
			// Recieve message from index
			read(s, &r_msg, BUFSIZE);
			if (r_msg.type == ACKNOWLEDGEMENT){
				printf("Content was successfully deregistered\n");
			}
			else if (r_msg.type == ERROR){
				printf("Error: %s\n", r_msg.data);
			}
		}
		else if (choice[0] == 'S'){
			printf("Please enter the name of the content you'd like to search for: ");
			scanf("%9s", input);
			// Send search request to the index
			msg.type = SEARCH;
			strcpy(msg.data, peer_name);
			strcat(msg.data, ";");
			strcat(msg.data, input);
			write(s, &msg, BUFSIZE);
			read(s, &r_msg, BUFSIZE);
			sscanf(r_msg.data, "%[a-zA-Z0-9];%[a-zA-Z0-9];%[0-9];%[0-9]", temp_content_name, temp_peer_name, temp_addr, temp_port);
			printf("Here is the information you searched for: \n");
			printf("Content Name: %s\n", temp_content_name);
			printf("Peer Name: %s\n", temp_peer_name);
			printf("Address: %s\n", temp_addr);
			printf("Port Number: %s\n", temp_port);
		}
		else if (choice[0] == 'Q') { //QUIT CONTENT
			// Send quit request to the index
			msg.type = QUIT;
			strcpy(msg.data, peer_name);
			write(s, &msg, BUFSIZE);
			// Read return message from index
			read(s, &r_msg, BUFSIZE);
			// If the return message is type 'Q', peer can quit
			if (r_msg.type == QUIT){
				break;
			}
			// If the return message is type 'E', peer can't quit
			else if (r_msg.type == ERROR){
				printf("%s\n", r_msg.data);
			}
		}
	}
	close(s);
	exit(0);
}