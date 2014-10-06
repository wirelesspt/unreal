/*
 * m_securequery.c, v0.0.3 by Stealth (stealth@x-tab.org)
 *
 * This module provides the user mode Z. This user mode will allow
 * only secure (SSL) users to query or notice you..
 *
 * Changelog:
 *  0.0.3
 *    - Disallow messages from +Z users to non-SSL users.
 *    - Changed message not sent notice to include notices.
 *    - Changed message not sent notice to say "non-SSL" instead
 *      of "insecure".
 *
 *  0.0.2
 *    - Code cleanups.
 *    - Only SSL users should be able to set the mode.
 *
 *  0.0.1
 *    - Initial release.
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#include "config.h"
#include "struct.h"
#include "common.h"
#include "sys.h"
#include "numeric.h"
#include "msg.h"
#include "channel.h"
#include "h.h"

#ifdef _WIN32
#include <io.h>
#include "version.h"
#endif

#ifdef STRIPBADWORDS
#include "badwords.h"
#endif

#ifndef DYNAMIC_LINKING
ModuleHeader m_securequery_Header
#else
#define m_securequery_Header Mod_Header
ModuleHeader Mod_Header
#endif
  = {
	"m_securequery",
	"v0.0.3",
	"Secure Query (user mode Z) by Stealth",
	"3.2-b8-1",
	NULL
    };

int umode_allow_secure(aClient *sptr, int what)
{
        if (MyClient(sptr))
                if (sptr->umodes & UMODE_SECURE) return 1; else return 0;
        else
                return 1;
}

static long 	UMODE_SECUREQUERY 	= 0;
static Umode 	*UmodeSecureQuery 	= NULL;
static Hook 	*CheckMsg;

DLLFUNC char *securequery_checkmsg(aClient *, aClient *, aClient *, char *, int);

DLLFUNC int MOD_INIT(m_securequery)(ModuleInfo *modinfo)
{
	UmodeSecureQuery = UmodeAdd(modinfo->handle, 'Z', UMODE_GLOBAL, umode_allow_secure, &UMODE_SECUREQUERY);
	if (!UmodeSecureQuery) {
		config_error("m_securequery: Could not add usermode 'Z': %s", ModuleGetErrorStr(modinfo->handle));
		return MOD_FAILED;
	}

	CheckMsg = HookAddPCharEx(modinfo->handle, HOOKTYPE_USERMSG, securequery_checkmsg);

	ModuleSetOptions(modinfo->handle, MOD_OPT_PERM);

	return MOD_SUCCESS;
}

DLLFUNC int MOD_LOAD(m_securequery)(int module_load)
{ return MOD_SUCCESS; }

DLLFUNC int MOD_UNLOAD(m_securequery)(int module_unload)
{ return MOD_SUCCESS; }

DLLFUNC char *securequery_checkmsg(aClient *cptr, aClient *sptr, aClient *acptr, char *text, int notice)
{
        if ((acptr->umodes & UMODE_SECUREQUERY) && !IsAnOper(sptr) && !IsULine(sptr) && !IsServer(sptr) && !(sptr->umodes & UMODE_SECURE)) {
                sendnotice(sptr, "Message to '%s' not delivered: User does not accept private messages or notices from non-SSL users. Please visit http://wiki.barafranca.com/index.php/IRC/HOWTO:Connect_using_SSL", acptr->name);
                return NULL;
        } else if ((sptr->umodes & UMODE_SECUREQUERY) && !IsAnOper(acptr) && !IsULine(acptr) && !IsServer(acptr) && !(acptr->umodes & UMODE_SECURE)) {
        sendnotice(sptr, "Message to '%s' not delivered: For your security, your modes do not allow you to send private messages or notices to non-SSL users. If you really want to do this type /mode your_nick -Z and to revert type /mode your_nick +Z", acptr->name);
        return NULL;
    } else return text;
}

