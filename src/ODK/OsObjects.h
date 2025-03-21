// Copyright 2001 Chris Welty
//	All Rights Reserved
#ifndef ODK_OSOBJECTS_H
#define ODK_OSOBJECTS_H
#include "types.h"
#include "time.h"
#include <string>
#include <vector>
#include <iostream>
#include <map>
using namespace std;

// basic classes that make up /os messages
class COsMove {
public:
	// Creation
	COsMove();
	COsMove(int row, int col);

	// Modification
	void SetIOS(int iosmove);

	// I/O
	void In(istream& is);
	void Out(ostream& os) const;

	// Comparison
	bool operator==(const COsMove& b) const;
	bool operator<(const COsMove& b) const;

	bool fPass;
	int row,col;
};

inline COsMove::COsMove() {};
inline COsMove::COsMove(int arow, int acol) { row=arow; col=acol; fPass=false; }
inline istream& operator>>(istream& is, COsMove& mv) { mv.In(is); return is; }
inline ostream& operator<<(ostream& os, const COsMove &mv)  { mv.Out(os); return os; }

class COsMoveListItem {
public:
	COsMove mv;
	double dEval, tElapsed;

	void In(istream& is);
	void Out(ostream& os) const;

#if _MSC_VER
	bool operator<(const COsMoveListItem& b) const { return true; }
	bool operator==(const COsMoveListItem& b) const { return true; }
#endif
};

inline istream& operator>>(istream& is, COsMoveListItem& mli) { mli.In(is); return is; }
inline ostream& operator<<(ostream& os, const COsMoveListItem &mli)  { mli.Out(os); return os; }

class COsMoveList : public vector<COsMoveListItem> {
public:
	void Update(const COsMoveListItem& mli);
};

class COsClock {
public:
	COsClock();
	COsClock(double tCurrent, double tIncrement=0, double tGrace=120, int aiTimeout=0);

	void Update(double tElapsed, bool fIncludeIncrement=true);

	// I/O
	void In(istream& is);
	void Out(ostream& os) const;
	istream& InIOS(istream& is);

	void Clear();

	bool operator==(const COsClock& b) const;
	bool operator!=(const COsClock& b) const;
	bool operator<=(const COsClock& b) const;
	bool operator>=(const COsClock& b) const;

	bool EqualsToNearestSecond(const COsClock& b) const;

	double tCurrent, tIncrement, tGrace;
	int iTimeout;

protected:
	static double ReadTime(istream& is);
	static void WriteTime(ostream& os, int nSeconds);
};

inline istream& operator>>(istream& is, COsClock& ck) { ck.In(is); return is;}
inline ostream& operator<<(ostream& os, const COsClock& ck)  { ck.Out(os); return os;}

class COsPlayerInfo {
public:
	void In(istream& is);

	void Clear();

	string sName;
	double dRating;
};

inline istream& operator>>(istream& is, COsPlayerInfo& pi) { pi.In(is); return is;}

class COsBoardType {
public:
	int n;
	bool fOcto;

	int NRandDiscsMin() const;
	int NRandDiscsMax() const;
	int NPlayableSquares() const;
	int NTotalSquares() const;
	bool DummyCorner(int row, int col) const;
	const char* Description() const;

	void In(istream& is);
	void Out(ostream& os) const;

	void Clear();
};

inline istream& operator>>(istream& is, COsBoardType& bt) {bt.In(is); return is; }
inline ostream& operator<<(ostream& os, const COsBoardType& bt)  { bt.Out(os); return os;}

class COsRatingType {
public:
	COsBoardType bt;
	bool fRand, fAnti;

	void In(istream& is);
	void Out(ostream& os) const;
};

inline istream& operator>>(istream& is, COsRatingType& rt) {rt.In(is); return is; }
inline ostream& operator<<(ostream& os, const COsRatingType& rt)  { rt.Out(os); return os;}

class COsMatchType {
public:
	COsBoardType bt;
	bool fSynch, fRand, fKomi, fAnti, fBlack, fWhite;
	int nRandDiscs;

	void In(istream& is);
	void Out(ostream& os) const;

	void Clear();

	// error checking
	int	Validate() const;

	enum {
		kErrRandAndColor=0x8500, kErrSynchAndColor, kErrKomiAndColor, kErrBlackAndWhite, 	// color errors
		kErrSynchAndKomi, kErrRandUnbalanced,	// nonsynch games
		kErrTooFewRandDiscs, kErrTooManyRandDiscs };
};

