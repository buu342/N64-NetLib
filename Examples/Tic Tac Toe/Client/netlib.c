#include <string.h>
#include "netlib.h"
#include "usb.h"

#define NETLIB_VERSION      1
#define PACKET_HEADERSIZE   12
#define DATATYPE_NETPACKET  0x27

static volatile size_t global_writecursize;
static volatile byte   global_writebuffer[MAX_PACKETSIZE];
static volatile ClientNumber global_clnumber;
static vu8  global_polling;
static vu8  global_sendafterpoll;
static vu8  global_disconnected;
static void (*global_funcptr_disconnect)();
static void (*global_funcptr_reconnect)();
static void (*global_funcptrs[MAX_UNIQUEPACKETS])(size_t) = {0};

static volatile void* memcpy_v(volatile void* dest, const volatile void* src, size_t n)
{
    const volatile unsigned char* src_c = src;
    volatile unsigned char* dest_c      = dest;
    while (n > 0)
    {
        n--;
        dest_c[n] = src_c[n];
    }
    return dest;
}

void netlib_initialize()
{
    usb_initialize();
    global_clnumber = 0;
    global_writebuffer[0] = 'P';
    global_writebuffer[1] = 'K';
    global_writebuffer[2] = 'T';
    global_writebuffer[3] = (byte)NETLIB_VERSION;
    memset(global_funcptrs, sizeof(global_funcptrs), 1);
    global_polling = FALSE;
    global_sendafterpoll = FALSE;
    global_disconnected = FALSE;
    global_funcptr_disconnect = NULL;
    global_funcptr_reconnect = NULL;
}

void netlib_setclient(ClientNumber num)
{
    global_clnumber = num;
}

ClientNumber netlib_getclient()
{
    return global_clnumber;
}
    
void netlib_callback_disconnect(void (*callback)())
{
    global_funcptr_disconnect = callback;
}

void netlib_callback_reconnect(void (*callback)())
{
    global_funcptr_reconnect = callback;
}

void netlib_start(NetPacket id)
{
    global_writebuffer[4] = (byte)id;
    global_writecursize = PACKET_HEADERSIZE;
}

void netlib_writebyte(uint8_t data)
{
    #if SAFETYCHECKS
        if (global_writecursize + sizeof(uint8_t) > MAX_PACKETSIZE)
        {
            usb_write(DATATYPE_TEXT, "Warning: Writing more data than max packet size. Discarded!\n", 61);
            return;
        }
    #endif
    
    // N64 is also big endian, so data already respects Network Byte Order :D
    global_writebuffer[global_writecursize++] = (byte)data;
}

void netlib_writeword(uint16_t data)
{
    #if SAFETYCHECKS
        if (global_writecursize + sizeof(uint16_t) > MAX_PACKETSIZE)
        {
            usb_write(DATATYPE_TEXT, "Warning: Writing more data than max packet size. Discarded!\n", 61);
            return;
        }
    #endif
    
    // N64 is also big endian, so data already respects Network Byte Order :D
    global_writebuffer[global_writecursize++] = (data >> 8) & 0xFF;
    global_writebuffer[global_writecursize++] = data & 0xFF;
}

void netlib_writedword(uint32_t data)
{
    #if SAFETYCHECKS
        if (global_writecursize + sizeof(uint32_t) > MAX_PACKETSIZE)
        {
            usb_write(DATATYPE_TEXT, "Warning: Writing more data than max packet size. Discarded!\n", 61);
            return;
        }
    #endif
    
    // N64 is also big endian, so data already respects Network Byte Order :D
    global_writebuffer[global_writecursize++] = (data >> 24) & 0xFF;
    global_writebuffer[global_writecursize++] = (data >> 16) & 0xFF;
    global_writebuffer[global_writecursize++] = (data >> 8) & 0xFF;
    global_writebuffer[global_writecursize++] = data & 0xFF;
}

void netlib_writefloat(float data)
{
    #if SAFETYCHECKS
        if (global_writecursize + sizeof(float) > MAX_PACKETSIZE)
        {
            usb_write(DATATYPE_TEXT, "Warning: Writing more data than max packet size. Discarded!\n", 61);
            return;
        }
    #endif
    
    // N64 is also big endian, so data already respects Network Byte Order :D
    memcpy_v(&global_writebuffer[global_writecursize], &data, sizeof(float));
    global_writecursize += sizeof(float);
}

void netlib_writedouble(double data)
{
    #if SAFETYCHECKS
        if (global_writecursize + sizeof(double) > MAX_PACKETSIZE)
        {
            usb_write(DATATYPE_TEXT, "Warning: Writing more data than max packet size. Discarded!\n", 61);
            return;
        }
    #endif
    
    // N64 is also big endian, so data already respects Network Byte Order :D
    memcpy_v(&global_writebuffer[global_writecursize], &data, sizeof(double));
    global_writecursize += sizeof(double);
}

