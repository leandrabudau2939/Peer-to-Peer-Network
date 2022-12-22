// COE768 - Design Project 
// November 27th, 2022
// Leandra Budau
// 500873032

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/unistd.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <netdb.h>
#include <stdio.h>
#include <time.h>

// Stucture of the PDU
struct PDU {
	char type;
	char data[100];
};

// Structure to store registered content
typedef struct{
	char content[10];
	char peer[10];
	char address[10];
	char port_number[10];
} reg_data;

// Definitions
#define BUFSIZE sizeof(struct PDU)
#define ERROR 	'E' //report error
#define ONLINE	'O'
#define QUIT	'Q'
#define REGISTER	'R'
#define DEREG	'T'
#define SEARCH	'S'
#define ACKNOWLEDGEMENT	'A'

// Main section ///////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
	// Variables
	struct  sockaddr_in fsin;	
	char	buf[100];	
	char    *pts;
	int	sock;		
	time_t	now;	
	int	alen;		
	struct  sockaddr_in sin; 
	int     s, type, error_found =0;       
	int 	port=3000;
	
	struct PDU msg, send_msg;
	int 	socket_read_size, i=0;
	char	socket_buf[BUFSIZE];
	char	file_name[100], online_content[100];
	int	file_size, registered=0, found = 0, placement = 0;
	reg_data registered_content[10];
	char data_recieved[100], peer_name[10], content_name[10], address[10], port_num[10];
	int content_tracking[] = {0,0,0,0,0,0,0,0,0,0};

	// Set up UDP server connection
	switch(argc){
		case 1:
			break;
		case 2:
			port = atoi(argv[1]);
			break;
		default:
			fprintf(stderr, "Usage: %s [port]\n", argv[0]);
			exit(1);
	}

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);
                                                                                                 
    /* Allocate a socket */
	s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
		fprintf(stderr, "can't creat socket\n");
	}
	
    /* Bind the socket */
	if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		fprintf(stderr, "can't bind to %d port\n",port);
	}

	listen(s, 5);	
	alen = sizeof(fsin);

	// Main section, after UDP connection is set up 
	while (1) {

		// Reset variables
		memset(&send_msg, '\0', sizeof(send_msg));
		memset(&data_recieved, '\0', sizeof(data_recieved));
		registered = 0;
		found = 0;
		error_found = 0;
		
		// Get message from peer
		if (recvfrom(s, socket_buf, sizeof(socket_buf), 0, (struct sockaddr *)&fsin, &alen) < 0) {
			printf("recvfrom error\n");
		} 
		// Copy message into PDU format
		memcpy(&msg, socket_buf, sizeof(socket_buf)); 
		
		// Handle request for list of available content
		if (msg.type == ONLINE) {
			// Get and parse message from peer
			printf("Online content request\n");
			memcpy(data_recieved, msg.data, sizeof(socket_buf) - 1);
			// Prepare message to send to peer
			send_msg.type = ONLINE;
			for (i=0; i<10; ++i){ //Get list of existing content, seperate by commas
				if (content_tracking[i]==1){
					strcat(send_msg.data, registered_content[i].content);
					strcat(send_msg.data, ", ");
				}
			}
			// Send message to peer
			sendto(s, &send_msg, BUFSIZE, 0, (struct sockaddr *)&fsin, sizeof(fsin));
		}
		// Handle request to deregister content 
		if (msg.type == DEREG) {
			// Get and parse message from peer
			printf("Deregister content request\n");
			memcpy(data_recieved, msg.data, BUFSIZE - 1);
			sscanf(data_recieved, "%[a-zA-Z0-9];%[a-zA-Z0-9]", peer_name, content_name);
			// Print the peer and content name they want to deregister
			printf("peer name: %s\n", peer_name);
			printf("content name: %s\n", content_name);

			//Check and see if the content exists, and where it's stored
			for (i=0; i<10; ++i){
				if ((strcmp(registered_content[i].content, content_name)==0) && (content_tracking[i]==1) && (strcmp(registered_content[i].peer, peer_name)==0)){
					placement = i;
					found = 1;
				}
			}
			// If content doesn't exist, send back an error
			if (found == 0){
				send_msg.type = ERROR;
				strcpy(send_msg.data, "This content is not registered with this peer");
			}
			// If content exists, remove it from storage, and send back an acknowledgement
			else if (found == 1){
				//memset(&registered_content[placement], '\0', sizeof(msg));
				content_tracking[placement] = 0;
				send_msg.type = ACKNOWLEDGEMENT; 
				strcpy(send_msg.data, "content has been deregistered");
			}
			// Send message to peer
			sendto(s, &send_msg, BUFSIZE, 0, (struct sockaddr *)&fsin, sizeof(fsin));
		}
		// Handle request to seach for content 
		else if (msg.type == SEARCH){
			// Get and parse message from peer
			printf("Search content request\n");
			memcpy(data_recieved, msg.data, BUFSIZE - 1);
			sscanf(data_recieved, "%[a-zA-Z0-9];%[a-zA-Z0-9]", peer_name, content_name);
			// Print the peer and content name they want to find
			printf("peer name: %s\n", peer_name);
			printf("content name: %s\n", content_name);

			// Check and see if the content name exists in storage
			for (i=0; i<10; ++i){
				if ((strcmp(registered_content[i].content, content_name)==0) && (content_tracking[i]==1)){
					placement = i;
					found = 1;
				}
			}
			// If the content doesn't exist, send back an error
			if (found == 0){
				send_msg.type = ERROR;
				strcpy(send_msg.data, "No content by this name and peer was found");
			}
			//If the content exists, send back an 'S' type message with the address and port number
			else if (found == 1){
				send_msg.type = SEARCH;
				strcpy(send_msg.data, registered_content[placement].content);
				strcat(send_msg.data, ";");
				strcat(send_msg.data, registered_content[placement].peer);
				strcat(send_msg.data, ";");
				strcat(send_msg.data, registered_content[placement].address);
				strcat(send_msg.data, ";");
				strcat(send_msg.data, registered_content[placement].port_number);
			}
			// Send message to peer
			sendto(s, &send_msg, BUFSIZE, 0, (struct sockaddr *)&fsin, sizeof(fsin));
		}
		// Handle request to quit the program
		else if (msg.type == QUIT){
			// Get and parse the message 
			printf("The peer wants to quit the program\n");
			memcpy(peer_name, msg.data, BUFSIZE - 1);
			//Check to see if they have any open content
			for (i=0; i<10; ++i){
				if ((strcmp(registered_content[i].peer, peer_name)==0) && content_tracking[i]==1){
					found = 1;
				}
			}
			// If they still have open content, send back an error
			if (found == 1){
				send_msg.type = ERROR;
				strcpy(send_msg.data, "unable to close, content still registered to this peer");
				sendto(s, &send_msg, BUFSIZE, 0, (struct sockaddr *)&fsin, sizeof(fsin));
			}
			// If all their content is deregistered, send back 'Q' type message
			else if (found == 0){
				send_msg.type = QUIT;
				sendto(s, &send_msg, BUFSIZE, 0, (struct sockaddr *)&fsin, sizeof(fsin));
			}
		}
		// Handle request to register content
		else if (msg.type == REGISTER){
			// Get and parse message
			printf("The peer wants to register new data\n");
			memcpy(data_recieved, msg.data, BUFSIZE - 1);
			sscanf(data_recieved, "%[a-zA-Z0-9];%[a-zA-Z0-9];%[0-9];%[0-9]", peer_name, content_name, address, port_num);
			//Print information recieved
			printf("peer name: %s\n", peer_name);
			printf("content name: %s\n", content_name);
			printf("address: %s\n", address);
			printf("port: %s\n", port_num);
			//Find an empty spot to store the content information
			for (i=0; i<10; ++i){
				if ((strcmp(registered_content[i].peer, peer_name)==0) && (strcmp(registered_content[i].content, content_name)==0) && content_tracking[i]==1){
					error_found = 1;
				}
			}
			if (error_found ==0){
				for (i=0; i<10; ++i){
					if ((content_tracking[i]==0) && (registered ==0)){
						strcpy(registered_content[i].content, content_name);
						strcpy(registered_content[i].peer, peer_name);
						strcpy(registered_content[i].address, address);
						strcpy(registered_content[i].port_number, port_num);
						registered = 1;
						content_tracking[i] = 1;
					}
				}	
			}	
			//Send an acknowledgement if the content was stored properly
			if (registered == 1){
				send_msg.type = ACKNOWLEDGEMENT; 
				strcpy(send_msg.data, "content has been registered");
			}
			//Send an error if the content couldn't be stored
			else if (registered == 0){
				send_msg.type = ERROR; 
				strcpy(send_msg.data, "error registering content");
			}
			//Send message to the peer
			sendto(s, &send_msg, BUFSIZE, 0, (struct sockaddr *)&fsin, sizeof(fsin));

		}
	}
	// Exit the program
	close(s);
	exit(0);
}