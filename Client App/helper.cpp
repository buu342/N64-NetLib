#include "helper.h"

uint16_t swap_endian16(uint16_t val)
{
    return (val >> 8) | (val << 8);
}

uint32_t swap_endian32(uint32_t val)
{
    return ((val << 24)) | ((val << 8) & 0x00FF0000) | ((val >> 8) & 0x0000FF00) | ((val >> 24));
}

wxString stringhash_frombytes(uint8_t* bytes, uint32_t size)
{
    uint32_t i;
    wxString str = "";
    for (i=0; i<size; i++)
        str += wxString::Format("%02X", bytes[i]);
    return str;
}