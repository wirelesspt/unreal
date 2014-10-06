/**
 **	This module lets opers see some details about a client's SSL-certificate
 **	.
 **	On load the command /x509 is added which takes a nick as a parameter
 **	whose certificate information should be retrieved:
 **
 **	/X509 <nick>
 **
 **	Changelog:
 **
 **	Version 0.1
 **		- Initial release
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

#define MSG_X509	"X509"
#define TOK_X509	"XS"

static int	 m_X509(aClient *, aClient *, int, char *[]);
static void x509_print(aClient*, X509_NAME_ENTRY*);
static Command* cmdX509 = NULL;

ModuleHeader MOD_HEADER(m_X509)
  = {
	"m_X509",
	"$Id: m_X509.c,v 0.1 2007/01/25 cloud Exp $",
	"Shows client's certificate details",
	"3.2-b8-1",
	NULL 
    };

DLLFUNC int MOD_TEST(m_X509)(ModuleInfo *modinfo)
{
	return MOD_SUCCESS;
}

DLLFUNC int MOD_INIT(m_X509)(ModuleInfo *modinfo)
{
	if (CommandExists(MSG_X509))
    	{
		config_error("Command %s already exists", MSG_X509);
		return MOD_FAILED;
    	}
    	if (CommandExists(TOK_X509))
	{
		config_error("Token %s already exists", TOK_X509);
		return MOD_FAILED;
    	}

	cmdX509 = CommandAdd(modinfo->handle, MSG_X509, TOK_X509, m_X509, MAXPARA, 0);

	if (!cmdX509 || (ModuleGetError(modinfo->handle) != MODERR_NOERROR)) {
		config_error("Error while loading module.");
		return MOD_FAILED;
	}

	return MOD_SUCCESS;
}

DLLFUNC int MOD_LOAD(m_X509)(int module_load)
{
	return MOD_SUCCESS;
}

DLLFUNC int MOD_UNLOAD(m_X509)(int module_unload)
{
	if (cmdX509) {
		CommandDel(cmdX509);
		cmdX509 = NULL;
	}

	return MOD_SUCCESS;
}

void x509_print(aClient* sptr, X509_NAME_ENTRY* xsne) {
	char* buf = NULL;
	int size = 0;

	buf = (char*)malloc(1);
	size = OBJ_obj2txt(buf, 1, xsne->object, 0);
	free(buf);

	buf = (char*)malloc(size + 1);
	size = OBJ_obj2txt(buf, size + 1, xsne->object, 0);
	buf[size] = '\0';
	sendto_one(sptr, ":%s %s %s :*** %s: %s", me.name, IsWebTV(sptr) ? "PRIVMSG" : "NOTICE", sptr->name, buf, (char*)xsne->value->data);
	if (buf) {
		free(buf);
		buf = NULL;
	}
}

int m_X509(aClient *cptr, aClient *sptr, int parc, char *parv[]) {
	char* nick = NULL;
	aClient* acptr;
	SSL* ssl = NULL;
	X509* x509 = NULL;
	X509_NAME *xsn = NULL;
	X509_NAME_ENTRY* xsne = NULL;
	int i;

	if ((parc > 1) && (IsServer(cptr) || IsOper(sptr)) && (hunt_server_token(cptr, sptr, MSG_X509, TOK_X509, "%s", 1, parc, parv) != HUNTED_ISME)) {
		return 0;
	}

	if (IsPerson(sptr)) {
		if (!IsOper(sptr)) {
			sendto_one(sptr, err_str(ERR_NOPRIVILEGES), me.name, parv[0]);
			return -1;
		}
		if (!MyConnect(sptr) && !IsServer(cptr)) {
			return 0;
		}
	}

	nick = (parc > 1 && !BadPtr(parv[1])) ? parv[1] : NULL;
	if (!nick) {
		sendto_one(sptr, err_str(ERR_NEEDMOREPARAMS), me.name, parv[0], "X509");
		return -1;
	}

	acptr = find_person(nick, NULL);
	if (!acptr) {
		sendto_one(sptr, err_str(ERR_NOSUCHNICK), me.name, sptr->name, nick);
		return -1;
	}

	if (acptr->ssl) {
		ssl = (SSL*)acptr->ssl;
	}
	if (!ssl) {
		sendto_one(sptr, ":%s %s %s :*** Couldn't retrieve SSL/TLS structure of %s.", me.name, IsWebTV(sptr) ? "PRIVMSG" : "NOTICE", sptr->name, acptr->name);
		return -1;
	}

	x509 = SSL_get_peer_certificate(ssl);

	if (!x509) {
		sendto_one(sptr, ":%s %s %s :*** Couldn't retrieve certificate of %s.", me.name, IsWebTV(sptr) ? "PRIVMSG" : "NOTICE", sptr->name, acptr->name);
		return -1;
	}

	xsn = X509_get_subject_name(x509);
	if (!xsn) {
		sendto_one(sptr, ":%s %s %s :*** Couldn't retrieve certificate's subject name of %s.", me.name, IsWebTV(sptr) ? "PRIVMSG" : "NOTICE", sptr->name, acptr->name);
		return -1;
	}

	/* Error checking done.. */
	sendto_one(sptr, ":%s %s %s :*** %s is connected with %s / %s, %d bits.", me.name, IsWebTV(sptr) ? "PRIVMSG" : "NOTICE", sptr->name, acptr->name,
				SSL_get_cipher_version(ssl),
				SSL_get_cipher(ssl),
				SSL_get_cipher_bits(ssl, NULL)
			);

	if (sk_X509_NAME_ENTRY_num(xsn->entries) < 1) {
		sendto_one(sptr, ":%s %s %s :*** Couldn't retrieve more certificate information from %s.", me.name, IsWebTV(sptr) ? "PRIVMSG" : "NOTICE", sptr->name, acptr->name);
		return -1;
	}

	for (i = 0; i < sk_X509_NAME_ENTRY_num(xsn->entries); i++) {
		xsne = sk_X509_NAME_ENTRY_value(xsn->entries, i);
		x509_print(sptr, xsne);
	}
}
