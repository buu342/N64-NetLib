#pragma once

#include <stdint.h> 
#include <wx/string.h>

uint32_t swap_endian32(uint32_t val);
wxString stringhash_frombytes(uint8_t* bytes, uint32_t size);