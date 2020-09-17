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

#define PORT 6335
#define MAX_NUMBER_OF_CLIENTS 3
#define BOARD_SIZE 8
#define MAX_ERRORS 10

int clientSocket[MAX_NUMBER_OF_CLIENTS];
struct sockaddr_in clientAddrs[MAX_NUMBER_OF_CLIENTS];
int clientCount = 0;
int errors = 0;
bool canPlay = true;
bool endGame = false;
bool endGameStatus = false;
bool next = true;
int turnToPlay = 0;
char board[4096];
int boardMatrix[BOARD_SIZE][BOARD_SIZE] = { {0,0,0,0,0,0,0,0},  
                                            {0,0,0,0,0,0,0,0},
                                            {0,0,0,0,0,0,0,0}, 
                                            {0,0,0,0,0,0,0,0}, 
                                            {0,0,0,0,0,0,0,0}, 
                                            {0,0,0,0,0,0,0,0}, 
                                            {0,0,0,0,0,0,0,0}, 
                                            {0,0,0,0,0,0,0,0}, };

struct message{
    char value[2];
};
struct message msgFromClient;

//Create Socket
int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
int c = sizeof(struct sockaddr_in);

//Functions
void communicationHandler();
void drawBoard(bool allowInput);
void checkShot();
void endMatch();
void selectRadomBoard();


int main(){

    socklen_t structClientAddrSize;
    int enable = 1;
	setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    //Define server address
    struct sockaddr_in server;

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = htons(INADDR_ANY);

    //Bind socket and listen to port
    if(bind(serverSocket,(struct sockaddr *) &server , sizeof(server)) == -1) return 0;

	if(listen(serverSocket,1024) == -1) return 0;

	cout<<">> Server started and listenting on port "<<PORT<<";"<<endl;

    //Wait for the connections to start the game
    while(clientCount < MAX_NUMBER_OF_CLIENTS){
        structClientAddrSize = sizeof(clientAddrs[clientCount]);
        clientSocket[clientCount] = accept(serverSocket, (struct sockaddr*) &clientAddrs[clientCount], &structClientAddrSize );
        cout<<">> Connection established with Player "<<clientCount<<" on Socket "<<clientSocket[clientCount]<<endl;
        clientCount++;
    }
    
    //Generate board
    selectRadomBoard();
    //Call communication handler
    communicationHandler();
}



void communicationHandler(){
    char *msg;

    cout<<">> All players connected. Match started "<<endl;
    msg = "The game started! \n";
    for(int i = 0; i < clientCount; i++)
    {
        cout<<">> Sending start message to Player "<<i + 1<<" on Socket "<<clientSocket[i]<<endl;
        //send(clientSocket[i], msg, strlen(msg),0);
        sendto(clientSocket[i], msg, strlen( msg ), 0, ( struct sockaddr * ) &clientAddrs[i], sizeof( clientAddrs[i] ) );
    }

    while(!endGame){
        //If its the player's turn, the player will send a coordinate to the server, otherwise the player will just receive the board
        for(int i = 0; i < clientCount; i++)
        {
            //Since the recv() is blocking, the player who will send the coodrinate is the last one to receive the board
            if (i != turnToPlay) {
                cout<<">> Drawing board"<<endl;
                //Send false to the drawBoard function, not allowing the player to enter a coordinate
                drawBoard(false);
                cout<<">> Sending board to Player "<<i + 1<<endl;
                strcat(board,"x");
                //send(clientSocket[i], board, strlen(board),0);
                sendto(clientSocket[i], &board, sizeof( board ), 0, ( struct sockaddr * ) &clientAddrs[i], sizeof( clientAddrs[i] ) );
                
            }
        }
        

        //After all the other players receive the board, the player that will send the coordinate can receive the board now and the server will wait for the coordinate
        cout<<">> Drawing board"<<endl;
        //Send true to the drawBoard function, allowing the player to enter a coordinate
        drawBoard(true);
        cout<<">> Sending board to Player "<<turnToPlay + 1<<endl;
        strcat(board,"x");
        //send(clientSocket[turnToPlay], board, strlen(board),0);
        sendto(clientSocket[turnToPlay], &board, sizeof( board ), 0, ( struct sockaddr * ) &clientAddrs[turnToPlay], sizeof( clientAddrs[turnToPlay] ) );
        cout<<">> Waiting coordinate from Player "<<turnToPlay + 1<<endl;
        recv(clientSocket[turnToPlay], &msgFromClient, sizeof(struct message),0);
        cout<<">> Message received from player "<< turnToPlay + 1<<": "<<msgFromClient.value<<endl;
        //Check if the shot from client hit the ship
        checkShot();

        //Update the next player to send the coordinate
        turnToPlay++;
        if (turnToPlay == clientCount) {
            turnToPlay = 0;
        }
    }

    cout<<">> Game has ended with status "<<endGameStatus<<endl;
    //Finish the game based on game status;
    endMatch();
    
}

