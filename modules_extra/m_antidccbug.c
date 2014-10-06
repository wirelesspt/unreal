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

ModuleHeader Mod_Header
  = {
	"m_antidccbug",	/* Name of module */
	"v0.0.2", /* Version */
	"Anti mIRC dcc exploit", /* Short description of module */
	"3.2-b8-1",
	NULL 
    };

static ModuleInfo ADCMod;

static Hook *UserMsg = NULL, *ChanMsg = NULL;

DLLFUNC char *adc_usermsg(aClient *, aClient *, aClient *, char *, int);
DLLFUNC char *adc_chanmsg(aClient *, aClient *, aChannel *, char *, int);

/* This is called on module init, before Server Ready */
DLLFUNC int	Mod_Init(ModuleInfo *modinfo)
{
	bcopy(modinfo,&ADCMod,modinfo->size);
	UserMsg = HookAddPCharEx(ADCMod.handle, HOOKTYPE_USERMSG, adc_usermsg);
	ChanMsg = HookAddPCharEx(ADCMod.handle, HOOKTYPE_CHANMSG, adc_chanmsg);
	return MOD_SUCCESS;
}

DLLFUNC int	Mod_Load(int module_load)
{
	return MOD_SUCCESS;
}

DLLFUNC int	Mod_Unload(int module_unload)
{
	HookDel(UserMsg);
	HookDel(ChanMsg);
	return MOD_SUCCESS;
}

DLLFUNC char *adc_usermsg(aClient *cptr, aClient *sptr, aClient *acptr, char *text, int notice)
{
	if (text && (text[0] == '\001') && !strncasecmp(text+1, "DCC ", 4))
	{
		int spaces=0;
		char *p;
		int len = strlen(text);

		for (p = text; *p; p++)
			if (*p == ' ')
				spaces++;
		if ((spaces > 20) || (len > 200))
		{
			if (MyClient(sptr))
			{
				sendto_one(sptr, ":%s NOTICE %s :Suspicious DCC command detected (possible mIRC exploit). "
			    	             "DCC blocked and opers have been notified.", me.name, sptr->name);
				sptr->since += 7;
				sptr->flags |= FLAGS_DCCBLOCK;
			} else {
				/* Only happens if other servers don't have the module loaded */
				sendto_one(sptr, ":%s NOTICE %s :Suspicious DCC command detected (possible mIRC exploit). "
			    	             "Opers have been notified.", me.name, sptr->name);
			}
			sendto_realops("%s (%s@%s) used suspicious DCC command (possible mIRC exploit) [victim was %s]",
				sptr->name, sptr->user->username, sptr->user->realhost, acptr->name);
			return NULL;
		}
	}
	return text;
}

DLLFUNC char *adc_chanmsg(aClient *cptr, aClient *sptr, aChannel *chptr, char *text, int notice)
{
	if (text && (text[0] == '\001') && !strncasecmp(text+1, "DCC ", 4))
	{
		int spaces=0;
		char *p;
		int len = strlen(text);

		for (p = text; *p; p++)
			if (*p == ' ')
				spaces++;
		if ((spaces > 20) || (len > 200))
		{
			sendto_one(sptr, ":%s NOTICE %s :Suspicious DCC command detected (possible mIRC exploit). "
		    	             "Opers have been notified.", me.name, sptr->name);
			if (MyClient(sptr))
			{
				sptr->since += 7;
				sptr->flags |= FLAGS_DCCBLOCK;
			}
			sendto_realops("%s (%s@%s) used suspicious DCC command (possible mIRC exploit) [target was %s]",
				sptr->name, sptr->user->username, sptr->user->realhost, chptr->chname);
			return NULL;
		}
	}
	return text;
}
