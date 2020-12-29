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