void drawBoard(bool allowInput){

    char buffer[4096];
    char lineNumber[2];
    memset(board, 0, sizeof(board));
    memset(buffer, 0, sizeof(buffer));
    memset(lineNumber, 0, sizeof(lineNumber));
    strcat(buffer,"   0   1   2   3   4   5   6   7\n   ------------------------------\n");
    for(int i = 0; i < BOARD_SIZE; i++)
    {
        sprintf(lineNumber, "%d| ", i);
        strcat(buffer,lineNumber);
        memset(lineNumber, 0, sizeof(lineNumber));
        for(int j = 0; j < BOARD_SIZE; j++)
        {
            if(boardMatrix[i][j] == 0 || boardMatrix[i][j] == 1){
                strcat(buffer, ".   ");
            }
            else{
                strcat(buffer, "N   ");
            }
        }
        strcat(buffer, "\n");        
    }

    if(allowInput == true){
        //The board will be draw allowing the player to send a coordinate
        strcat(buffer,"\nPlease type a coordinate on the board (line and then column):\n");
        strcpy(board,buffer);
    }
    else{
        //Player will just receive the board, will not be able to send a coordinate
        strcat(buffer,"\nPlease wait while the other player enters the coordinate\n");
        strcpy(board,buffer);
    }
}

void checkShot(){
    int m = msgFromClient.value[0] - '0';
    int n = msgFromClient.value[1] - '0';
    bool shipExists;
    if(boardMatrix[m][n] == 1){
        boardMatrix[m][n] = 2;
        cout<<">> Player hit the shot. Checking if the game was won"<<endl;
        //Check if there is still ships to be discovered in the board
        shipExists = false;
        for(int i = 0; i < BOARD_SIZE; i++)
        {
            for(int j = 0; j < BOARD_SIZE; j++)
            {
                if (boardMatrix[i][j] == 1) {
                    shipExists = true;
                }
            }
        }

        //If there are no more ships to be found, the game was won
        if (shipExists == false) {
            //Game ended
            endGame = true; 
            //Players won the game
            cout<<">> Players won the game"<<endl;
            endGameStatus = true;
        }
        
        
    }else
    {
        errors++;
        cout<<">> Player missed the shot. Total errors: "<<errors<<endl;
        if (errors == MAX_ERRORS) {
            //Game ended
            endGame = true; 
            //Players lost the game
            cout<<">> Players lost the game"<<endl;
            endGameStatus = false;
        }
    }
}

void endMatch(){
    char buffer[4096];
    memset(board, 0, sizeof(board));
    memset(buffer, 0, sizeof(buffer));
    if (endGameStatus == true) {
        //Players won
        cout<<">> Building win message"<<endl;
        strcat(buffer, "You won! :)\n");
        strcpy(board,buffer);
    }
    else{
        //Players lost
        cout<<">> Building lost message"<<endl;
        strcat(buffer, "You lost! :(\n");
        strcpy(board,buffer);
    }

    //x is the signal for end of the message
    strcat(board,"x");
    for(int i = 0; i < clientCount; i++)
    {
        cout<<">> Sending game final status to Player "<<i + 1<<endl;
        
        sendto(clientSocket[i], &board, sizeof( board ), 0, ( struct sockaddr * ) &clientAddrs[i], sizeof( clientAddrs[i] ) );
    }
}

