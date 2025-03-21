// Copyright 2001 Chris Welty
//	All Rights Reserved

#ifndef ODK_GGSMESSAGE_H
#define ODK_GGSMESSAGE_H

#include "types.h"

#include <map>
#include "GGSObjects.h"

// base message class

class ggsstream;

class CMsg {
public:
	// handle the message
	virtual void Handle()=0;
	virtual void In(istream& is);

	ggsstream* pgs;
	string sFrom;
	string sRawText;
};

class CMsgGGSAlias: public CMsg {
public:
	virtual void Handle();
	void In(istream& is);

	int nAlias1, nAlias2;
	vector<CGGSAlias> valiases;
};

class CMsgGGSErr: public CMsg {
public:
	virtual void Handle();
	void In(istream& is);

	enum { kErrUnknown=0x8400, kErrCommandNotRecognized } err;
};

class CMsgGGSFinger: public CMsg {
public:
	virtual void Handle();
	void In(istream& is);

	map<string, string> keyToValue;
};

class CMsgGGSHelp: public CMsg {
public:
	virtual void Handle();
	void In(istream& is);

	string sText;
};

class CMsgGGSUnknown: public CMsg {
public:
	virtual void Handle();
	void In(istream& is);

	string sMsgType;
	string sText;
};

class CMsgGGSUserDelta: public CMsg {
public:
	CMsgGGSUserDelta(bool fPlus);

	virtual void Handle();
	void In(istream& is);

	bool fPlus;
	string sLogin;
};

class CMsgGGSTell : public CMsg {
public:
	virtual void Handle();
	void In(istream& is);

	string sText;
};

class CMsgGGSWho : public CMsg {
public:
	virtual void Handle();
	void In(istream& is);

	int nUsers;
	vector<CGGSWhoUser> wus;
};

// Fake messages

class CMsgGGSDisconnect : public CMsg {
public:
	virtual void Handle();
};

class CMsgGGSLogin : public CMsg {
public:
	virtual void Handle();
};

#endif // ODK_GGSMESSAGE_H