/*
========================================================================================
* File Name:	QNX_IPC_Server.c
* Compiler:		QNX QCC
* Author:		Fleming Patel
* Course:		Real-Time Programming
* Assignment:	06
* Date:			Friday April 20, 2018
* Professor:	RICHARD HAGEMEYER, MOHAMMAD PATOARY
* Purpose:		This file is create a channel which owned by the process.
* 				The fils is also resonsible for registering client's up to maximim
* 				limit three.
* 				The new feature implemented which trackes the client activness,
* 				and depending on that server deregisters client.
========================================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/netmgr.h>
#include <sys/neutrino.h>
#include "struct.h"

/***************************************************************************************
						Define Constants
***************************************************************************************/
#define TRUE 1
#define FALSE 0
#define ERROR_CODE -1
#define MY_PULSE_CODE _PULSE_CODE_MAXAVAIL
/***************************************************************************************
					Function Prototypes
***************************************************************************************/
int isTableEmpty();
int getClientIndexInTable(int clientPid);
int isRegister(int index);
int getClientPid(int index);

void startTimer(int channelId,ClientMessage *msg,struct sigevent event,struct itimerspec itime,timer_t timer_id);
void deleteTimer(ClientMessage *msg);

void initializingReply(ClientMessage *msg, ServerMessage *reply, FILE *fp,int clientIndex);

void error(char *msg) {
	perror(msg);
	exit(1);
}

/***************************************************************************************
* Purpose:			create a channel which owned by the process, getting message
* 					from client and replying back to client
* Author:			Fleming Patel
* Called Function:	ChannelCreate(), MsgReceive(), initializingReply(),
* 					MsgReply()
* Parameters:		int argc, char *argv[]
* Return Value:		return 0 Exit success
***************************************************************************************/

int main(int argc, char *argv[]) {
	struct sigevent event;
	struct itimerspec itime;
	timer_t timer_id;

	ClientMessage msg;
	ServerMessage reply;
	int recvId, channelId;
	FILE *fp = NULL;

	/*
	 * creates file if does not exist and if exist then it will
	 * Open for both reading and appending in binary mode.
	 */
	fp = fopen("/home/qnxuser/logs.txt", "ab+");
	// getpid() function will return current process id
	printf("PID :- %d : Running...\n",getpid());
	fprintf(fp,"Server PID :- %d : Running...\n",getpid());
	fclose(fp); // closing file

	// Server creates a channel and that channel is attached to the process
	channelId = ChannelCreate(0);

	while(1){
		/*
		 * creates file if does not exist and if exist then it will
		 * Open for both reading and appending in binary mode.
		 */
		fp = fopen("/home/qnxuser/logs.txt", "ab+");

		memset(reply.message, '\0', 256);

		recvId = MsgReceive(channelId,&msg,sizeof(msg),NULL);

		if(recvId == -1){
			printf("Problem receiving the message from client.\n");
			return EXIT_FAILURE;
		}else if(recvId == 0){ // receiving pulse
			if(fp == NULL){
				fp = fopen("/home/qnxuser/logs.txt", "ab+");
			}
			//here msg.pulse.code represent the index number of client in statusTable
			int clientPid = getClientPid(msg.pulse.code);
			printf("Server Message: Time out for client: %d\n",clientPid);
			fprintf(fp,"Server Message: Time out for client: %d\n",clientPid);
			msg.optionFlag = '2';
			//assiginging timedout status for the client in statustable for displaying appropriate message
			statusTable[msg.pulse.code][TIME_OUT_COLUMN] = TIMEDOUT;
			initializingReply(&msg,&reply,fp,msg.pulse.code);
			fclose(fp); // closing file
			continue;
		}else{
			if(msg.optionFlag == '0'){ // if client application starts adding client entry in table
				if(isTableEmpty() == TRUE){ // checking for table empty spaces
					initializingReply(&msg,&reply,fp,-1);
				}else{
					fprintf(fp,"Server Message: Client %d: Storage of table is full\n",msg.pid);
					printf("Server Message: Client %d: Storage of table is full\n",msg.pid);
					strcpy(reply.message,"Server Message: Storage of table is full\n");
				}
			}else{
				int clientIndex;
				/*
				 * Here the whole functionality is depend on the client index number.
				 * Whenever any request comes from the client, before that it's require to verify the client entry in table.
				 * Whitout entry in table client can not do any thing. After checking entry the client in table the registration and
				 * deregistration functionality and other functionality allowed.
				 *
				 * In this program option 3 is used for entering the message and selecting options on that message.
				 * If client select the option 3 and enter a message, it considered as busy (active) because the client program
				 * architecture is like that. Client sending message and seleting options on that message works together, if that both
				 * part is done then client considered as not active and timer starts again.
				 *
				 */
				if((clientIndex = getClientIndexInTable(msg.pid)) != ERROR_CODE){ // checking the client's entry in table
					// for flag 1 and flag 6 no registartion checking required
					if(msg.optionFlag == '1'){
						statusTable[clientIndex][TIME_OUT_COLUMN] = EMPTY;
						initializingReply(&msg,&reply,fp,clientIndex);
						startTimer(channelId, &msg,event,itime,timer_id);
					}else if(msg.optionFlag == '6'){
						deleteTimer(&msg); // when client leaves the application delete the timer
						initializingReply(&msg,&reply,fp,clientIndex);
					}else{
						if(isRegister(clientIndex) == REGISTER){ // checking if client is register or not
							if(msg.optionFlag == '2' || msg.optionFlag == '3'){
								deleteTimer(&msg);
								initializingReply(&msg,&reply,fp,clientIndex);
							}else if(msg.optionFlag == '7'){
								reply.isRegister = TRUE;
								memset(reply.message, '\0', 256);
							}else{
								initializingReply(&msg,&reply,fp,clientIndex);
								startTimer(channelId, &msg,event,itime,timer_id);
							}
						}else{
							/*
							 * Sending message to the client according to the timedout deregistration or
							 * client deregister by its choice.
							 */
							if(statusTable[clientIndex][TIME_OUT_COLUMN] == TIMEDOUT){
								reply.isRegister = FALSE;
								fprintf(fp,"Server Message: Client %d register timed out\n",msg.pid);
								printf("Server Message: Client %d register timed out\n",msg.pid);
								strcpy(reply.message,"Server Message: Timed out please register again\n");
							}else{
								reply.isRegister = FALSE;
								fprintf(fp,"Server Message: Client %d Please register first before using service\n",msg.pid);
								printf("Server Message: Client %d Please register first before using service\n",msg.pid);
								strcpy(reply.message,"Server Message: Please register first before using service\n");
							}
						}
					}
				}else{
					fprintf(fp,"Server Message: Client %d You are not eligible for using service\n",msg.pid);
					printf("Server Message: Client %d You are not eligible for using service\n",msg.pid);
					strcpy(reply.message,"Server Message: You are not eligible for using service\n");
				}
			}
		}

		int msgReplyStatus = MsgReply(recvId,1,&reply,sizeof(reply));

		if(msgReplyStatus == -1){
			printf("Problem while replying the message to client.\n");
			return EXIT_FAILURE;
		}
		fclose(fp); // closing file
	}
	return 0;
}

