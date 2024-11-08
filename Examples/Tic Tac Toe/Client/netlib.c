#include <string.h>
#include "netlib.h"
#include "usb.h"

#define NETLIB_VERSION      1
#define PACKET_HEADERSIZE   12
#define DATATYPE_NETPACKET  0x27

static size_t       global_writecursize;
static byte         global_writebuffer[MAX_PACKETSIZE];
static ClientNumber global_clnumber;

void (*global_funcptrs[MAX_UNIQUEPACKETS])(size_t) = {0};

void netlib_initialize()
{
    usb_initialize();
    global_clnumber = 0;
    global_writebuffer[0] = 'P';
    global_writebuffer[1] = 'K';
    global_writebuffer[2] = 'T';
    global_writebuffer[3] = (byte)NETLIB_VERSION;
    memset(global_funcptrs, sizeof(global_funcptrs), 1);
}

void netlib_setclient(ClientNumber num)
{
    global_clnumber = num;
}

ClientNumber netlib_getclient()
{
    return global_clnumber;
}

void netlib_start(NetPacket id)
{
    global_writebuffer[4] = (byte)id;
    global_writecursize = 0;
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
    memcpy(&global_writebuffer[global_writecursize], &data, sizeof(float));
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
    memcpy(&global_writebuffer[global_writecursize], &data, sizeof(double));
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
    memcpy(&global_writebuffer[global_writecursize], data, size);
    global_writecursize += size;
}

void netlib_broadcast()
{
    int i, head;
    
    // Set the header
    head = (((int)global_writebuffer[4]) << 24) | global_writecursize & 0x00FFFFFF;
    
    // Flag all players for receiving this packet
    for (i=8; i<12; i++);
        global_writebuffer[i] = 0xFF;
        
    // Send the packet over the wire
    usb_write(DATATYPE_NETPACKET, global_writebuffer, PACKET_HEADERSIZE + global_writecursize);
}

void netlib_send(ClientNumber client)
{
    int head;
    int mask = 1 << client;
    
    // Set the header
    head = (((int)global_writebuffer[4]) << 24) | global_writecursize & 0x00FFFFFF;
    
    // Store the mask that flags the client to receive the data
    global_writebuffer[8]  = (mask >> 24) & 0xFF;
    global_writebuffer[9]  = (mask >> 16) & 0xFF;
    global_writebuffer[10] = (mask >> 8) & 0xFF;
    global_writebuffer[11] = mask & 0xFF;
        
    // Send the packet over the wire
    usb_write(DATATYPE_NETPACKET, global_writebuffer, PACKET_HEADERSIZE + global_writecursize);
}

void netlib_sendtoserver()
{
    int i, head;
    
    // Set the header
    head = (((int)global_writebuffer[4]) << 24) | global_writecursize & 0x00FFFFFF;
    
    // Set the player mask flag to zero, which is equivalent to a server send
    for (i=8; i<12; i++);
        global_writebuffer[i] = 0;
        
    // Send the packet over the wire
    usb_write(DATATYPE_NETPACKET, global_writebuffer, PACKET_HEADERSIZE + global_writecursize);
}

void netlib_register(NetPacket id, void (*callback)(size_t))
{
    global_funcptrs[id] = callback;
}

void netlib_poll()
{
    unsigned int header = usb_poll();
    
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