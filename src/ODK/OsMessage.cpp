// Copyright 2001 Chris Welty
//	All Rights Reserved
#include "../rev.h"
#include "types.h"
#include "OsMessage.h"
#include "ggsstream.h"
#include <strstream>
//using namespace std;

///////////////////////////////////
// CMsgOs
///////////////////////////////////

COsGame* CMsgOs::PGame(const string& idg) const {
	map<string,COsGame>& idToGame = pgs->idToGame;
	if (idToGame.find(idg)==idToGame.end())
		return NULL;
	else
		return &(idToGame[idg]);
}

COsMatch* CMsgOs::PMatch(const string& idm) const {
	map<string,COsMatch>& idToMatch = pgs->idToMatch;
	if (idToMatch.find(idm)==idToMatch.end())
		return NULL;
	else
		return &(idToMatch[idm]);
}

COsRequest* CMsgOs::PRequest(const string& idr) const {
	map<string,COsRequest>& idToRequest = pgs->idToRequest;
	if (idToRequest.find(idr)==idToRequest.end())
		return NULL;
	else
		return &(idToRequest[idr]);
}

///////////////////////////////////
// CMsgOsAbortRequest
///////////////////////////////////

void CMsgOsAbortRequest::In(istream& is) {
	string sDummy;

	is >> idg >> sLogin >> ws;
	getline(is, sDummy);

	_ASSERT(sDummy=="is asking");
}

void CMsgOsAbortRequest::Handle() {
	pgs->HandleOsAbortRequest(this);
}

///////////////////////////////////
// CMsgOsComment
///////////////////////////////////

CMsgOsComment::CMsgOsComment(const string& aidg) {
	idg=aidg;
}

// /os: .44 A 1720 n3: test from me
void CMsgOsComment::In(istream& is) {
	string sPretext;

	getline(is, sPretext, ':');
	istrstream isPretext(sPretext.c_str());
	isPretext >> cTo >> pi;
	is >> ws;
	getline(is, sComment);
}

void CMsgOsComment::Handle() {
	pgs->HandleOsComment(this);
}

///////////////////////////////////
// CMsgOsEnd
///////////////////////////////////

// /os: end .2.0 ( pamphlet vs. ant ) +46.00
void CMsgOsEnd::In(istream& is) {
	char c;
	string s;

	is >> idg >> c;
	_ASSERT(c=='(');
	is >> sPlayers[0] >> s >> sPlayers[1] >> c;
	_ASSERT(s=="vs.");
	_ASSERT(c==')');
	is >> result;
}

void CMsgOsEnd::Handle() {
	pgs->HandleOsEnd(this);
}

///////////////////////////////////
// CMsgOsErr
///////////////////////////////////

// /os: error .25 corrupt move
// /os: watch + ERR not found: .3

void CMsgOsErr::In(istream& is) {
	string sText;
	getline(is, sText);
	if (sText=="your request doesn't fit the opponent's formula.")
		err=kErrRequestDoesntFitFormula;
	else if (sText=="board type")
		err=kErrIllegalBoardType;
	else if (sText=="rank <type> [login]")
		err=kErrBadRankMessage;
	else if (sText.find("corrupt move")!=sText.npos)
		err=kErrCorruptMove;
	else if (sText=="Your open value is too low for new matches.")
		err=kErrOpenTooLow;
	else if (sText=="rated variable mismatch.")
		err=kErrRatedMismatch;
	else if (sText=="Rated/unrated mismatch")
		err=kErrRatedMismatch;
	else if (sText=="you are not registered.")
		err=kErrUnregistered;
	else if (sText.find("illegal move")!=-1)
		err=kErrIllegalMove;
	else if (sText.find("not found")!=-1)
		err=kErrUserNotFound;
	else if (sText=="Player is not accepting new matches.")
		err=kErrNotAcceptingMatches;
	else if (sText=="only one color preference allowed")
		err=kErrOneColorPreference;
	else if (sText.find("watch + ERR not found:")==0 ||
			sText.find("watch - ERR not found:")==0)
		err=kErrWatchNotFound;
	else
		err=kErrUnknown;
}

