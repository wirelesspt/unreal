#include "config.h"
#include "struct.h"
#include "common.h"
#include "sys.h"
#include "numeric.h"
#include "msg.h"
#include "channel.h"
#include <time.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <io.h>
#endif
#include <fcntl.h>
#include "h.h"
#include "proto.h"
#ifdef STRIPBADWORDS
#include "badwords.h"
#endif
#ifdef _WIN32
#include "version.h"
#endif

static int m_gqline(aClient *cptr, aClient *sptr, int parc, char* parv[]);
#define MSG_GQLINE "GQLINE"
#define TOK_GQLINE NULL		/* Token? We don't need no stinking token! */
Command* CmdGQLine = NULL;
static int m_qline(aClient *cptr, aClient *sptr, int parc, char* parv[]);
#define MSG_QLINE "QLINE"
#define TOK_QLINE NULL		/* Token? We don't need no stinking token! */
Command* CmdQLine = NULL;

static ModuleInfo* myinfo = NULL;

ModuleHeader MOD_HEADER(m_gqline)
	= {
		"gqline",
		"$Id: m_gqline.c,v 1.0.0.0 2005/05/13 10:17:00 aquanight Exp $",
		"command /gqline",
		"3.2-b8-1",
		NULL
	};

DLLFUNC int MOD_INIT(m_gqline)(ModuleInfo *modinfo)
{
	myinfo = modinfo;
	CmdGQLine = CommandAdd(myinfo->handle, MSG_GQLINE, TOK_GQLINE, m_gqline, 3, 0);
	if (!CmdGQLine) {
		config_error("m_gqline: Failed to add command /GQLINE : %s", ModuleGetErrorStr(myinfo->handle));
		return MOD_FAILED;
	}
	CmdQLine = CommandAdd(myinfo->handle, MSG_QLINE, TOK_QLINE, m_qline, 3, 0);
	if (!CmdQLine) {
		CommandDel(CmdGQLine);
		config_error("m_gqline: Failed to add command /QLINE : %s", ModuleGetErrorStr(myinfo->handle));
		return MOD_FAILED;
	}
	return MOD_SUCCESS;
}

DLLFUNC int MOD_LOAD(m_gqline)(int module_load)
{
	return MOD_SUCCESS;
}

DLLFUNC int MOD_UNLOAD(m_gqline)(int module_unload)
{
	CommandDel(CmdQLine);
	CommandDel(CmdGQLine);
	return MOD_SUCCESS;
}

/*	m_gqline - Global Q:Line
**	parv[0] = sender
**	parv[1] = nickname mask
**	parv[2] = Optional expiration time
**	parv[3] = reason
*/
static int m_gqline(aClient *cptr, aClient *sptr, int parc, char* parv[])
{
	TS secs;
	int whattodo = 0;
	int i;
	aClient *acptr = NULL;
	char *mask = NULL;
	char mo[1024], mo2[1024];
	char *p;
	char *tkllayer[9] = {
		me.name,	/* 0 = Server Name */
		NULL,		/* 1 = + / - */
		"Q",		/* 2 = Q - Global Q:Line */
		"*",		/* 3 = * = normal qline, H = hold */
		NULL,		/* 4 = Nickname mask */
		NULL,		/* 5 = setby */
		"0",		/* 6 = expire ts */
		NULL,		/* 7 = set ts */
		"no reason"	/* 8 = reason */
	};
	struct tm *t = NULL;
	if (IsServer(sptr)) return 0;
	if (!OPCanTKL(sptr) || !IsOper(sptr))
	{
		sendto_one(sptr, err_str(ERR_NOPRIVILEGES), me.name, sptr->name);
		return 0;
	}
	if (parc == 1)
	{
		tkl_stats(sptr, TKL_NICK|TKL_GLOBAL, NULL);
		sendto_one(sptr, rpl_str(RPL_ENDOFSTATS), me.name, sptr->name, 'Q');
		return 0;
	}
	mask = parv[1];
	if (*mask == '-') {
		whattodo = 1;
		mask++;
	}
	else if (*mask == '+') {
		whattodo = 0;
		mask++;
	}
	if (!whattodo) {
		char c;
		i = 0;
		for (p = mask; *p; p++) if (*p != '*' && *p != '?') i++;
		if (i < 4) {
			sendto_one(sptr, ":%s NOTICE %s :*** [error] Too broad mask", me.name, sptr->name);
			return 0;
		}
	}
	tkl_check_expire(NULL);
	secs = 0;
	if (whattodo == 0 && (parc > 3))
	{
		secs = atime(parv[2]);
		if (secs < 0) {
			sendto_one(sptr, ":%s NOTICE %s :*** [error] Specified time out of range", me.name, sptr->name);
			return 0;
		}
	}
	tkllayer[1] = whattodo == 0 ? "+" : "-";
	tkllayer[4] = mask;
	tkllayer[5] = make_nick_user_host(sptr->name, sptr->user->username, GetHost(sptr));
	if (whattodo == 0)
	{
		if (secs == 0)
		{
			if (DEFAULT_BANTIME && (parc <= 3))
				ircsprintf(mo, "%li", DEFAULT_BANTIME + TStime());
			else
				ircsprintf(mo, "%li", secs);
		}
		else
			ircsprintf(mo, "%li", secs + TStime());
		ircsprintf(mo2, "%li", TStime());
		tkllayer[6] = mo;
		tkllayer[7] = mo2;
		if (parc > 3) {
			tkllayer[8] = parv[3];
		} else if (parc > 2) {
			tkllayer[8] = parv[2];
		}
		i = atol(mo);
		t = gmtime((TS*)&i);
		if (!t)
		{
			sendto_one(sptr, ":%s NOTICE %s :*** [error] Specified time is out of range", me.name, sptr->name);
			return 0;
		}
		m_tkl(&me, &me, 9, tkllayer);
	}
	else {
		m_tkl(&me, &me, 6, tkllayer);
	}
	return 0;
}

