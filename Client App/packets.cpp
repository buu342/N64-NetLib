#include <stdio.h>
#include "packets.h"
#include "helper.h"

#define PACKET_HEADER "S64PKT"
#define PACKET_VERSION 1

#define USB_HEADER "DMA@"
#define USB_TAIL "CMPH"

S64Packet::S64Packet(int version, wxString type, int size, char* data)
{
    this->m_Version = version;
    this->m_Type = type;
    this->m_Size = size;
    this->m_Data = (char*)malloc(size);
    memcpy(this->m_Data, data, size);
}

S64Packet::S64Packet(wxString type, int size, char* data)
{
    this->m_Version = PACKET_VERSION;
    this->m_Type = type;
    this->m_Size = size;
    this->m_Data = data;
}

S64Packet::~S64Packet()
{
    if (this->m_Size > 0)
        free(this->m_Data);
}

S64Packet* S64Packet::ReadPacket(wxSocketClient* socket)
{
    short version;
    int typesize;
    int size;
    char header[64];
    char* data;
    wxString type = "";

    // Read the packet header
    socket->Read(header, 6);
    if (socket->LastCount() == 0)
        return NULL;
    if (socket->LastError() != wxSOCKET_NOERROR)
    {
        printf("Socket threw error %d\n", socket->LastError());
        return NULL;
    }
    if (strncmp(header, PACKET_HEADER, 6) != 0)
    {
        printf("Received bad packet header %.6s\n", header);
        return NULL;
    }

    // Read the packet version
    socket->Read(&version, 2);
    if (socket->LastError() != wxSOCKET_NOERROR)
    {
        printf("Socket threw error %d\n", socket->LastError());
        return NULL;
    }
    version = swap_endian16(version);

    // Read the packet type size
    socket->Read(&header, 1);
    if (socket->LastError() != wxSOCKET_NOERROR)
    {
        printf("Socket threw error %d\n", socket->LastError());
        return NULL;
    }
    typesize = header[0];

    // Read the packet type
    socket->Read(&header, typesize);
    if (socket->LastError() != wxSOCKET_NOERROR)
    {
        printf("Socket threw error %d\n", socket->LastError());
        return NULL;
    }
    header[typesize] = '\0';
    type = wxString(header);

    // Read the packet size
    socket->Read(&size, 4);
    size = swap_endian32(size);
    if (socket->LastError() != wxSOCKET_NOERROR)
    {
        printf("Socket threw error %d\n", socket->LastError());
        return NULL;
    }

    // Malloc a buffer for the packet data, and then read it
    if (size > 0)
    {
        data = (char*)malloc(size);
        if (data == NULL)
        {
            printf("Unable to allocate buffer of %d bytes for packet data\n", size);
            return NULL;
        }
        socket->Read(data, size);
        if (socket->LastError() != wxSOCKET_NOERROR)
        {
            free(data);
            printf("Socket threw error %d\n", socket->LastError());
            return NULL;
        }
    }
    else
        data = NULL;

    // Create the packet
    return new S64Packet(version, type, size, data);
}

void S64Packet::SendPacket(wxSocketClient* socket)
{
    char out[64];
    short data_short;
    int data_int;

    // Write the header
    sprintf(out, PACKET_HEADER);
    socket->Write(out, strlen(PACKET_HEADER));

    // Version
    data_short = swap_endian16(PACKET_VERSION);
    socket->Write(&data_short, 2);

    // Type string length and the string itself
    out[0] = this->m_Type.Length();
    socket->Write(out, 1);
    strncpy(out, (const char*)this->m_Type.mb_str(), 64);
    socket->Write(out, this->m_Type.Length());

    // Data size and the data itself
    data_int = swap_endian32(this->m_Size);
    socket->Write(&data_int, 4);
    if (this->m_Size > 0)
        socket->Write(this->m_Data, this->m_Size);
}

int S64Packet::GetVersion()
{
    return this->m_Version;
}

wxString S64Packet::GetType()
{
    return this->m_Type;
}

int S64Packet::GetSize()
{
    return this->m_Size;
}

char* S64Packet::GetData()
{
    return this->m_Data;
}


USBPacket::USBPacket(int type, int size, char* data)
{
    this->m_Type = type;
    this->m_Size = size;
    this->m_Data = (char*)malloc(size);
    memcpy(this->m_Data, data, size);
}

USBPacket::~USBPacket()
{
    if (this->m_Size > 0)
        free(this->m_Data);
}

USBPacket* USBPacket::ReadPacket(wxSocketClient* socket)
{
    int size;
    int type;
    char header[4];
    char* data;
    USBPacket* pkt;

    // Read the packet header
    socket->Read(header, 4);
    if (socket->LastCount() == 0)
        return NULL;
    if (socket->LastError() != wxSOCKET_NOERROR)
    {
        printf("Socket threw error %d\n", socket->LastError());
        return NULL;
    }
    if (strncmp(header, USB_HEADER, 4) != 0)
    {
        printf("Received bad packet header %.4s\n", header);
        return NULL;
    }

    // Read the packet datatype header
    socket->Read(&size, 4);
    if (socket->LastError() != wxSOCKET_NOERROR)
    {
        printf("Socket threw error %d\n", socket->LastError());
        return NULL;
    }
    size = swap_endian32(size);
    type = (size & 0xFF000000) >> 24;
    size = size & 0x00FFFFFF;

    // Malloc a buffer for the packet data, and then read it
    if (size > 0)
    {
        data = (char*)malloc(size);
        if (data == NULL)
        {
            printf("Unable to allocate buffer of %d bytes for packet data\n", size);
            return NULL;
        }
        socket->Read(data, size);
        if (socket->LastError() != wxSOCKET_NOERROR)
        {
            free(data);
            printf("Socket threw error %d\n", socket->LastError());
            return NULL;
        }
    }
    else
        data = NULL;

    // Read the packet tail
    socket->Read(header, 4);
    if (socket->LastCount() == 0)
        return NULL;
    if (socket->LastError() != wxSOCKET_NOERROR)
    {
        printf("Socket threw error %d\n", socket->LastError());
        return NULL;
    }
    if (strncmp(header, USB_TAIL, 4) != 0)
    {
        printf("Received bad packet tail %.4s\n", header);
        return NULL;
    }

    // Create the packet
    pkt = new USBPacket(type, size, data);
    free(data);
    return pkt;
}

void USBPacket::SendPacket(wxSocketClient* socket)
{
    char out[5];
    int data_int;

    // Write the header
    sprintf(out, USB_HEADER);
    socket->Write(out, strlen(USB_HEADER));

    // Data size and the data itself
    data_int = (this->m_Size & 0xFFFFFF) | (((uint32_t)this->m_Type) << 24);
    data_int = swap_endian32(data_int);
    socket->Write(&data_int, 4);
    if (this->m_Size > 0)
        socket->Write(this->m_Data, this->m_Size);

    // Write the tail
    sprintf(out, USB_TAIL);
    socket->Write(out, strlen(USB_TAIL));
}

int USBPacket::GetType()
{
    return this->m_Type;
}

int USBPacket::GetSize()
{
    return this->m_Size;
}

char* USBPacket::GetData()
{
    return this->m_Data;
}