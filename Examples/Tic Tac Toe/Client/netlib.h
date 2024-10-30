#ifndef _N64_NETLIB_H
#define _N64_NETLIB_H
    
    
    /*==============================================================
                             Configuration
    ==============================================================*/
    
    //#define LIBDRAGON  1
    
    #define MAX_UNIQUEPACKETS  64
    #define MAX_PACKETSIZE     4*1024
    
    #define SAFETYCHECKS  1
    
    
    /*==============================================================
                                Includes
    ==============================================================*/

    #include <stdlib.h>
    #ifndef LIBDRAGON
        #include <ultra64.h>
    #else
        #include <stdint.h>
    #endif
    
    /*==============================================================
                              Custom types
    ==============================================================*/

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
    
    typedef uint8_t NetPacket;
    typedef uint8_t ClientNumber;
    
    
    /*==============================================================
                    Initialization and Configuration
    ==============================================================*/
    
    void netlib_initialize();
    
    
    /*==============================================================
                        N64 -> Network Functions
    ==============================================================*/
    
    void netlib_start(NetPacket id);
    
    void netlib_writebyte(uint8_t data);
    
    void netlib_writeword(uint16_t data);
    
    void netlib_writedword(uint32_t data);
    
    void netlib_writefloat(float data);
    
    void netlib_writedouble(double data);
    
    void netlib_writebytes(byte* data, size_t size);
    
    void netlib_broadcast();
    
    void netlib_send(ClientNumber client);
    
    void netlib_sendtoserver();
   
    
    /*==============================================================
                        Network -> N64 Functions
    ==============================================================*/
    
    void netlib_register(NetPacket id, void (*callback)(size_t, ClientNumber));
    
    void netlib_poll();
    
    void netlib_readbyte(uint8_t* output);
    
    void netlib_readword(uint16_t* output);
    
    void netlib_readdword(uint32_t* output);
    
    void netlib_readfloat(float* output);
    
    void netlib_readdouble(double* output);
    
    void netlib_readbytes(byte* output, size_t maxsize);
    
#endif