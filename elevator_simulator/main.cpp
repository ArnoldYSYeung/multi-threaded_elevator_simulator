//	Name: Arnold Yeung
//	Date: November 19, 2015
//	Elevator Simulator - Main
//	arnoldyeung.com

#include <stdio.h>
#include "C:\Users\Arnold Yeung\Desktop\elevator_simulator\rt.h"

struct 	    mydatapooldata {
	int floor;				// floor corresponding to lifts current position
	int direction;			// direction of travel of lift
	int floors[10];			// an array representing the floors and whether requests are set 
};

int main() {

	printf("Parent attempting to create/use the datapool.....\n");
	CDataPool 		dp1("Car1", sizeof(struct mydatapooldata));
	CDataPool 		dp2("Car2", sizeof(struct mydatapooldata));


	struct mydatapooldata 	 *MyDataPool1 = (struct mydatapooldata *)(dp1.LinkDataPool());
	struct mydatapooldata 	 *MyDataPool2 = (struct mydatapooldata *)(dp2.LinkDataPool());

	printf("Parent linked to datapool1 at address %p.....\n", MyDataPool1);
	printf("Parent linked to datapool2 at address %p.....\n", MyDataPool2);

	printf("Creating processes...\n");

	// pipeline
	printf("Parent attempting to create pipeline....\n");
	CPipe	pipe1("Dispatcher_IO", 1024);

	printf("Parent lineked to pipeline...\n");

	CProcess dispatcher("C:\\Users\\Arnold Yeung\\Desktop\\elevator_simulator\\Release\\Dispatcher.exe",
		NORMAL_PRIORITY_CLASS,
		OWN_WINDOW,
		ACTIVE
		);


	CProcess io("C:\\Users\\Arnold Yeung\\Desktop\\elevator_simulator\\Release\\IO.exe",
		NORMAL_PRIORITY_CLASS,
		OWN_WINDOW,
		ACTIVE
		);

	CProcess elevator1("C:\\Users\\Arnold Yeung\\Desktop\\elevator_simulator\\Release\\Elevator1.exe",
		NORMAL_PRIORITY_CLASS,
		OWN_WINDOW,
		ACTIVE
		);

	CProcess elevator2("C:\\Users\\Arnold Yeung\\Desktop\\elevator_simulator\\Release\\Elevator2.exe",
		NORMAL_PRIORITY_CLASS,
		OWN_WINDOW,
		ACTIVE
		);

	printf("Starting processes...\n");

	printf("Waiting for processes....\n");
	dispatcher.WaitForProcess();
	io.WaitForProcess();
	elevator1.WaitForProcess();
	elevator2.WaitForProcess();

	printf("All processes ended\n");

	return 0;
}