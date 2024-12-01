#include <stdio.h>
#include "packets.h"
#include "helper.h"
#include <wx/buffer.h>
#include <wx/tokenzr.h>

#define MAX_PACKETSIZE   4096
#define MAX_RESENDCOUNT  5
#define MAX_SEQUENCENUM  0xFFFF

#define TIME_RESEND    1000
#define TIME_TIMEOUT   1000*30
#define TIME_ACKRETRY  1000*5

#define S64PACKET_HEADER "S64"
#define S64PACKET_VERSION 1

#define NETLIBPACKET_HEADER "NLP"
#define NETLIBPACKET_VERSION 1


inline bool sequence_greaterthan(uint16_t s1, uint16_t s2)
{
    return ((s1 > s2) && (s1 - s2 <= ((MAX_SEQUENCENUM/2)+1))) || ((s1 < s2) && (s2 - s1 > ((MAX_SEQUENCENUM/2)+1)));
}

inline bool sequence_delta(uint32_t s1, uint32_t s2)
{
    int delta = (s1 - s2);
    if (delta < 0)
        delta += MAX_SEQUENCENUM+1;
    return delta;
}

inline uint32_t sequence_increment(uint32_t seqnum)
{
    return (seqnum + 1) % (MAX_SEQUENCENUM + 1);
}


UDPHandler::UDPHandler(wxDatagramSocket* socket, wxString address, int port)
{
    this->m_Socket = socket;
    this->m_Address = address;
    this->m_Port = port;
    this->m_LocalSeqNum_S64 = 0;
    this->m_RemoteSeqNum_S64 = 0;
    this->m_AckBitfield_S64 = 0;
    this->m_AcksLeft_RX_S64 = std::deque<S64Packet*>();
    this->m_AcksLeft_TX_S64 = std::deque<S64Packet*>();
    this->m_LocalSeqNum_NLP = 0;
    this->m_RemoteSeqNum_NLP = 0;
    this->m_AckBitfield_NLP = 0;
    this->m_AcksLeft_RX_NLP = std::deque<NetLibPacket*>();
    this->m_AcksLeft_TX_NLP = std::deque<NetLibPacket*>();
}

UDPHandler::UDPHandler(wxDatagramSocket* socket, wxString fulladdress)
{
    wxStringTokenizer tokenizer(fulladdress, ":");
    this->m_Address = tokenizer.GetNextToken();
    if (tokenizer.HasMoreTokens())
        tokenizer.GetNextToken().ToInt(&this->m_Port);
    this->m_Socket = socket;
    this->m_LocalSeqNum_S64 = 0;
    this->m_RemoteSeqNum_S64 = 0;
    this->m_AckBitfield_S64 = 0;
    this->m_AcksLeft_RX_S64 = std::deque<S64Packet*>();
    this->m_AcksLeft_TX_S64 = std::deque<S64Packet*>();
    this->m_LocalSeqNum_NLP = 0;
    this->m_RemoteSeqNum_NLP = 0;
    this->m_AckBitfield_NLP = 0;
    this->m_AcksLeft_RX_NLP = std::deque<NetLibPacket*>();
    this->m_AcksLeft_TX_NLP = std::deque<NetLibPacket*>();
}

UDPHandler::~UDPHandler()
{
    for (S64Packet* pkt : this->m_AcksLeft_RX_S64)
        free(pkt);
    for (S64Packet* pkt : this->m_AcksLeft_TX_S64)
        free(pkt);
    for (NetLibPacket* pkt : this->m_AcksLeft_RX_NLP)
        free(pkt);
    for (NetLibPacket* pkt : this->m_AcksLeft_TX_NLP)
        free(pkt);
}

wxString UDPHandler::GetAddress()
{
    return this->m_Address;
}

int UDPHandler::GetPort()
{
    return this->m_Port;
}

wxDatagramSocket* UDPHandler::GetSocket()
{
    return this->m_Socket;
}

