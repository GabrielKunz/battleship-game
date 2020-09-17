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

#define BOARD_SIZE 8

int board[BOARD_SIZE][BOARD_SIZE] = {0};
char *boardOutput = "";

void drawBoard(bool allowInput);

int main(){

    drawBoard(true);
    printf("%s", boardOutput);
    return 0;
}

void drawBoard(bool allowInput){

    char buffer[1024];
    boardOutput = "";

    //Build board output
    for(int m = 0; m < BOARD_SIZE; m++)
    {
        for(int n = 0; n < BOARD_SIZE; n++)
        {
            strcpy(buffer, boardOutput);
            //Water
            if (board[m][n] == 0) {
                strcat(buffer, "A \t");
            }
            //Ship
            else{
                strcat(buffer, "S \t");
            }
        }
        strcat(buffer, "\n");
    }
    boardOutput = buffer;

    if(allowInput == true){
        //strcpy(buffer, boardOutput);
        //The board will be draw allowing the player to send a coordinate
        strcat(buffer,"\nPlease type a coordinate on the boardddd:\n");
        boardOutput = buffer;
    }
    else{
        //strcpy(buffer, boardOutput);
        //Player will just receive the board, will not be able to send a coordinate
        strcat(buffer,"\nPlease wait while the other player enters the coordinate\n");
        boardOutput = buffer;
    }
}