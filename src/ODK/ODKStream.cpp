// Copyright 2001 Chris Welty
//	All Rights Reserved
#include "../rev.h"
#include "../hash.h"
#include "types.h"
#include "ODKStream.h"
#include "GGSMessage.h"
#include "OsMessage.h"
#include "GetMove.h"
#include <iostream>

string idg0;							// name of GGS game
void end_game_ynm(COsGame *pgame,COsGame *pgame2); // defined in "sp_big"

// pass a message to GGS
void CODKStream::TellGGS(char* pmsg) {
	(*this) << pmsg;
	flush();
	pass_message_to_GUI(pmsg); // copy all messages to GUI
}

// pass a message to GGS
void CODKStream::TellGGS0(char* pmsg) {
	(*this) << pmsg;
	flush();
}

void CODKStream::HandleGGS(const CMsg* pmsg) {
	cout << pmsg->sRawText << "\n";
}

void CODKStream::HandleGGSLogin() {
	BaseGGSLogin();
	(*this) << "mso\n";
	flush();
}

void CODKStream::HandleGGSTell(const CMsgGGSTell* pmsg) {
	cout << pmsg->sFrom << " " << pmsg->sText << "\n";

	/**********************************************************************/
	if(	//(pmsg->sFrom=="romano" || pmsg->sFrom=="HCyrano" || pmsg->sFrom=="delorme") &&
		(pmsg->sText.substr(0,11) == "t /td join " || pmsg->sText.substr(0,14) == "tell /td join "
		 || pmsg->sText.substr(0,14) == "tell /os open " || pmsg->sText.substr(0,11) == "t /os open ")){
			(*this) << pmsg->sText << "\n";
			flush();
	}
	/**********************************************************************/

	if (pmsg->sFrom=="ymatiou2" ) { // defines "master" - commands from master are just executed
		if (pmsg->sText=="quit"){
			Logout();
			exit(0);
		}
		else {
			(*this) << pmsg->sText << "\n";
			flush();
		}
	}
}

void CODKStream::HandleGGSUnknown(const CMsgGGSUnknown* pmsg) {
	cout << "Unknown GGS message: \n";
	HandleGGS(pmsg);
}

void CODKStream::HandleOsJoin(const CMsgOsJoin* pmsg) {
	ggsstream::BaseOsJoin(pmsg);
	MakeMoveIfNeeded(pmsg->idg);
}

void CODKStream::HandleOsLogin() {
	BaseOsLogin();
	(*this) << "ts trust +\n"
			<< "tell /os open 1\n";
	flush();
}

// Example handler from ntest:
void CODKStream::HandleOsMatchDelta(const CMsgOsMatchDelta* pmsg) {
	if (pmsg->fPlus) {// this code is executed when game begins **************************************************
		pmsg->match;
		clear_hash();
	}else{// this code is executed when game ends
		// problem: this is called in the middle of the game! This is called when some game terminates, not mine!
		string idg;
		COsGame *pgame,*pgame2;
	}
	BaseOsMatchDelta(pmsg);
}

  // Example handler from ntest:
void CODKStream::HandleOsRequestDelta(const CMsgOsRequestDelta* pmsg) {
	BaseOsRequestDelta(pmsg);
	// here is auto-accept formula ***************************************************************************************
	// it works for request from myself.
	if (pmsg->fPlus && pmsg->IAmChallenged()) {
		//if (
			//pmsg->RequireBoardSize(8) && // only 8x8
			//pmsg->RequireKomi(false) && // only not komi
			//pmsg->RequireAnti(false) && // only not anti
			//pmsg->RequireRand(false) && // only not random
			//pmsg->RequireSynch(false) && // only not synch
			//pmsg->RequireRated(true) && // rated games only
			//pmsg->RequireEqualClocks() && // only games with equal clock
			//pmsg->RequireMaxOpponentClock(COsClock(15*60,0,2*60)) && // max opponent time: 15 min+2 min increment
			//pmsg->RequireMinMyClock(COsClock(60,0,0)) // min my time: 1 min
			//)
			(*this) << "t /os accept " << pmsg->idr << "\n";
		//else
		//	(*this) << "t /os decline " << pmsg->idr << "\n";

		flush();
	}
}

void CODKStream::HandleOsUnknown(const CMsgOsUnknown* pmsg) {
	cout << "Unknown /os message: ";
	HandleOs(pmsg);
}

void CODKStream::HandleOsUpdate(const CMsgOsUpdate* pmsg) {
	BaseOsUpdate(pmsg);
	MakeMoveIfNeeded(pmsg->idg);
}

// helper function for join and update messages
void CODKStream::MakeMoveIfNeeded(const string& idg) {
	COsGame* pgame=PGame(idg);
	if (pgame!=NULL) {
		bool fMyMove=pgame->ToMove(GetLogin());
		COsMoveListItem mli;

		if (fMyMove){// call the solver.****************************************************
			idg0=idg; // save it as global variable
			GetMove(*pgame, mli);
			(*this) << "tell /os play " << idg << " " << mli << "\n";
			flush();
		}
	}
	else
		_ASSERT(0);
}

void CODKStream::PassMessage(const char *s) {
	(*this) << "tell /os tell " << idg0 << " " << s << "\n";
	flush();
}
