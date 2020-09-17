#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <iostream>

using namespace std;

#define IP "127.0.0.1" //Server IP
#define PORT 6335 //Server Port
#define BUFFER_SIZE 4096

struct message{
    char value[2];
};
struct message msgToServer;
char global_buffer[ BUFFER_SIZE ];
int clientSocket;
struct sockaddr_in server;
socklen_t server_len = sizeof( server );
bool gameStarted = false;

void receiveMessage();

int main(){

    //Create Socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(clientSocket < 0){
        cout<<"Error while creating socket."<< endl;
        exit(1);
    }

	int enable = 1;
	setsockopt(clientSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr(IP);

    //Connect to the server
    if(connect(clientSocket, (struct sockaddr*) &server, sizeof(server)) == -1) return 0;

    cout<<"Connection established on Server "<<IP<<":"<<PORT<<endl;

    int readSize;
    char msgFromServer0[4096];
	char msgFromServer1[4096];
	char cm0[] = "started";
	char cm1[] = "type";
	char cm2[] = "win";
	char cm3[] = "lost";

	memset(msgFromServer0, 0, 4096);
	memset(msgFromServer1, 0, 4096);

	while(1){
		receiveMessage();
		
		if (strstr(global_buffer, cm0)){
			//printf("%s\n", global_buffer);
			gameStarted = true;
		}


		if (gameStarted){
			system("clear");
			printf("%s\n", global_buffer);
			if (strstr(global_buffer, cm1)) {
				scanf("%s", msgToServer.value);
				sendto(clientSocket, &msgToServer, sizeof(struct message), 0, (struct sockaddr*) &server, sizeof(server));
			} else if (strstr(global_buffer, cm2) || strstr(global_buffer, cm3)) {
				break;
			}
		}
	}

	close(clientSocket);
}



//-------------------------
void receiveMessage()
{
	// Variável para armazenar o tamanho lido do buffer.
	int read_size;
	// Variável que irá armazenar a mensagem recebida do tamanho do buffer.
	char buffer[ BUFFER_SIZE ];
	//
	bool msgComplete = false;

	memset( global_buffer, 0, sizeof( global_buffer ) );

	while(!msgComplete){
		memset( buffer, 0, sizeof( buffer ) );
		// Armazena no buffer a mensagem recebida do ciente.
		read_size = recvfrom( clientSocket, buffer, BUFFER_SIZE, 0, ( struct sockaddr * ) &server, &server_len  );
		if (strstr(buffer, "x")) {
			//printf("message complete\n");
			msgComplete = true;
		}
		else
		{
			//printf("message not complete\n");
			msgComplete = false;
			strcat(global_buffer, buffer);
		}
	}
	
	// Coloca o caractere de fim no buffer.
	buffer[ read_size ] = '\0';
	strcat( global_buffer, buffer );
}