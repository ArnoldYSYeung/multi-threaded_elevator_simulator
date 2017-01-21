//	Name: Arnold Yeung
//	Date: November 19, 2015
//	Elevator Simulator - Elevator2
//	arnoldyeung.com

#include <stdio.h>
#include "C:\Users\Arnold Yeung\Desktop\elevator_simulator\rt.h"

// global

int FLOOR_TIME = 2500;			// time it takes to move thru 1 floor
int STOP_TIME = 10000;			// time it takes to stop at a requested floor

CRendezvous	rendez("StartProcess", 4);
CRendezvous vous("EndProcess", 4);
CSemaphore	p3("PS3", 0, 1);
CSemaphore	c3("CS3", 1, 1);
CSemaphore	p4("PS4", 0, 1);
CSemaphore	c4("CS4", 1, 1);

int command2[2];
int stop_ee = 0;
int fault = 0;
int sleep_time = STOP_TIME;

// Dispatcher to Elevator 2 pipeline
CPipe	pipe_de2("E2_Dispatcher", 1024);


// elevator 2 status
struct 	    mydatapooldata {
	int floor;				// floor corresponding to lifts current position
	int direction;			// direction of travel of lift
	int floors[10];			// an array representing the floors and whether requests are set 
	int door;				// open up door
};
CDataPool 		dp2("Car2", sizeof(struct mydatapooldata));
struct mydatapooldata 	 *MyDataPool2 = (struct mydatapooldata *)(dp2.LinkDataPool());

// this array marks 1 at the floor the elevator is suppose to stop at
int stop_floors[10];
int e2_door;
int e2_floor;

int ascii_to_int(int ascii_value) {						// this function converts an ascii value to an integer

	int integer = ascii_value - 48;

	return integer;
}


int main() {
	rendez.Wait();

	// set initial elevator 1 parameters
	MyDataPool2->floor = 0;
	MyDataPool2->direction = 0;
	MyDataPool2->door = 1;
	for (int i = 0; i < 10; i++) {
		MyDataPool2->floors[i] = i;
	}
	for (int i = 0; i < 10; i++) {				// unmark all floors (no stops set at any floor)
		stop_floors[i] = 0;
		printf("Unmarking floors\n");
	}

	Sleep(STOP_TIME);

	MyDataPool2->door = 0;						// door closes

	printf("Elevator 2 started...\n");

	while (stop_ee == 0) {

		p3.Wait();						// wait for IO to finish reading
		p4.Wait();						// wait for Dispatcher to finish reading		

		if (pipe_de2.TestForData() >= sizeof(command2)) {	// check to see if a new command has been sent or not

			pipe_de2.Read(&command2, sizeof(command2));
			printf("Read from pipeline\n");
			printf("Received command: %d %d\n", command2[0], command2[1]);


			if (command2[1] == 37) {				// if 'ee'
				for (int i = 0; i < 10; i++) {				// unmark all floors (no stops set at any floor)
					stop_floors[i] = 0;
				}
				stop_floors[0] = 1;					// mark floor 1
				MyDataPool2->direction = 2;			// elevator heads down
			}
			else if (command2[1] == 45) {				// fault activated
				printf("Fault received");
				SLEEP(FLOOR_TIME);
				fault = 1;
			}
			else if (command2[1] == 43) {
				printf("Fault ended");
				SLEEP(FLOOR_TIME);
				fault = 0;
			}
			else {			// if fault is not activated
							// update stop_floors[]
				stop_floors[ascii_to_int(command2[1])] = 1;		// mark that floor as potential stop

																// change direction ONLY if original direction = 0 (i.e. elevator not moving)
				if (MyDataPool2->direction == 0 && command2[0] == 50) {		// inside command and elevator not moving
					if (ascii_to_int(command2[1]) > MyDataPool2->floor) {		// if desired floor is above
						MyDataPool2->direction = 1;								// direction is UP
						printf("Direction changed to UP\n");
					}
					else if (ascii_to_int(command2[1]) < MyDataPool2->floor) {		// if desired floor is below
						MyDataPool2->direction = 2;
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

			e2_floor = MyDataPool2->floor;
			// check to see if the current floor is a marked floor
			printf("Stop floor: %d\n", stop_floors[e2_floor]);

			if (stop_floors[e2_floor] == 1) {					// this must be entered into before Sleep time is activated
				printf("Arrived at stop...\n");
				MyDataPool2->door = 1;						// open up door
				e2_door = MyDataPool2->door;
				sleep_time = STOP_TIME;
				// Sleep(STOP_TIME);						// If include STOP_TIME here, the IO won't update because whole 
				// system is stalled
				stop_floors[MyDataPool2->floor] = 0;
			}
			else {

				MyDataPool2->door = 0;
				e2_door = MyDataPool2->door;
				stop_floors[MyDataPool2->floor] = 0;
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
				if (MyDataPool2->direction == 1 && floors_left == 1) {			// if elevator is moving up

					MyDataPool2->floor = (MyDataPool2->floor) + 1;
					//Sleep(FLOOR_TIME);											// wait for each floor change

				}
				else if (MyDataPool2->direction == 2 && floors_left == 1) {			// if elevator is moving down

					MyDataPool2->floor = (MyDataPool2->floor) - 1;
					//Sleep(FLOOR_TIME);											// wait for each floor change
				}
				else {						// no more floors left
					MyDataPool2->direction = 0;
				}
			}
		}

		c3.Signal();
		c4.Signal();

		printf("Getting ready to sleep: %d\n", sleep_time);
		printf("Door status: %d\n", e2_door);

		Sleep(sleep_time);

		if (command2[0] == 101 && command2[1] == 101)		//ee
			stop_ee = 1;
	}


	printf("Elevator2 stopped...\n");

	getchar();
	vous.Wait();
	return 0;
}