/*	m_qline - Local Q:Line
**	parv[0] = sender
**	parv[1] = nickname mask
**	parv[2] = Optional expiration time
**	parv[3] = reason
*/
static int m_qline(aClient *cptr, aClient *sptr, int parc, char* parv[])
{
	TS secs;
	int whattodo = 0;
	int i;
	aClient *acptr = NULL;
	char *mask = NULL;
	char mo[1024], mo2[1024];
	char *p;
	char *tkllayer[9] = {
		me.name,	/* 0 = Server Name */
		NULL,		/* 1 = + / - */
		"q",		/* 2 = Q - Local Q:Line */
		"*",		/* 3 = * = normal qline, H = hold */
		NULL,		/* 4 = Nickname mask */
		NULL,		/* 5 = setby */
		"0",		/* 6 = expire ts */
		NULL,		/* 7 = set ts */
		"no reason"	/* 8 = reason */
	};
	struct tm *t = NULL;
	if (IsServer(sptr)) return 0;
	if (!OPCanTKL(sptr) || !IsOper(sptr))
	{
		sendto_one(sptr, err_str(ERR_NOPRIVILEGES), me.name, sptr->name);
		return 0;
	}
	if (parc == 1)
	{
		tkl_stats(sptr, TKL_NICK, NULL);
		sendto_one(sptr, rpl_str(RPL_ENDOFSTATS), me.name, sptr->name, 'q');
		return 0;
	}
	mask = parv[1];
	if (*mask == '-') {
		whattodo = 1;
		mask++;
	}
	else if (*mask == '+') {
		whattodo = 0;
		mask++;
	}
	if (!whattodo) {
		char c;
		i = 0;
		for (p = mask; *p; p++) if (*p != '*' && *p != '?') i++;
		if (i < 4) {
			sendto_one(sptr, ":%s NOTICE %s :*** [error] Too broad mask", me.name, sptr->name);
			return 0;
		}
	}
	tkl_check_expire(NULL);
	secs = 0;
	if (whattodo == 0 && (parc > 3))
	{
		secs = atime(parv[2]);
		if (secs < 0) {
			sendto_one(sptr, ":%s NOTICE %s :*** [error] Specified time out of range", me.name, sptr->name);
			return 0;
		}
	}
	tkllayer[1] = whattodo == 0 ? "+" : "-";
	tkllayer[4] = mask;
	tkllayer[5] = make_nick_user_host(sptr->name, sptr->user->username, GetHost(sptr));
	if (whattodo == 0)
	{
		if (secs == 0)
		{
			if (DEFAULT_BANTIME && (parc <= 3))
				ircsprintf(mo, "%li", DEFAULT_BANTIME + TStime());
			else
				ircsprintf(mo, "%li", secs);
		}
		else
			ircsprintf(mo, "%li", secs + TStime());
		ircsprintf(mo2, "%li", TStime());
		tkllayer[6] = mo;
		tkllayer[7] = mo2;
		if (parc > 3) {
			tkllayer[8] = parv[3];
		} else if (parc > 2) {
			tkllayer[8] = parv[2];
		}
		i = atol(mo);
		t = gmtime((TS*)&i);
		if (!t)
		{
			sendto_one(sptr, ":%s NOTICE %s :*** [error] Specified time is out of range", me.name, sptr->name);
			return 0;
		}
		m_tkl(&me, &me, 9, tkllayer);
	}
	else {
		m_tkl(&me, &me, 6, tkllayer);
	}
	return 0;
}