/***************************************************************************************
* Purpose:			The purpose of this function is checking the empty space in table.
* 					I created this function as a future consideration, right now the
* 					table does not have functionality for dynamic allocation.
* 					If the table is full then the client should receive the message of
* 					table is full.
* Author:			Fleming Patel
* Called Function:	sizeof()
* Parameters:		None
* Return Value:		TRUE For success and FALSE for failure
***************************************************************************************/
int isTableEmpty(){
	int rows = sizeof(statusTable) / sizeof(statusTable[0]);
	for(int index = 0; index < rows ; index++){
		if(statusTable[index][PID_COLUMN] == EMPTY){
			return TRUE;
		}
	}
	return FALSE;
}

/***************************************************************************************
* Purpose:			This method returns client's index in the table. If client entry is
* 					not in table then it return ERROR_CODE.
* Author:			Fleming Patel
* Called Function:	sizeof()
* Parameters:		int clientPid
* Return Value:		returns Client's index or ERROR_CODE, if client's entry is not found
***************************************************************************************/
int getClientIndexInTable(int clientPid){
	int rows = sizeof(statusTable) / sizeof(statusTable[0]);
	for(int index = 0; index < rows ; index++){
		if(statusTable[index][PID_COLUMN] == clientPid){
			return index;
		}
	}
	return ERROR_CODE;
}

/***************************************************************************************
* Purpose:			This method return client registration status
* Author:			Fleming Patel
* Called Function:	None
* Parameters:		int index
* Return Value:		returns REGISTER 11 or DEREGISTER 22
***************************************************************************************/
int isRegister(int index){
	return statusTable[index][STATUS_COLUMN];
}

/***************************************************************************************
* Purpose:			This method returns client pid depending on client's index
* 					in the table.
* Author:			Fleming Patel
* Called Function:	None
* Parameters:		int index
* Return Value:		returns client's pid
***************************************************************************************/
int getClientPid(int index){
	return statusTable[index][PID_COLUMN];
}

