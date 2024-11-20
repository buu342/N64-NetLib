#include <stdio.h>
#include "packets.h"
#include "helper.h"
#include <wx/buffer.h>

#define MAX_PACKETSIZE  4096

#define TIME_TIMEOUT   1000*30
#define TIME_ACKRETRY  1000*5

#define S64PACKET_HEADER "S64"
#define S64PACKET_VERSION 1

#define USB_HEADER "DMA@"
#define USB_TAIL "CMPH"

#define NETLIBPACKET_HEADER "NLP"
#define NETLIBPACKET_HEADERSIZE 12
#define NETLIBPACKET_VERSION 1


inline bool sequence_greaterthan(uint16_t s1, uint16_t s2)
{
    return ((s1 > s2) && (s1 - s2 <= 32768)) || ((s1 < s2) && (s2 - s1 > 32768));
}

UDPHandler::UDPHandler(wxDatagramSocket* socket, wxString address, int port)
{
    this->m_Socket = socket;
    this->m_Address = address;
    this->m_Port = port;
    this->m_LocalSeqNum = 0;
    this->m_RemoteSeqNum = 0;
    this->m_AckBitfield = 0;
    this->m_AcksLeft = std::deque<uint16_t>();
}

UDPHandler::~UDPHandler()
{

}

wxString UDPHandler::GetAddress()
{
    return this->m_Address;
}

int UDPHandler::GetPort()
{
    return this->m_Port;
}

void UDPHandler::SendPacket(S64Packet* pkt)
{
    uint8_t* data;
    uint16_t ackbitfield = 0;
    wxIPV4address address;
    
    // Set the sequence data
    pkt->SetSequenceNumber(this->m_LocalSeqNum);
    pkt->SetAck(this->m_RemoteSeqNum);
    for (uint16_t ack : this->m_AcksLeft)
        if (sequence_greaterthan(this->m_RemoteSeqNum, ack))
            ackbitfield |= 1 << ((this->m_RemoteSeqNum - ack) - 1);
    pkt->SetAckBitfield(ackbitfield);

    // Send the packet
    data = pkt->GetAsBytes();
    address.Hostname(this->m_Address);
    address.Service(this->m_Port);
    this->m_Socket->SendTo(address, data, pkt->GetAsBytes_Size());
    
    // Increase the local sequence number and cleanup
    this->m_LocalSeqNum++;
    free(data);
}

void UDPHandler::SendPacketWaitAck(S64Packet* pkt)
{
    wxLongLong packettime = 0;
    uint8_t* response = (uint8_t*)malloc(MAX_PACKETSIZE);
    while (true)
    {
        S64Packet* ack;
        this->SendPacket(pkt);
        packettime = wxGetLocalTimeMillis();
        
        // Wait for a response
        this->m_Socket->Read(response, MAX_PACKETSIZE);
        while (this->m_Socket->LastReadCount() > 0)
        {
            if ((packettime - wxGetLocalTimeMillis()) > TIME_TIMEOUT)
            {
                free(response);
                return; // TODO: Need a timeout exception or something
            }
            this->m_Socket->Read(response, MAX_PACKETSIZE);
        }
        if (pkt == NULL || !S64Packet::IsS64Packet(response))
            continue;
        
        // If we got an ack, we're done
        ack = this->ReadS64Packet(response);
        if (ack != NULL && ack->GetType() == "ACK")
        {
            free(response);
            return;
        }
        delete ack;
    }
}

S64Packet* UDPHandler::ReadS64Packet(uint8_t* data)
{
    S64Packet* pkt;
    if (!S64Packet::IsS64Packet(data))
        return NULL;
    pkt = S64Packet::FromBytes(data);
    if (pkt != NULL)
    {
        if (sequence_greaterthan(pkt->GetSequenceNumber(), this->m_RemoteSeqNum)) {
            this->m_RemoteSeqNum = pkt->GetSequenceNumber();
        }
        if (this->m_AcksLeft.size() > 17)
            this->m_AcksLeft.pop_front();
        this->m_AcksLeft.push_back(pkt->GetSequenceNumber());
    }
    return pkt;
}

S64Packet::S64Packet(uint8_t version, wxString type, uint16_t size, uint8_t* data, uint16_t seqnum, uint16_t acknum, uint16_t ackbitfield)
{
    this->m_Version = version;
    this->m_Type = type;
    this->m_Size = size;
    this->m_SequenceNum = seqnum;
    this->m_Ack = acknum;
    this->m_AckBitField = ackbitfield;
    if (size > 0)
    {
        this->m_Data = (uint8_t*)malloc(size);
        memcpy(this->m_Data, data, size);
    }
    else
        this->m_Data = NULL;
}

S64Packet::S64Packet(wxString type, uint16_t size, uint8_t* data)
{
    this->m_Version = S64PACKET_VERSION;
    this->m_Type = type;
    this->m_Size = size;
    this->m_SequenceNum = 0;
    this->m_Ack = 0;
    this->m_AckBitField = 0;
    if (size > 0)
    {
        this->m_Data = (uint8_t*)malloc(size);
        memcpy(this->m_Data, data, size);
    }
    else
        this->m_Data = NULL;
}

