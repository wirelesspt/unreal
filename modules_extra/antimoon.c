/*
 *   IRC - Internet Relay Chat, antimoon.c
 *   (C) Copyright 2004, Bram Matthys (Syzop)
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

/* Configurable items: */
#define ANTIMOON_BANACT		BAN_ACT_GZLINE
#define ANTIMOON_REASON		"Infected with MyMoon virus"
#define ANTIMOON_BANTIME	86400
/* End of configurable items */

ModuleHeader MOD_HEADER(antimoon)
  = {
	"antimoon",
	"v0.2",
	"MyMoon trojan detector by Syzop",
	"3.2-b8-1",
	NULL 
    };

static char *nameslist[] = {
	"adams",
	"allen",
	"allison",
	"alvarez",
	"andrson",
	"andrews",
	"armsong",
	"arnold",
	"avila",
	"bailey",
	"baker",
	"barnes",
	"bennett",
	"bishop",
	"boyd",
	"bradley",
	"brooks",
	"brown",
	"bryan",
	"burke",
	"burton",
	"butler",
	"campbell",
	"carlson",
	"carr",
	"carter",
	"chase",
	"chen",
	"christ",
	"clark",
	"collins",
	"comer",
	"cook",
	"cooper",
	"cox",
	"crawford",
	"cunnin",
	"davis",
	"day",
	"dean",
	"dickins",
	"edwards",
	"elliott",
	"ellis",
	"evans",
	"fischer",
	"fisher",
	"fong",
	"ford",
	"freeman",
	"frost",
	"garcia",
	"gardner",
	"gomes",
	"gomez",
	"gonzales",
	"graham",
	"green",
	"griffin",
	"mymoon",
	"Fight",
	"Fool",
	"Makar",
	"Foremos",
	"Frien",
	"Gargar",
	"Gere",
	"Grai",
	"Grant",
	"Grenic",
	"Guneu",
	"Gygae",
	"Gyrti",
	"Gyrto",
	"Haem",
	"Herm",
	"Her",
	"Hono",
	"Honour",
	"Hours",
	"Hyads",
	"Hyperik",
	"Hyperk",
	"Hypse",
	"Hyppyle",
	"Hyria",
	"Hyrmine",
	"Hyrtacus",
	"Hyrtius",
	"Iaera",
	"Ialmenus",
	"Iamenus",
	"Ianassa",
	"Ianeira",
	"Iapetus",
	"Iardanus",
	"Iasus",
	"Icarian",
	"Idaeus",
	"Ides",
	"Idomens",
	"Ielysus",
	"Ilioneus",
	"Ilithuia",
	"Ilithui",
	"Ilius",
	"Ilus",
	"Imbras",
	"Imbrius",
	"Imbros",
	"Imbrus",
	"Insolen",
	"Laodame",
	"Leonte",
	"Lesbia",
	"Lesbos",
	"Lethus",
	"Leto",
	"Leucus",
	"Lycian",
	"Lycians",
	"Lycomed",
	"Lycon",
	"Lycophon",
	"Lycoph",
	"Lyctus",
	"Lycur",
	"Maera",
	"Magnetes",
	"Mantinea",
	"Maris",
	"Marpsa",
	"Mases",
	"Mastor",
	"Melappus",
	"Manthus",
	"Menheus",
	"Menehius",
	"Menoius",
	"Menon",
	"Mentes",
	"Mydon",
	"Mygdon",
	"Mynes",
	"Myrine",
	"Myrm",
	NULL
};

DLLFUNC int antimoon_useroverride(Cmdoverride *ovr, aClient *cptr, aClient *sptr, int parc, char *parv[]);

ModuleInfo AntiMoon;

DLLFUNC int MOD_INIT(antimoon)(ModuleInfo *modinfo)
{
	bcopy(modinfo, &AntiMoon,modinfo->size);
	return MOD_SUCCESS;
}

DLLFUNC int MOD_LOAD(antimoon)(int module_load)
{
	CmdoverrideAdd(AntiMoon.handle, "USER", antimoon_useroverride);
	return MOD_SUCCESS;
}

DLLFUNC int MOD_UNLOAD(antimoon)(int module_unload)
{
	return MOD_SUCCESS;
}

static int is_in_nameslist(char *name)
{
char **s;
	for (s=nameslist; *s; *s++)
	{
		if (!strcmp(name, *s))
			return 1;
	}
	return 0;
}

/* USER <name> 0 * :<name><digits> */
DLLFUNC int antimoon_useroverride(Cmdoverride *ovr, aClient *cptr, aClient *sptr, int parc, char *parv[])
{
	if (MyConnect(sptr) && !IsServer(sptr) && (parc > 4) &&
	    !strcmp(parv[2], "0") && !strcmp(parv[3], "*") && (sptr->name[0] == '\0'))
	{
		char **s, match=0;
		/* Check if <name> is present in 1st parameter... */
		if (is_in_nameslist(parv[1]))
		{
			/* It was, now check for <name><digits> in realname */
			char buf[32], *p;
			strlcpy(buf, parv[4], sizeof(buf));
			for (p=buf; *p; p++)
				if (isdigit(*p))
				{
					match++;
					*p = '\0';
					break;
				}
			if (match && is_in_nameslist(buf)) /* stuff before digits also matched a name? */
			{
				return place_host_ban(sptr, ANTIMOON_BANACT, ANTIMOON_REASON, ANTIMOON_BANTIME);
			}
		}
	}

	if (ovr->prev)
		return ovr->prev->func(ovr->prev, cptr, sptr, parc, parv);
	else
		return ovr->command->func(cptr, sptr, parc, parv);
}
