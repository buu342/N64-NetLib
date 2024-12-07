/***************************************************************
                           helper.cpp

A collection of helper functions
***************************************************************/

#include "helper.h"


/*==============================
    swap_endian16
    Swaps the endianness of a 16-bit value
    @param  The value to swap 
    @return The swapped value
==============================*/

uint16_t swap_endian16(uint16_t val)
{
    return (val >> 8) | (val << 8);
}


/*==============================
    swap_endian32
    Swaps the endianness of a 32-bit value
    @param  The value to swap 
    @return The swapped value
==============================*/

uint32_t swap_endian32(uint32_t val)
{
    return ((val << 24)) | ((val << 8) & 0x00FF0000) | ((val >> 8) & 0x0000FF00) | ((val >> 24));
}


/*==============================
    stringhash_frombytes
    Turns a set of bytes representing a hash into a string
    @param  The bytes to represent 
    @param  The amount of bytes in the array 
    @return The string representation
==============================*/

wxString stringhash_frombytes(uint8_t* bytes, uint32_t size)
{
    wxString str = "";
    for (uint32_t i=0; i<size; i++)
        str += wxString::Format("%02X", bytes[i]);
    return str;
}