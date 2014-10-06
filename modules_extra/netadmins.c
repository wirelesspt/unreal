/*
 * =================================================================
 * Filename:     netadmins.c
 * =================================================================
 * Description:  Protects all NetAdmins from being killed and banned
 *               by non-NetAdmins, or if you want, by any IRC
 *               operators (except U:Lines). The module must be
 *               installed on all Unreal servers of your network in
 *               order to make it work properly.
 * =================================================================
 * Author:       AngryWolf
 * Email:        angrywolf@flashmail.com
 * =================================================================
 *
 * I accept bugreports, ideas and opinions, and if you have any
 * questions, just send an email to me!
 *
 * Thank you for using my module!
 *
 * =================================================================
 * Requirements:
 * =================================================================
 *
 * o Unreal >=3.2-beta19
 * o One of the supported operating systems (see unreal32docs.html)
 *
 * =================================================================
 * Installation:
 * =================================================================
 *
 * See http://angrywolf.linktipp.org/compiling.php?lang=en
 *
 * =================================================================
 * Configuration:
 * =================================================================
 *
 * Optionally, you can set some options to specify how the module
 * should work. The configuration looks like this:
 *
 *         set {
 *             // Choose "no" here if you would like to allow
 *             // netadmins to kill each other. Otherwise "yes".
 *             netadmins-are-gods yes;
 *         }; 
 *
 * =================================================================
 * Mirror files:
 * =================================================================
 *
 * http://angrywolf.linktipp.org/netadmins.c [Germany]
 * http://angrywolf.uw.hu/netadmins.c [Hungary]
 * http://angrywolf.fw.hu/netadmins.c [Hungary]
 *
 * =================================================================
 * Changes:
 * =================================================================
 *
 * $Log: netadmins.c,v $
 * Revision 1.8  2004/04/19 19:37:27  angrywolf
 * - Fixed a bug with servers not being able to kill netadmins
 *   (reported by YonderBoY).
 *
 * Revision 1.7  2004/03/08 21:26:29  angrywolf
 * - Fixed some bugs that could cause crash if you compile the module
 *   statically (for example, under Windows).
 *
 * Revision 1.6  2004/02/04 09:45:30  angrywolf
 * - Code cleanup.
 *
 * Revision 1.5  2004/01/29 07:34:50  angrywolf
 * - Fixed a number of Windows compilation errors
 *
 * Revision 1.4  2004/01/28 22:10:48  angrywolf
 * - Fixed a crash bug caused by a wrong removal of command overrides.
 *
 * Revision 1.3  2004/01/09 20:38:40  angrywolf
 * - Minor doc correction.
 *
 * Revision 1.2  2004/01/09 13:27:36  angrywolf
 * - Added a new configuration directive: set::netadmins-are-gods.
 *
 * Revision 1.1  2004/01/09 12:04:11  angrywolf
 * Initial revision
 *
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

extern ConfigEntry		*config_find_entry(ConfigEntry *ce, char *name);
extern void			sendto_one(aClient *to, char *pattern, ...);
extern void			sendto_realops(char *pattern, ...);

#define DelOverride(cmd, ovr)	if (ovr && CommandExists(cmd)) CmdoverrideDel(ovr); ovr = NULL
#define DelHook(x)		if (x) HookDel(x); x = NULL
#define IsParam(x)		(parc > (x) && !BadPtr(parv[(x)]))
#define IsNotParam(x)		(parc <= (x) || BadPtr(parv[(x)]))
#define OVR_FUNC(x)		int (x)(Cmdoverride *ovr, aClient *cptr, aClient *sptr, int parc, char *parv[])

/* Command types */
#define CMD_KLINE		0
#define CMD_ZLINE		1
#define CMD_GLINE		2
#define CMD_GZLINE		3
#define CMD_SHUN		4

/* Ban types */
#define BT_IP			0	/* Only IP */
#define BT_ANY			1	/* IP or hostname */

/* Mask types */
#define MT_NICK			0	/* Nickname */
#define MT_NUH			1	/* n!u@h mask */

static OVR_FUNC(ovr_kill);
static OVR_FUNC(ovr_kline);
static OVR_FUNC(ovr_zline);
static OVR_FUNC(ovr_gline);
static OVR_FUNC(ovr_gzline);
static OVR_FUNC(ovr_shun);

