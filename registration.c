
/*
 * Copyright (c) 2010-2019 Belledonne Communications SARL.
 *
 * This file is part of Liblinphone.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @defgroup registration_tutorials Basic registration
 * @ingroup tutorials
 *This program is a _very_ simple usage example of liblinphone.
 *Desmonstrating how to  initiate a SIP registration from a sip uri identity passed from the command line.
 *first argument must be like sip:jehan@sip.linphone.org , second must be password .
 *<br>
 *ex registration sip:jehan@sip.linphone.org secret
 *<br>Registration is cleared on SIGINT
 *<br>
 *@include registration.c

 *
 */

#include "linphone/core.h"

#include <signal.h>

static bool_t running=TRUE;

static void stop(int signum){
	running=FALSE;
}

/**
 * Registration state notification callback
 */
static void registration_state_changed(struct _LinphoneCore *lc, LinphoneProxyConfig *cfg, LinphoneRegistrationState cstate, const char *message){
		printf("New registration state %s for user id [%s] at proxy [%s]\n"
				,linphone_registration_state_to_string(cstate)
				,linphone_address_get_display_name(linphone_proxy_config_get_identity_address(cfg))
				,linphone_proxy_config_get_addr(cfg));
}

LinphoneCore *lc;
int main(int argc, char *argv[]){
	//LinphoneCoreVTable vtable={0};
	LinphoneFactory *factory = linphone_factory_get();

	// replacement for vtable to hold callbacks
	LinphoneCoreCbs *cbs = linphone_factory_create_core_cbs(factory);

	// transports to hold the configuration for TCP
	LinphoneTransports *transports = linphone_factory_create_transports(factory);
	// adjusting transports configuration
	// a zero value port for a given transport means the transport is not used
	linphone_transports_set_udp_port(transports, LC_SIP_TRANSPORT_DISABLED);
	linphone_transports_set_tcp_port(transports, 5060);

	LinphoneProxyConfig* proxy_cfg;
	LinphoneAddress *from;
	LinphoneAuthInfo *info;

	char* identity=NULL;
	char* password=NULL;
	const char* server_addr;

	/* takes   sip uri  identity from the command line arguments */
	if (argc>1){
		identity=argv[1];
	}

	/* takes   password from the command line arguments */
	if (argc>2){
		password=argv[2];
	}

	signal(SIGINT,stop);

#ifdef DEBUG_LOGS
	linphone_core_enable_logs(NULL); /*enable liblinphone logs.*/
#endif
	/*
	 Fill the LinphoneCoreVTable with application callbacks.
	 All are optional. Here we only use the registration_state_changed callbacks
	 in order to get notifications about the progress of the registration.
	 */
	//vtable.registration_state_changed=registration_state_changed;
	//TODO: set callbacks as above
	linphone_core_cbs_set_registration_state_changed(cbs, registration_state_changed);

	/*
	 Instanciate a LinphoneCore object given the LinphoneCoreVTable
	*/
	//lc=linphone_core_new(&vtable,NULL,NULL,NULL);

	// EDIT: created linphoneCore by new methods
	lc = linphone_factory_create_core_3(factory, NULL, NULL, NULL);

	// add callbacks
	linphone_core_add_callbacks(lc, cbs);

	// add transports
	linphone_core_set_transports(lc, transports);

	// int tcp_port = linphone_core_get_transports_used(lc)->tcp_port;
	// int udp_port = linphone_core_get_transports_used(lc)->udp_port;

	// printf("core's transports used tcp port: %s", tcp_port);
	// printf("core's transports used udp port: %s", udp_port);
	printf("core's transports: %i \n", linphone_transports_get_tcp_port(linphone_core_get_transports_used(lc)));
	// above causes a segmentation fault (???) HYPOTHESIS: %s is for string, so it looks for \0 and goes out of bounds that way

	/*create proxy config*/
	proxy_cfg = linphone_core_create_proxy_config(lc);
	printf("proxy transport: %s \n", linphone_proxy_config_get_transport(proxy_cfg));
	// above is printing (null) for some unknown reason

	/*parse identity*/
	from = linphone_address_new(identity);
	if (from==NULL){
		printf("%s not a valid sip uri, must be like sip:toto@sip.linphone.org \n",identity);
		goto end;
	}
	if (password!=NULL){
		info=linphone_auth_info_new(linphone_address_get_username(from),NULL,password,NULL,NULL,NULL); /*create authentication structure from identity*/
		linphone_core_add_auth_info(lc,info); /*add authentication info to LinphoneCore*/
	}

	// configure proxy entries
	linphone_proxy_config_set_identity_address(proxy_cfg,from); /*set identity with user name and domain*/
	printf("proxy transport after setting identity address: %s \n", linphone_proxy_config_get_transport(proxy_cfg));
	server_addr = linphone_address_get_domain(from); /*extract domain address from identity*/
	linphone_proxy_config_set_server_addr(proxy_cfg,server_addr); /* we assume domain = proxy server address*/
	printf("proxy transport after linphone_proxy_config_set_server_addr: %s \n", linphone_proxy_config_get_transport(proxy_cfg));
	linphone_proxy_config_enable_register(proxy_cfg,TRUE); /*activate registration for this proxy config*/
	printf("proxy transport after linphone_proxy_config_enable_register: %s \n", linphone_proxy_config_get_transport(proxy_cfg));
	linphone_address_unref(from); /*release resource*/

	linphone_core_add_proxy_config(lc,proxy_cfg); /*add proxy config to linphone core*/
	printf("proxy transport after linphone_core_add_proxy_config: %s \n", linphone_proxy_config_get_transport(proxy_cfg));
	linphone_core_set_default_proxy_config(lc,proxy_cfg); /*set to default proxy*/
	printf("proxy transport after linphone_core_set_default_proxy_config: %s \n", linphone_proxy_config_get_transport(proxy_cfg));
	printf("proxy config routes: %s \n", linphone_proxy_config_get_routes(proxy_cfg));

	/* main loop for receiving notifications and doing background linphonecore work: */
	while(running){
		linphone_core_iterate(lc); /* first iterate initiates registration */
		ms_usleep(50000);
		printf("first while loop running\n");
	}

	proxy_cfg = linphone_core_get_default_proxy_config(lc); /* get default proxy config*/
	linphone_proxy_config_edit(proxy_cfg); /*start editing proxy configuration*/
	linphone_proxy_config_enable_register(proxy_cfg,FALSE); /*de-activate registration for this proxy config*/
	linphone_proxy_config_done(proxy_cfg); /*initiate REGISTER with expire = 0*/

	while(linphone_proxy_config_get_state(proxy_cfg) !=  LinphoneRegistrationCleared){
		linphone_core_iterate(lc); /*to make sure we receive call backs before shutting down*/
		ms_usleep(50000);
	}

end:
	printf("Shutting down...\n");
	// DEPRECATED:
	// linphone_core_destroy(lc);

	linphone_core_unref(lc);
	printf("Exited\n");
	return 0;
}