S64Packet::~S64Packet()
{
    if (this->m_Size > 0)
        free(this->m_Data);
}

bool S64Packet::IsS64Packet(uint8_t* bytes)
{
    return !strncmp((const char*)bytes, S64PACKET_HEADER, strlen(S64PACKET_HEADER));
}

S64Packet* S64Packet::FromBytes(uint8_t* bytes)
{
    uint8_t  version;
    uint16_t seqnum;
    uint16_t acknum;
    uint16_t ackbitfield;
    uint8_t  typestr_len;
    char*    typestr;
    uint16_t data_len;
    uint8_t* data = NULL;
    S64Packet* ret = NULL;
    uint32_t readcount = 0;

    // Read the header
    if (!S64Packet::IsS64Packet(bytes))
        return NULL;
    readcount += strlen(S64PACKET_HEADER);
    memcpy(&version, bytes+readcount, sizeof(version));
    readcount += sizeof(version);

    // Read the sequence data
    memcpy(&seqnum, bytes+readcount, sizeof(seqnum));
    seqnum = swap_endian16(seqnum);
    readcount += sizeof(seqnum);
    memcpy(&acknum, bytes+readcount, sizeof(acknum));
    acknum = swap_endian16(acknum);
    readcount += sizeof(acknum);
    memcpy(&ackbitfield, bytes+readcount, sizeof(ackbitfield));
    ackbitfield = swap_endian16(ackbitfield);
    readcount += sizeof(ackbitfield);

    // Read the type string
    memcpy(&typestr_len, bytes+readcount, sizeof(typestr_len));
    readcount += sizeof(typestr_len);
    typestr = (char*)malloc(typestr_len + 1);
    memcpy(typestr, bytes+readcount, typestr_len);
    typestr[typestr_len] = '\0';
    readcount += typestr_len;

    // Read the data
    memcpy(&data_len, bytes+readcount, sizeof(data_len));
    readcount += sizeof(data_len);
    if (data_len > 0)
    {
        data = (uint8_t*)malloc(data_len);
        memcpy(data, bytes+readcount, data_len);
        readcount += data_len;
    }

    // Build the packet object and cleanup
    ret = new S64Packet(version, wxString(typestr), data_len, data, seqnum, acknum, ackbitfield);
    free(typestr);
    if (data_len > 0)
        free(data);
    return ret;
}

uint8_t S64Packet::GetVersion()
{
    return this->m_Version;
}

wxString S64Packet::GetType()
{
    return this->m_Type;
}

uint16_t S64Packet::GetSize()
{
    return this->m_Size;
}

uint8_t* S64Packet::GetData()
{
    return this->m_Data;
}

uint16_t S64Packet::GetSequenceNumber()
{
    return this->m_SequenceNum;
}

uint8_t* S64Packet::GetAsBytes()
{
    uint8_t  write8b;
    uint16_t write16b;
    uint8_t* bytes = NULL;
    uint32_t writecount = 0;

    // Malloc the bytes
    bytes = (uint8_t*)malloc(this->GetAsBytes_Size());
    if (bytes == NULL)
        return NULL;

    // Write the header
    memcpy(bytes+writecount, S64PACKET_HEADER, strlen(S64PACKET_HEADER));
    writecount += strlen(S64PACKET_HEADER);
    memcpy(bytes+writecount, &this->m_Version, sizeof(this->m_Version));
    writecount += sizeof(this->m_Version);

    // Read the sequence data
    write16b = swap_endian16(this->m_SequenceNum);
    memcpy(bytes+writecount, &write16b, sizeof(write16b));
    writecount += sizeof(write16b);
    write16b = swap_endian16(this->m_Ack);
    memcpy(bytes+writecount, &write16b, sizeof(write16b));
    writecount += sizeof(write16b);
    write16b = swap_endian16(this->m_AckBitField);
    memcpy(bytes+writecount, &write16b, sizeof(write16b));
    writecount += sizeof(write16b);

    // Type string
    write8b = this->m_Type.Length();
    memcpy(bytes+writecount, &write8b, sizeof(write8b));
    writecount += sizeof(write8b);
    memcpy(bytes+writecount, this->m_Type.mb_str(), this->m_Type.Length());
    writecount += this->m_Type.Length();

    // Data
    write16b = swap_endian16(this->m_Size);
    memcpy(bytes+writecount, &write16b, sizeof(write16b));
    writecount += sizeof(write16b);
    if (this->m_Size > 0)
    {
        memcpy(bytes+writecount, this->m_Data, this->m_Size);
        writecount += this->m_Size;
    }

    // Done
    return bytes;
}

uint16_t S64Packet::GetAsBytes_Size()
{
    return sizeof(S64PACKET_HEADER) + sizeof(this->m_Version) +
        sizeof(this->m_SequenceNum) + sizeof(this->m_Ack) + sizeof(this->m_AckBitField) +
        1 + this->m_Type.Length() +
        sizeof(this->m_Size) + this->m_Size;
}

