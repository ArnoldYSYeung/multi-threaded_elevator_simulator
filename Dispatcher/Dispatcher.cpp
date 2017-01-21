//	Name: Arnold Yeung
//	Date: November 19, 2015
//	Elevator Simulator - Dispatcher
//	arnoldyeung.com

#include <stdio.h>
#include "C:\Users\Arnold Yeung\Desktop\elevator_simulator\rt.h"

// global
CRendezvous	rendez("StartProcess", 4);
CRendezvous vous("EndProcess", 4);

// Semaphores
// We want dispatcher and IO to read first
CSemaphore p2("PS2", 0, 1);
CSemaphore c2("CS2", 1, 1);			// starts at 1 - open for consumer (dispatcher) to read 
CSemaphore p4("PS4", 0, 1);
CSemaphore c4("CS4", 1, 1);

// IO to Dispatcher pipeline
CPipe	pipe1("Dispatcher_IO", 1024);

// Dispatcher to Elevators pipelines
CPipe	pipe_de1("E1_Dispatcher", 1024);
CPipe	pipe_de2("E2_Dispatcher", 1024);
//CPipe	pipe_int1("Internal_E1", 1024);
//CPipe	pipe_int2("Internal_E2", 1024);


// elevator status 1 and 2 - datapools
struct 	    mydatapooldata {
	int floor;				// floor corresponding to lifts current position
	int direction;			// direction of travel of lift
	int floors[10];			// an array representing the floors and whether requests are set 
	int door;				// status of door (0 = closed, 1 = opened)
};
CDataPool 		dp1("Car1", sizeof(struct mydatapooldata));
CDataPool		dp2("Car2", sizeof(struct mydatapooldata));
struct mydatapooldata	*MyDataPool1 = (struct mydatapooldata *)(dp1.LinkDataPool());
struct mydatapooldata	*MyDataPool2 = (struct mydatapooldata *)(dp2.LinkDataPool());

int stop_ee = 0;			// activated when received command "ee"
int fault1 = 0;				// activated when a fault1 command is received
int fault2 = 0;
int stop_ee1 = 0;
int stop_ee2 = 0;

int command[2]; // array that stores inputted command
int command1[2];
int command2[2];
UINT unsigned_command;
int elevator1_message_value = (49 - 20) * 1000;			// this value represents Elevator 1 in Message
int elevator2_message_value = (50 - 20) * 1000;


UINT convert_array_UINT(int array[2]) {		// this function converts a 2-cell array into an unsigned integer

	UINT unsigned_int = (array[0] - 20) * 1000 + command[1];

	printf("Unsigned: %u\n", unsigned_int);

	return unsigned_int;
}

int convert_UINT_array1(int unsigned_int) {		// this function converts an unsigned integer into a 2-celled array

	int array1;
	array1 = unsigned_int % 1000;

	printf("Array[1]: %d\n", array1);

	return array1;
}

int convert_UINT_array0(int unsigned_int, int array1) {		// this function converts an unsigned integer into a 2-celled array

	int array0 = (unsigned_int - array1) / 1000 + 20;


	printf("Array[0]: %d\n", array0);

	return array0;
}

int find_distance(int floor1, int floor2) {				// this function finds the distance between 2 floors
	if (floor1 > floor2) {
		return (floor1 - floor2);
	}
	else {
		return (floor2 - floor1);
	}
}

int ascii_to_int(int ascii_value) {						// this function converts an ascii value to an integer

	int integer = ascii_value - 48;

	return integer;
}