void selectRadomBoard(){
    int randomNumber;

    //Initialize random seed
    srand (time(NULL));

    //Generate random number
    randomNumber = rand() % 4;

    cout<<">> Selecting board. Random Number = "<<randomNumber<<endl;

    switch (randomNumber)
    {
        case 0:
            /*
            boardMatrix = { {1,1,0,0,0,0,0,0},  
                            {0,0,0,0,0,0,0,0},
                            {0,0,0,0,0,0,0,0}, 
                            {0,0,0,0,0,0,0,0}, 
                            {0,0,0,0,0,0,0,0}, 
                            {0,0,0,0,0,0,0,0}, 
                            {0,0,0,0,0,0,0,0}, 
                            {0,0,0,0,0,0,0,0}, };
            */
            boardMatrix[0][0] = 1;
            boardMatrix[0][1] = 1;

            break;
        case 1:
            /*
            boardMatrix = { {0,0,0,0,0,0,0,0},  
                            {1,1,0,0,0,1,1,1},
                            {0,0,0,0,0,0,0,0}, 
                            {1,0,1,1,1,1,0,0}, 
                            {1,0,0,0,0,0,0,0}, 
                            {1,0,0,0,0,0,0,0}, 
                            {1,0,0,0,0,0,0,0}, 
                            {1,0,0,0,0,0,0,0}, };
            */
            boardMatrix[1][0] = 1;
            boardMatrix[1][1] = 1;

            boardMatrix[1][5] = 1;
            boardMatrix[1][6] = 1;
            boardMatrix[1][7] = 1;

            boardMatrix[3][2] = 1;
            boardMatrix[3][3] = 1;
            boardMatrix[3][4] = 1;
            boardMatrix[3][5] = 1;

            boardMatrix[3][0] = 1;
            boardMatrix[4][0] = 1;
            boardMatrix[5][0] = 1;
            boardMatrix[6][0] = 1;
            boardMatrix[7][0] = 1;
            break;
        case 2:
            /*
            boardMatrix = { {0,0,1,0,0,0,0,0},  
                            {0,0,1,0,0,0,0,0},
                            {1,1,1,0,0,0,0,0}, 
                            {0,0,0,0,0,0,0,0}, 
                            {1,1,1,1,1,0,0,1}, 
                            {0,0,0,0,0,0,0,1}, 
                            {0,0,0,0,0,0,0,1}, 
                            {0,0,0,0,0,0,0,1}, };
            */
            boardMatrix[2][0] = 1;
            boardMatrix[2][1] = 1;

            boardMatrix[0][2] = 1;
            boardMatrix[1][2] = 1;
            boardMatrix[2][2] = 1;

            boardMatrix[4][7] = 1;
            boardMatrix[5][7] = 1;
            boardMatrix[6][7] = 1;
            boardMatrix[7][7] = 1;

            boardMatrix[5][0] = 1;
            boardMatrix[5][1] = 1;
            boardMatrix[5][2] = 1;
            boardMatrix[5][3] = 1;
            boardMatrix[5][4] = 1;
            break;
        case 3:
            /*
            boardMatrix = { {0,0,0,0,0,0,0,0},  
                            {0,1,1,1,1,1,0,0},
                            {0,0,0,0,1,1,1,1}, 
                            {1,1,0,0,0,0,0,0}, 
                            {0,0,0,0,0,0,0,0}, 
                            {0,1,0,0,0,0,0,0}, 
                            {0,1,0,0,0,0,0,0}, 
                            {0,1,0,0,0,0,0,0}, };
            */
            boardMatrix[3][0] = 1;
            boardMatrix[3][1] = 1;

            boardMatrix[5][1] = 1;
            boardMatrix[6][1] = 1;
            boardMatrix[7][1] = 1;

            boardMatrix[2][4] = 1;
            boardMatrix[2][5] = 1;
            boardMatrix[2][6] = 1;
            boardMatrix[2][7] = 1;

            boardMatrix[1][1] = 1;
            boardMatrix[1][2] = 1;
            boardMatrix[1][3] = 1;
            boardMatrix[1][4] = 1;
            boardMatrix[1][5] = 1;
            break;
            
        default:
            /*
            boardMatrix = { {0,0,1,0,0,0,0,0},  
                            {0,0,1,0,0,0,0,0},
                            {0,0,1,0,1,0,0,0}, 
                            {0,0,0,0,1,0,0,0}, 
                            {1,1,0,0,1,0,0,0}, 
                            {0,0,0,0,1,0,0,0}, 
                            {0,0,0,1,1,1,1,1}, 
                            {0,0,0,0,0,0,0,0}, };
            */
            boardMatrix[4][0] = 1;
            boardMatrix[4][1] = 1;

            boardMatrix[0][2] = 1;
            boardMatrix[1][2] = 1;
            boardMatrix[2][2] = 1;

            boardMatrix[2][4] = 1;
            boardMatrix[3][4] = 1;
            boardMatrix[4][4] = 1;
            boardMatrix[5][4] = 1;

            boardMatrix[6][3] = 1;
            boardMatrix[6][4] = 1;
            boardMatrix[6][5] = 1;
            boardMatrix[6][6] = 1;
            boardMatrix[6][7] = 1;
            break;
    }
}