/***************************************************************************************
* Purpose:			The purpose of this method is creating a specific event and timer
* 					for speicific client.
* 						-	The timer is set for 15 seconds.
*						-	The timer id is storing in the pTimers Table
*							according to client index.
* Author:			Fleming Patel
* Called Function:	ConnectAttach(), getClientIndexInTable(), timer_create(),
* 					timer_settime()
* Parameters:		None
* Return Value:		TRUE For success and FALSE for failure
***************************************************************************************/
void startTimer(int channelId,ClientMessage *msg,struct sigevent event,struct itimerspec itime,timer_t timer_id){
	event.sigev_notify = SIGEV_PULSE;
	event.sigev_coid = ConnectAttach(ND_LOCAL_NODE,0,channelId,_NTO_SIDE_CHANNEL, 0);
	//event.sigev_priority = getpriority(0);
	event.sigev_code = getClientIndexInTable(msg->pid);
	timer_create(CLOCK_REALTIME, &event, &timer_id);
	itime.it_value.tv_sec = 15;
	timer_settime(timer_id, 0, &itime, NULL);
	pTimers[event.sigev_code] = timer_id;
}

/***************************************************************************************
* Purpose:			The purpose of this method is deleting specific timer for sepcifc
* 					client's pid
* Author:			Fleming Patel
* Called Function:	getClientIndexInTable(),timer_delete()
* Parameters:		ClientMessage *msg
* Return Value:		TRUE For success and FALSE for failure
***************************************************************************************/
void deleteTimer(ClientMessage *msg){
	int clientIndexInTable = getClientIndexInTable(msg->pid);
	timer_delete(pTimers[clientIndexInTable]);
}

