/*
 *   IRC - Internet Relay Chat, joinpartsno.c
 *   (C) 2003 Dominick Meglio
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 1, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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

ModuleHeader MOD_HEADER(joinpartsno)
  = {
	"joinpartsno",
	"1.0",
	"join/part snomask", 
	"3.2-b8-1",
	NULL 
    };

long SNO_JOINPART = 0L;

int h_jpsno_join(aClient *cptr, aClient *sptr, aChannel *chptr, char *parv[]);
int h_jpsno_part(aClient *cptr, aClient *sptr, aChannel *chptr, char *comment);
int h_jpsno_oper(aClient *sptr, int oper);

DLLFUNC int MOD_INIT(joinpartsno)(ModuleInfo *modinfo)
{
	SnomaskAdd(modinfo->handle, 'J', umode_allow_opers, &SNO_JOINPART);
	HookAddEx(modinfo->handle, HOOKTYPE_LOCAL_JOIN, h_jpsno_join);
	HookAddEx(modinfo->handle, HOOKTYPE_LOCAL_PART, h_jpsno_part);
	HookAddEx(modinfo->handle, HOOKTYPE_LOCAL_OPER, h_jpsno_oper);
	return MOD_SUCCESS;
}

DLLFUNC int MOD_LOAD(joinpartsno)(int module_load)
{
	return MOD_SUCCESS;
}

DLLFUNC int MOD_UNLOAD(joinpartsno)(int module_unload)
{
	return MOD_SUCCESS;
}

int h_jpsno_join(aClient *cptr, aClient *sptr, aChannel *chptr, char *parv[])
{
	sendto_snomask(SNO_JOINPART, "%s (%s@%s) joined %s", sptr->name, 
		sptr->user->username, sptr->user->realhost, chptr->chname);
	return 0;
}

int h_jpsno_part(aClient *cptr, aClient *sptr, aChannel *chptr, char *comment)
{
	sendto_snomask(SNO_JOINPART, "%s (%s@%s) left %s", sptr->name,
		sptr->user->username, sptr->user->realhost, chptr->chname);
	return 0;
}

int h_jpsno_oper(aClient *sptr, int oper)
{
	if (!oper)
		sptr->user->snomask &= ~SNO_JOINPART;
	return 0;
}
