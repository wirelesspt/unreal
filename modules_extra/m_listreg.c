/*
**********************************************************************
*   Module:                m_listreg.c
*   Author:                craftsman <craftsman@sinoptic.net>
*   Description:	   command /list only nicknames registered
*  
*  INSTALL
* copy m_listreg src/modules
* make custommodule MODULEFILE=m_listreg
*
* unrealircd.conf
* loadmodule "src/modules/m_listreg.so";
*
* IRC
* //REHASH
*
* READY !
*
*
***********************************************************************
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

Cmdoverride *listreg_override = NULL;

ModuleInfo *m_listreg_modinfo = NULL;

DLLFUNC int listreg(Cmdoverride *anoverride, aClient *cptr, aClient *sptr, int parc, char *parv[]);

#define MSG_LIST  "LIST"

ModuleHeader MOD_HEADER(m_listreg)
  = {
	"m_listreg",	/* Name of module */
	"1.0", /* Version */
	"command /list only nicknames registered <craftsman@sinoptic.net>", /* Descripcion */
	"3.2-b8-1",
	NULL 
    };

DLLFUNC int MOD_INIT(m_listreg)(ModuleInfo *modinfo)
{
	m_listreg_modinfo = modinfo;
	return MOD_SUCCESS;
}

DLLFUNC int MOD_LOAD(m_listreg)(int module_load)
{
	listreg_override = CmdoverrideAdd(m_listreg_modinfo->handle, MSG_LIST, listreg);
	if (!listreg_override)
	{
		sendto_realops("m_listreg: Failed to allocate override: %s", ModuleGetErrorStr(m_listreg_modinfo->handle));
		return MOD_FAILED;
	}
	return MOD_SUCCESS;
}

DLLFUNC int MOD_UNLOAD(m_listreg)(int module_unload)
{
	CmdoverrideDel(listreg_override);


	return MOD_SUCCESS;
}

DLLFUNC int listreg(Cmdoverride *anoverride, aClient *cptr, aClient *sptr, int parc, char *parv[])
{
	if (!IsAnOper(sptr))
	{
		if (!IsARegNick(sptr))
		{

			sendto_one(sptr, ":%s %s %s :in order to use this command to need to register your nickname", me.name, IsWebTV(sptr) ? "PRIVMSG" : "NOTICE", sptr->name);
				
			sendto_one(sptr, rpl_str(RPL_LISTSTART), me.name, sptr->name);
			sendto_one(sptr, rpl_str(RPL_LISTEND), me.name, sptr->name);
			return 0;
		}
	}


	    return CallCmdoverride(listreg_override, cptr, sptr, parc, parv);
}
