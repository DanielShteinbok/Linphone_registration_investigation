# Use of Linphone (in C):
## The LinphoneFactory:
This is a typedef for the struct \_LinphoneFactory, which is a singleton
used to create a lot of other stuff (including the LinphoneCore, LinphoneCoreCbs,
and LinphoneTransports).

Create with:

```c
LinphoneFactory* linphone_factory_get();
```

The above returns the LinphoneFactory that was created.

This LinphoneFactory is often absent in outdated linphone example code, probably
because it was added recently. Many "constructor"-like functions that create
the various structs we use do not use LinphoneFactory, but are deprecated and
have been replaced by functions that do use LinphoneFactory (examples below).

LinphoneFactory can supposedly be used to change things like logging behaviour.

**TODO:** describe these LinphoneFactory behaviours more in-depth.

## The LinphoneCoreCbs:
This is a typedef for the struct \_LinphoneCbs, which is used for managing
on callbacks. It is the replacement for LinphoneCoreVTable used in older linphone examples.

Create from the factory with:

```c
LinphoneCoreCbs* linphone_factory_create_core_cbs(factory);
```

There are various functions that start with
```c
linphone_core_cbs_set
```
that allow the programmer to pass a callback function that will be called when a certain event occurs.
For example, in the registration program there is a call to

```c
linphone_core_cbs_set_registration_state_changed()
```

## The LinphoneProxyConfig:
This is a real pain to deal with. It seems that the underlying struct here is used to manage the individual
user's registration. More information can be found [here](https://linphone.org/snapshots/docs/liblinphone/4.5.0/c/group__proxies.html).

