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

#define MSG_SCANIP "SCANIP"
#define TOK_SCANIP "S1"

#ifndef DYNAMIC_LINKING
ModuleHeader m_bopmhelper_Header
#else
#define m_bopmhelper_Header Mod_Header
ModuleHeader Mod_Header
#endif
  = {
	"m_bopmhelper",	/* Name of module */
	"v0.0.1", /* Version */
	"BOPM Helper", /* Short description of module */
	"3.2-b8-1",
	NULL 
    };

ModuleInfo BOPMHelperModInfo;

static Hook *HookConnect = NULL, *HookConfTest = NULL, *HookConfRun = NULL;
static Hook *HookRehash = NULL, *HookPostConf;

DLLFUNC int bopmhelper_connect(aClient *);
DLLFUNC int bopmhelper_config_test(ConfigFile *, ConfigEntry *, int, int *);
DLLFUNC int bopmhelper_config_run(ConfigFile *, ConfigEntry *, int);
DLLFUNC int bopmhelper_rehash();
DLLFUNC int bopmhelper_config_posttest(int *);

DLLFUNC int m_scanip(aClient *cptr, aClient *sptr, int parc, char *parv[]);

static char *bopmnick = NULL;
static int bopmnick_present = 0;

#ifdef DYNAMIC_LINKING
DLLFUNC int Mod_Test(ModuleInfo *modinfo)
#else
int    m_bopmhelper_Test(ModuleInfo *modinfo)
#endif
{
	bcopy(modinfo,&BOPMHelperModInfo,modinfo->size);
	HookConfTest = HookAddEx(BOPMHelperModInfo.handle, HOOKTYPE_CONFIGTEST, bopmhelper_config_test);
	HookPostConf = HookAddEx(BOPMHelperModInfo.handle, HOOKTYPE_CONFIGPOSTTEST, bopmhelper_config_posttest);
	return MOD_SUCCESS;
}

/* This is called on module init, before Server Ready */
#ifdef DYNAMIC_LINKING
DLLFUNC int	Mod_Init(ModuleInfo *modinfo)
#else
int    m_bopmhelper_Init(ModuleInfo *modinfo)
#endif
{
	bcopy(modinfo,&BOPMHelperModInfo,modinfo->size);
	HookConnect = HookAddEx(BOPMHelperModInfo.handle, HOOKTYPE_LOCAL_CONNECT, bopmhelper_connect);
	HookConfRun = HookAddEx(BOPMHelperModInfo.handle, HOOKTYPE_CONFIGRUN, bopmhelper_config_run);
	HookRehash = HookAddEx(BOPMHelperModInfo.handle, HOOKTYPE_REHASH, bopmhelper_rehash);
	add_Command(MSG_SCANIP, TOK_SCANIP, m_scanip, MAXPARA);
	return MOD_SUCCESS;
}

/* Is first run when server is 100% ready */
#ifdef DYNAMIC_LINKING
DLLFUNC int	Mod_Load(int module_load)
#else
int    m_bopmhelper_Load(int module_load)
#endif
{
	return MOD_SUCCESS;
}

/* Called when module is unloaded */
#ifdef DYNAMIC_LINKING
DLLFUNC int	Mod_Unload(int module_unload)
#else
int	m_bopmhelper_Unload(int module_unload)
#endif
{
	HookDel(HookConnect);
	HookDel(HookRehash);
	HookDel(HookConfRun);
	HookDel(HookConfTest);
	HookDel(HookPostConf);
	if (del_Command(MSG_SCANIP, TOK_SCANIP, m_scanip) < 0)
		sendto_realops("Failed to delete commands when unloading BOPM Helper module");

	return MOD_SUCCESS;
}

/***********************************************************************************/

DLLFUNC int bopmhelper_config_test(ConfigFile *cf, ConfigEntry *ce, int type, int *errs)
{
int errors = 0;

	if (type != CONFIG_SET)
		return 0;

	if (!strcmp(ce->ce_varname, "bopm-nick"))
	{
		if (!ce->ce_vardata)
		{
			config_error("%s:%i: set::bopmhelper has no value", ce->ce_fileptr->cf_filename, ce->ce_varlinenum);
			errors++;
		}
		bopmnick_present = 1;
		*errs = errors;
		return errors ? -1 : 1;
	}
	else
		return 0;
}

DLLFUNC int bopmhelper_config_run(ConfigFile *cf, ConfigEntry *ce, int type)
{
	if (type != CONFIG_SET)
		return 0;

	if (!strcmp(ce->ce_varname, "bopm-nick"))
	{
		if (bopmnick)
			free(bopmnick);
		bopmnick = strdup(ce->ce_vardata);
		return 1;
	}
	else
		return 0;
}

DLLFUNC int bopmhelper_config_posttest(int *errs)
{
int errors = 0;
	if (!bopmnick_present)
	{
		config_error("set::bopm-nick missing");
		errors++;
	}
	*errs = errors;
	return errors ? -1 : 1;
}

DLLFUNC int bopmhelper_rehash()
{
	if (bopmnick) {
		free(bopmnick);
		bopmnick = NULL;
		bopmnick_present = 0;
	}
	return 1;
}

static aClient *hunt_bopm()
{
aClient *acptr = find_client(bopmnick, (aClient *)NULL);
	if (!acptr || !IsOper(acptr))
		return NULL;
	else
		return acptr;
}


DLLFUNC int bopmhelper_connect(aClient *sptr)
{
aClient *acptr;
	/* do it here plz */
	acptr = hunt_bopm();
	if (!acptr)
		goto end;

	if (MyClient(acptr))
		goto end; /* Don't send for local clients */
		
	sendto_one(acptr->from, "%s %s %s :%s", TOK_SCANIP, sptr->name, sptr->user->username, Inet_ia2p(&sptr->ip));
end:
	return 0;
}

DLLFUNC int m_scanip(aClient *cptr, aClient *sptr, int parc, char *parv[])
{
aClient *acptr;

	if (!IsServer(sptr) || (parc < 4) || BadPtr(parv[1]))
		return 0;

	acptr = hunt_bopm();
	if (!acptr)
		return 0;
	
	if (MyClient(acptr))
	{
		/* Deliver it to BOPM */
		sendto_one(acptr, ":%s NOTICE %s :*** Notice -- Client connecting: %s (%s@%s) [%s] {remote}",
			me.name, acptr->name,
			parv[1], parv[2], parv[3], /* %s (%s@%s) */
			parv[3]); /* [%s] */
	} else {
		/* Forward... */
		sendto_one(acptr->from, "%s %s %s :%s", TOK_SCANIP, parv[1], parv[2], parv[3]);
	}
	return 0;
}
