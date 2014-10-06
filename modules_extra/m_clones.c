/*
 * =================================================================
 * Filename:      m_clones.c
 * =================================================================
 * Description:   Command /clones
 * Documentation: m_clones.txt (comes with the package)
 * =================================================================
 * Requested by:  Bahiano
 * Written by:    AngryWolf <angrywolf@flashmail.com>
 * =================================================================
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
#ifdef STRIPBADWORDS
#include "badwords.h"
#endif
#ifdef _WIN32
#include "version.h"
#endif

extern void		sendto_one(aClient *to, char *pattern, ...);

#define MSG_CLONES	"CLONES"
#define TOK_CLONES	"CL"

#define IsParam(x)      (parc > (x) && !BadPtr(parv[(x)]))
#define IsNotParam(x)   (parc <= (x) || BadPtr(parv[(x)]))
#define DelCommand(x)	if (x) CommandDel(x); x = NULL

#ifndef INET6
#define SameHost(cptr1, cptr2)		((cptr1)->ip.S_ADDR == (cptr2)->ip.S_ADDR)
#else
#define SameHost(cptr1, cptr2)		!bcmp((cptr1)->ip.S_ADDR, (cptr2)->ip.S_ADDR, sizeof((cptr1)->ip.S_ADDR))
#endif

DLLFUNC int		m_clones(aClient *cptr, aClient *sptr, int parc, char *parv[]);
static Command		*AddCommand(Module *module, char *msg, char *token, iFP func);

Command			*CmdClones;

ModuleHeader MOD_HEADER(m_clones)
  = {
	"clones",
	"$Id: m_clones.c,v 1.8 2004/03/08 21:24:57 angrywolf Exp $",
	"command /clones",
	"3.2-b8-1",
	NULL 
    };

DLLFUNC int MOD_INIT(m_clones)(ModuleInfo *modinfo)
{
	CmdClones = AddCommand(modinfo->handle, MSG_CLONES, TOK_CLONES, m_clones);

	if (!CmdClones)
		return MOD_FAILED;

	return MOD_SUCCESS;
}

DLLFUNC int MOD_LOAD(m_clones)(int module_load)
{
	return MOD_SUCCESS;
}

DLLFUNC int MOD_UNLOAD(m_clones)(int module_unload)
{
	DelCommand(CmdClones);

	return MOD_SUCCESS;
}

static Command *AddCommand(Module *module, char *msg, char *token, iFP func)
{
	Command *cmd;

	if (CommandExists(msg))
    	{
		config_error("Command %s already exists", msg);
		return NULL;
    	}
    	if (CommandExists(token))
	{
		config_error("Token %s already exists", token);
		return NULL;
    	}

	cmd = CommandAdd(module, msg, token, func, MAXPARA, 0);

#ifndef STATIC_LINKING
	if (ModuleGetError(module) != MODERR_NOERROR || !cmd)
#else
	if (!cmd)
#endif
	{
#ifndef STATIC_LINKING
		config_error("Error adding command %s: %s", msg,
			ModuleGetErrorStr(module));
#else
		config_error("Error adding command %s", msg);
#endif
		return NULL;
	}

	return cmd;
}

/*
 * m_clones
 *
 * parv[0]: sender prefix
 * parv[1]: min. clone count or nickname
 * parv[2]: servername [optional]
 */

DLLFUNC int m_clones(aClient *cptr, aClient *sptr, int parc, char *parv[])
{
	aClient		*acptr;
	int		i, j;
	u_int		min, count, found = 0;

	if (!IsPerson(sptr))
		return -1;

	if (!IsAnOper(sptr))
	{
		sendto_one(sptr, err_str(ERR_NOPRIVILEGES), me.name, sptr->name);
		return 0;
	}

	if (!IsParam(1))
	{
    		sendto_one(sptr, err_str(ERR_NEEDMOREPARAMS),
            		me.name, parv[0], MSG_CLONES);
    		return 0;
	}

	if (IsParam(2))
	{
		if (hunt_server_token(cptr, sptr, MSG_CLONES, TOK_CLONES, "%s :%s",
		    2, parc, parv) != HUNTED_ISME)
			return 0;
	}

	if (isdigit(*parv[1]))
	{
		if ((min = atoi(parv[1])) < 2)
		{
    			sendto_one(sptr, ":%s NOTICE %s :*** %s: Invalid minimal clone count number",
            			me.name, parv[0], parv[1]);
    			return 0;
		}

		for (i = LastSlot; i >= 0; i--)
		{
			if (!local[i] || !IsPerson(local[i]))
				continue;

			count = 0;

			for (j = LastSlot; j >= 0; j--)
			{
				if (!local[j] || !IsPerson(local[j]))
					continue;
				if (SameHost(local[i], local[j]))
					count++;
			}

			if (count >= min)
			{
				found++;
				sendto_one(sptr, ":%s NOTICE %s :%s (%s!%s@%s)",
					me.name, sptr->name, Inet_ia2p(&local[i]->ip),
					local[i]->name, local[i]->user->username,
					local[i]->user->realhost);
			}
		}
	}
	else
	{
		if (!(acptr = find_person(parv[1], NULL)))
		{
			sendto_one(sptr, err_str(ERR_NOSUCHNICK),
				me.name, parv[0], parv[1]);
			return 0;
		}
		if (!MyConnect(acptr))
		{
			sendto_one(sptr, ":%s NOTICE %s :*** %s is not my client",
				me.name, parv[0], acptr->name);
			return 0;
		}

		found = 0;

		for (i = LastSlot; i >= 0; i--)
		{
			if (!local[i] || !IsPerson(local[i]) || local[i] == acptr)
				continue;
			if (SameHost(local[i], acptr))
			{
				found++;

				sendto_one(sptr, ":%s NOTICE %s :%s (%s!%s@%s)",
					me.name, sptr->name, Inet_ia2p(&local[i]->ip),
					local[i]->name, local[i]->user->username,
					local[i]->user->realhost);
			}
		}
	}

	sendto_one(sptr, ":%s NOTICE %s :%d clone%s found",
		me.name, sptr->name, found,
		(!found || found > 1) ? "s" : "");

        return 0;
}