static Cmdoverride		*AddOverride(char *msg, iFP cb);
static int			netadmin_ban(Cmdoverride *, u_short, aClient *, aClient *, int, char *[]);
static int			cb_config_test(ConfigFile *, ConfigEntry *, int, int *);
static int			cb_config_run(ConfigFile *, ConfigEntry *, int);
static int			cb_config_rehash();
static int			cb_stats(aClient *sptr, char *stats);
static void			InitConf();
/* static void			FreeConf(); */

static Cmdoverride		*OvrKill, *OvrKline, *OvrZline, *OvrGline, *OvrGzline, *OvrShun;
static Hook			*HookConfTest = NULL, *HookConfRun = NULL, *HookConfRehash = NULL;
static Hook			*HookStats = NULL;
unsigned			netadmins_are_gods;

#ifndef STATIC_LINKING
static ModuleInfo	*MyModInfo;
 #define MyMod		MyModInfo->handle
 #define SAVE_MODINFO	MyModInfo = modinfo;
#else
 #define MyMod		NULL
 #define SAVE_MODINFO
#endif

ModuleHeader MOD_HEADER(netadmins)
  = {
	"netadmins",
	"$Id: netadmins.c,v 1.8 2004/04/19 19:37:27 angrywolf Exp $",
	"Kill & ban protection for netadmins",
	"3.2-b8-1",
	NULL 
    };

DLLFUNC int MOD_TEST(netadmins)(ModuleInfo *modinfo)
{
	SAVE_MODINFO
	HookConfTest = HookAddEx(modinfo->handle, HOOKTYPE_CONFIGTEST, cb_config_test);

        return MOD_SUCCESS;
}

DLLFUNC int MOD_INIT(netadmins)(ModuleInfo *modinfo)
{
	SAVE_MODINFO
	InitConf();

	HookConfRun	= HookAddEx(modinfo->handle, HOOKTYPE_CONFIGRUN, cb_config_run);
	HookConfRehash	= HookAddEx(modinfo->handle, HOOKTYPE_REHASH, cb_config_rehash);
	HookStats	= HookAddEx(modinfo->handle, HOOKTYPE_STATS, cb_stats);

        return MOD_SUCCESS;
}

DLLFUNC int MOD_LOAD(netadmins)(int module_load)
{
	int ret = MOD_SUCCESS;

	OvrKill		= AddOverride("kill", ovr_kill);
	OvrKline	= AddOverride("kline", ovr_kline);
	OvrZline	= AddOverride("zline", ovr_zline);
	OvrGline	= AddOverride("gline", ovr_gline);
	OvrGzline	= AddOverride("gzline", ovr_gzline);
	OvrShun		= AddOverride("shun", ovr_shun);

	if (!OvrKill || !OvrKline || !OvrZline || !OvrGline || !OvrGzline || !OvrShun)
		ret = MOD_FAILED;

	return ret;
}

DLLFUNC int MOD_UNLOAD(netadmins)(int module_unload)
{
	// FreeConf();

	DelHook(HookStats);
	DelHook(HookConfRehash);
	DelHook(HookConfRun);
	DelHook(HookConfTest);

        DelOverride("shun", OvrShun);
        DelOverride("gzline", OvrGzline);
        DelOverride("gline", OvrGline);
        DelOverride("zline", OvrZline);
        DelOverride("kline", OvrKline);
        DelOverride("kill", OvrKill);

        return MOD_SUCCESS;
}
/* ================================================================= */

static void InitConf()
{
	netadmins_are_gods = 0;
}

/*
 * static void FreeConf()
 * {
 * }
 */

static int cb_config_rehash()
{
	// FreeConf();
	InitConf();

	return 1;
}

static int cb_config_test(ConfigFile *cf, ConfigEntry *ce, int type, int *errs)
{
	int errors = 0;

	if (type != CONFIG_SET)
		return 0;

	if (!strcmp(ce->ce_varname, "netadmins-are-gods"))
	{
		if (!ce->ce_vardata)
		{
			config_error("%s:%i: set::netadmins-are-gods without contents",
					ce->ce_fileptr->cf_filename,
					ce->ce_varlinenum);
			errors++;
		}

		*errs = errors;
		return errors ? -1 : 1;
	}
	else
		return 0;
}

