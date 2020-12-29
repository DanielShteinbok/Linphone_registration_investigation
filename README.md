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
for linphone core
