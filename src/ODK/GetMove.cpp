// Copyright 2001 Chris Welty
//	All Rights Reserved
#include "../rev.h"
#include "types.h"
#include "GetMove.h"
#include "OsObjects.h"

#ifndef ODK_ODKSTREAM_H
#define ODK_ODKSTREAM_H

#include "ggsstream.h"

class CODKStream: public ggsstream {
public:
	virtual void TellGGS(char *str);
	virtual void TellGGS0(char *str);
	virtual void HandleGGS				(const CMsg* pmsg);
	virtual void HandleGGSLogin			();
	virtual void HandleGGSTell			(const CMsgGGSTell* pmsg);
	virtual void HandleGGSUnknown		(const CMsgGGSUnknown* pmsg);

	virtual void HandleOsJoin			(const CMsgOsJoin* pmsg);
	virtual void HandleOsLogin			();
	virtual void HandleOsUnknown		(const CMsgOsUnknown* pmsg);
	virtual void HandleOsUpdate			(const CMsgOsUpdate* pmsg);

	virtual void MakeMoveIfNeeded(const string& idg);
	virtual void PassMessage(const char*);

	void HandleOsMatchDelta(const CMsgOsMatchDelta* pmsg);//
	void HandleOsRequestDelta(const CMsgOsRequestDelta* pmsg);//
};
#endif // ODK_GGSSTREAM_H


extern CODKStream gs;
void pass_message_to_GGS(char *s){// passes message to current game window
	gs.TellGGS(s);
}

void pass_message_to_GGS0(char *s){// passes message to current game window
	gs.TellGGS0(s);
}

void pass_message_to_game(const char *s){// passes message to current game window
	gs.PassMessage(s);
}

extern int ggs_score;
void GetMove(const COsGame& game, COsMoveListItem& mli) {
	// this function gets called automatically when i have to move.
	TimePoint t1,t2;
	t1 = timeGetTime();
	COsBoard board=game.pos.board;
	COsClock clock=game.pos.cks[board.fBlackMove==true];

	//sync_game=game.mt.fSynch;// indicates sync games

	// compare positions
	UINT64 *pos_GGS = &b_m.pos[0];
	pos_GGS[0]=pos_GGS[1]=0;
	for(int i1=7;i1>=0;--i1)
		for(int j1=7;j1>=0;--j1){
			char c1=board.Piece(i1,j1);
			int a1=0,a2=0;
			if(board.fBlackMove==true){
				switch(c1){
				case '*':
					a1=1;
					break;
				case 'O':
					a1=-1;
					break;
				}
			}else{
				switch(c1){
				case 'O':
					a1=1;
					break;
				case '*':
					a1=-1;
					break;
				}
			}
			pos_GGS[0]=(pos_GGS[0]<<1)+(a1==1);
			pos_GGS[1]=(pos_GGS[1]<<1)+(a1==-1);
		}
	double t_rem=clock.tCurrent;

	// code to check for pass
	if(!bit_mob_mask(b_m.pos[0],b_m.pos[1])){ // pass
        pass_message_to_GUI("pass!\n");
        mli.mv.fPass = true;
		mli.dEval = 0.; // expected score
		return;
	}
	int mmm = top_solve(std::max(100, (int)(t_rem * 1000 - 1000)));// use t-1 sec, not less than .1
	t2 = timeGetTime();
	mli.tElapsed = (t2 - t1) / 1000.f;// time
	mli.mv.fPass = false;
	mli.mv.row = mmm / 8;
	mli.mv.col = mmm % 8;
	mli.dEval = ggs_score; // expected score
}