void CMsgOsErr::Handle() {
	pgs->HandleOsErr(this);
}

///////////////////////////////////
// CMsgOsFatalTimeout
///////////////////////////////////

void CMsgOsFatalTimeout::In(istream& is) {
	is >> idg >> sLogin >> ws;
}

void CMsgOsFatalTimeout::Handle() {
	pgs->HandleOsFatalTimeout(this);
}

///////////////////////////////////
// CMsgOsFinger
///////////////////////////////////
void CMsgOsFinger::In(istream& is) {
	string sLine, sKey, sValue;
	COsFingerRating fr;

	is >> sLogin >> ws;

	// first section, key : value
	while (getline(is, sLine)) {
		if (!is)
			break;
		if (sLine.find(':')==string::npos)
			break;

		istrstream isLine(sLine.c_str());

		// get key and strip terminal spaces
		getline(isLine, sKey, ':');
		sKey.resize(1+sKey.find_last_not_of(' '));

		// get value. No value means this is the separator row
		//	between the first section and the second section
		isLine >> ws;
		getline(isLine, sValue);

		// insert (key, value) pair
		_ASSERT(keyToValue.find(sKey)==keyToValue.end());
		keyToValue[sKey]=sValue;
	}

	// second section, rating info
	while (getline(is, sLine)) {
		istrstream isLine(sLine.c_str());
		isLine >> fr;
		frs.push_back(fr);
	}
}

void CMsgOsFinger::Handle() {
	pgs->HandleOsFinger(this);
}

///////////////////////////////////
// CMsgOsJoin
///////////////////////////////////

void CMsgOsJoin::In(istream& is) {
	is >> idg >> ws;
	is >> game;
}

void CMsgOsJoin::Handle() {
	pgs->HandleOsJoin(this);
}

///////////////////////////////////
// CMsgOsLook
///////////////////////////////////

void CMsgOsLook::In(istream& is) {
	is >> nGames;
	_ASSERT(nGames==1 || nGames==2);

	games.reserve(nGames);
	COsGame game;
	int i;
	for (i=0; i<nGames; i++) {
		is >> game;
		games.push_back(game);
	}
}

void CMsgOsLook::Handle() {
	pgs->HandleOsLook(this);
}

///////////////////////////////////
// CMsgOsHistory
///////////////////////////////////
void CMsgOsHistory::In(istream& is) {
	is >> n >> sLogin;
	COsHistoryItem hi;
	while (is >> hi)
		his.push_back(hi);
	_ASSERT(his.size()==n);
}

void CMsgOsHistory::Handle() {
	pgs->HandleOsHistory(this);
}

///////////////////////////////////
// CMsgOsMatch
///////////////////////////////////

void CMsgOsMatch::In(istream& is) {
	char c;

	is >> n1 >> c >> n2;
	_ASSERT(c=='/');

	COsMatch match;
	while (match.In(is))
		matches.push_back(match);

	_ASSERT(matches.size()==n2);
}

void CMsgOsMatch::Handle() {
	pgs->HandleOsMatch(this);
}

///////////////////////////////////
// CMsgOsMatchDelta
///////////////////////////////////

CMsgOsMatchDelta::CMsgOsMatchDelta(bool afPlus) {
	fPlus=afPlus;
}

// /os: - match .2 1833 nasai 2342 booklet 8 R nasai left
void CMsgOsMatchDelta::In(istream& is) {
	match.InDelta(is);
	if (!fPlus)
		is >> result;
}

void CMsgOsMatchDelta::Handle() {
	pgs->HandleOsMatchDelta(this);
}


///////////////////////////////////
// CMsgOsRank
///////////////////////////////////
void CMsgOsRank::In(istream& is) {
	string sInactive, sAScore, sWin, sDraw, sLoss;
	COsRankData rd;

	is >> rating >> sInactive >> sAScore >> sWin >> sDraw >> sLoss >> n >> ws;
	while (is >> rd) {
		rds.push_back(rd);
	}
}

void CMsgOsRank::Handle() {
	pgs->HandleOsRank(this);
}

