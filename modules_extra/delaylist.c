/*
 * ==================================================================
 * Module:			delaylist.c
 * Author:			w00t <surreal.w00t@gmail.com>
 * Version:			1.0.0
 * Written For:		Stskeeps
 * Licence:			GPL
 * Description:		Delays list for newly connected clients.
 * ==================================================================
 */


/* ChangeLog:
 *	12/11/2005 - 1.0.0
 *		-Initial version
 */

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

static Cmdoverride *delaylist_override = NULL;
static ModuleInfo *delaylist_modinfo = NULL;

DLLFUNC int m_delaylist(Cmdoverride *anoverride, aClient *cptr, aClient *sptr, int parc, char *parv[]);

#define MSG_LIST	"LIST"


ModuleHeader MOD_HEADER(delaylist)
  = {
	"delaylist",	/* Name */
	"1.0.0", /* Ver */
	"Delay /list for newly connected users by w00t <surreal.w00t@gmail.com>", /* Desc. */
	"3.2.3",
	NULL 
    };

DLLFUNC int MOD_INIT(delaylist)(ModuleInfo *modinfo)
{
	delaylist_modinfo = modinfo;
	return MOD_SUCCESS;
}

DLLFUNC int MOD_LOAD(delaylist)(int module_load)
{
	delaylist_override = CmdoverrideAdd(delaylist_modinfo->handle, MSG_LIST, m_delaylist);
	if (!delaylist_override)
	{
		sendto_realops("delaylist: Failed to allocate override: %s", ModuleGetErrorStr(delaylist_modinfo->handle));
		return MOD_FAILED;
	}
	return MOD_SUCCESS;
}

DLLFUNC int MOD_UNLOAD(delaylist)(int module_unload)
{
	CmdoverrideDel(delaylist_override);

	return MOD_SUCCESS;
}

DLLFUNC int m_delaylist(Cmdoverride *anoverride, aClient *cptr, aClient *sptr, int parc, char *parv[])
{
	if (!IsAnOper(sptr))
	{
		if (sptr->firsttime + 10 > TStime())
		{
			/* DENIED! */
			sendto_one(sptr, ":%s %s %s :*** You have not been connected long enough "
			                 "to use /list. You must wait 30 seconds after connecting",
				me.name, IsWebTV(sptr) ? "PRIVMSG" : "NOTICE", sptr->name);
				
			/* if we don't do this, some clients have a hissy :p */
			sendto_one(sptr, rpl_str(RPL_LISTSTART), me.name, sptr->name);
			sendto_one(sptr, rpl_str(RPL_LISTEND), me.name, sptr->name);
			return 0;
		}
	}

	/* aww, no fun. they've been connected a while :( - let it through */
	return CallCmdoverride(delaylist_override, cptr, sptr, parc, parv);
}
