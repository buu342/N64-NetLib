#ifndef _N64_NETLIB_H
#define _N64_NETLIB_H
    
    
    /*********************************
              Configuration
    *********************************/
    
    // Uncomment this line if using Libdragon
    //#define LIBDRAGON  1
    
    // The max number of packet types supported
    #define MAX_UNIQUEPACKETS  64
    
    // The Max size of a packet's payload
    #define MAX_PACKETSIZE     4*1024
    
    // Whether to check if the packet write will go past the buffer
    #define SAFETYCHECKS  1
    
    
    /*********************************
                 Includes
    *********************************/

    #include <stdlib.h>
    #ifndef LIBDRAGON
        #include <ultra64.h>
    #else
        #include <stdint.h>
    #endif
    
    /*********************************
               Custom types
    *********************************/

    #ifndef bool
        #define bool char
    #endif

    #ifndef byte
        #define byte uint8_t
    #endif
    
    #ifndef LIBDRAGON
        typedef u8 uint8_t;
        typedef u16 uint16_t;
        typedef u32 uint32_t;
    #endif
    
    // Packet and client types
    typedef uint8_t NetPacket;
    typedef uint8_t ClientNumber;
    
    // Packet flags
    typedef enum {
        FLAG_UNRELIABLE = 0x01,
        FLAG_EXPLICITACK = 0x02,
    } PacketFlag;
    
    
    /*********************************
            Initialization and
          Configuration Functions
    *********************************/
    
    /*==============================
        netlib_initialize
        Initializes the NetLib library.
        Also initializes the USB library internally
    ==============================*/
    
    extern void netlib_initialize();
    
    
    /*==============================
        netlib_setclient
        Sets our own client number
        @param Our client number
    ==============================*/
    
    extern void netlib_setclient(ClientNumber num);
    
    
    /*==============================
        netlib_getclient
        Gets our own client number
        @return Our client number
    ==============================*/
    
    extern ClientNumber netlib_getclient();
    
    
    /*==============================
        netlib_callback_disconnect
        Set the callback function for when we disconnect
        @param A pointer to function to call when we disconnect
    ==============================*/
    
    extern void netlib_callback_disconnect(void (*callback)());
    
    
    /*==============================
        netlib_callback_reconnect
        Set the callback function for when we reconnect
        @param A pointer to function to call when we reconnect
    ==============================*/
    
    extern void netlib_callback_reconnect(void (*callback)());
    
    
    /*********************************
         N64 -> Network Functions
    *********************************/
    
    /*==============================
        netlib_start
        Begins a new net packet. If another net packet is already
        started and hasn't been sent yet, it will be discarded.
        @param The type of the packet
    ==============================*/
    
    extern void netlib_start(NetPacket type);
    
    
    /*==============================
        netlib_writebyte
        Appends a byte to the current net packet
        @param The byte to append to the packet
    ==============================*/
    
    extern void netlib_writebyte(uint8_t data);
    
    
    /*==============================
        netlib_writeword
        Appends a word to the current net packet
        @param The word to append to the packet
    ==============================*/
    
    extern void netlib_writeword(uint16_t data);
    
    
    /*==============================
        netlib_writedword
        Appends a double word to the current net packet
        @param The double word to append to the packet
    ==============================*/
    
    extern void netlib_writedword(uint32_t data);
    
    
    /*==============================
        netlib_writefloat
        Appends a float to the current net packet
        @param The float to append to the packet
    ==============================*/
    
    extern void netlib_writefloat(float data);
    
    
    /*==============================
        netlib_writedouble
        Appends a double to the current net packet
        @param The double to append to the packet
    ==============================*/
    
    extern void netlib_writedouble(double data);
    
    
    /*==============================
        netlib_writebytes
        Appends a set of bytes to the current net packet
        @param The data to append to the packet
        @param The size of said data
    ==============================*/
    
    extern void netlib_writebytes(byte* data, size_t size);
    
    
    /*==============================
        netlib_setflags
        Set the flag(s) of this packet
        @param The flag(s) to set
    ==============================*/
    
    extern void netlib_setflags(PacketFlag flags);
    
    
    /*==============================
        netlib_broadcast
        Sends the current net packet to all connected players
    ==============================*/
    
    extern void netlib_broadcast();
    
    
    /*==============================
        netlib_send
        Sends the current net packet to a single player
        @param The player to send the packet to
    ==============================*/
    
    extern void netlib_send(ClientNumber client);
    
    
    /*==============================
        netlib_sendtoserver
        Sends the current net packet to the server
    ==============================*/
    
    extern void netlib_sendtoserver();
   
    
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
    
    extern void netlib_register(NetPacket type, void (*callback)(size_t));
    
    
    /*==============================
        netlib_poll
        Polls the USB for NetLib packets.
    ==============================*/
    
    extern void netlib_poll();
    
    
    /*==============================
        netlib_readbyte
        Reads a byte from the received net packet
        @param A pointer to the byte to read into
    ==============================*/
    
    extern void netlib_readbyte(uint8_t* output);
    
    
    /*==============================
        netlib_readword
        Reads a word from the received net packet
        @param A pointer to the word to read into
    ==============================*/
    
    extern void netlib_readword(uint16_t* output);
    
    
    /*==============================
        netlib_readdword
        Reads a double word from the received net packet
        @param A pointer to the double word to read into
    ==============================*/
    
    extern void netlib_readdword(uint32_t* output);
    
    
    /*==============================
        netlib_readfloat
        Reads a float from the received net packet
        @param A pointer to the float to read into
    ==============================*/
    
    extern void netlib_readfloat(float* output);
    
    
    /*==============================
        netlib_readdouble
        Reads a double from the received net packet
        @param A pointer to the double to read into
    ==============================*/
    
    extern void netlib_readdouble(double* output);
    
    
    /*==============================
        netlib_readbytes
        Reads a set of bytes from the received net packet
        @param A pointer to the buffer to read into
        @param The number of bytes to read into this buffer
    ==============================*/
    
    extern void netlib_readbytes(byte* output, size_t size);
    
#endif