void UDPHandler::SendPacket(S64Packet* pkt)
{
    uint8_t* data;
    uint16_t ackbitfield = 0;
    wxIPV4address address;
        
    // Check for timeouts
    pkt->UpdateSendAttempt();
    if (pkt->GetSendAttempts() > MAX_RESENDCOUNT)
        throw ClientTimeoutException(wxString::Format("%s:%d", this->m_Address, this->m_Port));
    
    // Set the sequence data
    pkt->SetSequenceNumber(this->m_LocalSeqNum_S64);
    pkt->SetAck(this->m_RemoteSeqNum_S64);
    for (S64Packet* pkt2ack : this->m_AcksLeft_RX_S64)
        if (sequence_greaterthan(this->m_RemoteSeqNum_S64, pkt2ack->GetSequenceNumber()))
            ackbitfield |= 1 << sequence_delta(this->m_RemoteSeqNum_S64, pkt2ack->GetSequenceNumber());
    pkt->SetAckBitfield(ackbitfield);

    // Send the packet
    data = pkt->GetAsBytes();
    address.Hostname(this->m_Address);
    address.Service(this->m_Port);
    this->m_Socket->SendTo(address, data, pkt->GetAsBytes_Size());
    
    // Add it to our list of packets that need an ack
    if ((pkt->GetFlags() & FLAG_UNRELIABLE) == 0 && pkt->GetSendAttempts() == 1)
    {
        this->m_AcksLeft_TX_S64.push_back(pkt);
        this->m_LocalSeqNum_S64 = sequence_increment(this->m_LocalSeqNum_S64);
    }

    // Cleanup
    free(data);
}

S64Packet* UDPHandler::ReadS64Packet(uint8_t* data)
{
    S64Packet* pkt;
    if (!S64Packet::IsS64Packet(data))
        return NULL;
    pkt = S64Packet::FromBytes(data);
    if (pkt != NULL)
    {
        std::deque<S64Packet*>::iterator it;
            
        // If a packet with this sequence number already exists in our RX list, ignore this packet
        for (S64Packet* rxpkt : this->m_AcksLeft_RX_S64)
        {
            if (rxpkt->GetSequenceNumber() == pkt->GetSequenceNumber())
            {
                if ((pkt->GetFlags() & FLAG_EXPLICITACK) != 0)
                    this->SendPacket(new S64Packet("ACK", 0, NULL, FLAG_UNRELIABLE));
                return NULL;
            }
        }

        // Go through transmitted packets and remove all which were acknowledged in the one we received
        it = this->m_AcksLeft_TX_S64.begin();
        while (it != this->m_AcksLeft_TX_S64.end())
        {
            S64Packet* pkt2ack = *it;
            if (pkt->IsAcked(pkt2ack->GetSequenceNumber()))
            {
                delete pkt2ack;
                it = this->m_AcksLeft_TX_S64.erase(it);
            }
            else
                ++it;
        }

        // Increment the sequence number to the packet's highest value
        if (sequence_greaterthan(pkt->GetSequenceNumber(), this->m_RemoteSeqNum_S64))
            this->m_RemoteSeqNum_S64 = pkt->GetSequenceNumber();
            
        // Handle reliable packets
        if ((pkt->GetFlags() & FLAG_UNRELIABLE) == 0)
        {
            // Update our received packet list
            if (this->m_AcksLeft_RX_S64.size() > 17)
            {
                delete this->m_AcksLeft_RX_S64.front();
                this->m_AcksLeft_RX_S64.pop_front();
            }
            this->m_AcksLeft_RX_S64.push_back(pkt);
        }
        
        // If the packet wants an explicit ack, send it
        if ((pkt->GetFlags() & FLAG_EXPLICITACK) != 0)
            this->SendPacket(new S64Packet("ACK", 0, NULL, FLAG_UNRELIABLE));
    }
    return pkt;
}

