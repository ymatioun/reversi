// Copyright 2001 Chris Welty
//	All Rights Reserved
#include "../rev.h"
#include "types.h"
#include "ODKStream.h"

CODKStream gs;

int connect_to_GGS(void){
	int err;

	gs.Disconnect();// always start by disconnectiong previous attempt

	// Connect(server, port)
	if((err = gs.Connect("skatgame.net", 5000))){
		FILE *f1 = fopen("GGSerr.txt", "w");
		const char *ccc = gs.ErrText(err);
		fprintf(f1, "Connect error: %s", ccc);
		fclose(f1);
		return err;
	}

	// Login(name, password)
	err = gs.Login("ymatioun", "kirpich");// game
	if(err){
		FILE *f1 = fopen("GGSerr.txt", "w");
		const char *ccc = gs.ErrText(err);
		fprintf(f1, "Login error: %s", ccc);
		fclose(f1);
		gs.Disconnect();
		return err;
	}

	gs.TellGGS("t /os open 1 \n");

	gs.Process();			// receive, parse, and pass on messages
	return 0;
}

void* threadGGS1(){ // connection thread
    while(1) // should i loop around this to reconnect? Yes. When GGS disconects, this returns, and loop makes it reconnect.
        connect_to_GGS(); // GGS connection. Returns only when disconected.
}

void* threadGGS2(){ // 'keep alive' thread
    int delay = 60 * 5; // in seconds; set to 5 min
    TimePoint t1, t2;
    t1 = timeGetTime(); // starting time
    while(1){ // should i loop around this to reconnect? Yes. When GGS disconects, this returns, and loop makes it reconnect.
        sleep(delay + 1); // in seconds
        t2 = timeGetTime(); // current time
        if(t2 - t1 > 1000 * delay){ // every 5 minutes
            gs.TellGGS("t /os who 8\n");
            t1 = t2;
        }
    }
}

void start_GGS(void){
    static pthread_t id1, id2;

    // start GGS connection thread
	pthread_create(&id1, NULL, &threadGGS1, NULL);

	// start GGS 'keep alive' thread
	pthread_create(&id2, NULL, &threadGGS2, NULL);
}
