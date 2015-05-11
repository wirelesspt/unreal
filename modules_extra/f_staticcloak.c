/*
 * f_staticcloak.c
 * (c) fez - sets sets a static hostname for cloak
 * requires .conf directive set::static-cloak
 */

// includes
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
#ifdef _WIN32
#include "version.h"
#endif

// module defines
#define STATICCLOAK_DIRECTIVE	"static-cloak"

// function declarations
DLLFUNC int f_staticcloak_config_test(ConfigFile *cf, ConfigEntry *ce, int type, int *errs);
DLLFUNC int f_staticcloak_config_run(ConfigFile *cf, ConfigEntry *ce, int type);
DLLFUNC int f_staticcloak_config_posttest(int *errs);
DLLFUNC int f_staticcloak_config_rehash();
DLLFUNC char *f_staticcloak_cloak(char *host);
DLLFUNC char *f_staticcloak_cloakcsum();

// global variables
ModuleInfo *StaticcloakModInfo = NULL;		//
static int staticcloak_conf_counter = 0;	// for config file
static Hook *StaticcloakHookTest = NULL;	// for config hashiing
static Hook *StaticcloakHookPosttest = NULL;	//
static Hook *StaticcloakHookRun = NULL;		//
static Hook *StaticcloakHookRehash = NULL;	//
Callback *StaticcloakCloak = NULL;		//
Callback *StaticcloakCloakcsum = NULL;		//
static char staticcloak_hostname[512];		//
static char staticcloak_checksum[64];		//

// description of module
ModuleHeader MOD_HEADER(f_staticcloak)
  = {
	"f_staticcloak",
	"$Id: f_staticcloak.c, v1.0.2 2007/10/16 20:43:00 fez Exp $",
	"Static cloaking module",
	"3.2.3",
	NULL
  };

// called before config test
DLLFUNC int MOD_TEST(f_staticcloak)(ModuleInfo *modinfo)
{
	int ret = MOD_SUCCESS;
	StaticcloakHookTest = HookAddEx(modinfo->handle, HOOKTYPE_CONFIGTEST, f_staticcloak_config_test);
	StaticcloakHookPosttest = HookAddEx(modinfo->handle, HOOKTYPE_CONFIGPOSTTEST, f_staticcloak_config_posttest);
	StaticcloakCloak = CallbackAddPCharEx(modinfo->handle, CALLBACKTYPE_CLOAK, f_staticcloak_cloak);
	StaticcloakCloakcsum = CallbackAddPCharEx(modinfo->handle, CALLBACKTYPE_CLOAKKEYCSUM, f_staticcloak_cloakcsum);
	if (!StaticcloakCloak || !StaticcloakCloakcsum)
	{
		config_error("f_staticcloak: failed to load cloaking callback functions.");
		ret = MOD_FAILED;
	}
	staticcloak_conf_counter = 0;
	return ret;
}

// called during module initialization
DLLFUNC int MOD_INIT(f_staticcloak)(ModuleInfo *modinfo)
{
	int ret = MOD_SUCCESS;
	StaticcloakModInfo = modinfo;
	StaticcloakHookRun = HookAddEx(modinfo->handle, HOOKTYPE_CONFIGRUN, f_staticcloak_config_run);
	StaticcloakHookRehash = HookAddEx(modinfo->handle, HOOKTYPE_REHASH, f_staticcloak_config_rehash);
	return ret;
}

// called when ircd is 100% ready
DLLFUNC int MOD_LOAD(f_staticcloak)(int module_load)
{
	int ret = MOD_SUCCESS;
	return ret;
}

// called when unloading a module
DLLFUNC int MOD_UNLOAD(f_staticcloak)(int module_unload)
{
	int ret = MOD_SUCCESS;
	CallbackDel(StaticcloakCloak);
	CallbackDel(StaticcloakCloakcsum);
	HookDel(StaticcloakHookTest);
	HookDel(StaticcloakHookPosttest);
	HookDel(StaticcloakHookRun);
	HookDel(StaticcloakHookRehash);
	StaticcloakCloak = StaticcloakCloakcsum = NULL;
	StaticcloakHookTest = StaticcloakHookPosttest = StaticcloakHookRun = StaticcloakHookRehash = NULL;
	return ret;
}

// this is where we TEST the config file
DLLFUNC int f_staticcloak_config_test(ConfigFile *cf, ConfigEntry *ce, int type, int *errs)
{
	int errors = 0;
	if (type != CONFIG_SET)
		return 0;
	if (!strcmp(ce->ce_varname, STATICCLOAK_DIRECTIVE))
	{
		if (!ce->ce_vardata)
		{
			config_error("%s:%i: set::%s without contents",
					ce->ce_fileptr->cf_filename, ce->ce_varlinenum,
					ce->ce_varname );
			errors++;
		}
		else if (staticcloak_conf_counter)
		{
			config_error("%s:%i: set::%s already defined on line %i",
					ce->ce_fileptr->cf_filename, ce->ce_varlinenum,
					ce->ce_varname, staticcloak_conf_counter );
			errors++;
		}
		else
		{
			staticcloak_conf_counter = ce->ce_varlinenum;
			!staticcloak_conf_counter && ++staticcloak_conf_counter;
		}
		*errs = errors;
		return errors ? -1 : 1;
	}
	return 0;
}

// this is run AFTER testing config file
DLLFUNC int f_staticcloak_config_posttest(int *errs)
{
	int errors = 0;
	if (!staticcloak_conf_counter)
	{
		config_error("set::%s is not set!", STATICCLOAK_DIRECTIVE);
		errors++;
		////or...
		//strcpy(staticcloak_hostname, STATICCLOAK_HOSTNAME);
		//strcpy(staticcloak_checksum, "OK");
	}
	*errs = errors;
	return errors ? -1 : 1;
}

// this is run when LOADING SETTINGS from the config file
DLLFUNC int f_staticcloak_config_run(ConfigFile *cf, ConfigEntry *ce, int type)
{
	if (type != CONFIG_SET)
		return 0;
	if (!strcmp(ce->ce_varname, STATICCLOAK_DIRECTIVE))
	{
		//ircsprintf(cloak_hostname, "%s", ce->ce_vardata);
		//ircsprintf(cloak_checksum, "OK");
		strcpy(staticcloak_hostname, ce->ce_vardata);
		strcpy(staticcloak_checksum, "OK");
		return 1;
	}
	return 0;
}

// this is run when REHASHING the config file
DLLFUNC int f_staticcloak_config_rehash()
{
	staticcloak_conf_counter = 0;
	return 1;
}

DLLFUNC char *f_staticcloak_cloak(char *host)
{
	return staticcloak_hostname;
}


DLLFUNC char *f_staticcloak_cloakcsum()
{
	return staticcloak_checksum;
}

/* end of code */