void UDPHandler::SendPacket(NetLibPacket* pkt)
{
    uint8_t* data;
    uint16_t ackbitfield = 0;
    wxIPV4address address;
        
    // Check for timeouts
    pkt->UpdateSendAttempt();
    if (pkt->GetSendAttempts() > MAX_RESENDCOUNT)
        throw ClientTimeoutException(wxString::Format("%s:%d", this->m_Address, this->m_Port));
    
    // Set the sequence data
    pkt->SetSequenceNumber(this->m_LocalSeqNum_NLP);
    pkt->SetAck(this->m_RemoteSeqNum_NLP);
    for (NetLibPacket* pkt2ack : this->m_AcksLeft_RX_NLP)
        if (sequence_greaterthan(this->m_RemoteSeqNum_NLP, pkt2ack->GetSequenceNumber()))
            ackbitfield |= 1 << sequence_delta(this->m_RemoteSeqNum_NLP, pkt2ack->GetSequenceNumber());
    pkt->SetAckBitfield(ackbitfield);

    // Send the packet
    data = pkt->GetAsBytes();
    address.Hostname(this->m_Address);
    address.Service(this->m_Port);
    this->m_Socket->SendTo(address, data, pkt->GetAsBytes_Size());
    
    // Add it to our list of packets that need an ack
    if ((pkt->GetFlags() & FLAG_UNRELIABLE) == 0 && pkt->GetSendAttempts() == 1)
    {
        this->m_AcksLeft_TX_NLP.push_back(pkt);
        this->m_LocalSeqNum_NLP = sequence_increment(this->m_LocalSeqNum_NLP);
    }

    // Cleanup
    free(data);
}

NetLibPacket* UDPHandler::ReadNetLibPacket(uint8_t* data)
{
    NetLibPacket* pkt;
    if (!NetLibPacket::IsNetLibPacket(data))
        return NULL;
    pkt = NetLibPacket::FromBytes(data);
    if (pkt != NULL)
    {
        std::deque<NetLibPacket*>::iterator it;
            
        // If a packet with this sequence number already exists in our RX list, ignore this packet
        for (NetLibPacket* rxpkt : this->m_AcksLeft_RX_NLP)
        {
            if (rxpkt->GetSequenceNumber() == pkt->GetSequenceNumber())
            {
                if ((pkt->GetFlags() & FLAG_EXPLICITACK) != 0)
                    this->SendPacket(new NetLibPacket(0, 0, NULL, FLAG_UNRELIABLE));
                return NULL;
            }
        }

        // Go through transmitted packets and remove all which were acknowledged in the one we received
        it = this->m_AcksLeft_TX_NLP.begin();
        while (it != this->m_AcksLeft_TX_NLP.end())
        {
            NetLibPacket* pkt2ack = *it;
            if (pkt->IsAcked(pkt2ack->GetSequenceNumber()))
            {
                delete pkt2ack;
                it = this->m_AcksLeft_TX_NLP.erase(it);
            }
            else
                ++it;
        }
        
        // Increment the sequence number to the packet's highest value
        if (sequence_greaterthan(pkt->GetSequenceNumber(), this->m_RemoteSeqNum_NLP))
            this->m_RemoteSeqNum_NLP = pkt->GetSequenceNumber();
            
        // Handle reliable packets
        if ((pkt->GetFlags() & FLAG_UNRELIABLE) == 0)
        {
            // Update our received packet list
            if (this->m_AcksLeft_RX_NLP.size() > 17)
            {
                delete this->m_AcksLeft_RX_NLP.front();
                this->m_AcksLeft_RX_NLP.pop_front();
            }
            this->m_AcksLeft_RX_NLP.push_back(pkt);
        }
        
        // If the packet wants an explicit ack, send it
        if ((pkt->GetFlags() & FLAG_EXPLICITACK) != 0)
            this->SendPacket(new NetLibPacket(0, 0, NULL, FLAG_UNRELIABLE));
    }
    return pkt;
}
    