UINT __stdcall SendtoElevator1(void *args) {			// send commands to elevator 1

	CMailbox Mailbox_E1;		// mailbox that stores commands for Elevator 1

	printf("Pipeline to Elevator 1 activated...\n");

	while (1) {
		c2.Wait();										// wait for Elevator 1 to finish writing to datapool

														//if (pipe_int1.TestForData() >= sizeof(command)) {
		if (Mailbox_E1.TestForMessage() == TRUE && stop_ee1 == 0) {			// if mail is inside mailbox

																			// read information from data pool
			int current_floor = MyDataPool1->floor;
			int current_direction = MyDataPool1->direction;

			// algorithm to test if we should send command
			int ascii_current_floor = current_floor + 48;			// convert current_floor to ascii

			if (Mailbox_E1.TestForMessage(17037, 17037) == TRUE) {						// check for 'ee' command
				printf("Received 'ee1'\n");

				UINT message = Mailbox_E1.GetMessage();
				command1[1] = 37;
				command1[0] = 37;
				pipe_de1.Write(&command1, sizeof(command1));
				stop_ee1 = 1;

			}
			else if (Mailbox_E1.TestForMessage(elevator1_message_value + 43, elevator1_message_value + 45) == TRUE) {	// if fault1 command is received					// fault1 activated
				UINT message = Mailbox_E1.GetMessage();
				command1[1] = convert_UINT_array1(message);
				command1[0] = convert_UINT_array0(message, command1[1]);	// does not work with 'e' ATM

				if (command1[1] == 43) {
					fault1 = 0;				// fault1 is off
					printf("fault1 ended");
					pipe_de1.Write(&command1, sizeof(command1));		// write to pipeline -> send command to Elevator1
				}
				else if (command1[1] == 45) {
					fault1 = 1;				// fault1 is on
					pipe_de1.Write(&command1, sizeof(command1));		// write to pipeline -> send command to Elevator1

																		// clear mailbox
					while (Mailbox_E1.TestForMessage() == TRUE) {
						UINT message = Mailbox_E1.GetMessage();				// read messages until there are no more messages
						printf("Clearing messages...\n");
					}
				}
			}
			else if (((Mailbox_E1.TestForMessage( // going up and along the way or elevator is not moving or going down and along the way
				(elevator1_message_value + ascii_current_floor), (elevator1_message_value + 9 + 48)) == TRUE
				&& current_direction == 1)
				|| (Mailbox_E1.TestForMessage((elevator1_message_value + 0 + 48),
					(elevator1_message_value + ascii_current_floor)) == TRUE
					&& current_direction == 2)) && fault1 == 0) {
				UINT message = Mailbox_E1.GetMessage();
				command1[1] = convert_UINT_array1(message);
				command1[0] = convert_UINT_array0(message, command1[1]);	// does not work with 'e' ATM
				printf("Command to Elevator confirmed...\n");
				pipe_de1.Write(&command1, sizeof(command1));		// write to pipeline -> send command to Elevator1
			}
			else if (current_direction == 0 && fault1 == 0) {
				UINT message = Mailbox_E1.GetMessage();
				command1[1] = convert_UINT_array1(message);
				command1[0] = convert_UINT_array0(message, command1[1]);	// does not work with 'e' ATM
				printf("Starting new path...\n");
				pipe_de1.Write(&command1, sizeof(command1));		// write to pipeline -> send command to Elevator1
			}

		}
		p2.Signal();									// dispatcher is done reading from data pool
		printf("P2.signal\n");
		/*
		while (stop_ee == 1){
		printf("Infinite stop\n");
		UINT message = Mailbox_E1.GetMessage();				// keeps on getting new messages but don't do anything with them
		}
		*/
	}

	return 0;
}


