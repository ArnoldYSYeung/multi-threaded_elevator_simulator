//	Name: Arnold Yeung
//	Date: November 19, 2015
//	Elevator Simulator - IO
//	arnoldyeung.com

#include <stdio.h>
#include "C:\Users\Arnold Yeung\Desktop\elevator_simulator\rt.h"

// global
CRendezvous	rendez("StartProcess", 4);
CRendezvous vous("EndProcess", 4);
CSemaphore	p1("PS1", 0, 1);
CSemaphore  c1("CS1", 1, 1);			// starts at 1 - open for consumer (IO) to read
CSemaphore	p3("PS3", 0, 1);
CSemaphore	c3("CS3", 1, 1);

CPipe	pipe1("Dispatcher_IO", 1024);

int input1_ascii; int input2_ascii;
int command[2];			// stores input1_ascii and input2_ascii -> sends as command
int stop_ee = 0;		// when 0, runs; when 1, stops program

						// elevator status -datapools
struct 	    mydatapooldata {
	int floor;				// floor corresponding to lifts current position
	int direction;			// direction of travel of lift
	int floors[10];			// an array representing the floors and whether requests are set 
	int door;				// status of door (0 = closed, 1 = opened)
};
CDataPool 		dp1("Car1", sizeof(struct mydatapooldata));
CDataPool 		dp2("Car2", sizeof(struct mydatapooldata));
struct		mydatapooldata 	*MyDataPool1 = (struct mydatapooldata *)(dp1.LinkDataPool());
struct		mydatapooldata	*MyDataPool2 = (struct mydatapooldata *)(dp2.LinkDataPool());


// elevator status parameters
int e1_floor; int e1_door;
int e2_floor; int e2_door;

UINT __stdcall ChildConsumer1(void *args) {		// read elevator 1 status
	printf("ChildConsumer1 started...\n");

	while (stop_ee == 0) {
		// read from datapool
		c1.Wait();

		e1_floor = MyDataPool1->floor;
		e1_door = MyDataPool1->door;

		p1.Signal();
	}
	return 0;
}

UINT __stdcall ChildConsumer2(void *args) {		// read elevator 2 status
	printf("ChildConsumer2 started...\n");

	while (stop_ee == 0) {
		// read from datapool
		c3.Wait();

		e2_floor = MyDataPool2->floor;
		e2_door = MyDataPool2->door;

		p3.Signal();
	}
	return 0;
}

UINT __stdcall InputGetter(void *args) {		// this thread gets the input from the user's keyboard
	int input1; char input1char; int gotinput1 = 0;
	int input2; char input2char; int gotinput2 = 0;

	printf("InputGetter started...\n");

	while (stop_ee == 0) {
		// takes command
		// printf("Enter in a command: ");

		gotinput1 = 0; gotinput2 = 0;			// reset gotinput

		while (gotinput1 == 0) {

			input1_ascii = _getch();

			// translate ASCII
			if (input1_ascii == 49) {	// 1
				input1 = 1;
				//printf("%d", input1);
				gotinput1 = 1;
			}
			else if (input1_ascii == 50) {	//2
				input1 = 2;
				//printf("%d", input1);
				gotinput1 = 1;
			}
			else if (input1_ascii == 117) {		//u
				input1char = 'u';
				//printf("%c", input1char);
				gotinput1 = 1;
			}
			else if (input1_ascii == 100) {		//d
				input1char = 'd';
				//printf("%c", input1char);
				gotinput1 = 1;
			}
			else if (input1_ascii == 43) {		//+
				input1char = '+';
				//printf("%c", input1char);
				gotinput1 = 1;
			}
			else if (input1_ascii == 45) {		// -
				input1char = '-';
				//printf("%c", input1char);
				gotinput1 = 1;
			}
			else if (input1_ascii == 101) {		//e
				input1char = 'e';
				//printf("%c", input1char);
				gotinput1 = 1;
			}
		}

		while (gotinput2 == 0) {
			input2_ascii = _getch();

			// translate ascii
			if (input2_ascii >= 48 && input2_ascii <= 57) {
				input2 = input2_ascii - 48;
				//printf("%d", input2);
				gotinput2 = 1;
			}
			else if (input2_ascii == 101) {
				input2char = 'e';
				//printf("%c", input2char);
				gotinput2 = 1;
			}
		}
		//printf("\n");

		command[0] = input1_ascii;
		command[1] = input2_ascii;

		//printf("\n%d %d\n", input1_ascii, input2_ascii);

		// check to see if simulation has stopped 'ee'
		if (command[0] == 101 && command[1] == 101) {
			command[0] = 37;			// indicate that command is sent to both elevators
			command[1] = 37;			// indicate that command is 'ee'
										//stop_ee = 1;
		}

		pipe1.Write(&command, sizeof(command));		// write to pipeline -> send command to Dispatcher

	}
	return 0;
}

int main() {

	rendez.Wait();
	printf("IO started...");

	printf("IO linked to datapool1 at address %p.....\n", MyDataPool1);
	printf("IO linked to datapool2 at address %p.....\n", MyDataPool2);

	// create threads
	CThread c1(ChildConsumer1, ACTIVE, NULL);
	CThread c2(ChildConsumer2, ACTIVE, NULL);
	CThread i1(InputGetter, ACTIVE, NULL);

	// print out operations graphically
	while (stop_ee == 0) {
		system("cls");				// clears the screen

									// print each floor
		for (int i = 9; i >= 0; i--)
		{
			printf("%d     [", i);    // ][]\n", i);

									  // print elevator 1
			if (e1_floor == i) {			// if elevator 1 is on this floor
				if (e1_door == 1) {			// if door is opened
					printf("| |]");
				}
				else {
					printf(" | ]");
				}
			}
			else {
				printf("   ]");
			}

			printf("        [");
			// print elevator 2
			if (e2_floor == i) {			// if elevator 2 is on this floor
				if (e2_door == 1) {			// if door is opened
					printf("| |]\n");
				}
				else {
					printf(" | ]\n");
				}
			}
			else {
				printf("   ]\n");
			}
		}
	}


	c1.WaitForThread();		// wait for the child process to terminate if necessary
	c2.WaitForThread();
	i1.WaitForThread();


	getchar();
	vous.Wait();
	return 0;
}