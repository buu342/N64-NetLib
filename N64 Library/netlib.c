#include "netlib.h"
#include "usb.h"

static size_t    global_writecursize = 0;
static NetPacket global_curpacketid = 0;
static byte      global_writebuffer[MAX_PACKETSIZE];

static size_t global_curfuncptrs = 0;
void (*global_funcptrs[MAX_UNIQUEPACKETS])(size_t, ClientNumber);

void netlib_initialize()
{
    usb_initialize();
}

bool netlib_start(NetPacket id)
{
    global_curpacketid = id;
    global_writecursize = 0;
}