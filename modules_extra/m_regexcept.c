/*
 * Reg except: extban ~E. (C) Copyright 2004 Bram Matthys.
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
 * See README for instructions
 *
 * $Id: m_regexcept.c,v 1.2 2004/04/22 19:45:34 syzop Exp $
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

ModuleHeader MOD_HEADER(m_regexcept)
  = {
	"m_regexcept",
	"v0.1",
	"ExtBan ~E",
	"3.2-b8-1",
	NULL 
    };

Extban *extbanE;

char *extban_modeE_conv_param(char *para)
{
static char retbuf[64];

	strlcpy(retbuf, para, sizeof(retbuf));
	if (do_nick_name(retbuf+3) == 0)
		return NULL;
	return retbuf;
};

int extban_modeE_is_banned(aClient *sptr, aChannel *chptr, char *xban, int type)
{
char *ban = xban + 3;

	if (IsRegNick(sptr) && !strcasecmp(ban, sptr->name))
		return 1;
	return 0;
}

DLLFUNC int MOD_INIT(m_regexcept)(ModuleInfo *modinfo)
{
ExtbanInfo req;

	memset(&req, 0, sizeof(ExtbanInfo));
	req.flag = 'E';
	req.conv_param = extban_modeE_conv_param;
	req.is_banned = extban_modeE_is_banned;
	extbanE = ExtbanAdd(modinfo->handle, req);
	if (!extbanE)
	{
		config_error("m_regexcept module: adding extban ~E failed! module NOT loaded");
		return MOD_FAILED;
	}

	ModuleSetOptions(modinfo->handle, MOD_OPT_PERM);

	return MOD_SUCCESS;
}

DLLFUNC int MOD_LOAD(m_regexcept)(int module_load)
{
	return MOD_SUCCESS;
}

DLLFUNC int MOD_UNLOAD(m_regexcept)(int module_unload)
{
	/* This is unexpected as of RC2, but might work in the future... */
	ExtbanDel(extbanE);
	return MOD_SUCCESS;
}