inline istream& operator>>(istream& is, COsMatchType& mt) {mt.In(is); return is; }
inline ostream& operator<<(ostream& os, const COsMatchType& mt)  { mt.Out(os); return os;}

class COsResult {
public:
	double dResult;
	enum TStatus { kUnfinished, kNormalEnd, kTimeout, kResigned, kAgreedScore, kAdjourned, kAborted} status;

	void In(istream& is);
	void Out(ostream& os) const;

	void Set(double dResult, TStatus status=kNormalEnd);
	void Clear();
};

inline istream& operator>>(istream& is, COsResult& result) {result.In(is); return is; }
inline ostream& operator<<(ostream& os, const COsResult& result)  { result.Out(os); return os;}

class COsBoard {
public:
	COsBoardType bt;
	string sBoard;
	bool fBlackMove;

	enum { BLACK = '*', WHITE='O', EMPTY='-', DUMMY='d', UNKNOWN='?'};

	void Initialize(const COsBoardType& bt);
	void Update(const COsMove& mv);

	void GetPieceCounts(int& nBlack, int& nWhite, int& nEmpty) const;
	char Piece(int row, int col) const;
	void SetPiece(int row, int col, char piece);

	void In(istream& is);
	void Out(ostream& is) const;
	void OutFormatted(ostream& os) const;

	char* GetText(char* sBoard, bool& fBlackMove, bool fTrailingNull=true) const;
	void SetText(const char* sBoard);
	char CMover() const;

	int NetBlackSquares() const;
	int Result(bool fAnti=false) const;

	// Legal moves etc
	bool GameOver() const;
	int NPass() const;
	vector<COsMove> GetMoves(bool fMover=true) const;
	bool HasLegalMove() const;
	int IsMoveLegal(const COsMove& move) const;

	void Clear();

protected:
	int UpdateDirection(int row, int col, int dRow, int dCol, char cMover, char cOpponent);
	int IsMoveLegalDirection(int row, int col, int dRow, int dCol, char cMover, char cOpponent) const;
	int IsMoveLegal(int row, int col) const;
	int IsMoveLegal(int row, int col, bool fBlackMover) const;
	bool HasLegalMove(bool fBlackMover) const;
	void OutHeader(ostream& os) const;
};

inline istream& operator>>(istream& is, COsBoard& board) {board.In(is); return is; }
inline ostream& operator<<(ostream& os, const COsBoard& board)  { board.Out(os); return os;}
inline int COsBoard::IsMoveLegal(int row, int col) const { return IsMoveLegal(row, col, fBlackMove); }
inline bool COsBoard::HasLegalMove() const { return HasLegalMove(fBlackMove); }

class COsGame;

class COsPosition {
public:
	COsBoard board;
	COsClock cks[2];
	void Update(const COsMoveListItem& mli);
	void Update(const COsMoveList& ml, int nMoves=100000);
	void UpdateKomiSet(const COsMoveListItem mlis[2]);
	void Calculate(const COsGame& game, int nMoves=100000);

	void Clear();
};

class COsGame {
public:
	string sPlace, sDateTime;
	COsPlayerInfo pis[2];
	COsPosition posStart, pos;
	COsMoveList ml;
	COsMoveListItem mlisKomi[2];
	COsMatchType mt;
	COsResult result;

	void Update(const COsMoveListItem& mli);
	void UpdateKomiSet(const COsMoveListItem mlis[2]);

	// I/O
	void In(istream& is);
	void Out(ostream& os) const;
	istream& InLogbook(istream& is);	// read logbook.gam
	istream& InIOS(istream& is);		// read ios game file

	// Creation
	void SetTime(time_t t);
	void SetCurrentTime();

	void Clear();

	// Modification
	virtual void Undo();
	void SetResult(const COsResult& result, const COsPlayerInfo pis[2]);
	void SetResult(const COsResult& result, const string sNames[2]);

	// Information
	bool ToMove(const string& sLogin) const;
	bool GameOver() const;

	bool operator<(const COsGame& b) const { return this<&b; };
	bool operator==(const COsGame& b) const { return this==&b; };

	bool NeedsKomi() const;

protected:
	void CalcCurrentPos();
};