void UDPHandler::ResendMissingPackets()
{
    for (S64Packet* pkt2ack : this->m_AcksLeft_TX_S64)
        if (pkt2ack->GetSendTime() > TIME_RESEND)
            this->SendPacket(pkt2ack);
    for (NetLibPacket* pkt2ack : this->m_AcksLeft_TX_NLP)
        if (pkt2ack->GetSendTime() > TIME_RESEND)
            this->SendPacket(pkt2ack);
}


/*=============================================================

=============================================================*/

S64Packet::S64Packet(uint8_t version, uint8_t flags, wxString type, uint16_t size, uint8_t* data, uint16_t seqnum, uint16_t acknum, uint16_t ackbitfield)
{
    this->m_Version = version;
    this->m_Flags = flags;
    this->m_Type = type;
    this->m_Size = size;
    if (size > 0)
    {
        this->m_Data = (uint8_t*)malloc(size);
        memcpy(this->m_Data, data, size);
    }
    else
        this->m_Data = NULL;
    this->m_SequenceNum = seqnum;
    this->m_Ack = acknum;
    this->m_AckBitField = ackbitfield;
    this->m_SendTime = 0;
    this->m_SendAttempts = 0;
}

S64Packet::S64Packet(wxString type, uint16_t size, uint8_t* data, uint8_t flags)
{
    this->m_Version = S64PACKET_VERSION;
    this->m_Flags = flags;
    this->m_Type = type;
    this->m_Size = size;
    if (size > 0)
    {
        this->m_Data = (uint8_t*)malloc(size);
        memcpy(this->m_Data, data, size);
    }
    else
        this->m_Data = NULL;
    this->m_SequenceNum = 0;
    this->m_Ack = 0;
    this->m_AckBitField = 0;
    this->m_SendTime = 0;
    this->m_SendAttempts = 0;
}

S64Packet::S64Packet(wxString type, uint16_t size, uint8_t* data)
{
    this->m_Version = S64PACKET_VERSION;
    this->m_Flags = 0;
    this->m_Type = type;
    this->m_Size = size;
    if (size > 0)
    {
        this->m_Data = (uint8_t*)malloc(size);
        memcpy(this->m_Data, data, size);
    }
    else
        this->m_Data = NULL;
    this->m_SequenceNum = 0;
    this->m_Ack = 0;
    this->m_AckBitField = 0;
    this->m_SendTime = 0;
    this->m_SendAttempts = 0;
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
    uint8_t  flags;
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
    memcpy(&flags, bytes+readcount, sizeof(flags));
    readcount += sizeof(flags);

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
    data_len = swap_endian16(data_len);
    readcount += sizeof(data_len);
    if (data_len > 0)
    {
        data = (uint8_t*)malloc(data_len);
        memcpy(data, bytes+readcount, data_len);
        readcount += data_len;
    }

    // Build the packet object and cleanup
    ret = new S64Packet(version, flags, wxString(typestr), data_len, data, seqnum, acknum, ackbitfield);
    free(typestr);
    if (data_len > 0)
        free(data);
    return ret;
}

uint8_t S64Packet::GetVersion()
{
    return this->m_Version;
}

