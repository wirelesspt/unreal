/*
 * f_ulinessl.c
 * by fez (c) 2010 - provides a patch to prevent ULINE mode +z and insecure users on channel desynch.
 */

// includes
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

// ***** module-specific defines *****
#define F_ULINESSL_NAME		"f_ulinessl"
#define F_ULINESSL_VERS		"$Id: f_ulinessl.c, v1.0.2 2010/02/14 20:43:00 fez Exp $"
#define F_ULINESSL_DESC		"part insecure users on ULINE mode +z"
#define F_ULINESECUREACTION	"uline-secure-action"
#define F_NOTHING	0
#define F_PART		1
#define F_KICK		2

// ***** Function Declarations *****
DLLFUNC int f_config_test (ConfigFile *cf, ConfigEntry *ce, int type, int *errs);
DLLFUNC int f_config_posttest (int *errs);
DLLFUNC int f_config_run (ConfigFile *cf, ConfigEntry *ce, int type);
DLLFUNC int f_config_rehash ();
DLLFUNC int f_handle_mode (aClient *cptr, aClient *sptr, aChannel *chptr, char *mbuf, char *pbuf, time_t sendts, int samode);
DLLFUNC int f_count_secure_users (aChannel *chptr);
DLLFUNC void f_part_insecure_users (aChannel *chptr);

// ***** Global Variables *****
ModuleInfo *f_module_info = NULL;		// for local storage of modinfo
static Hook *f_hook_config_test = NULL;		// testing config file
static Hook *f_hook_config_posttest = NULL;	// check missing config settings
static Hook *f_hook_config_run = NULL;		// loading settings
static Hook *f_hook_config_rehash = NULL;	// for rehashing the module
static Hook *f_hook_local_chanmode = NULL;	// for local channel modes
static Hook *f_hook_remote_chanmode = NULL;	// for remote channel modes
static int f_count_ulinesecureaction = 0;	// config counter for ulinesecureaction
static int f_config_ulinesecureaction = 0;	// config setting for ulinesecureaction

// a description of this module
ModuleHeader MOD_HEADER(f_auth)
  = {
	F_ULINESSL_NAME,
	F_ULINESSL_VERS,
	F_ULINESSL_DESC,
	"3.2.3",
	NULL
  };

// called before config test
DLLFUNC int MOD_TEST(f_auth)(ModuleInfo *modinfo)
{
	f_count_ulinesecureaction = 0;
	if     (!(f_hook_config_test = HookAddEx(modinfo->handle, HOOKTYPE_CONFIGTEST, f_config_test)) ||
		!(f_hook_config_posttest = HookAddEx(modinfo->handle, HOOKTYPE_CONFIGPOSTTEST, f_config_posttest)) )
		return MOD_FAILED;
	return MOD_SUCCESS;
}

// called during module initialization
DLLFUNC int MOD_INIT(f_auth)(ModuleInfo *modinfo)
{
	f_module_info = modinfo;
	if     (!(f_hook_config_run = HookAddEx(modinfo->handle, HOOKTYPE_CONFIGRUN, f_config_run)) ||
		!(f_hook_config_rehash = HookAddEx(modinfo->handle, HOOKTYPE_REHASH, f_config_rehash)) ||
		!(f_hook_local_chanmode = HookAddEx(modinfo->handle, HOOKTYPE_LOCAL_CHANMODE, f_handle_mode)) ||
		!(f_hook_remote_chanmode = HookAddEx(modinfo->handle, HOOKTYPE_REMOTE_CHANMODE, f_handle_mode)) )
		return MOD_FAILED;
	return MOD_SUCCESS;
}

// called when ircd is 100% ready
DLLFUNC int MOD_LOAD(f_auth)(int module_load)
{
	return MOD_SUCCESS;
}

// called when unloading a module
DLLFUNC int MOD_UNLOAD(f_auth)(int module_unload)
{
	HookDel(f_hook_config_test);
	HookDel(f_hook_config_posttest);
	HookDel(f_hook_config_run);
	HookDel(f_hook_config_rehash);
	HookDel(f_hook_local_chanmode);
	HookDel(f_hook_remote_chanmode);
	f_hook_config_test = f_hook_config_posttest = f_hook_config_run = f_hook_config_rehash =
		f_hook_local_chanmode = f_hook_remote_chanmode = NULL;
	f_count_ulinesecureaction = f_config_ulinesecureaction = 0;
	return MOD_SUCCESS;
}