///////////////////////////////////
// CMsgOsRatingUpdate
///////////////////////////////////
void CMsgOsRatingUpdate::In(istream& is) {
	string s;

	is >> idm;
	is >> sPlayers[0] >> rOlds[0] >> dDeltas[0] >> s >> rNews[0];
	_ASSERT(s=="->");
	is >> sPlayers[1] >> rOlds[1] >> dDeltas[1] >> s >> rNews[1];
	_ASSERT(s=="->");
}

void CMsgOsRatingUpdate::Handle() {
	pgs->HandleOsRatingUpdate(this);
}

///////////////////////////////////
// CMsgOsRequestDelta
///////////////////////////////////

CMsgOsRequestDelta::CMsgOsRequestDelta(bool afPlus) {
	fPlus=afPlus;
}

bool CMsgOsRequestDelta::IAmChallenged() const {
	return request.pis[1].sName==pgs->GetLogin();
}

bool CMsgOsRequestDelta::IAmChallenging() const {
	return request.pis[0].sName==pgs->GetLogin();
}

bool CMsgOsRequestDelta::RequireBoardSize(int n) const {
	bool fOK=request.mt.bt.n==n;
	if (!fOK)
		(*pgs) << "t " << request.pis[0].sName << " I only play on an " << n << "x" << n << " board\n";
	return fOK;
}

bool CMsgOsRequestDelta::RequireKomi(bool fKomi) const {
	bool fOK=request.mt.fKomi==fKomi;
	if (!fOK)
		(*pgs) << "t " << request.pis[0].sName << " I " << (fKomi?"only":"don't") << " play komi games\n";
	return fOK;
}

bool CMsgOsRequestDelta::RequireAnti(bool fAnti) const {
	bool fOK=request.mt.fAnti==fAnti;
	if (!fOK)
		(*pgs) << "t " << request.pis[0].sName << " I " << (fAnti?"only":"don't") << " play anti games\n";
	return fOK;
}

bool CMsgOsRequestDelta::RequireRand(bool fRand) const {
	bool fOK=request.mt.fRand==fRand;
	if (!fOK)
		(*pgs) << "t " << request.pis[0].sName << " I " << (fRand?"only":"don't") << " play rand games\n";
	return fOK;
}

bool CMsgOsRequestDelta::RequireRated(bool fRated) const {
	bool fOK=request.fRated==fRated;
	if (!fOK)
		(*pgs) << "t " << request.pis[0].sName << " I " << (fRated?"only":"don't") << " play rated games\n";
	return fOK;
}

bool CMsgOsRequestDelta::RequireSynch(bool fSynch) const {
	bool fOK=request.mt.fSynch==fSynch;
	if (!fOK)
		(*pgs) << "t " << request.pis[0].sName << " I " << (fSynch?"only":"don't") << " play synch games\n";
	return fOK;
}

bool CMsgOsRequestDelta::RequireEqualClocks() const {
	bool fOK=request.cks[0]==request.cks[1];
	if (!fOK)
		(*pgs) << "t " << request.pis[0].sName << " I only play games with equal clocks\n";
	return fOK;
}

bool CMsgOsRequestDelta::RequireMaxOpponentClock(const COsClock& ck) const {
	bool fOK=request.cks[0]<=ck;
	if (!fOK)
		(*pgs) << "t " << request.pis[0].sName << " Your clock can be at most " << ck << "\n";
	return fOK;
}

bool CMsgOsRequestDelta::RequireMinMyClock(const COsClock& ck) const {
	bool fOK=request.cks[1]>=ck;
	if (!fOK)
		(*pgs) << "t " << request.pis[0].sName << " My clock can be at most " << ck << "\n";
	return fOK;
}


void CMsgOsRequestDelta::In(istream& is) {
	is >> idr >> request;
}

void CMsgOsRequestDelta::Handle() {
	pgs->HandleOsRequestDelta(this);
}

void CMsgOsStored::In(istream& is) {
	COsStoredMatch sm;
	is >> nStored >> sLogin;
	while (is >> sm) {
		sms.push_back(sm);
	}
	_ASSERT(sms.size()==nStored);
}

void CMsgOsStored::Handle() {
	pgs->HandleOsStored(this);
}