uint8_t S64Packet::GetFlags()
{
    return this->m_Flags;
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

bool S64Packet::IsAcked(uint16_t number)
{
    if (this->m_Ack == number)
        return true;
    return ((this->m_AckBitField & (1 << sequence_delta(this->m_Ack, number))) != 0);
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
    memcpy(bytes+writecount, &this->m_Flags, sizeof(this->m_Flags));
    writecount += sizeof(this->m_Flags);

    // Write the sequence data
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
    return sizeof(S64PACKET_HEADER) + sizeof(this->m_Version) + sizeof(this->m_Flags) + 
        sizeof(this->m_SequenceNum) + sizeof(this->m_Ack) + sizeof(this->m_AckBitField) +
        1 + this->m_Type.Length() +
        sizeof(this->m_Size) + this->m_Size;
}

wxLongLong S64Packet::GetSendTime()
{
    return wxGetLocalTimeMillis() - this->m_SendTime;
}

uint8_t S64Packet::GetSendAttempts()
{
    return this->m_SendAttempts;
}

void S64Packet::EnableFlags(uint8_t flags)
{
    this->m_Flags = flags;
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

void S64Packet::UpdateSendAttempt()
{
    this->m_SendAttempts++;
    this->m_SendTime = wxGetLocalTimeMillis();
}


/*=============================================================

=============================================================*/

NetLibPacket::NetLibPacket(uint8_t version, uint8_t type, uint8_t flags, uint32_t recipients, uint16_t size, uint8_t* data, uint16_t seqnum, uint16_t acknum, uint16_t ackbitfield)
{
    this->m_Version = version;
    this->m_Type = type;
    this->m_Flags = flags;
    this->m_Size = size;
    this->m_Recipients = recipients;
    if (size > 0)
    {
        this->m_Data = (uint8_t*)malloc(size);
        memcpy(this->m_Data, data, size);
    }
    else
        this->m_Data = NULL;
    this->m_SequenceNum = seqnum;
    this->m_Ack = acknum;
    this->m_AckBitField = ackbitfield;
    this->m_SendTime = 0;
    this->m_SendAttempts = 0;
}

NetLibPacket::NetLibPacket(uint8_t type, uint16_t size, uint8_t* data, uint8_t flags)
{
    this->m_Version = NETLIBPACKET_VERSION;
    this->m_Type = type;
    this->m_Flags = flags;
    this->m_Size = size;
    this->m_Recipients = 0;
    if (size > 0)
    {
        this->m_Data = (uint8_t*)malloc(size);
        memcpy(this->m_Data, data, size);
    }
    else
        this->m_Data = NULL;
    this->m_SequenceNum = 0;
    this->m_Ack = 0;
    this->m_AckBitField = 0;
    this->m_SendTime = 0;
    this->m_SendAttempts = 0;
}

NetLibPacket::~NetLibPacket()
{
    if (this->m_Size > 0)
        free(this->m_Data);
}

bool NetLibPacket::IsNetLibPacket(uint8_t* bytes)
{
    return !strncmp((const char*)bytes, NETLIBPACKET_HEADER, strlen(NETLIBPACKET_HEADER));
}

NetLibPacket* NetLibPacket::FromBytes(uint8_t* bytes)
{
    uint8_t  version, type, flags;
    uint16_t seqnum, acknum, ackbitfield;
    uint32_t recipients;
    uint16_t data_len;
    uint8_t* data = NULL;
    NetLibPacket* ret = NULL;
    uint32_t readcount = 0;

    // Read the header
    if (!NetLibPacket::IsNetLibPacket(bytes))
        return NULL;
    readcount += strlen(NETLIBPACKET_HEADER);
    memcpy(&version, bytes+readcount, sizeof(version));
    readcount += sizeof(version);

    // Read the type and flags
    memcpy(&type, bytes+readcount, sizeof(type));
    readcount += sizeof(type);
    memcpy(&flags, bytes+readcount, sizeof(flags));
    readcount += sizeof(flags);

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

    // Read the recipients
    memcpy(&recipients, bytes+readcount, sizeof(recipients));
    recipients = swap_endian32(recipients);
    readcount += sizeof(recipients);

    // Read the data
    memcpy(&data_len, bytes+readcount, sizeof(data_len));
    data_len = swap_endian16(data_len);
    readcount += sizeof(data_len);
    if (data_len > 0)
    {
        data = (uint8_t*)malloc(data_len);
        memcpy(data, bytes+readcount, data_len);
        readcount += data_len;
    }

    // Build the packet object and cleanup
    ret = new NetLibPacket(version, type, flags, recipients, data_len, data, seqnum, acknum, ackbitfield);
    if (data_len > 0)
        free(data);
    return ret;
}

uint8_t NetLibPacket::GetVersion()
{
    return this->m_Version;
}

uint8_t NetLibPacket::GetType()
{
    return this->m_Type;
}

uint8_t NetLibPacket::GetFlags()
{
    return this->m_Flags;
}

uint32_t NetLibPacket::GetRecipients()
{
    return this->m_Recipients;
}

uint16_t NetLibPacket::GetSequenceNumber()
{
    return this->m_SequenceNum;
}

bool NetLibPacket::IsAcked(uint16_t number)
{
    if (this->m_Ack == number)
        return true;
    return ((this->m_AckBitField & (1 << sequence_delta(this->m_Ack, number))) != 0);
}

uint16_t NetLibPacket::GetSize()
{
    return this->m_Size;
}

uint8_t* NetLibPacket::GetData()
{
    return this->m_Data;
}

uint8_t* NetLibPacket::GetAsBytes()
{
    uint16_t write16b;
    uint32_t write32b;
    uint8_t* bytes = NULL;
    uint32_t writecount = 0;

    // Malloc the bytes
    bytes = (uint8_t*)malloc(this->GetAsBytes_Size());
    if (bytes == NULL)
        return NULL;

    // Write the header
    memcpy(bytes+writecount, NETLIBPACKET_HEADER, strlen(NETLIBPACKET_HEADER));
    writecount += strlen(NETLIBPACKET_HEADER);
    memcpy(bytes+writecount, &this->m_Version, sizeof(this->m_Version));
    writecount += sizeof(this->m_Version);

    // Write the type and the flags
    memcpy(bytes+writecount, &this->m_Type, sizeof(this->m_Type));
    writecount += sizeof(this->m_Type);
    memcpy(bytes+writecount, &this->m_Flags, sizeof(this->m_Flags));
    writecount += sizeof(this->m_Flags);

    // Write the sequence data
    write16b = swap_endian16(this->m_SequenceNum);
    memcpy(bytes+writecount, &write16b, sizeof(write16b));
    writecount += sizeof(write16b);
    write16b = swap_endian16(this->m_Ack);
    memcpy(bytes+writecount, &write16b, sizeof(write16b));
    writecount += sizeof(write16b);
    write16b = swap_endian16(this->m_AckBitField);
    memcpy(bytes+writecount, &write16b, sizeof(write16b));
    writecount += sizeof(write16b);

    // Write the recipients
    write32b = swap_endian32(this->m_Recipients);
    memcpy(bytes+writecount, &write32b, sizeof(write32b));
    writecount += sizeof(write32b);

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

uint16_t NetLibPacket::GetAsBytes_Size()
{
    return sizeof(NETLIBPACKET_HEADER) + sizeof(this->m_Version) +
        sizeof(this->m_Type) + sizeof(this->m_Flags) +
        sizeof(this->m_SequenceNum) + sizeof(this->m_Ack) + sizeof(this->m_AckBitField) +
        sizeof(this->m_Recipients) +
        sizeof(this->m_Size) + this->m_Size;
}

wxLongLong NetLibPacket::GetSendTime()
{
    return wxGetLocalTimeMillis() - this->m_SendTime;
}

uint8_t NetLibPacket::GetSendAttempts()
{
    return this->m_SendAttempts;
}

void NetLibPacket::EnableFlags(uint8_t flags)
{
    this->m_Flags = flags;
}

void NetLibPacket::SetSequenceNumber(uint16_t seqnum)
{
    this->m_SequenceNum = seqnum;
}

void NetLibPacket::SetAck(uint16_t acknum)
{
    this->m_Ack = acknum;
}

void NetLibPacket::SetAckBitfield(uint16_t bitfield)
{
    this->m_AckBitField = bitfield;
}

void NetLibPacket::UpdateSendAttempt()
{
    this->m_SendAttempts++;
    this->m_SendTime = wxGetLocalTimeMillis();
}