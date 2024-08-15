#include "netlib.h"
#include "usb.h"

static size_t global_writecursize = 0;
static byte global_writebuffer[MAX_PACKETSIZE];

static size_t global_curfuncptrs = 0;
void (*global_funcptrs[MAX_UNIQUEPACKETS])(size_t, ClientNumber);

void netlib_initialize()
{
    usb_initialize();
}