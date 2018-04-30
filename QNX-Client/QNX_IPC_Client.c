/*
 ========================================================================================
 * File Name:	QNX_IPC_Client.c
 * Compiler:	QNX QCC
 * Author:	Fleming Patel
 * Course:	Real-Time Programming
 * Assignment:	06
 * Date:	Friday April 20, 2018
 * Professor:	RICHARD HAGEMEYER, MOHAMMAD PATOARY
 * Purpose:	Establish a connection between the calling process and the channel
 * 		This file responsible for making request for registering client and
 * 		deregister client.
 ========================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/neutrino.h>
#include <sys/netmgr.h>
#include <time.h>
#include <ctype.h>
#include "struct.h"

/***************************************************************************************
						Define Constants
***************************************************************************************/
#define TRUE 1
#define FALSE 0

int secondMenuErrorFlag;

/***************************************************************************************
					Function Prototypes
***************************************************************************************/
void menu();
void secondMenu();
void checkType(char *input,ClientMessage *msg);
void doIPC(int connectionId,ClientMessage msg,ServerMessage reply);
void resetOptionFlag(ClientMessage *msg);

void error(char *msg) {
	perror(msg);
	exit(0);
}

/***************************************************************************************
* Purpose:			Establish a connection between the calling process and the
* 				channel specified by chid owned by the process specified by pid on
* 				the node specified by nd.
*				This Function is also responsible for getting client's selection and
*				assigning flag on that specific selection which server can
*				understand. for e.g.,
*					Flag 0:	Stands for sending client pid to server and adding that
*						in to table
*					Flag 1:	Registering Client
*					Flag 2:	Deregistering Client
*					Flag 3:	Sending message to server and doing some query on
*						the message
*					Flag 4: If client send digit in a query
*					Flag 5:	If client send character in a query
*					Flag 6:	Terminating client program
* Author:		Fleming Patel
* Called Function:	ConnectAttach(), atoi(), menu(), fflush(), fgets(), doIPC(),
* 			secondMenu(), checkType(), resetOptionFlag(), ConnectDetach()
* Parameters:		int argc, char *argv[]
* Return Value:		return 0 Exit success
***************************************************************************************/
int main(int argc, char *argv[]) {

	if (argc > 2) {
		printf("Too many arguments.\n");
		printf("Usage: ./QNX_TCP_Client_IPC PID\n");
		return EXIT_FAILURE;
	} else if (argc < 2) {
		printf("Argument missing\n");
		printf("Usage: ./QNX_TCP_Client_IPC PID\n");
		return EXIT_FAILURE;
	}

	ClientMessage msg;
	ServerMessage reply;

	int connectionId;
	if((connectionId = ConnectAttach(0, atoi(argv[1]), 1,_NTO_SIDE_CHANNEL, 0)) == -1){
		error("Cannot Connect to Server. Please try again\n");
		return EXIT_FAILURE;
	}

	//storing pid to client structure so server can print
	memset(msg.message, '\0', 256);
	msg.pid = getpid();
	msg.optionFlag = '0';
	doIPC(connectionId,msg,reply); // sending null message and client pid for adding in table
	resetOptionFlag(&msg); // reseting client selected option */

	int terminationCondition = FALSE;
	while(terminationCondition != TRUE){
		menu(); //printing menu
		fflush(stdout);
		int option = 0;
		scanf("%d",&option); //getting user selection
		while ((getchar()) != '\n'); // clearing buffer
		switch(option){
			case 1:
				msg.optionFlag = '1'; // registering client
				doIPC(connectionId,msg,reply);
				resetOptionFlag(&msg); // reseting client selected option */
				break;

			case 2:
				msg.optionFlag = '2'; // Deregistering client
				doIPC(connectionId,msg,reply);
				resetOptionFlag(&msg); // reseting client selected option */
				break;

			case 3:
				msg.optionFlag = '7'; // just checking registration
				doIPC(connectionId,msg,reply); // passing message to server
				if(registrationStatus == TRUE){
					printf("Enter a message: ");
					fflush(stdout);
					fgets(msg.message, 256, stdin); // getting client's message
					msg.optionFlag = '3'; // assigning  option flag 3 for only message printing
					doIPC(connectionId,msg,reply); // passing message to server
					resetOptionFlag(&msg); // reseting client selected option
					do{
						secondMenu(); // printing second menu
						secondMenuErrorFlag = TRUE;
						char input[256];
						fflush(stdout);
						fgets(input, 256, stdin); // getting client's input
						checkType(input,&msg); // checking client's input is valid or not
					} while(secondMenuErrorFlag == TRUE);

					doIPC(connectionId,msg,reply); // passing option to server
					resetOptionFlag(&msg); // reseting client selected option
				}
				break;

			case 4:
				msg.optionFlag = '6';
				doIPC(connectionId,msg,reply);
				resetOptionFlag(&msg); // reseting client selected option
				//ConnectDetach(connectionId); // terminating connection
				terminationCondition = TRUE;
				break;

			default:
				printf("Please chose appropriate selection\n");
				break;
		}
	}

	return EXIT_SUCCESS;
}

