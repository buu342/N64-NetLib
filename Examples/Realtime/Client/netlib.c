#include <string.h>
#include "netlib.h"
#include "usb.h"


/*********************************
           Definitions
*********************************/

// The version of the NetLib packet supported by this library
#define NETLIB_VERSION      1

// The size of the NetLib packet header (AKA the minimum size of a packet)
#define PACKET_HEADERSIZE   18

// The datatype to use for UNFLoader
#define DATATYPE_NETPACKET  0x27
    
    
/*********************************
             Globals
*********************************/

// Configuration
static u32 global_timeouttime;

// Write buffers
static size_t global_writecursize;
static byte   global_writebuffer[MAX_PACKETSIZE];

// Client info
static ClientNumber global_clnumber;

// Library state
static u64 global_lastpkt;
static u8  global_sendafterpoll;
static u8  global_disconnected;

// Callback functions
static void (*global_funcptr_disconnect)();
static void (*global_funcptr_reconnect)();
static void (*global_funcptrs[MAX_UNIQUEPACKETS])(size_t) = {0};


/*********************************
        Initialization and
      Configuration Functions
*********************************/

/*==============================
    netlib_initialize
    Initializes the NetLib library.
    Also initializes the USB library internally
==============================*/

void netlib_initialize()
{
    int i;
    usb_initialize();
    global_writebuffer[0] = 'N';
    global_writebuffer[1] = 'L';
    global_writebuffer[2] = 'P';
    global_writebuffer[3] = (byte)NETLIB_VERSION;
    for (i=6; i<12; i++)
        global_writebuffer[i] = 0;
    memset(global_funcptrs, sizeof(global_funcptrs), 1);
    global_clnumber = 0;
    global_sendafterpoll = FALSE;
    global_disconnected = FALSE;
    global_funcptr_disconnect = NULL;
    global_funcptr_reconnect = NULL;
    global_lastpkt = 0;
    global_timeouttime = 0;
}


/*==============================
    netlib_setclient
    Sets our own client number
    @param Our client number
==============================*/

void netlib_setclient(ClientNumber num)
{
    global_clnumber = num;
}

/*==============================
    netlib_getclient
    Gets our own client number
    @return Our client number
==============================*/

ClientNumber netlib_getclient()
{
    return global_clnumber;
}

/*==============================
    netlib_callback_disconnect
    Set the callback function for when we disconnect
    @param How many ms without messages for a timeout to happen
    @param A pointer to function to call when we disconnect
==============================*/
    
void netlib_callback_disconnect(u32 timeout, void (*callback)())
{
    #ifndef LIBDRAGON
        global_timeouttime = OS_USEC_TO_CYCLES(timeout*1000);
    #else
        global_timeouttime = TIMER_TICKS(1000000);
    #endif
    global_funcptr_disconnect = callback;
}

/*==============================
    netlib_callback_reconnect
    Set the callback function for when we reconnect
    @param A pointer to function to call when we reconnect
==============================*/

void netlib_callback_reconnect(void (*callback)())
{
    global_funcptr_reconnect = callback;
}
    
    
/*********************************
     N64 -> Network Functions
*********************************/

/*==============================
    netlib_start
    Begins a new net packet. If another net packet is already
    started and hasn't been sent yet, it will be discarded.
    @param The type of the packet
==============================*/

void netlib_start(NetPacket type)
{
    global_writebuffer[4] = (byte)type;
    global_writebuffer[5] = (byte)0; // Flags
    global_writecursize = PACKET_HEADERSIZE;
}


/*==============================
    netlib_writebyte
    Appends a byte to the current net packet
    @param The byte to append to the packet
==============================*/

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


/*==============================
    netlib_writeword
    Appends a word to the current net packet
    @param The word to append to the packet
==============================*/

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


/*==============================
    netlib_writedword
    Appends a double word to the current net packet
    @param The double word to append to the packet
==============================*/

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


/*==============================
    netlib_writeqword
    Appends a quad word to the current net packet
    @param The quad word to append to the packet
==============================*/