static int cb_config_run(ConfigFile *cf, ConfigEntry *ce, int type)
{
	if (type != CONFIG_SET)
		return 0;

	if (!strcmp(ce->ce_varname, "netadmins-are-gods"))
	{
		netadmins_are_gods = config_checkval(ce->ce_vardata, CFG_YESNO);
		return 1;		
	}

	return 0;
}

static int cb_stats(aClient *sptr, char *stats)
{
	if (*stats == 'S')
	{
		sendto_one(sptr, ":%s %i %s :netadmins-are-gods: %d",
			me.name, RPL_TEXT, sptr->name, netadmins_are_gods);
	}
        return 0;
}

/* ================================================================= */

Cmdoverride *AddOverride(char *msg, iFP cb)
{
	Cmdoverride *ovr = CmdoverrideAdd(MyMod, msg, cb);

#ifndef STATIC_LINKING
	if (ModuleGetError(MyMod) != MODERR_NOERROR ||!ovr)
#else
	if (!ovr)
#endif
	{
#ifndef STATIC_LINKING
		config_error("Error replacing command %s when loading module %s: %s",
			msg, MOD_HEADER(netadmins).name, ModuleGetErrorStr(MyMod));
#else
		config_error("Error replacing command %s when loading module %s",
			msg, MOD_HEADER(netadmins).name);
#endif
		return NULL;
	}

	return ovr;
}

/* ================================================================= */

static char *find_tkl_cmd(u_int cmd)
{
	switch (cmd)
	{
		case CMD_KLINE:		return "KLINE";
		case CMD_ZLINE:		return "ZLINE";
		case CMD_GLINE:		return "GLINE";
		case CMD_GZLINE:	return "GZLINE";
		case CMD_SHUN:		return "SHUN";
	}

	return "???";
}

static int match_tkl(u_int btype, char *mask, aClient *cptr)
{
	char *s;

	if (btype != BT_IP)
	{
		s = make_user_host(cptr->user->username, cptr->user->realhost);
		if (!match(mask, s))
			return 1;
	}

	if (MyConnect(cptr))
	{
		s = make_user_host(cptr->user->username, Inet_ia2p(&cptr->ip));
		if (!match(mask, s))
			return 1;
	}

	return 0;
}

static int check_netadmins(u_int mtype, u_int btype, char *mask)
{
	int		i, j, found = 0;
	aClient		*cptr;

#ifdef NO_FDLIST
	for (i = 0; i <= LastSlot; i++)
		if ((cptr = local[i]) && !IsServer(cptr) && !IsMe(cptr) &&
		    IsOper(cptr) && IsNetAdmin(cptr))
#else
	for (i = oper_fdlist.entry[j = 1]; j <= oper_fdlist.last_entry; i = oper_fdlist.entry[++j])
		if ((cptr = local[i]) && IsNetAdmin(cptr))
#endif
		{
			if (mtype == MT_NICK)
			{
				if (!strcasecmp(cptr->name, mask))
				{
					found = 1;
				        break;
				}
			}
			else if (mtype == MT_NUH)
			{
				if (match_tkl(btype, mask, cptr))
				{
					found = 1;
					break;
				}
			}
		}

	return found;
}

/* ================================================================= */

static OVR_FUNC(ovr_kline)
{
	if (!IsPerson(sptr) || !OPCanKline(sptr) || !IsAnOper(sptr))
		return CallCmdoverride(ovr, cptr, sptr, parc, parv);

	return netadmin_ban(ovr, CMD_KLINE, cptr, sptr, parc, parv);
}

static OVR_FUNC(ovr_zline)
{
	if (!IsPerson(sptr) || !OPCanZline(sptr) || !IsAnOper(sptr))
		return CallCmdoverride(ovr, cptr, sptr, parc, parv);

	return netadmin_ban(ovr, CMD_ZLINE, cptr, sptr, parc, parv);
}

static OVR_FUNC(ovr_gline)
{
	if (!IsPerson(sptr) || !OPCanTKL(sptr) || !IsOper(sptr))
		return CallCmdoverride(ovr, cptr, sptr, parc, parv);

	return netadmin_ban(ovr, CMD_GLINE, cptr, sptr, parc, parv);
}