void S64Packet::SetSequenceNumber(uint16_t seqnum)
{
    this->m_SequenceNum = seqnum;
}

void S64Packet::SetAck(uint16_t acknum)
{
    this->m_Ack = acknum;
}

void S64Packet::SetAckBitfield(uint16_t bitfield)
{
    this->m_AckBitField = bitfield;
}


#if 0
    NetLibPacket::NetLibPacket(int version, int id, int recipients, int size, char* data)
    {
        this->m_Version = version;
        this->m_ID = id;
        this->m_Recipients = recipients;
        this->m_Size = size;
        if (size > 0)
        {
            this->m_Data = (char*)malloc(size);
            memcpy(this->m_Data, data, size);
        }
        else
            this->m_Data = NULL;
    }

    NetLibPacket::~NetLibPacket()
    {
        if (this->m_Size > 0)
            free(this->m_Data);
    }

    NetLibPacket* NetLibPacket::ReadPacket(wxSocketClient* socket)
    {
        int version;
        int id;
        int recipients;
        int size;
        char* data;
        char header[4];
        NetLibPacket* pkt;

        // Read the packet header
        socket->Read(header, 4);
        if (socket->LastCount() == 0)
            return NULL;
        if (socket->LastError() != wxSOCKET_NOERROR)
        {
            printf("Socket threw error %d\n", socket->LastError());
            return NULL;
        }
        if (strncmp(header, NETLIBPACKET_HEADER, 3) != 0)
        {
            printf("Received bad packet header %.3s\n", header);
            return NULL;
        }
        version = header[3];

        // Read the size and ID
        socket->Read(&size, 4);
        if (socket->LastError() != wxSOCKET_NOERROR)
        {
            printf("Socket threw error %d\n", socket->LastError());
            return NULL;
        }
        size = swap_endian32(size);
        id = (size & 0xFF000000) >> 24;
        size = size & 0x00FFFFFF;

        // Read the recepients
        socket->Read(&recipients, 4);
        if (socket->LastError() != wxSOCKET_NOERROR)
        {
            printf("Socket threw error %d\n", socket->LastError());
            return NULL;
        }
        recipients = swap_endian32(recipients);

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
        pkt = new NetLibPacket(version, id, recipients, size, data);
        free(data);
        return pkt;
    }

    NetLibPacket* NetLibPacket::FromBytes(char* data)
    {
        int version;
        int id;
        int size;
        int recipients;
        version = data[3];
        memcpy(&id, data+4, sizeof(int));
        id = swap_endian32(id);
        size = id & 0x00FFFFFF;
        id = (id & 0xFF000000) >> 24;
        memcpy(&recipients, data+8, sizeof(int));
        recipients = swap_endian32(recipients);
        if (id != 4)
        {
            printf("Got packet from USB: ");
            for (int i=0; i<size; i++)
                printf("%02x ", data[i]);
            printf("\n");
        }
        return new NetLibPacket(version, id, recipients, size, data+12);
    }

    void NetLibPacket::SendPacket(wxSocketClient* socket)
    {
        char* out = this->GetAsBytes();
        if (this->m_ID != 4)
        {
            printf("Sending packet to server: ");
            for (int i=0; i<this->m_Size; i++)
                printf("%02x ", this->m_Data[i]);
            printf("\n");
            printf("Written as: ");
            for (int i=0; i<this->m_Size; i++)
                printf("%02x ", out[i+12]);
            printf("\n");
        }
        socket->Write(out, this->GetAsBytes_Size());
        free(out);
    }

    int NetLibPacket::GetVersion()
    {
        return this->m_Version;
    }

    int NetLibPacket::GetID()
    {
        return this->m_Recipients;
    }

    int NetLibPacket::GetSize()
    {
        return this->m_Size;
    }

    int NetLibPacket::GetRecipients()
    {
        return this->m_Recipients;
    }

    char* NetLibPacket::GetData()
    {
        return this->m_Data;
    }

    char* NetLibPacket::GetAsBytes()
    {
        int data_int;
        char* bytes = (char*)malloc(this->m_Size + NETLIBPACKET_HEADERSIZE);
        bytes[0] = NETLIBPACKET_HEADER[0];
        bytes[1] = NETLIBPACKET_HEADER[1];
        bytes[2] = NETLIBPACKET_HEADER[2];
        bytes[3] = this->m_Version;
        data_int = (this->m_ID << 24) | (this->m_Size & 0x00FFFFFFF);
        data_int = swap_endian32(data_int);
        memcpy(bytes+4, &data_int, sizeof(int));
        data_int = swap_endian32(this->m_Recipients);
        memcpy(bytes+8, &data_int, sizeof(int));
        if (this->m_Size > 0)
            memcpy(bytes+12, this->m_Data, this->m_Size);
        return bytes;
    }

    int NetLibPacket::GetAsBytes_Size()
    {
        return this->m_Size + NETLIBPACKET_HEADERSIZE;
    }
#endif