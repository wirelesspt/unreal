/*
 * ULines-as-privmsg module.
 * (C) Copyright 2004 Bram Matthys (Syzop).
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * ============================================================================
 * 
 * See 'README' for more info (what it is, how to use/install, etc).
 *
 * This module uses a semi-"illegal" trick to convert NOTICEs into PRIVMSGs,
 * however since we area dealing with U-lines (which go trough anything like
 * spamfilter, +G, etc) this shouldn't be a problem.
 * However, module coders looking at this code are highly discouraged to use
 * this technique since it's very likely you'll break something.
 *
 * ============================================================================
 *
 * $Id: m_umsg.c,v 1.1.1.1 2004/02/27 00:57:02 syzop Exp $
 */
   
#include "config.h"
#include "struct.h"
#include "common.h"
#include "sys.h"
#include "numeric.h"
#include "msg.h"
#include "proto.h"
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
#ifdef STRIPBADWORDS
#include "badwords.h"
#endif
#ifdef _WIN32
#include "version.h"
#endif

#define UMSG_VERSION "v0.2"

ModuleHeader MOD_HEADER(m_umsg)
  = {
	"m_umsg",
	UMSG_VERSION,
	"U-Lines-as-PRIVMSG module",
	"3.2-b8-1",
	NULL 
    };

static Cmdoverride *privmsg_override;
static Hook *HookNotice = NULL;
DLLFUNC char *umsg_notice(aClient *, aClient *, aClient *, char *, int);

DLLFUNC int MOD_INIT(m_umsg)(ModuleInfo *modinfo)
{
	HookNotice = HookAddPCharEx(modinfo->handle, HOOKTYPE_USERMSG, umsg_notice);
	return MOD_SUCCESS;
}

DLLFUNC int MOD_LOAD(m_umsg)(int module_load)
{
	return MOD_SUCCESS;
}

DLLFUNC int MOD_UNLOAD(m_umsg)(int module_unload)
{
	return MOD_SUCCESS;
}

DLLFUNC char *umsg_notice(aClient *cptr, aClient *sptr, aClient *acptr, char *text, int notice)
{
Hook *tmphook;
int found = 0;

	if (!notice || !IsULine(sptr))
		return text;

	/* We are going to deal with it ourselves! */
	
	for (tmphook = Hooks[HOOKTYPE_USERMSG]; tmphook; tmphook = tmphook->next)
	{
		if (tmphook->func.pcharfunc == umsg_notice)
		{
			found = 1;
			continue;
		}
		if (!found)
			continue;
		text = (*(tmphook->func.pcharfunc))(cptr, sptr, acptr, text, notice);
		if (!text)
			return NULL;
	}

	sendto_message_one(acptr, sptr, sptr->name, MSG_PRIVATE, acptr->name, text);

	return NULL; /* always! we dealt with it! */
}