inline istream& operator>>(istream& is, COsGame& ggf) {ggf.In(is); return is; }
inline ostream& operator<<(ostream& os, const COsGame& ggf) {ggf.Out(os); return os; }

class COsMatch {
public:
	istream& InDelta(istream& is);
	istream& In(istream& is);

	string idm;
	COsPlayerInfo pis[2];
	COsMatchType mt;
	bool fRated;
	int nObservers;	// not in matchDelta msgs

	bool IsPlaying(const string& sLogin) const;

	bool operator<(const COsMatch& b) const { return idm<b.idm; };
	bool operator==(const COsMatch& b) const { return idm==b.idm; };
};

inline istream& operator>>(istream& is, COsMatch& match) {match.In(is); return is; }

class COsRequest {
public:
	void In(istream& is);

	COsPlayerInfo pis[2];
	COsClock cks[2];
	COsMatchType mt;
	bool fRated;
};

inline istream& operator>>(istream& is, COsRequest& request) {request.In(is); return is; }

// 24 Feb 2001 18:08:41
class COsDateTime {
public:
	int day;
	string sMonth;
	int year;
	string sTime;

	void In(istream& is);
	string Text() const;
};

inline istream& operator>>(istream& is, COsDateTime& dt) {dt.In(is); return is; }

// .42661   24 Feb 2001 18:08:41 pamphlet ant      s8r16:l

class COsStoredMatch {
public:
	string idsm;
	string sPlayers[2];
	COsMatchType mt;
	COsDateTime dt;

	void In(istream& is);

	bool operator<(const COsStoredMatch& b) const { return this<&b; };
	bool operator==(const COsStoredMatch& b) const { return this==&b; };
};

inline istream& operator>>(istream& is, COsStoredMatch& sg) {sg.In(is); return is; }

class COsRating {
public:
	double dRating, dSD;

	void In(istream& is);
	double AdjustedRating() const;
};

inline istream& operator>>(istream& is, COsRating& r) {r.In(is); return is; }

class COsRatingData {
public:
	COsRating rating;
	string sInactive;
	double d1, d2;	// what are these?
	int nWins, nDraws, nLosses;

	void In(istream& is);
};

inline istream& operator>>(istream& is, COsRatingData& rd) {rd.In(is); return is; }

//     1 leaflet  2103.9@ 33.8=   1.07:08:24+@ 31.3  +4.5    551     65    395
class COsRankData {
public:
	int iRank;
	COsRatingData rd;
	string sLogin;
	bool fMe;

	void In(istream& is);

	bool operator<(const COsRankData& b) const { return iRank<b.iRank; };
	bool operator==(const COsRankData& b) const { return iRank==b.iRank; };
};

inline istream& operator>>(istream& is, COsRankData& rd) {rd.In(is); return is; }

class COsFingerRating {
public:
	COsRatingType rt;
	COsRatingData rd;

	void In(istream& is);

	bool operator<(const COsFingerRating& b) const { return this<&b; };
	bool operator==(const COsFingerRating& b) const { return this==&b; };
};

inline istream& operator>>(istream& is, COsFingerRating& fr) {fr.In(is); return is; }

/*
/os: who    26      change:    win    draw    loss         match(es)
|tit4tat  + 1145.7@301.5 ->   +14.3   -12.1   -38.4 @ 79.6 
|n2       + 1384.1@ 80.5 
|ant      + 1426.2@ 50.8 ->   +39.2    +4.2   -30.8 @ 78.5 
etc.
*/


class COsWhoItem {
public:
	string sLogin;
	COsRating rating;
	double dWin, dDraw, dLoss, dSDNew;
	bool fMe;

	void In(istream& is);

	bool operator<(const COsWhoItem& b) const;
	bool operator==(const COsWhoItem& b) const;
};

inline istream& operator>>(istream& is, COsWhoItem& wi) {wi.In(is); return is; }

class COsHistoryItem {
public:
	string idsm;
	COsDateTime dt;
	COsPlayerInfo pis[2];
	COsResult result;
	COsMatchType mt;
	void In(istream& is);

	bool operator<(const COsHistoryItem& b) const { return idsm<b.idsm; };
	bool operator==(const COsHistoryItem& b) const { return idsm==b.idsm; };
};
inline istream& operator>>(istream& is, COsHistoryItem& hi) {hi.In(is); return is; }
#endif //ODK_OSOBJECTS_H