UINT __stdcall SendtoElevator2(void *args) {			// send commands to elevator 2

	CMailbox Mailbox_E2;		// mailbox that stores commands for Elevator 2

	printf("Pipeline to Elevator 2 activated...\n");

	while (1) {
		c4.Wait();										// wait for Elevator 2 to finish writing to datapool

														//if (pipe_int2.TestForData() >= sizeof(command)) {
		if (Mailbox_E2.TestForMessage() == TRUE && stop_ee2 == 0) {			// if mail is inside mailbox

																			// read information from data pool
			int current_floor = MyDataPool2->floor;
			int current_direction = MyDataPool2->direction;

			// algorithm to test if we should send command
			int ascii_current_floor = current_floor + 48;			// convert current_floor to ascii

			if (Mailbox_E2.TestForMessage(17037, 17037) == TRUE) {						// check for 'ee' command
				printf("Received 'ee2'\n");

				UINT message = Mailbox_E2.GetMessage();
				command2[1] = 37;
				command2[0] = 37;
				pipe_de2.Write(&command1, sizeof(command2));
				stop_ee2 = 1;
			}
			else if (Mailbox_E2.TestForMessage(elevator2_message_value + 43, elevator2_message_value + 45) == TRUE) {	// if fault2 command is received
				UINT message = Mailbox_E2.GetMessage();
				command2[1] = convert_UINT_array1(message);
				command2[0] = convert_UINT_array0(message, command2[1]);	// does not work with 'e' ATM

				if (command2[1] == 43) {
					fault2 = 0;				// fault1 is off
					printf("fault2 ended");
					pipe_de2.Write(&command2, sizeof(command2));		// write to pipeline -> send command to Elevator1
				}
				else if (command2[1] == 45) {
					fault2 = 1;				// fault1 is on
					pipe_de2.Write(&command2, sizeof(command2));		// write to pipeline -> send command to Elevator1

																		// clear mailbox
					while (Mailbox_E2.TestForMessage() == TRUE) {
						UINT message = Mailbox_E2.GetMessage();				// read messages until there are no more messages
						printf("Clearing messages...\n");
					}
				}
			}
			else if (((Mailbox_E2.TestForMessage( // going up and along the way or elevator is not moving or going down and along the way
				(elevator2_message_value + ascii_current_floor), (elevator2_message_value + 9 + 48)) == TRUE
				&& current_direction == 1)
				|| (Mailbox_E2.TestForMessage((elevator2_message_value + 0 + 48),
					(elevator2_message_value + ascii_current_floor)) == TRUE
					&& current_direction == 2)) && fault2 == 0) {
				UINT message = Mailbox_E2.GetMessage();
				command2[1] = convert_UINT_array1(message);
				command2[0] = convert_UINT_array0(message, command2[1]);	// does not work with 'e' ATM
				printf("Command to Elevator confirmed...\n");
				pipe_de2.Write(&command2, sizeof(command2));		// write to pipeline -> send command to Elevator1
			}
			else if (current_direction == 0 && fault2 == 0) {
				UINT message = Mailbox_E2.GetMessage();
				command2[1] = convert_UINT_array1(message);
				command2[0] = convert_UINT_array0(message, command2[1]);	// does not work with 'e' ATM
				printf("Starting new path...\n");
				pipe_de2.Write(&command2, sizeof(command2));		// write to pipeline -> send command to Elevator1
			}

		}
		p4.Signal();									// dispatcher is done reading from data pool
	}

	return 0;
}