///////////////////////////////////
// CMsgOsTimeout
///////////////////////////////////

void CMsgOsTimeout::In(istream& is) {
	is >> idg >> sLogin >> ws;
}

void CMsgOsTimeout::Handle() {
	pgs->HandleOsTimeout(this);
}


void CMsgOsTop::In(istream& is) {
	string sInactive, sAScore, sWin, sDraw, sLoss;
	COsRankData rd;

	is >> rating >> sInactive >> sAScore >> sWin >> sDraw >> sLoss >> n >> ws;
	while (is >> rd) {
		rds.push_back(rd);
	}
}

void CMsgOsTop::Handle() {
	pgs->HandleOsTop(this);
}

// /os: trust-violation .56 pamphlet (*) delta= 4 + 0.7 secs
void CMsgOsTrustViolation::In(istream& is) {
	char c1, c2;
	string sDelta, sSecs;

	is >> idg >> sLogin >> c1 >> cColor >> c2;
	_ASSERT(c1=='(');
	_ASSERT(cColor==COsBoard::BLACK || cColor==COsBoard::WHITE || cColor==COsBoard::UNKNOWN);
	_ASSERT(c2==')');

	is >> sDelta >> delta1 >> c1 >> delta2 >> sSecs;
	_ASSERT(sDelta=="delta=");
	_ASSERT(sSecs=="secs");
}

void CMsgOsTrustViolation::Handle() {
	pgs->HandleOsTrustViolation(this);
}

///////////////////////////////////
// CMsgOsUndoRequest
///////////////////////////////////

void CMsgOsUndoRequest::In(istream& is) {
	string sDummy;

	is >> idg >> sLogin >> ws;
	getline(is, sDummy);

	_ASSERT(sDummy=="is asking");
}

void CMsgOsUndoRequest::Handle() {
	pgs->HandleOsUndoRequest(this);
}

///////////////////////////////////
// CMsgOsUnknown
///////////////////////////////////

CMsgOsUnknown::CMsgOsUnknown(const string& asMsgType) {
	sMsgType=asMsgType;
}

void CMsgOsUnknown::In(istream& is) {
	getline(is, sText, (char)EOF);
}

void CMsgOsUnknown::Handle() {
	pgs->HandleOsUnknown(this);
}

///////////////////////////////////
// CMsgOsUpdate
///////////////////////////////////

void CMsgOsUpdate::In(istream& is) {
	is >> idg >> mli;
}

void CMsgOsUpdate::Handle() {
	pgs->HandleOsUpdate(this);
	//MakeMoveIfNeeded(PGame(idg), *pgs, idg);
}

///////////////////////////////////
// CMsgOsWatch
///////////////////////////////////

// /os: watch 1 : .41(1)
void CMsgOsWatch::In(istream& is) {
	char c1, c2;
	string idm;
	int nWatchers;

	is >> nMatches >> c1;
	_ASSERT(c1==':');

	while (is >> idm >> c1 >> nWatchers >> c2) {
		_ASSERT(c1=='(');
		_ASSERT(c2==')');
		_ASSERT(idToNWatchers.find(idm)==idToNWatchers.end());
		idToNWatchers[idm]=nWatchers;
	}
}

void CMsgOsWatch::Handle() {
	pgs->HandleOsWatch(this);
}

///////////////////////////////////
// CMsgOsWatchChange
///////////////////////////////////

CMsgOsWatchDelta::CMsgOsWatchDelta(bool afPlus, const string& asLogin) {
	fPlus=afPlus;
	sLogin=asLogin;
}

void CMsgOsWatchDelta::In(istream& is) {
	is >> idm;
}

void CMsgOsWatchDelta::Handle() {
	pgs->HandleOsWatchDelta(this);
}

///////////////////////////////////
// CMsgOsWho
///////////////////////////////////

void CMsgOsWho::Handle() {
	pgs->HandleOsWho(this);
}


void CMsgOsWho::In(istream& is) {
	string s;
	is >> n;
	getline(is, s);

	COsWhoItem wi;
	while (is >> wi) {
		wis.push_back(wi);
	}
}