The thing that is primarily painful about this is setting things like transports. There is a LinphoneTransports typedef to
a struct, which is supposedly used to configure the transports that the LinphoneCore will use (after the LinphoneTransports
is configured and set with [`linphone_core_set_transports()`](https://linphone.org/snapshots/docs/liblinphone/4.5.0/c/group__network__parameters.html#gaf1d2c470e683d5ef6aa1936f1b65ac89)

However, when a LinphoneProxyConfig is created, it seems that it totally ignores what has been set by LinphoneTransports;
instead, it decides that the transports being used are UDP. if you like detective stories (or are interested in solving this
mystery) keep reading below:

### The Mystery of the Transport Method
<details>
	<summary>Click here to read about the investigation</summary>

LinphoneTransports struct is created, configured to set the UDP port to 0 (disable it) and the tcp port to 5060.
This LinphoneTransports struct is then set as the transport for the LinphoneCore being used.

It is confirmed that the LinphoneCore's port settings are as intended. 

A LinphoneProxyConfig is created and set for linphone core.

The LinphoneProxyConfig's transports are checked, returns UDP (before setting the domain, returns null. 
After setting the domain (calling `linphone_proxy_config_set_server_addr()`)), returns UDP. Gives an error:
`liblinphone-error-Cannot guess transport for proxy with identity [sip:daniel@parsedata.xyz]`.

Here is a part of the code. "Part" is a key word here; there is other authentication stuff that happens with the proxy config that
isn't relevant to the transports stuff, so it was omitted. The code will not run.

```c
LinphoneFactory *factory = linphone_factory_get();
LinphoneTransports *transports = linphone_factory_create_transports(factory);
linphone_transports_set_udp_port(transports, LC_SIP_TRANSPORT_DISABLED);
// LC_SIP_TRANSPORT_DISABLED is a preprocessor macro for 0
linphone_transports_set_tcp_port(transports, 5060);

LinphoneProxyConfig* proxy_cfg;	
LinphoneAddress *from;
LinphoneAuthInfo *info;
	
char* identity="sip:daniel@parsedata.xyz";
char* password="123456";
// in the code, both of the above are entered as command line arguments, but hardcoded here for simplicity

LinphoneCore *lc = linphone_factory_create_core_3(factory, NULL, NULL, NULL);

printf("core's transports: tcp:%i, udp:%i\n", linphone_transports_get_tcp_port(linphone_core_get_transports_used(lc)), 
linphone_transports_get_udp_port(linphone_core_get_transports_used(lc)));
// The above prints: core's transports: tcp:5060, udp:0
// evidently the LinphoneCore is set to use tcp and not udp

proxy_cfg = linphone_core_create_proxy_config(lc);
printf("proxy transport: %s \n", linphone_proxy_config_get_transport(proxy_cfg));
// prints: proxy transport: (null)

from = linphone_address_new(identity);
linphone_proxy_config_set_identity_address(proxy_cfg,from); /*set identity with user name and domain*/
// linphone_proxy_config_get_transport(proxy_cfg) would still return null

linphone_proxy_config_set_server_addr(proxy_cfg,server_addr); /* we assume domain = proxy server address*/
// AHAH! linphone_proxy_config_get_transport(proxy_cfg) returns "udp"

linphone_proxy_config_enable_register(proxy_cfg, TRUE); /*activate registration for this proxy config*/

linphone_core_add_proxy_config(lc,proxy_cfg); /*IMPORTANT PART: add proxy config to linphone core*/
linphone_core_set_default_proxy_config(lc,proxy_cfg); /*set to default proxy*/

printf("core's transports after adding proxy_config: tcp:%i, udp:%i\n", linphone_transports_get_tcp_port(linphone_core_get_transports_used(lc)), 
	linphone_transports_get_udp_port(linphone_core_get_transports_used(lc)));
// above prints: core's transports after adding proxy_config: tcp:5060, udp:0

// run in a while loop to maintain registration in the code, but put here once so you get the point
linphone_core_iterate(lc); /* first iterate initiates registration */
```

see commit 3297406e717b130e149e8e458c14fefeb6389430 for the original code (in registration.c).

The important things in the above are that:
* `proxy_cfg`'s transports are not set to anything until the server address is set
* Afterwards, `proxy_cfg`'s transports are set to udp
* `linphone_core_set_transports` changes the transport method used by the LinphoneCore, but doesn't seem to impact the proxy
* The LinphoneCore's transport settings stay different from the proxy even after the proxy is set; at the time of writing the
question remains as to which of the two linphone follows; it appears to be UDP given that the program doesn't register

It is the case that there is a `linphone_proxy_config_get_transport()` function, but there is no equivalent for setting it.
In fact, there seems to be no function at all to set the transport method for the LinphoneProxyConfig struct!

**The big question: how does one set the transport method for the LinphoneProxyConfig struct?**

To answer this, I began by trying to figure out how the transport method is actually stored.
To figure _that_ out, I looked at the code for `linphone_proxy_config_get_transport()`.

`linphone_proxy_config_get_transport()` is defined in [liblinphone/coreapi/proxy.c](https://github.com/BelledonneCommunications/liblinphone/blob/master/coreapi/proxy.c)
as:
```c
const char* linphone_proxy_config_get_transport(const LinphoneProxyConfig *cfg) {
	const char* addr=NULL;
	const char* ret="udp"; /*default value*/
	const SalAddress* route_addr=NULL;
	bool_t destroy_route_addr = FALSE;

	if (linphone_proxy_config_get_service_route(cfg)) {
		route_addr = L_GET_CPP_PTR_FROM_C_OBJECT(linphone_proxy_config_get_service_route(cfg))->getInternalAddress();
	} else if (linphone_proxy_config_get_route(cfg)) {
		addr=linphone_proxy_config_get_route(cfg);
	} else if(linphone_proxy_config_get_addr(cfg)) {
		addr=linphone_proxy_config_get_addr(cfg);
	} else {
		ms_error("Cannot guess transport for proxy with identity [%s]", cfg->reg_identity);
		return NULL;
	}

	if (!route_addr) {
		if (!((*(SalAddress **)&route_addr) = sal_address_new(addr)))
			return NULL;
		destroy_route_addr = TRUE;
	}

	ret=sal_transport_to_string(sal_address_get_transport(route_addr));
	if (destroy_route_addr)
		sal_address_unref((SalAddress *)route_addr);

	return ret;
}
```
This explains the first and second points above; if `linphone_proxy_config_get_route(cfg)` returns something
that isn't NULL or null or false (which would probably be/evidently is the case after `linphone_proxy_config_set_server_addr()`
is called. However, since `route_addr` is only changed if `linphone_proxy_config_get_service_route(cfg)` evaluates
to true, and `sal_address_get_transport` (defined in [liblinphone/coreapi/bellesip\_sal/sal\_address\_impl.c](https://github.com/BelledonneCommunications/liblinphone/blob/master/coreapi/bellesip_sal/sal_address_impl.c)
as returning `SalTransportUDP` if the address passed is null, `linphone_proxy_config_get_transport` could only return something
non-NULL, non-udp if `linphone_proxy_config_get_service_route(cfg)` returns something that evaluates to true. When the `server_addr`
was not set, the function returned NULL and threw the error descibed above, but after the `server_addr` was set it started to directly
return udp.

`linphone_proxy_config_get_service_route` is defined in [liblinphone/src/sal/op.h](https://github.com/BelledonneCommunications/liblinphone/blob/master/src/sal/op.h) as simply returning `mServiceRoute`

`mServiceRoute` is changed in `SalOp::setServiceRoute` in [liblinphone/src/sal/op.cpp](https://github.com/BelledonneCommunications/liblinphone/blob/master/src/sal/op.cpp)

`SalOp::setServiceRoute` is called from `SalRegisterOp::registerRefresherListener` in [liblinphone/src/sal/register-op.cpp](https://github.com/BelledonneCommunications/liblinphone/blob/master/src/sal/register-op.cpp) if the third argument is equal to `200`.

`SalRegisterOp::registerRefresherListener` is passed as the last argument (`listener`) to `SalOp::sendRequestAndCreateRefresher()` 
(in [liblinphone/src/sal/op.cpp](https://github.com/BelledonneCommunications/liblinphone/blob/master/src/sal/op.cpp))
by `SalRegisterOp::sendRegister` (in [liblinphone/src/sal/op.cpp](https://github.com/BelledonneCommunications/liblinphone/blob/master/src/sal/register-op.cpp)

`SalOp::sendRequestAndCreateRefresher` (on line 690) passes `listener` as the second argument to `belle_sip_refresher_set_listener(mRefresher, listener, this)`

`belle_sip_refresher_set_listener` is defined in [belle-sip/src/refresher.c](https://github.com/BelledonneCommunications/belle-sip/blob/master/src/refresher.c) as:

```c
void belle_sip_refresher_set_listener(belle_sip_refresher_t* refresher, belle_sip_refresher_listener_t listener,void* user_pointer) {
	refresher->listener=listener;
	refresher->user_data=user_pointer;
}
```
On a different note, there is a `linphone_proxy_config_set_routes()` defined in [liblinphone/coreapi/proxy.c](https://github.com/BelledonneCommunications/liblinphone/blob/master/coreapi/proxy.c),
documented in 
[the Proxies module of the documentation](https://linphone.org/snapshots/docs/liblinphone/4.5.0/c/group__proxies.html#ga46f8e03a3fa4e408209f8438639376c0),
which is defined as follows:
```c
LinphoneStatus linphone_proxy_config_set_routes(LinphoneProxyConfig *cfg, const bctbx_list_t *routes) {
	if (cfg->reg_routes != NULL) {
		bctbx_list_free_with_data(cfg->reg_routes, ms_free);
		cfg->reg_routes = NULL;
	}
	bctbx_list_t *iterator = (bctbx_list_t *)routes;
	while (iterator != NULL) {
		char *route = (char *)bctbx_list_get_data(iterator);
		if (route != NULL && route[0] !='\0') {
			SalAddress *addr;
			char *tmp;
			/*try to prepend 'sip:' */
			if (strstr(route,"sip:") == NULL && strstr(route,"sips:") == NULL) {
				tmp = ms_strdup_printf("sip:%s",route);
			} else {
				tmp = ms_strdup(route);
			}
			addr = sal_address_new(tmp);
			if (addr != NULL) {
				sal_address_unref(addr);
				cfg->reg_routes = bctbx_list_append(cfg->reg_routes, tmp);
			} else {
				ms_free(tmp);
				return -1;
			}
		}
		iterator = bctbx_list_next(iterator);
	}
	return 0;
}
```

Unfortunately, there is no documentation on how to specify the `routes` etc, so using this will require a bit more digging.
Additionally, I have no idea as to whether or not this would solve the issue at hand at all.

**Remaining questions:**
* How do the transport settings of the LinphoneCore and LinphoneProxyConfig relate? Which is used when registering?
* How does one set the transport method of the LinphoneProxyConfig to TCP/whatever instead of UDP?
* What does the `linphone_proxy_config_set_routes` function have to do with the transport method? How does one call this function (what is `routes`)?

</details>

## Compilation and Linking
I built everything with CMake, using the CMakeLists.txt in this project. However, it will probably not work for you!
Opening it up, you will see that I added four libraries, by shared objects (I am using SO files on linux, if you are
using archives/static libraries you will probably have to go about this differently):
* linphone
* mediastreamer
* ortp
* bctoolbox

All of the above seem to be necessary for this program, since I have experimentally seen that the linker throws
errors if not all of them are included. The other libraries included in the SDK will probably be needed for more
complicated programs.

I then set the `IMPORTED_LOCATION` property for all of them to the location of the SO file on my computer.
You will probably need to change this.

I linked all these libraries

I included the entire `include/` directory that was created when I built the SDK with CMake, this is wherever
the header files end up (obviously, it should include `linphone/core.h`).

A word of caution: I don't know that this is actually the "correct" way of doing this; it does work, but seems
a bit "hacky", and I haven't found a better way of doing this in any linphone documentation.
