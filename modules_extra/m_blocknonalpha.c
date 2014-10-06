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

#define DelCmode(x)		if (x) CmodeDel(x); x = NULL
#define DelHook(x)		if (x) HookDel(x); x = NULL

Hook	*HookPreJoin; /* our /join hook */
static int m_myjoin(aClient *, aChannel *, char *[]); /* the called function */
static int alnum_check(const char *s); /* check whether the nick is valid */
Cmode_t EXTCMODE_BLOCK = 0L;
Cmode *ModeBlock = NULL;

ModuleHeader MOD_HEADER(m_blocknonalpha)
  = {
	"m_blocknonalpha",
	"$Id: m_blocknonalpha.c,v 1.0 2004/04/04 Certus Exp $",
	"prevents nicks with non-alphabetic chars from joining a +y channel",
	"3.2-b8-1",
	NULL 
    };

DLLFUNC int MOD_INIT(m_blocknonalpha)(ModuleInfo *modinfo)
{
	CmodeInfo req;
	ModuleSetOptions(modinfo->handle, MOD_OPT_PERM); /* We are PERM */
	ircd_log(LOG_ERROR, "debug: mod_init called from m_blocknonalpha.c");
	sendto_realops("loading m_blocknonalpha");
	memset(&req, 0, sizeof(req));
	req.paracount = 0;
	req.is_ok = extcmode_default_requirehalfop;
	req.flag = 'y';
	ModeBlock = CmodeAdd(modinfo->handle, req, &EXTCMODE_BLOCK);

	HookPreJoin = HookAddEx(modinfo->handle, HOOKTYPE_PRE_LOCAL_JOIN, m_myjoin);
	return MOD_SUCCESS;
}

DLLFUNC int MOD_LOAD(m_blocknonalpha)(int module_load)
{
	return MOD_SUCCESS;
}

DLLFUNC int MOD_UNLOAD(m_blocknonalpha)(int module_unload)
{
	DelCmode(ModeBlock);
	DelHook(HookPreJoin);
	sendto_realops("[m_blocknonalpha] mod_unload called, this shouldn't have happened.");
	return MOD_FAILED;
}

u_int is_invited(aClient *sptr, aChannel *chptr)
{
	Link	*lp;

	for (lp = sptr->user->invited; lp; lp = lp->next)
		if (lp->value.chptr == chptr)
			return 1;

	return 0;
}

static int m_myjoin(aClient *sptr, aChannel *chptr, char *parv[])
{
	if (chptr == NULL) {
		return HOOK_CONTINUE;
	} else if (is_invited(sptr, chptr)) {
		return HOOK_CONTINUE;
	} else {
			if (chptr->mode.extmode & EXTCMODE_BLOCK) {
				if (alnum_check(sptr->name) == 0) {
					if (!IsRegNick(sptr) && !IsULine(sptr) && !IsOper(sptr)) {
						sendto_one(sptr, ":%s NOTICE %s :*** Your nick contains banned characters. Please register to nickserv" \
									" or use a nick with alphabetic letters for being able to join channel %s.", me.name, sptr->name, chptr->chname);
				return HOOK_DENY;
					}
				}
			}
	}
	return HOOK_CONTINUE;
}


/* As the unrealircd dev team changed isalpha(), we have to create our own function */
int charcheck(int c) {
	if ((c == '|') || (c == '\\') || (c == '^') || (c == '-') || (c == '_') || (c == '{') || (c == '}') || (c == '[') || (c == ']'))
		return 0;
	else
		return 1;
}

/* does the string contain a non-alpha character? return 0 if so */
extern int alnum_check(const char *s) {
	register int c;

    while (c = *s) {
		if (!isalnum(c) || !charcheck(c)) {
			return 0;
		}
	s++;
	}
	return 1;
}