void netlib_writeqword(uint64_t data)
{
    #if SAFETYCHECKS
        if (global_writecursize + sizeof(uint64_t) > MAX_PACKETSIZE)
        {
            usb_write(DATATYPE_TEXT, "Warning: Writing more data than max packet size. Discarded!\n", 61);
            return;
        }
    #endif
    
    // N64 is also big endian, so data already respects Network Byte Order :D
    global_writebuffer[global_writecursize++] = (data >> 56) & 0xFF;
    global_writebuffer[global_writecursize++] = (data >> 48) & 0xFF;
    global_writebuffer[global_writecursize++] = (data >> 40) & 0xFF;
    global_writebuffer[global_writecursize++] = (data >> 32) & 0xFF;
    global_writebuffer[global_writecursize++] = (data >> 24) & 0xFF;
    global_writebuffer[global_writecursize++] = (data >> 16) & 0xFF;
    global_writebuffer[global_writecursize++] = (data >> 8) & 0xFF;
    global_writebuffer[global_writecursize++] = data & 0xFF;
}


/*==============================
    netlib_writefloat
    Appends a float to the current net packet
    @param The float to append to the packet
==============================*/

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


/*==============================
    netlib_writedouble
    Appends a double to the current net packet
    @param The double to append to the packet
==============================*/

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


/*==============================
    netlib_writebytes
    Appends a set of bytes to the current net packet
    @param The data to append to the packet
    @param The size of said data
==============================*/

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


/*==============================
    netlib_setflags
    Set the flag(s) of this packet
    @param The flag(s) to set
==============================*/

void netlib_setflags(PacketFlag flags)
{
    global_writebuffer[5] = flags;
}

/*==============================
    netlib_broadcast
    Sends the current net packet to all connected players
==============================*/

void netlib_broadcast()
{
    u32 mask = 0xFFFFFFFF  & ~(1 << (global_clnumber-1));
    u16 datasize = global_writecursize - PACKET_HEADERSIZE;
    
    // Write the client list and data size
    memcpy(&global_writebuffer[12], &mask, 4);
    memcpy(&global_writebuffer[16], &datasize, 2);
    
    // Send the packet over the wire once it's safe to do so
    global_sendafterpoll = TRUE;
    netlib_poll();
}


/*==============================
    netlib_send
    Sends the current net packet to a single player
    @param The player to send the packet to
==============================*/

void netlib_send(ClientNumber client)
{
    u32 mask = 1 << client;
    u16 datasize = global_writecursize - PACKET_HEADERSIZE;
    
    // Write the client list and data size
    memcpy(&global_writebuffer[12], &mask, 4);
    memcpy(&global_writebuffer[16], &datasize, 2);
    
    // Send the packet over the wire once it's safe to do so
    global_sendafterpoll = TRUE;
    netlib_poll();
}


/*==============================
    netlib_sendtoserver
    Sends the current net packet to the server
==============================*/

void netlib_sendtoserver()
{
    u32 mask = 0; // Zero is a server send
    u16 datasize = global_writecursize - PACKET_HEADERSIZE;
    
    // Write the client list and data size
    memcpy(&global_writebuffer[12], &mask, 4);
    memcpy(&global_writebuffer[16], &datasize, 2);
    
    // Send the packet over the wire once it's safe to do so
    global_sendafterpoll = TRUE;
    netlib_poll();
}
   
    
/*********************************
     Network -> N64 Functions
*********************************/

/*==============================
    netlib_register
    Registers a callback function to be used to handle
    packets of a specific type.
    @param The type of the packet
    @param A pointer to the callback function to execute
==============================*/

void netlib_register(NetPacket type, void (*callback)(size_t))
{
    #if SAFETYCHECKS
        if (type > MAX_UNIQUEPACKETS)
        {
            usb_write(DATATYPE_TEXT, "Warning: registering more callbacks than supporting. Registration discared!\n", 77);
            return;
        }
    #endif
    global_funcptrs[type] = callback;
}


/*==============================
    netlib_poll
    Polls the USB for NetLib packets.
==============================*/

