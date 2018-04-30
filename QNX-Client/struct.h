/*
 *  Created on: Friday April 20, 2018
 *  Author: 	Fleming Patel
 */

#ifndef STRUCT_H_
#define STRUCT_H_

typedef struct msgSendToServer {
	struct _pulse pulse;
	char message[256];
	char option;
	char optionFlag;
	int digitOption;
	int pid;
}ClientMessage;

typedef struct replyToClient{
	char message[256];
	int isRegister;
}ServerMessage;

int registrationStatus;


#endif /* STRUCT_H_ */