static OVR_FUNC(ovr_gzline)
{
	if (!IsPerson(sptr) || !OPCanGZL(sptr) || !IsOper(sptr))
		return CallCmdoverride(ovr, cptr, sptr, parc, parv);

	return netadmin_ban(ovr, CMD_GZLINE, cptr, sptr, parc, parv);
}

static OVR_FUNC(ovr_shun)
{
	if (!IsPerson(sptr) || !OPCanTKL(sptr) || !IsOper(sptr))
		return CallCmdoverride(ovr, cptr, sptr, parc, parv);

	return netadmin_ban(ovr, CMD_SHUN, cptr, sptr, parc, parv);
}

/* ================================================================= */

static OVR_FUNC(ovr_kill)
{
	static char	buf[BUFSIZE + 1];
	char		*mask, *reason, *nick;
	char		*p = NULL;
	aClient		*acptr;

	if (IsServer(sptr) || IsULine(sptr) || !IsAnOper(sptr) ||
	    (!netadmins_are_gods && IsOper(sptr) && IsNetAdmin(sptr)) ||
	    !IsParam(2))
		return CallCmdoverride(ovr, cptr, sptr, parc, parv);

	mask	= parv[1];
	reason	= parv[2];
	buf[0]	= 0;

	if (MyClient(sptr))
		mask = canonize(mask);

	for (nick = strtoken(&p, mask, ","); nick; nick = strtoken(&p, NULL, ","))
	{
		if (!(acptr = find_client(nick, NULL)) && !(acptr = get_history(nick, (long)KILLCHASETIMELIMIT)))
		{
			sendto_one(sptr, err_str(ERR_NOSUCHNICK),
				me.name, sptr->name, nick);
			continue;
		}

		if (IsOper(acptr) && IsNetAdmin(acptr))
		{
			sendto_one(sptr, ":%s NOTICE %s :You are not allowed to KILL %s",
				me.name, sptr->name, acptr->name);
			sendto_realops("*** %s tried to KILL %s (%s)",
				sptr->name, acptr->name, reason);
			continue;
		}

		if (buf[0])
			strcat(buf, ",");
		strcat(buf, acptr->name);
	}

	if (!buf[0])
		return 0;

	parv[1] = buf;
	return CallCmdoverride(ovr, cptr, sptr, parc, parv);
}

int netadmin_ban(Cmdoverride *ovr, u_short cmd, aClient *cptr, aClient *sptr, int parc, char *parv[])
{
	char		*mask, *mid;
	u_short		mtype, btype;
	int		ok = 1;

	/* Check if we actually have to handle the command or not */

	if (!netadmins_are_gods && IsOper(sptr) && IsNetAdmin(sptr))
		ok = 0;
	else if (IsULine(sptr) || IsNotParam(1) || *parv[1] == '-')
		ok = 0;

	if (!ok)
		return CallCmdoverride(ovr, cptr, sptr, parc, parv);

	/* Check for the validity of the first parameter */

	mask = parv[1];

	if (*mask == '+')
		mask++;

        if (strchr(mask, '!'))
        {
                sendto_one(sptr, ":%s NOTICE %s :[error] Cannot have ! in masks.", me.name,
            		sptr->name);
                return 0;
        }
        if (strchr(mask, ' ') || ((mid = strchr(mask, '@')) && (mid == mask || !mid[1])))
	{
                sendto_one(sptr, ":%s NOTICE %s :[error] Invalid mask.", me.name,
            		sptr->name);
                return 0;
	}

	/* Check if the first parameter matches a netadmin */

	mtype = (strchr(mask, '@') ? MT_NUH : MT_NICK);
	btype = (cmd == CMD_ZLINE || cmd == CMD_GZLINE) ? BT_IP : BT_ANY;

	if (check_netadmins(mtype, btype, mask))
	{
		sendto_one(sptr, ":%s NOTICE %s :You are not allowed to add a %s on %s",
			me.name, sptr->name, find_tkl_cmd(cmd), mask);
		sendto_realops("*** %s tried to add a %s on %s",
			sptr->name, find_tkl_cmd(cmd), mask);
		return 0;
	}

	return CallCmdoverride(ovr, cptr, sptr, parc, parv);
}
