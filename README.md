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
LinphoneTransports object is created, configured to set the UDP port to 0 (disable it) and the tcp port to 5060.
This LinphoneTransports struct is then set as the transport for the LinphoneCore being used.
It is confirmed that the LinphoneCore's port settings are as intended. A LinphoneProxyConfig is created and set
for linphone core. The LinphoneProxyConfig's transports are checked, returns UDP (before setting the domain,
returns null. After setting the domain (calling `linphone_proxy_config_set_server_addr()`)), returns UDP. Gives an error:
```
liblinphone-error-Cannot guess transport for proxy with identity [sip:daniel@parsedata.xyz]
```
.
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

`linphone_proxy_config_get_transport()` is defined in 
(see commit 3297406e717b130e149e8e458c14fefeb6389430).
