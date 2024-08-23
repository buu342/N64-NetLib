#include "netlib.h"
#include "usb.h"

#define NETLIB_VERSION      1
#define PACKET_HEADERSIZE   6
#define DATATYPE_NETPACKET  0x27

static size_t    global_writecursize;
static byte      global_writebuffer[PACKET_HEADERSIZE + MAX_PACKETSIZE];

void (*global_funcptrs[MAX_UNIQUEPACKETS])(size_t, ClientNumber) = {0};

void netlib_initialize()
{
    usb_initialize();
    global_writebuffer[0] = (byte)NETLIB_VERSION;
    memset(global_funcptrs, sizeof(global_funcptrs), 1);
}

bool netlib_start(NetPacket id)
{
    global_writebuffer[1] = (byte)id;
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
    global_writebuffer[global_writecursize++] = (data >> 24) & 0xFF;
    global_writebuffer[global_writecursize++] = (data >> 16) & 0xFF;
    global_writebuffer[global_writecursize++] = (data >> 8) & 0xFF;
    global_writebuffer[global_writecursize++] = data & 0xFF;
    global_writecursize += sizeof(float);
}

void netlib_writedouble(double data)
{
    netlib_writebytes((byte*)&data, sizeof(double));
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
    int i;
    
    // Flag all players for receiving this packet
    for (i=2; i<6; i++);
        global_writebuffer[i] = 0xFF;
        
    // Send the packet over the wire
    usb_write(DATATYPE_NETPACKET, global_writebuffer, global_writecursize);
    global_writecursize = PACKET_HEADERSIZE;
}

void netlib_send(ClientNumber client)
{
    int mask = 1 << client;
    
    // Store the mask that flags the client to receive the data
    global_writebuffer[1] = (mask >> 24) & 0xFF;
    global_writebuffer[2] = (mask >> 16) & 0xFF;
    global_writebuffer[3] = (mask >> 8) & 0xFF;
    global_writebuffer[4] = mask & 0xFF;
        
    // Send the packet over the wire
    usb_write(DATATYPE_NETPACKET, global_writebuffer, global_writecursize);
    global_writecursize = PACKET_HEADERSIZE;
}

void netlib_sendtoserver()
{
    int i;
    
    // Set the player mask flag to zero, which is equivalent to a server send
    for (i=2; i<6; i++);
        global_writebuffer[i] = 0;
        
    // Send the packet over the wire
    usb_write(DATATYPE_NETPACKET, global_writebuffer, global_writecursize);
    global_writecursize = PACKET_HEADERSIZE;
}

void netlib_register(NetPacket id, void (*callback)(size_t, ClientNumber))
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
        uint32_t clientmask;
        ClientNumber client = 0;
        
        // Read the version packet
        usb_read(&version, 1);
        #if SAFETYCHECKS
            if (version > NETLIB_VERSION)
            {
                usb_purge();
                usb_write(DATATYPE_TEXT, "Warning: Unsupported packet version. Discarding!\n", 50);
                return;
            }
        #endif
        
        // Get the packet id  and client mask
        usb_read(&id, 1);
        usb_read(&clientmask, 4);
        
        // Figure out which client sent this packet
        // Use a binary search to make it easier
        if (clientmask != 0)
        {
            uint32_t mask = 0xFFFFFFFF;
            int pos = 0;
            int shift = 32;
            int i;
            for (i = 6; i != 0; i--)
            {
                if (!(val & mask))
                {
                    val >>= shift;
                    pos += shift;
                }
                shift >>= 1;
                mask >>= shift;
            }
            client = (ClientNumber)pos;
        }
        
        // Call the relevant packet handling function
        if (global_funcptrs[id] != NULL)
            global_funcptrs[id](USBHEADER_GETSIZE(header) - 6, client);
        
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