/***************************************************************************************
* Purpose:		Printing First default Selection
* Author:		Fleming Patel
* Called Function:	printf()
* Parameters:		None
* Return Value:		None
***************************************************************************************/
void menu(){
	printf("1. Register with server\n2. Deregister with server\n3. Find char position or Count Character in String\n4. exit\n");
}

/***************************************************************************************
* Purpose:		Printing Second menu Selection
* Author:		Fleming Patel
* Called Function:	printf()
* Parameters:		None
* Return Value:		None
***************************************************************************************/
void secondMenu(){
	printf("Please enter a integer value or character: ");
}

/***************************************************************************************
* Purpose:			This function is responsible for validating user input and
* 				assigning optionFlag.
* 				(1) Check if user put single character value or more than two
* 				    character.
* 				(2) If user input value is only one character then:-
* 				 	 -	If user input start from the character then algorithm takes
* 						character as a default argument and pass that
* 						character to server
* 					 -	If user input start from digit value then it will pass to
* 					 	server as index number
* 				(3) If user input value is more than two character:-
* 					 -	If it's a character then algorithm takes first character as
* 						a default argument and pass that to server
* 					 -	If it's digit then algorithm check all input is digit or not.
* 						 If all input is digit then it will pass to server as
* 						 index number. If input contains any character that is
* 						 not digit then it will give an error message to user and let
* 						 them input again
* Author:		Fleming Patel
* Called Function:	strlen(), isdigit(), isalpha(), atoi()
* Parameters:		ClientMessage *msg, ServerMessage *reply
* Return Value:		None
***************************************************************************************/
void checkType(char *input,ClientMessage *msg){
	if(strlen(input) > 2){ // checking if user input value is not one character
		msg->option = input[0]; // getting first character
		if(isdigit(msg->option)){ // checking if it's digit or not
			//check all letter is digit or not
			size_t messageLength = strlen(input);
			int isAlphaFound = FALSE; // false
			int i = 0;
			int count = 0;
			/*
			 * loops runs until end of line character found in string while checking
			 * each character are digit
			 */
			while(input[i] != '\0'){
				if(isalpha(input[i])){
					/*
					 * if it's found an unexpected character from user input with digit
					 * it will give an error
					 */
					isAlphaFound = TRUE;
				}
				i++;
			}

			if(!isAlphaFound){ // if string is correct then setting up the digit value
				msg->optionFlag = '4';
				msg->digitOption = atoi(input);
				secondMenuErrorFlag = FALSE;
			}
		}else if(isalpha(msg->option)){
			//if user entered two character then first character is taken by default
			msg->optionFlag = '5';
			secondMenuErrorFlag = FALSE;
		}
	}else{ // if input is character or digit (range 0 - 9)
		msg->option = input[0]; // getting first character
		if(isdigit(msg->option)){
			msg->optionFlag = '4'; // setting flag
			msg->digitOption = atoi(input); //getting integer value
			secondMenuErrorFlag = FALSE;
		}else if(isalpha(msg->option)){
			msg->optionFlag = '5';
			secondMenuErrorFlag = FALSE;
		}
	}
}

/***************************************************************************************
* Purpose:		Sending message to server and getting message from server
* Author:		Fleming Patel
* Called Function:	MsgSend()
* Parameters:		ClientMessage *msg, ServerMessage *reply
* Return Value:		None
***************************************************************************************/
void doIPC(int connectionId,ClientMessage msg,ServerMessage reply){

	int msgSendStatus = MsgSend(connectionId, &msg, sizeof(msg), &reply,sizeof(reply));

	memset(msg.message, '\0', 256);

	if (msgSendStatus == -1) {
		error("Problem while sending the message");
	}

	if(!(reply.message[0] == '\0' || reply.message[0] == '\001')){
		printf("%s\n", reply.message);
	}

	if(reply.isRegister == TRUE){ // cheking server registartion flags
		registrationStatus = TRUE;
	}else{
		registrationStatus = FALSE;
	}

}

/***************************************************************************************
* Purpose:		Reseting client previous selected option to 0
* Author:		Fleming Patel
* Called Function:	None
* Parameters:		ClientMessage *msg
* Return Value:		None
***************************************************************************************/
void resetOptionFlag(ClientMessage *msg){
	msg->optionFlag = -1;
}
