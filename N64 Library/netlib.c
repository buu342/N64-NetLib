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
    global_writebuffer[global_writecursize] = (byte)data;
    global_writecursize += sizeof(uint8_t);
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
    global_writebuffer[0] = (data >> 8) & 0xFF;
    global_writebuffer[1] = data & 0xFF;
    global_writecursize += sizeof(uint16_t);
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
    global_writebuffer[0] = (data >> 24) & 0xFF;
    global_writebuffer[1] = (data >> 16) & 0xFF;
    global_writebuffer[2] = (data >> 8) & 0xFF;
    global_writebuffer[3] = data & 0xFF;
    global_writecursize += sizeof(uint32_t);
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
    global_writebuffer[0] = (data >> 24) & 0xFF;
    global_writebuffer[1] = (data >> 16) & 0xFF;
    global_writebuffer[2] = (data >> 8) & 0xFF;
    global_writebuffer[3] = data & 0xFF;
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
    
    memcpy(&global_writebuffer[global_writecursize], data, size);
    global_writecursize += size;
}