int main() {
	rendez.Wait();
	printf("Dispatcher started...\n");

	unsigned_command = 0;

	Sleep(2000);



	CThread	e1(SendtoElevator1, ACTIVE, NULL);
	CThread e2(SendtoElevator2, ACTIVE, NULL);

	// Reader

	while (stop_ee == 0) {
		// read information from pipeline
		if (pipe1.TestForData() >= sizeof(command)) {
			pipe1.Read(&command, sizeof(command));

			printf("Received command: %d %d\n", command[0], command[1]);

			if (command[0] == 37) {						// if command is 'ee'
				printf("Read 'ee'\n");

				//send command to SendtoElevator1
				e1.Post(convert_array_UINT(command));
				//send command to SendtoElevator2
				e2.Post(convert_array_UINT(command));
			}
			else if (command[0] == 43 || command[0] == 45) {			// if command is from outside - fault command

				if (command[1] == 49) {					// for Elevator1
					command[1] = command[0];
					command[0] = 49;					// switch command[0] and command[1] for convention
					e1.Post(convert_array_UINT(command));	// send command to SendtoElevator1
				}


				else if (command[1] == 50) {				// for Elevator2
					command[1] = command[0];
					command[0] = 50;					// switch command[0] and command[1] for convention
					e2.Post(convert_array_UINT(command));	// send command to SendtoElevator2
				}


			}

			else if (command[0] == 117 || command[0] == 100) {		// outside command - up or down

																	// need to check status of elevator
				c2.Wait();
				c4.Wait();
				// read from data pools
				int e1_direction = MyDataPool1->direction;
				int e2_direction = MyDataPool2->direction;
				int e1_floor = MyDataPool1->floor;
				int e2_floor = MyDataPool2->floor;
				int comm_floor = command[1] - 48;
				int comm_dir;

				if (command[0] == 117) {
					comm_dir = 1;
				}
				else {
					comm_dir = 2;
				}

				if (fault1 == 1 && fault2 == 0) { // if fault1 send command to elevator 2 automatically
												  //e2 takes command
					command[0] = 50; command[1] = comm_floor + 48;
					e2.Post(convert_array_UINT(command));
				}
				else if (fault2 == 1 && fault1 == 0) {
					//e1 takes command
					command[0] = 49; command[1] = comm_floor + 48;
					e1.Post(convert_array_UINT(command));
				}
				else if (fault1 == 1 && fault2 == 1) {		// if both at fault
															// nothing happens
				}
				else if (e1_direction == e2_direction) {			// both in same direction
					if (find_distance(e1_floor, comm_floor) > find_distance(e2_floor, comm_floor)) {
						// e2 takes command
						command[0] = 50; command[1] = comm_floor + 48;
						e2.Post(convert_array_UINT(command));
					}
					else {
						// e1 takes command
						command[0] = 49; command[1] = comm_floor + 48;
						e1.Post(convert_array_UINT(command));
					}
				}
				else if (e1_direction == 1 && e2_direction == 0) {		// elevator 1 up, elevator 2 out
					if (e1_floor <= comm_floor) {
						// e1 takes command
						command[0] = 49; command[1] = comm_floor + 48;
						e1.Post(convert_array_UINT(command));
					}
					else {
						// e2 takes command
						command[0] = 50; command[1] = comm_floor + 48;
						e2.Post(convert_array_UINT(command));
					}
				}
				else if (e1_direction == 2 && e2_direction == 0) {		// elevator 1 down, elevator 2 out
					if (e2_floor >= comm_floor) {
						// e2 takes command
						command[0] = 50; command[1] = comm_floor + 48;
						e2.Post(convert_array_UINT(command));
					}
					else {
						// e1 takes command
						command[0] = 49; command[1] = comm_floor + 48;
						e1.Post(convert_array_UINT(command));
					}
				}
				else if (e1_direction == 0 && e2_direction == 1) {		// elevator 1 out, elevator 2 up
					if (e2_floor <= comm_floor) {
						// e2 takes command
						command[0] = 50; command[1] = comm_floor + 48;
						e2.Post(convert_array_UINT(command));
					}
					else {
						// e1 takes command
						command[0] = 49; command[1] = comm_floor + 48;
						e1.Post(convert_array_UINT(command));
					}
				}
				else if (e1_direction == 0 && e2_direction == 2) {		// elevator 1 out, elevator 2 down
					if (e2_floor >= comm_floor) {
						// e2 takes command
						command[0] = 50; command[1] = comm_floor + 48;
						e2.Post(convert_array_UINT(command));
					}
					else {
						// e1 takes command
						command[0] = 49; command[1] = comm_floor + 48;
						e1.Post(convert_array_UINT(command));
					}
				}
				else if ((e1_direction == 1 && e2_direction == 2) || (e1_direction == 2 && e2_direction == 1)) {		// both in opposite direction
					if (e1_direction = comm_dir) {
						// e1 takes command
						command[0] = 49; command[1] = comm_floor + 48;
						e1.Post(convert_array_UINT(command));
					}
					else {
						// e2 takes command
						command[0] = 50; command[1] = comm_floor + 48;
						e2.Post(convert_array_UINT(command));
					}
				}

				p2.Signal();
				p4.Signal();


			}


			else if (command[0] == 49) {			// if command is targeted at elevator 1
				e1.Post(convert_array_UINT(command));			// send command to SendtoElevator1 (stores command in mailbox)
			}

			else if (command[0] == 50) {			// if command is targeted at elevator 2
				e2.Post(convert_array_UINT(command));			// send command to SendtoElevator2 (stores command in mailbox)
			}
		}

		if (command[0] == 101 && command[1] == 101) {		//ee
			stop_ee = 1;
		}
	}

	e1.WaitForThread();
	e2.WaitForThread();

	getchar();
	vous.Wait();
	return 0;
}