// this is where we TEST the config file
DLLFUNC int f_config_test (ConfigFile *cf, ConfigEntry *ce, int type, int *errs)
{
	int errors = 0;
	if (type != CONFIG_SET)
		return 0;
	else if (!strcmp(ce->ce_varname, F_ULINESECUREACTION))
	{
		if (!ce->ce_vardata || (strcmp(ce->ce_vardata, "nothing") &&
			strcmp(ce->ce_vardata, "part") && strcmp(ce->ce_vardata, "kick")) )
		{
			config_error("%s:%i: set::%s with unrecognized value, should be \"nothing\", \"part\", or \"kick\".",
				ce->ce_fileptr->cf_filename, ce->ce_varlinenum, ce->ce_varname );
			errors++;
		}
		else if (f_count_ulinesecureaction)
		{
			config_error("%s:%i: set::%s already defined on line %i",
				ce->ce_fileptr->cf_filename, ce->ce_varlinenum, ce->ce_varname, f_count_ulinesecureaction );
			errors++;
		}
		else
		{
			f_count_ulinesecureaction = ce->ce_varlinenum;
			!f_count_ulinesecureaction && ++f_count_ulinesecureaction;
		}
		*errs = errors;
		return errors ? -1 : 1;
	}
	return 0;
}

// this is run AFTER testing the config file
DLLFUNC int f_config_posttest (int *errs)
{
	int errors = 0;
	if (!f_count_ulinesecureaction)
	{
		//config_error("set::%s not set.", F_ULINESECUREACTION );
		//errors++;
		////or...
		f_config_ulinesecureaction = F_PART;
	}
	*errs = errors;
	return errors ? -1 : 1;
}

// this is run WHEN LOADING SETTINGS from the config file
DLLFUNC int f_config_run (ConfigFile *cf, ConfigEntry *ce, int type)
{
	if (type != CONFIG_SET)
		return 0;
	else if (!strcmp(ce->ce_varname, F_ULINESECUREACTION))
	{
		if (!ce->ce_vardata)
			return 0;
		else if (!strcmp(ce->ce_vardata, "nothing"))
			f_config_ulinesecureaction = F_NOTHING;
		else if (!strcmp(ce->ce_vardata, "part"))
			f_config_ulinesecureaction = F_PART;
		else if (!strcmp(ce->ce_vardata, "kick"))
			f_config_ulinesecureaction = F_KICK;
		else
			return 0;
		return 1;
	}
	return 0;
}

// this is run when REHASHING the config file
DLLFUNC int f_config_rehash ()
{
	f_count_ulinesecureaction = 0;
	return 1;
}

// this is where we handle the MODE command
DLLFUNC int f_handle_mode (aClient *cptr, aClient *sptr, aChannel *chptr, char *mbuf, char *pbuf, time_t sendts, int samode)
{
	int plusminus, securecount;
	char *p;
	if (f_config_ulinesecureaction == F_NOTHING)
		return MOD_SUCCESS;
	plusminus = 1;
	for (p = mbuf; *p; p++)
	{
		if (*p == '+')
			plusminus = 1;
		else if (*p == '-')
			plusminus = -1;
		else if (*p == 'z' && plusminus == 1 && (IsServer(sptr) || IsULine(sptr)))
		{
			securecount = f_count_secure_users(chptr);
			if (securecount < chptr->users)
			{
				sendto_snomask(SNO_EYES, "*** Uline %s set mode +z on %s: removing insecure users.",
					sptr->name, chptr->chname );
				if (f_config_ulinesecureaction == F_KICK || securecount > 0)
					kick_insecure_users(chptr);
				else
					f_part_insecure_users(chptr);
			}
		}
	}
	return MOD_SUCCESS;
}

// counts the number of secure users on a channel
int f_count_secure_users (aChannel *chptr)
{
	Member *member;
	aClient *cptr;
	int securecount = 0;
	for (member = chptr->members; member; member = member->next)
	{
		cptr = member->cptr;
		if (IsSecureConnect(cptr))
			securecount++;
	}
	return securecount;
}

// Parts all insecure users on a +z channel (NOT KICK)
void f_part_insecure_users (aChannel *chptr)
{
	Member *member, *mb2;
	aClient *cptr;
	char *comment = "Insecure user not allowed on secure channel (+z)";
	for (member = chptr->members; member; member = mb2)
	{
		mb2 = member->next;
		cptr = member->cptr;
		if (MyClient(cptr) && !IsSecureConnect(cptr) && !IsULine(cptr))
		{
			RunHook4(HOOKTYPE_LOCAL_PART, cptr, &me, chptr, comment);
			if ((chptr->mode.mode & MODE_AUDITORIUM) && is_chanownprotop(cptr, chptr))
			{
				sendto_chanops_butone(cptr, chptr, ":%s!%s@%s PART %s :%s",
					cptr->name, cptr->user->username, GetHost(cptr), chptr->chname, comment);
				sendto_prefix_one(cptr, &me, ":%s!%s@%s PART %s :%s",
					cptr->name, cptr->user->username, GetHost(cptr), chptr->chname, comment);
			} 
			else
			{
				sendto_channel_butserv(chptr, &me, ":%s!%s@%s PART %s :%s",
					cptr->name, cptr->user->username, GetHost(cptr), chptr->chname, comment);
			}
			sendto_one(cptr, err_str(ERR_SECUREONLYCHAN), me.name, cptr->name, chptr->chname);
			sendto_serv_butone_token(&me, cptr->name, MSG_PART, TOK_PART, "%s :%s", chptr->chname, comment);
			remove_user_from_channel(cptr, chptr);
		}
	}
}

/**/