void netlib_poll()
{
    u64 curtime;
    #ifndef LIBDRAGON
        curtime = osGetTime();
    #else
        curtime = timer_ticks();
    #endif
    
    // Check the USB did not time out from being disconnected
    // If it did (or reconnected), then execute the callback functions
    if (!global_disconnected && ((global_lastpkt+global_timeouttime) < curtime || usb_timedout() || usb_getcart == CART_NONE))
    {
        global_disconnected = TRUE;
        if (global_funcptr_disconnect != NULL)
            global_funcptr_disconnect();
    }
    else if (global_disconnected && (global_lastpkt+global_timeouttime) > curtime && usb_getcart != CART_NONE)
    {
        global_disconnected = FALSE;
        if (global_funcptr_reconnect != NULL)
            global_funcptr_reconnect();
    }
    
    // Read all incoming net packets first
    do
    {
        unsigned int header = usb_poll();
        while (USBHEADER_GETTYPE(header) == DATATYPE_NETPACKET)
        {
            uint8_t version, flags;
            NetPacket type;
            uint32_t recipients;
            uint16_t size;
            
            // Read the version packet
            #if SAFETYCHECKS
                usb_skip(3);
                usb_read(&version, 1);
                if (version > NETLIB_VERSION)
                {
                    usb_purge();
                    usb_write(DATATYPE_TEXT, "Warning: Unsupported packet version. Discarding!\n", 50);
                    return;
                }
            #else
                usb_skip(4);
            #endif
            
            // Get the packet type and flags
            usb_read(&type, 1);
            usb_read(&flags, 1);
            
            // Skip the sequence data as it's not important
            usb_skip(6);
            
            // Get the recipients list and the data size
            usb_read(&recipients, 4);
            usb_read(&size, 2);
                    
            // Call the relevant packet handling function
            #if SAFETYCHECKS
                if (global_funcptrs[type] == NULL)
                {
                    usb_purge();
                    usb_write(DATATYPE_TEXT, "Warning: Tried calling unregistered function!\n", 47);
                    return;
                }
            #endif
            global_funcptrs[type](size);
            
            // Refresh the packet time
            global_lastpkt = curtime;
            
            // Poll again
            usb_purge();
            header = usb_poll();
        }
        
        // If we queued up a message during polling, send it now (if it's safe to do so)
        if (global_sendafterpoll && header == 0)
        {
            if (usb_write(DATATYPE_NETPACKET, (void*)global_writebuffer, global_writecursize) != 0)
                global_sendafterpoll = FALSE;
        }
    } 
    while (global_sendafterpoll);
}


/*==============================
    netlib_readbyte
    Reads a byte from the received net packet
    @param A pointer to the byte to read into
==============================*/

void netlib_readbyte(uint8_t* output)
{
    usb_read(output, sizeof(uint8_t));
}


/*==============================
    netlib_readword
    Reads a word from the received net packet
    @param A pointer to the word to read into
==============================*/

void netlib_readword(uint16_t* output)
{
    usb_read(output, sizeof(uint16_t));
}


/*==============================
    netlib_readdword
    Reads a double word from the received net packet
    @param A pointer to the double word to read into
==============================*/

void netlib_readdword(uint32_t* output)
{
    usb_read(output, sizeof(uint32_t));
}


/*==============================
    netlib_readqword
    Reads a quad word from the received net packet
    @param A pointer to the quad word to read into
==============================*/

void netlib_readqword(uint64_t* output)
{
    usb_read(output, sizeof(uint64_t));
}

/*==============================
    netlib_readfloat
    Reads a float from the received net packet
    @param A pointer to the float to read into
==============================*/

void netlib_readfloat(float* output)
{
    usb_read(output, sizeof(float));
}
    
    
/*==============================
    netlib_readdouble
    Reads a double from the received net packet
    @param A pointer to the double to read into
==============================*/

void netlib_readdouble(double* output)
{
    usb_read(output, sizeof(double));
}


/*==============================
    netlib_readbytes
    Reads a set of bytes from the received net packet
    @param A pointer to the buffer to read into
    @param The number of bytes to read into this buffer
==============================*/

void netlib_readbytes(byte* output, size_t size)
{
    usb_read(output, size);
}