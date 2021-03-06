/*
 * "No caps", a very simple module written by Grunt
 * heavily based on m_nocodes, by Syzop
 * and m_blocknonalpha, by Certus
 *
 * This module will block all messages in caps to a
 * channel that has mode +d set. It's not supposed
 * to take in consideration non-alpha characters
 * like: ! ? ^ & etc., thus:
 * "A!A!A!???" is considered to be in caps!
 * 
 * Changes:
 *
 * 1.2
 * ---- Added option to convert the message to lowercase
 * 1.1
 * ---- Code cleanup
 * ---- Optimisation (stop searching when a lowercase letter is found)
 * 1.0
 * ---- Initial release
 */

// System includes before Unreal includes, thanks Stealth!

#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Unreal includes follow...

#include "config.h"
#include "struct.h"
#include "common.h"
#include "sys.h"
#include "numeric.h"
#include "msg.h"
#include "proto.h"
#include "channel.h"
#include "h.h"

#ifdef _WIN32
#include <io.h>
#include "version.h"
#endif

#ifdef STRIPBADWORDS
#include "badwords.h"
#endif

#define NOCAPS_VERSION "v1.2" // second release

/*
 * NOCAPS_ACTION - what action to take?
 * 1 = BLOCK
 * 0 = CONVERT the message to lowercase
 * any other number = DO NOTHING
*/
#define NOCAPS_ACTION 0

Cmode_t NOCAPS_BLOCK = 0L;
Cmode *ModeBlock = NULL;

ModuleHeader MOD_HEADER(m_nocaps)
  = {
	"m_nocaps",
	NOCAPS_VERSION,
	"Block all caps to +d channels (Grunt)",
	"3.2-b8-1",
	NULL 
    };

static ModuleInfo NoCapsModInfo;

static Hook *CheckMsg;

DLLFUNC char *nocaps_checkmsg(aClient *, aClient *, aChannel *, char *, int);

DLLFUNC int MOD_INIT(m_nocaps)(ModuleInfo *modinfo)
{
    ModuleSetOptions(modinfo->handle, MOD_OPT_PERM);
    CmodeInfo req;
    ircd_log(LOG_ERROR, "debug: mod_init called from m_nocaps");
    sendto_realops("loading m_nocaps");
    memset(&req, 0, sizeof(req));
    req.paracount = 0;
    req.is_ok = extcmode_default_requirehalfop;
    req.flag = 'd';
    ModeBlock = CmodeAdd(modinfo->handle, req, &NOCAPS_BLOCK);

    bcopy(modinfo,&NoCapsModInfo,modinfo->size);
    CheckMsg = HookAddPCharEx(NoCapsModInfo.handle, HOOKTYPE_CHANMSG, nocaps_checkmsg);
    return MOD_SUCCESS;
}

DLLFUNC int MOD_LOAD(m_nocaps)(int module_load)
{
	return MOD_SUCCESS;
}

DLLFUNC int MOD_UNLOAD(m_nocaps)(int module_unload)
{
	HookDel(CheckMsg);
	CmodeDel(ModeBlock);
        ircd_log(LOG_ERROR, "debug: mod_unload called from m_nocaps");
	sendto_realops("unloading m_nocaps");
	return MOD_SUCCESS;
}

DLLFUNC char *nocaps_checkmsg(aClient *cptr, aClient *sptr, aChannel *chptr, char *text, int notice)
{

	if (chptr->mode.extmode && NOCAPS_BLOCK)
	{
	    int contor;
	    unsigned short int rezultat=0;
	    for (contor=0 ; contor<=strlen(text) && (!rezultat) ; contor++)
		    if (islower(text[contor]))
			    rezultat=1;
	    if (rezultat==1) return text;
	    else {
		    switch(NOCAPS_ACTION)
		    {
			    case 1 :
				sendto_one(sptr, err_str(ERR_CANNOTSENDTOCHAN), me.name, sptr->name, sptr->name,
				"Messages with all letters in caps are not permitted in this channel",
				chptr->chname);
				return NULL;
				break;
			    case 0 :
				for (contor=0 ; contor<=strlen(text) ; contor++)
				    if (isupper(text[contor]))
					text[contor]=tolower(text[contor]);
				sendto_one(sptr, err_str(ERR_CANNOTSENDTOCHAN), me.name, sptr->name, sptr->name,
				"Your message to this channel was converted to lowercase",
				chptr->chname);
				return text;
				break;
			    default :
				return text;
				break;
			    
		    }
		    
		}
	}
}
