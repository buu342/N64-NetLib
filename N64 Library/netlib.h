#ifndef _N64_NETLIB_H
#define _N64_NETLIB_H
    
    
    /*==============================================================
                                Includes
    ==============================================================*/

    #include <stdint.h>
    #include <stdlib.h>
    
    
    /*==============================================================
                             Configuration
    ==============================================================*/
    
    #define MAX_UNIQUEPACKETS 64
    #define MAX_PACKETSIZE    4*1024
    
    
    /*==============================================================
                              Custom types
    ==============================================================*/

    #ifndef bool
        #define bool char
    #endif

    #ifndef byte
        #define byte uint8_t
    #endif
    
    typedef uint32_t NetPacket;
    
    typedef uint32_t ClientNumber;
    
    
    /*==============================================================
                    Initialization and Configuration
    ==============================================================*/
    
    void netlib_initialize();
    
    
    /*==============================================================
                        N64 -> Network Functions
    ==============================================================*/
    
    bool netlib_start(NetPacket name);
    
    void netlib_writebyte(uint8_t data);
    
    void netlib_writeword(uint16_t data);
    
    void netlib_writedword(uint32_t data);
    
    void netlib_writebytes(byte* data, size_t size);
    
    void netlib_broadcast();
    
    void netlib_send(ClientNumber client);
    
    void netlib_sendtoserver();
   
    
    /*==============================================================
                        Network -> N64 Functions
    ==============================================================*/
    
    void netlib_receive(NetPacket name, void* (*callback)(size_t, ClientNumber));
    
    void netlib_poll();
    
    void netlib_readbyte(uint8_t* output);
    
    void netlib_readword(uint16_t* output);
    
    void netlib_readdword(uint32_t* output);
    
    size_t netlib_readbytes(byte* output);
    
#endif