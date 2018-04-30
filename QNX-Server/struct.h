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

#define PID_COLUMN 0
#define STATUS_COLUMN 1
#define TIME_OUT_COLUMN 2

#define EMPTY 00
#define REGISTER 11
#define DEREGISTER 22
#define TIMEDOUT 33

/* This table is used for stroing client pid, registration status and timeout status */
int statusTable[10][3] = {EMPTY};
/* This table is used for mapping timer id for specific client index */
int pTimers[10] = {EMPTY};
int semaPhores = 0;

#endif /* STRUCT_H_ */