/***************************************************************************************
* Purpose:			Initializing what option client select with the help of flag.
* 					Flag 0: Adds the client pid int the table and by default it assigns
* 							that client is not register.
* 					Flag 1:	If semaphores is not zero than register's the client. If
* 							client is already registered than send appropriate message
* 							to client.
* 					Flag 2: Deregistering client and incrementing semaphores so another
* 							client can be register.
* 					Flag 3:	Sends the client message back, stating that server found
* 							message.
* 					Flag 4: Client used digit value for query, returns the specific
* 							character on that digit value index in it's valid.
* 					Flag 5: Client used character value for query, returns the total
* 							occurrences of that character which send by client.
* 					Flag 6:	When client close the connection without deregistering then
* 							this part deregistered client and increment semaphores and
* 							also removes client's entry from table so another client
* 							can be add and connect.
* Author:			Fleming Patel
* Called Function:	strlen(), strcpy(), malloc(), free(), strcat(), sizeof(),
* 					MsgReply()
* Parameters:		ClientMessage *msg, ServerMessage *reply, FILE *fp,int clientIndex
* Return Value:		None
***************************************************************************************/
void initializingReply(ClientMessage *msg, ServerMessage *reply, FILE *fp,int clientIndex){
	if(msg->optionFlag == '0'){ // Adding client in status table
		int rows = sizeof(statusTable) / sizeof(statusTable[0]);
		for(int index = 0; index < rows ; index++){ // loop through all table list
			if(statusTable[index][PID_COLUMN] == EMPTY){ // finding next empty space
				statusTable[index][PID_COLUMN] = msg->pid; // adding client pid in table
				statusTable[index][STATUS_COLUMN] = DEREGISTER; // make client deregistered in table
				reply->isRegister = FALSE; // assigning by default deregister so client can understand
				fprintf(fp,"Server Message: Client %d is connected to channel...\n",msg->pid);
				printf("Server Message: Client %d is connected to channel...\n",msg->pid);
				break;
			}
		}
	}else if(msg->optionFlag == '1'){ // Registering client in status table
		if(statusTable[clientIndex][STATUS_COLUMN] == REGISTER){
			reply->isRegister = TRUE; // assigning register flag so client can understand
			fprintf(fp,"Server Message: Client %d You are already Registered\n",msg->pid);
			printf("Server Message: Client %d You are already Registered\n",msg->pid);
			strcpy(reply->message,"Server Message: You are already Registered\n");
		}else{
			if(semaPhores!=3){
				statusTable[clientIndex][STATUS_COLUMN] = REGISTER;
				reply->isRegister = TRUE; // assigning register flag so client can understand
				semaPhores++; // incrementing semaphores
				fprintf(fp,"Server Message: Client %d You are now Registered\n",msg->pid);
				printf("Server Message: Client %d You are now Registered\n",msg->pid);
				strcpy(reply->message,"Server Message: You are now Registered\n");
			}else{
				reply->isRegister = FALSE; // assigning deregister so client can understand
				fprintf(fp,"Server Message: Client %d Unable to register Too Many Client\n",msg->pid);
				printf("Server Message: Client %d Unable to register Too Many Client\n",msg->pid);
				strcpy(reply->message,"Server Message: Unable to register Too Many Client\n");
			}
		}
	}else if(msg->optionFlag == '2'){ // deregistering client in status table
		int clientPid = getClientPid(clientIndex);
		statusTable[clientIndex][STATUS_COLUMN] = DEREGISTER; // make client deregistered in table
		reply->isRegister = FALSE; // assigning deregister so client can understand
		semaPhores--;  // decrementing semaphores
		fprintf(fp,"Server Message: Client %d You are now Deregistered\n",clientPid);
		printf("Server Message: Client %d You are now Deregistered\n",clientPid);
		strcpy(reply->message,"Server Message: You are now Deregistered\n");
	}else if(msg->optionFlag == '3'){ // if optionFlag is 3 then it will only send client message back as server message
		strcpy(reply->message,msg->message);
		fprintf(fp,"Client %d Message: %s",msg->pid,reply->message);
		printf("Client %d Message: %s",msg->pid,reply->message);
	}else if(msg->optionFlag == '4'){ // flag 1 is for digit type
		size_t messageLength = strlen(msg->message);
		messageLength = messageLength - 1; // index starts from 0
		int selectedIndex = msg->digitOption;
		fprintf(fp,"Client %d Message: Selected index option with value: %d\n",msg->pid,selectedIndex);
		printf("Client %d Message: Selected index option with value: %d\n",msg->pid,selectedIndex);
		/*
		 * if user send digit than if digit is more than client's
		 * message length than replying client with appropriate message
		 */
		if((selectedIndex > messageLength) || (selectedIndex < 0)){
			char *message = "Please chose appropriate index for string";
			strcpy(reply->message,message);
			fprintf(fp,"Server Message: Client %d: %s\n",msg->pid,reply->message);
			printf("Server Message: Client %d: %s\n",msg->pid,reply->message);
		}else{
			/*
			 * if client's digit option is valid than below code send the
			 * specific character with that specific client's digit option
			 */
			char foundCharacter = msg->message[selectedIndex]; //getting character on specific index
			char *message = "Server Found:- ";
			size_t messageLength = strlen(message);

			char *newString = (char *)malloc(sizeof(messageLength + 20));
			strcpy(newString,message); // combining strings

			newString[messageLength++] = foundCharacter; // adding found character to reply string
			newString[messageLength] = '\0';
			strcpy(reply->message,newString);
			free(newString);
			fprintf(fp,"Server Message: Client %d: %s\n",msg->pid,reply->message);
			printf("Server Message: Client %d: %s\n",msg->pid,reply->message);
		}
	}else if(msg->optionFlag == '5'){ //flag 2 is for character type
		/*
		 * if client send letter to server than below code is responsible for
		 * sending the total occurrences of that letter which used in client's message
		 */
		int i = 0;
		int count = 0;
		char chCount;
		fprintf(fp,"Client %d Message: Selected character option with value: %c\n",msg->pid,msg->option);
		printf("Client %d Message: Selected character option with value: %c\n",msg->pid,msg->option);
		while(msg->message[i] != '\0'){ // loops run until it reached end of line character
			if(msg->message[i] == msg->option){ // if letter found
				count++; // incrementing count
			}
			i++;
		}

		chCount = count + '0'; // converting integer to character
		char *message = "Server Found:- ";
		size_t messageLength = strlen(message);

		char *newString = (char *)malloc(sizeof(messageLength + 20));
		strcpy(newString,message);

		newString[messageLength++] = chCount;
		newString[messageLength] = '\0';

		strcat(newString," times letter used"); // combining strings
		strcpy(reply->message,newString);
		free(newString);
		fprintf(fp,"Server Message: Client %d: %s\n",msg->pid,reply->message);
		printf("Server Message: Client %d: %s\n",msg->pid,reply->message);
	}else if(msg->optionFlag == '6'){ // if client close the connection
		if(isRegister(clientIndex) == REGISTER){ // check the client is still register
			statusTable[clientIndex][PID_COLUMN] = EMPTY; // removing client entry from table
			statusTable[clientIndex][STATUS_COLUMN] = DEREGISTER; // make client registered in table
			semaPhores--; // decrementing semaphores
			fprintf(fp,"Server Message: Client %d: Exiting\n",msg->pid);
			printf("Server Message: Client %d: Exiting\n",msg->pid);
		}else{
			statusTable[clientIndex][PID_COLUMN] = EMPTY; // removing client entry from table
			fprintf(fp,"Server Message: Client %d: Exiting\n",msg->pid);
			printf("Server Message: Client %d: Exiting\n",msg->pid);
		}
	}
}
