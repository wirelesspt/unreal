/**
 **  When this module is loaded only secure connections are able to oper up.
 **  I know this doesn't prevent cleartext passwords from being sent when an
 **  oper not using SSL tries to oper up, but it ensures that private messages
 **  between opers are always encrypted.
 **
 **  Change text in `#define NOSSL_ALLOWED ""' to any IP from which
 **  one may oper without using SSL.
 **  E. g. #define NOSSL_ALLOWED "10.10.0.4"
 **
 **  Changelog:
 **  Version 0.1
 **   Initial release
 **  Version 0.2
 **   - Description fixed
 **   - Fixed Typo in year (MOD_HEADER, was 2004)
 **   - Added support for an ip (localhost?) from which one can oper without
 **     using SSL (useful for BOPM or such)
 **  Version 0.3
 **   - Fixed a compiler warning about implicit function declaration (Thx to Bram)
 **   
 **/

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
#include <fcntl.h>
#include "h.h"
#ifdef STRIPBADWORDS
#include "badwords.h"
#endif

#define NOSSL_ALLOWED ""

ModuleInfo*  modinfo_m_soper  = NULL;
Cmdoverride* cmd_soper        = NULL;
DLLFUNC int  m_f_soper(Cmdoverride *ovr, aClient *cptr, aClient *sptr, int parc, char *parv[]);

ModuleHeader MOD_HEADER(m_soper)
  = {
	"m_soper",	/* Name of module */
	"$Id: m_soper, v 0.3 2005/02/09 cloud", /* Version */
	"m_soper - Restricts the use of /oper to secure connections", /* Short description of module */
	"3.2-b8-1",
	NULL 
    };

DLLFUNC int MOD_INIT(m_soper)(ModuleInfo *modinfo)
{
 modinfo_m_soper = modinfo;
 return MOD_SUCCESS;
}

DLLFUNC int MOD_LOAD(m_soper)(int module_load) {
 cmd_soper = CmdoverrideAdd(modinfo_m_soper->handle, "oper", m_f_soper);
 if (NULL == cmd_soper) {
  return MOD_FAILED;
 }

 return MOD_SUCCESS;
}

DLLFUNC int MOD_UNLOAD(m_soper)(int module_unload)
{
 CmdoverrideDel(cmd_soper);
 return MOD_SUCCESS;
}

DLLFUNC int m_f_soper(Cmdoverride *ovr, aClient *cptr, aClient *sptr, int parc, char *parv[]) {
 if (IsServer(sptr)) return 0;

 /* If NOSSL_ALLOWED is not empty and matches the IP of the connecting client
    let it oper.
 */
 if ((NOSSL_ALLOWED[0] != '\0') && GetIP(sptr)) {
   if (!match(NOSSL_ALLOWED, GetIP(sptr))) {
    return CallCmdoverride(ovr, cptr, sptr, parc, parv);
   }
 }

 if (!(sptr->umodes & UMODE_SECURE)) {
  sendto_one(sptr,
             ":%s %s %s :*** The /oper command is restricted to secure connections.", me.name, IsWebTV(sptr) ? "PRIVMSG" : "NOTICE",
             sptr->name);
  return 0;
 }

 return CallCmdoverride(ovr, cptr, sptr, parc, parv);
}
