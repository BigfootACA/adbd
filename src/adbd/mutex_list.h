#ifndef ADB_MUTEX
#error ADB_MUTEX not defined when including this file
#endif
ADB_MUTEX(dns_lock)
ADB_MUTEX(socket_list_lock)
ADB_MUTEX(transport_lock)
ADB_MUTEX(usb_lock)
ADB_MUTEX(D_lock)
#undef ADB_MUTEX