void netlib_writebytes(byte* data, size_t size)
{
    #if SAFETYCHECKS
        if (global_writecursize + size > MAX_PACKETSIZE)
        {
            usb_write(DATATYPE_TEXT, "Warning: Writing more data than max packet size. Discarded!\n", 61);
            return;
        }
    #endif
    
    // Write the data to the global buffer
    memcpy_v(&global_writebuffer[global_writecursize], data, size);
    global_writecursize += size;
}

void netlib_broadcast()
{
    int head;
    int mask = 0xFFFFFFFF;
    int datasize = global_writecursize - PACKET_HEADERSIZE;
    
    // Set the header and client mask
    head = (((int)global_writebuffer[4]) << 24) | datasize & 0x00FFFFFF;
    memcpy_v(&global_writebuffer[4], &head, 4);
    memcpy_v(&global_writebuffer[8], &mask, 4);
        
    // Send the packet over the wire
    if (!global_polling)
        usb_write(DATATYPE_NETPACKET, (void*)global_writebuffer, global_writecursize);
    else
        global_sendafterpoll = TRUE;
}

void netlib_send(ClientNumber client)
{
    int head;
    int mask = 1 << client;
    int datasize = global_writecursize - PACKET_HEADERSIZE;
    
    // Set the header and client mask
    head = (((int)global_writebuffer[4]) << 24) | datasize & 0x00FFFFFF;
    memcpy_v(&global_writebuffer[4], &head, 4);
    memcpy_v(&global_writebuffer[8], &mask, 4);
        
    // Send the packet over the wire
    if (!global_polling)
        usb_write(DATATYPE_NETPACKET, (void*)global_writebuffer, global_writecursize);
    else
        global_sendafterpoll = TRUE;
}

void netlib_sendtoserver()
{
    int head;
    int mask = 0; // Zero is a server send
    int datasize = global_writecursize - PACKET_HEADERSIZE;
    
    // Set the header
    head = (((int)global_writebuffer[4]) << 24) | datasize & 0x00FFFFFF;
    memcpy_v(&global_writebuffer[4], &head, 4);
    memcpy_v(&global_writebuffer[8], &mask, 4);
    
    // Send the packet over the wire
    if (!global_polling)
        usb_write(DATATYPE_NETPACKET, (void*)global_writebuffer, global_writecursize);
    else
        global_sendafterpoll = TRUE;
}

void netlib_register(NetPacket id, void (*callback)(size_t))
{
    global_funcptrs[id] = callback;
}

void netlib_poll()
{
    unsigned int header;
    
    // Perform the poll
    global_polling = TRUE;
    header = usb_poll();
    
    // Check the USB did not time out from being disconnected
    // If it did (or reconnected), then execute the callback functions
    if (!global_disconnected && usb_timedout())
    {
        global_disconnected = TRUE;
        if (global_funcptr_disconnect != NULL)
            global_funcptr_disconnect();
    }
    else if (global_disconnected && !usb_timedout())
    {
        global_disconnected = FALSE;
        if (global_funcptr_reconnect != NULL)
            global_funcptr_reconnect();
    }
    
    // Read all net packets
    while (USBHEADER_GETTYPE(header) == DATATYPE_NETPACKET)
    {
        uint8_t version;
        NetPacket id;
        uint32_t headers;
        uint32_t recipients;
        uint32_t size;
        
        // Read the version packet
        usb_read(&headers, 3);
        usb_read(&version, 1);
        #if SAFETYCHECKS
            if (version > NETLIB_VERSION)
            {
                usb_purge();
                usb_write(DATATYPE_TEXT, "Warning: Unsupported packet version. Discarding!\n", 50);
                return;
            }
        #endif
        
        // Get the packet id  and size
        usb_read(&headers, 4);
        id = ((headers) & 0xFF000000) >> 24;
        size = (((headers) & 0x00FFFFFF));
        
        // Get the recepients list
        usb_read(&recipients, 4);
                
        // Call the relevant packet handling function
        #if SAFETYCHECKS
            if (global_funcptrs[id] == NULL)
            {
                usb_purge();
                usb_write(DATATYPE_TEXT, "Warning: Tried calling unregistered function!\n", 47);
                return;
            }
        #endif
        global_funcptrs[id](size);
        
        // Poll again
        usb_purge();
        header = usb_poll();
    }
    
    // If we queued up a message during polling, send it now (if it's safe to do so)
    if (global_sendafterpoll && header == 0)
    {
        usb_write(DATATYPE_NETPACKET, (void*)global_writebuffer, global_writecursize);
        global_sendafterpoll = FALSE;
    }
    global_polling = FALSE;
}

void netlib_readbyte(uint8_t* output)
{
    usb_read(output, sizeof(uint8_t));
}

void netlib_readword(uint16_t* output)
{
    usb_read(output, sizeof(uint16_t));
}

void netlib_readdword(uint32_t* output)
{
    usb_read(output, sizeof(uint32_t));
}

void netlib_readfloat(float* output)
{
    usb_read(output, sizeof(float));
}

void netlib_readdouble(double* output)
{
    usb_read(output, sizeof(double));
}

void netlib_readbytes(byte* output, size_t maxsize)
{
    usb_read(output, maxsize);
}