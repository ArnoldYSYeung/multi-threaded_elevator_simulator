//	Name: Arnold Yeung
//	Date: November 19, 2015
//	Elevator Simulator - Elevator1
//	arnoldyeung.com

#include <stdio.h>
#include "C:\Users\Arnold Yeung\Desktop\elevator_simulator\rt.h"

// global

int FLOOR_TIME = 2500;			// time it takes to move thru 1 floor
int STOP_TIME = 10000;			// time it takes to stop at a requested floor

CRendezvous	rendez("StartProcess", 4);
CRendezvous vous("EndProcess", 4);
CSemaphore	p1("PS1", 0, 1);
CSemaphore	c1("CS1", 1, 1);
CSemaphore	p2("PS2", 0, 1);
CSemaphore	c2("CS2", 1, 1);

int command1[2];
int stop_ee = 0;
int fault = 0;
int sleep_time = STOP_TIME;

// Dispatcher to Elevator 1 pipeline
CPipe	pipe_de1("E1_Dispatcher", 1024);


// elevator 1 status
struct 	    mydatapooldata {
	int floor;				// floor corresponding to lifts current position
	int direction;			// direction of travel of lift
	int floors[10];			// an array representing the floors and whether requests are set 
	int door;				// status of door (0 = closed, 1 = opened)
};
CDataPool 		dp1("Car1", sizeof(struct mydatapooldata));
struct mydatapooldata 	 *MyDataPool1 = (struct mydatapooldata *)(dp1.LinkDataPool());

// this array marks 1 at the floor the elevator is suppose to stop at
int stop_floors[10];
int e1_door;
int e1_floor;

int ascii_to_int(int ascii_value) {						// this function converts an ascii value to an integer

	int integer = ascii_value - 48;

	return integer;
}


int main() {
	rendez.Wait();

	// set initial elevator 1 parameters
	MyDataPool1->floor = 0;
	MyDataPool1->direction = 0;
	MyDataPool1->door = 1;						// door starts open
	for (int i = 0; i < 10; i++) {
		MyDataPool1->floors[i] = i;
	}
	for (int i = 0; i < 10; i++) {				// unmark all floors (no stops set at any floor)
		stop_floors[i] = 0;
		printf("Unmarking floors\n");
	}

	Sleep(STOP_TIME);

	MyDataPool1->door = 0;						// door closes

	printf("Elevator 1 started...\n");

	while (stop_ee == 0) {

		p1.Wait();						// wait for IO to finish reading
		p2.Wait();						// wait for Dispatcher to finish reading		

		if (pipe_de1.TestForData() >= sizeof(command1)) {	// check to see if a new command has been sent or not

			pipe_de1.Read(&command1, sizeof(command1));
			printf("Read from pipeline\n");
			printf("Received command: %d %d\n", command1[0], command1[1]);


			if (command1[1] == 37) {				// if 'ee'
				for (int i = 0; i < 10; i++) {				// unmark all floors (no stops set at any floor)
					stop_floors[i] = 0;
				}
				stop_floors[0] = 1;					// mark floor 1
				MyDataPool1->direction = 2;			// elevator heads down
			}
			else if (command1[1] == 45) {		// fault activated
				printf("Fault received");
				SLEEP(FLOOR_TIME);
				fault = 1;
			}
			else if (command1[1] == 43) {
				printf("Fault ended");
				SLEEP(FLOOR_TIME);
				fault = 0;
			}
			else {			// if fault is not activated
							// update stop_floors[]
				stop_floors[ascii_to_int(command1[1])] = 1;		// mark that floor as potential stop

																// change direction ONLY if original direction = 0 (i.e. elevator not moving)
				if (MyDataPool1->direction == 0 && command1[0] == 49) {		// inside command and elevator not moving
					if (ascii_to_int(command1[1]) > MyDataPool1->floor) {		// if desired floor is above
						MyDataPool1->direction = 1;								// direction is UP
						printf("Direction changed to UP\n");
					}
					else if (ascii_to_int(command1[1]) < MyDataPool1->floor) {		// if desired floor is below
						MyDataPool1->direction = 2;
						printf("Direction changed to DOWN\n");// direction is DOWN
					}
				}
			}
		}

		if (fault == 1) {
			printf("Fault activated\n");
			for (int i = 0; i < 10; i++) {				// unmark all floors (no stops set at any floor)
				stop_floors[i] = 0;
				printf("Fault: %d, %d\n", fault, i);
			}
		}
		else {

			e1_floor = MyDataPool1->floor;
			// check to see if the current floor is a marked floor
			printf("Stop floor: %d\n", stop_floors[e1_floor]);

			if (stop_floors[e1_floor] == 1) {					// this must be entered into before Sleep time is activated
				printf("Arrived at stop...\n");
				MyDataPool1->door = 1;						// open up door
				e1_door = MyDataPool1->door;
				sleep_time = STOP_TIME;
				// Sleep(STOP_TIME);						// If include STOP_TIME here, the IO won't update because whole 
				// system is stalled
				stop_floors[MyDataPool1->floor] = 0;
			}
			else {

				MyDataPool1->door = 0;							// close door
				e1_door = MyDataPool1->door;
				stop_floors[MyDataPool1->floor] = 0;
				sleep_time = FLOOR_TIME;

				// check to see whether all floors have been reached or not
				int floors_left = 0;
				printf("\nStop_floors: ");
				for (int i = 0; i < 10; i++) {
					printf("%d ", stop_floors[i]);
					if (stop_floors[i] == 1)
						floors_left = 1;
				}

				// elevator moves regardless of whether new command has been sent or not
				if (MyDataPool1->direction == 1 && floors_left == 1) {			// if elevator is moving up

					MyDataPool1->floor = (MyDataPool1->floor) + 1;
					//Sleep(FLOOR_TIME);											// wait for each floor change

				}
				else if (MyDataPool1->direction == 2 && floors_left == 1) {			// if elevator is moving down

					MyDataPool1->floor = (MyDataPool1->floor) - 1;
					//Sleep(FLOOR_TIME);											// wait for each floor change
				}
				else {						// no more floors left
					MyDataPool1->direction = 0;
				}
			}
		}

		c1.Signal();
		c2.Signal();

		printf("Getting ready to sleep: %d\n", sleep_time);
		printf("Door status: %d\n", e1_door);

		Sleep(sleep_time);

		if (command1[0] == 101 && command1[1] == 101)		//ee
			stop_ee = 1;
	}


	printf("Elevator1 stopped...\n");

	getchar();
	vous.Wait();
	return 0;
}