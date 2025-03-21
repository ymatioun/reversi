// Copyright 2001 Chris Welty
//	All Rights Reserved

#ifndef ODK_OSMESSAGE_H
#define ODK_OSMESSAGE_H

#include "types.h"

#include "GGSMessage.h"
#include "OsObjects.h"

class CMsgOs : public CMsg {
public:
	virtual void Handle()=0;

	virtual COsGame* PGame(const string& idg) const;
	virtual COsMatch* PMatch(const string& idm) const;
	virtual COsRequest* PRequest(const string& idr) const;
};

// /os: abort .7 n3 is asking
class CMsgOsAbortRequest: public CMsgOs {
public:
	virtual void Handle();
	void In(istream& is);

	string idg, sLogin;
};

// /os: .44 A 1720 n3: test from me
class CMsgOsComment : public CMsgOs {
public:
	CMsgOsComment(const string& idm);

	virtual void Handle();
	virtual void In(istream& is);

	string idg;
	char cTo;
	COsPlayerInfo pi;
	string sComment;
};

// /os: end .2.0 ( pamphlet vs. ant ) +46.00
class CMsgOsEnd : public CMsgOs {
public:
	virtual void Handle();
	virtual void In(istream& is);

	string idg;
	string sPlayers[2];
	COsResult result;
};

class CMsgOsErr: public CMsgOs {
public:
	virtual void Handle();
	void In(istream& is);

	enum { kErrUnknown=0x8300, kErrRequestDoesntFitFormula, kErrIllegalBoardType,
			kErrBadRankMessage, kErrCorruptMove, kErrOpenTooLow, 
			kErrRatedMismatch, kErrUnregistered, kErrIllegalMove,
			kErrUserNotFound, kErrNotAcceptingMatches, kErrOneColorPreference,
			kErrWatchNotFound } err;
};

class CMsgOsFatalTimeout: public CMsgOs {
public:
	virtual void Handle();
	void In(istream& is);

	string idg, sLogin;
};

class CMsgOsFinger: public CMsgOs {
public:
	virtual void Handle();
	void In(istream& is);

	string sLogin;
	map<string, string> keyToValue;
	vector<COsFingerRating> frs;
};

class CMsgOsHistory: public CMsgOs {
public:
	virtual void Handle();
	void In(istream& is);

	int n;
	string sLogin;
	vector<COsHistoryItem> his;
};

class CMsgOsJoin: public CMsgOs {
public:
	virtual void Handle();
	virtual void In(istream& is);

	string idg;
	COsGame game;
};

class CMsgOsLook: public CMsgOs {
public:
	virtual void Handle();
	virtual void In(istream& is);

	int nGames;
	vector<COsGame> games;
};

class CMsgOsMatch: public CMsgOs {
public:
	virtual void Handle();
	void In(istream& is);

	int n1, n2;
	vector<COsMatch> matches;
};

class CMsgOsMatchDelta: public CMsgOs {
public:
	CMsgOsMatchDelta(bool fPlus);

	virtual void Handle();
	void In(istream& is);
	//void UpdateOs();

	bool fPlus;
	COsMatch match;
	// '- match' messages only
	COsResult result;
};

class CMsgOsRank: public CMsgOs {
public:
	virtual void Handle();
	void In(istream& is);

	COsRating rating;
	int n;	// what is this?
	vector<COsRankData> rds;
};

// /os: rating_update .21
// |pamphlet 1720.00 @ 350.00  +316.06 -> 2036.06 @ 254.20
// |ant      1697.62 @ 113.02   -43.23 -> 1654.39 @ 110.44

class CMsgOsRatingUpdate: public CMsgOs {
public:
	virtual void Handle();
	void In(istream& is);

	string idm;
	string sPlayers[2];
	COsRating rOlds[2], rNews[2];
	double dDeltas[2];
};

// /os: +  .19 1735.1 pamphlet 15:00//02:00        8 U 1345.6 ant
class CMsgOsRequestDelta: public CMsgOs {
public:
	CMsgOsRequestDelta(bool fPlus);

	virtual void Handle();
	void In(istream& is);

	bool fPlus;
	string idr;
	COsRequest request;

	bool IAmChallenged() const;
	bool IAmChallenging() const;
	bool RequireBoardSize(int n) const;
	bool RequireKomi(bool fKomi) const;
	bool RequireAnti(bool fAnti) const;
	bool RequireRand(bool fRand) const;
	bool RequireRated(bool fRated) const;
	bool RequireSynch(bool fSynch) const;
	bool RequireEqualClocks() const;
	bool RequireMaxOpponentClock(const COsClock& ck) const;
	bool RequireMinMyClock(const COsClock& ck) const;
};

class CMsgOsStored: public CMsgOs {
public:
	virtual void Handle();
	void In(istream& is);

	int nStored;
	string sLogin;
	vector<COsStoredMatch> sms;
};

class CMsgOsTimeout: public CMsgOs {
public:
	virtual void Handle();
	void In(istream& is);

	string idg, sLogin;
};

class CMsgOsTop: public CMsgOs {
public:
	virtual void Handle();
	void In(istream& is);

	COsRating rating;
	int n;	// what is this?
	vector<COsRankData> rds;
};

// /os: trust-violation .56 pamphlet (*) delta= 4 + 0.7 secs
class CMsgOsTrustViolation : public CMsgOs {
public:
	virtual void Handle();
	void In(istream& is);

	string idg, sLogin;
	char cColor;
	double delta1, delta2;
};

// /os: undo .7 n3 is asking
class CMsgOsUndoRequest: public CMsgOs {
public:
	virtual void Handle();
	void In(istream& is);

	string idg, sLogin;
};

class CMsgOsUnknown: public CMsgOs {
public:
	CMsgOsUnknown(const string& sMsgType);

	virtual void Handle();
	void In(istream& is);

	string sMsgType;
	string sText;
};

class CMsgOsUpdate: public CMsgOs {
public:
	virtual void Handle();
	void In(istream& is);

	string idg;
	COsMoveListItem mli;
};

// /os: watch 1 : .41(1)
class CMsgOsWatch: public CMsgOs {
public:
	virtual void Handle();
	void In(istream& is);

	int nMatches;
	map<string, int> idToNWatchers;
};

// /os:  + n3 watch .44
class CMsgOsWatchDelta: public CMsgOs {
public:
	CMsgOsWatchDelta(bool fPlus, const string& sLogin);

	virtual void Handle();
	void In(istream& is);

	bool fPlus;
	string sLogin, idm;
};

/*
/os: who    26      change:    win    draw    loss         match(es)
|tit4tat  + 1145.7@301.5 ->   +14.3   -12.1   -38.4 @ 79.6 
|n2       + 1384.1@ 80.5 
|ant      + 1426.2@ 50.8 ->   +39.2    +4.2   -30.8 @ 78.5 
etc.
*/

class CMsgOsWho: public CMsgOs {
public:
	virtual void Handle();
	void In(istream& is);

	int n;
	vector<COsWhoItem> wis;
};

#endif //ODK_OSMESSAGE_H