#include <stdio.h>
#include "packets.h"
#include "helper.h"
#include <wx/buffer.h>
#include <wx/tokenzr.h>


/******************************
            Macros
******************************/

#define DEBUGPRINTS 0

#define MAX_PACKETSIZE   4096
#define MAX_RESENDCOUNT  5
#define MAX_SEQUENCENUM  0xFFFF

#define TIME_RESEND    1000
#define TIME_TIMEOUT   1000*30
#define TIME_ACKRETRY  1000*5


/*=============================================================
                       Helper Functions
=============================================================*/

/*==============================
    sequence_greaterthan
    Checks if a sequence number is larger than another, handling rollover
    @param  The first sequence number
    @param  The second sequence number
    @return Whether the first sequence number is larger
==============================*/

static inline bool sequence_greaterthan(uint16_t s1, uint16_t s2)
{
    return ((s1 > s2) && (s1 - s2 <= ((MAX_SEQUENCENUM/2)+1))) || ((s1 < s2) && (s2 - s1 > ((MAX_SEQUENCENUM/2)+1)));
}


/*==============================
    sequence_greaterthan
    Checks the delta between two sequence numbers, handling rollover
    @param  The first sequence number
    @param  The second sequence number
    @return The numerical difference between two sequence numbers
==============================*/

static inline int sequence_delta(uint32_t s1, uint32_t s2)
{
    int delta = (s1 - s2);
    if (delta < 0)
        delta += MAX_SEQUENCENUM+1;
    return delta;
}


/*==============================
    sequence_increment
    Increments a seqeuence number, handling rollover
    @param  The sequence number to increment
    @return The incremented number
==============================*/

static inline uint32_t sequence_increment(uint32_t seqnum)
{
    return (seqnum + 1) % (MAX_SEQUENCENUM + 1);
}


/*==============================
    getbits
    Gets a string representation of a number as bits
    @param  The size of the number (in bytes)
    @param  The number to represent
    @return The string representation of the bits
==============================*/

static wxString getbits(size_t size, void* num)
{
    wxString ret = "";
    uint8_t* bp = (uint8_t*) num;
    for (int i = size-1; i >= 0; i--)
    {
        for (int j = 7; j >= 0; j--)
        {
            uint8_t byte = (bp[i] >> j) & 1;
            ret += wxString::Format("%u", byte);
        }
    }
    return ret;
}


/*==============================
    MakeAck_S64Packet
    Creates an ack S64 Packet
    @return An ack S64 Packet
==============================*/

static AbstractPacket* MakeAck_S64Packet()
{
    return new S64Packet("ACK", 0, NULL, FLAG_UNRELIABLE);
}


/*==============================
    MakeAck_NetLibPacket
    Creates an ack NetLib Packet
    @return An ack NetLib Packet
==============================*/

static AbstractPacket* MakeAck_NetLibPacket()
{
    return new NetLibPacket(0, 0, NULL, FLAG_UNRELIABLE);
}


/*=============================================================
                          UDP Handler
=============================================================*/

/*==============================
    UDPHandler (Constructor)
    Initializes the class
    @param The socket for the handler to use
    @param The address for the handler to connect to
    @param The port for the handler to connect to
==============================*/

UDPHandler::UDPHandler(wxDatagramSocket* socket, wxString address, int port)
{
    this->m_Socket = socket;
    this->m_Address = address;
    this->m_Port = port;
    this->m_LocalSeqNum = 0;
    this->m_RemoteSeqNum = 0;
    this->m_AckBitfield = 0;
    this->m_AcksLeft_RX = std::deque<AbstractPacket*>();
    this->m_AcksLeft_TX = std::deque<AbstractPacket*>();
}


/*==============================
    UDPHandler (Constructor)
    Initializes the class
    @param The socket for the handler to use
    @param The address and port combination for the handler to connect to
==============================*/

UDPHandler::UDPHandler(wxDatagramSocket* socket, wxString fulladdress)
{
    wxStringTokenizer tokenizer(fulladdress, ":");
    this->m_Address = tokenizer.GetNextToken();
    if (tokenizer.HasMoreTokens())
        tokenizer.GetNextToken().ToInt(&this->m_Port);
    this->m_Socket = socket;
    this->m_LocalSeqNum = 0;
    this->m_RemoteSeqNum = 0;
    this->m_AckBitfield = 0;
    this->m_AcksLeft_RX = std::deque<AbstractPacket*>();
    this->m_AcksLeft_TX = std::deque<AbstractPacket*>();
}


/*==============================
    UDPHandler (Destructor)
    Cleans up the class before deletion
==============================*/

UDPHandler::~UDPHandler()
{
    for (AbstractPacket* pkt : this->m_AcksLeft_RX)
        free(pkt);
    for (AbstractPacket* pkt : this->m_AcksLeft_TX)
        free(pkt);
}


/*==============================
    UDPHandler::GetAddress
    Retreives the server address to connect to
    @return The server address to connect to
==============================*/

wxString UDPHandler::GetAddress()
{
    return this->m_Address;
}


/*==============================
    UDPHandler::GetPort
    Retreives the server port to use
    @return The server port to use
==============================*/

int UDPHandler::GetPort()
{
    return this->m_Port;
}


/*==============================
    UDPHandler::GetSocket
    Retreives the socket to use for networking
    @return The socket to use
==============================*/

wxDatagramSocket* UDPHandler::GetSocket()
{
    return this->m_Socket;
}


/*==============================
    UDPHandler::GetSocket
    Sends a packet to the server
    @param  The packet to send
    @throws ClientTimeoutException
==============================*/

void UDPHandler::SendPacket(AbstractPacket* pkt)
{
    uint8_t* data;
    uint16_t ackbitfield = 0;
    wxIPV4address address;
        
    // Check for timeouts
    pkt->UpdateSendAttempt();
    if (pkt->GetSendAttempts() > MAX_RESENDCOUNT)
        throw ClientTimeoutException(wxString::Format("%s:%d", this->m_Address, this->m_Port));
    
    // Set the sequence data
    if (pkt->GetSendAttempts() == 1)
    {
        pkt->SetSequenceNumber(this->m_LocalSeqNum);
        pkt->SetAck(this->m_RemoteSeqNum);
        for (AbstractPacket* pkt2ack : this->m_AcksLeft_RX)
            if (sequence_greaterthan(this->m_RemoteSeqNum, pkt2ack->GetSequenceNumber()))
                ackbitfield |= 1 << (sequence_delta(this->m_RemoteSeqNum, pkt2ack->GetSequenceNumber()) - 1);
        pkt->SetAckBitfield(ackbitfield);
    }

    // Send the packet
    data = pkt->GetAsBytes();
    address.Hostname(this->m_Address);
    address.Service(this->m_Port);
    this->m_Socket->SendTo(address, data, pkt->GetAsBytes_Size());
    
    // Add it to our list of packets that need an ack
    if ((pkt->GetFlags() & FLAG_UNRELIABLE) == 0 && pkt->GetSendAttempts() == 1)
    {
        this->m_AcksLeft_TX.push_back(pkt);
        this->m_LocalSeqNum = sequence_increment(this->m_LocalSeqNum);
    }

    // Debug prints for developers
    #if DEBUGPRINTS
        if (!pkt->IsAckBeat())
        {
            if (pkt->GetSendAttempts() > 1)
                printf("Re");
            printf("Sent %s\n", static_cast<const char*>(pkt->AsString().c_str()));
        }
    #endif

    // Cleanup
    free(data);
}


/*==============================
    UDPHandler::HandlePacketSequence
    Handles a packet's sequence information
    @param  The packet to handle
    @param  The ack maker function for this packet type
    @return Whether this packet's sequence was handled correctly
    @throws ClientTimeoutException
==============================*/

bool UDPHandler::HandlePacketSequence(AbstractPacket* pkt, AbstractPacket* (*ackmaker)())
{
    std::deque<AbstractPacket*>::iterator it;
            
    // If a packet with this sequence number already exists in our RX list, ignore this packet
    for (AbstractPacket* rxpkt : this->m_AcksLeft_RX)
    {
        if (rxpkt->GetSequenceNumber() == pkt->GetSequenceNumber())
        {
            if ((pkt->GetFlags() & FLAG_EXPLICITACK) != 0)
                this->SendPacket(ackmaker());
            delete pkt;
            return false;
        }
    }

    // Go through transmitted packets and remove all which were acknowledged in the one we received
    it = this->m_AcksLeft_TX.begin();
    while (it != this->m_AcksLeft_TX.end())
    {
        AbstractPacket* pkt2ack = *it;
        if (pkt->IsAcked(pkt2ack->GetSequenceNumber()))
        {
            delete pkt2ack;
            it = this->m_AcksLeft_TX.erase(it);
        }
        else
            ++it;
    }

    // Increment the sequence number to the packet's highest value
    if (sequence_greaterthan(pkt->GetSequenceNumber(), this->m_RemoteSeqNum))
        this->m_RemoteSeqNum = pkt->GetSequenceNumber();
        
    // Handle reliable packets
    if ((pkt->GetFlags() & FLAG_UNRELIABLE) == 0)
    {
        // Update our received packet list
        if (this->m_AcksLeft_RX.size() > 17)
        {
            delete this->m_AcksLeft_RX.front();
            this->m_AcksLeft_RX.pop_front();
        }
        this->m_AcksLeft_RX.push_back(pkt);
    }
    
    // If the packet wants an explicit ack, send it
    if ((pkt->GetFlags() & FLAG_EXPLICITACK) != 0)
        this->SendPacket(ackmaker());

    // Success
    return true;
}


/*==============================
    UDPHandler::ReadS64Packet
    Reads an S64 Packet from a collection of bytes 
    @param  The bytes with the packet information
    @return The created packet, or NULL
==============================*/

S64Packet* UDPHandler::ReadS64Packet(uint8_t* data)
{
    S64Packet* pkt;
    if (!S64Packet::IsS64Packet(data))
        return NULL;
    pkt = S64Packet::FromBytes(data);

    // Handle sequence numbers
    if (pkt == NULL || !HandlePacketSequence(pkt, &MakeAck_S64Packet))
        return NULL;

    // Debug prints for developers
    #if DEBUGPRINTS
        if (pkt->GetType() != 0)
            printf("Received %s\n", static_cast<const char*>(pkt->AsString().c_str()));
    #endif

    // Done
    return pkt;
}


/*==============================
    UDPHandler::ReadNetLibPacket
    Reads an NetLib Packet from a collection of bytes 
    @param  The bytes with the packet information
    @return The created packet, or NULL
==============================*/

NetLibPacket* UDPHandler::ReadNetLibPacket(uint8_t* data)
{
    NetLibPacket* pkt;
    if (!NetLibPacket::IsNetLibPacket(data))
        return NULL;
    pkt = NetLibPacket::FromBytes(data);

    // Handle sequence numbers
    if (pkt == NULL || !HandlePacketSequence(pkt, &MakeAck_NetLibPacket))
        return NULL;

    // Debug prints for developers
    #if DEBUGPRINTS
        if (pkt->GetType() != 0)
            printf("Received %s\n", static_cast<const char*>(pkt->AsString().c_str()));
    #endif

    // Done
    return pkt;
}


/*==============================
    UDPHandler::ResendMissingPackets
    Resends all packets which did not receive an ack after the resend time
    @param  The bytes with the packet information
    @return The created packet, or NULL
    @throws ClientTimeoutException
==============================*/

void UDPHandler::ResendMissingPackets()
{
    for (AbstractPacket* pkt2ack : this->m_AcksLeft_TX)
        if (pkt2ack->GetSendTime() > TIME_RESEND)
            this->SendPacket(pkt2ack);
}


/*=============================================================
                     Abstract Packet Class
=============================================================*/

/*==============================
    AbstractPacket (Constructor)
    Initializes the class
    @param The version of the packet
    @param The size of the packet data
    @param The packet data
    @param The flags to append to the packet
    @param The sequence number of the packet
    @param The seqeuence number of the last received packet
    @param The ack bitfield of the last received packets
==============================*/

AbstractPacket::AbstractPacket(uint8_t version, uint16_t size, uint8_t* data, uint8_t flags, uint16_t seqnum, uint16_t acknum, uint16_t ackbitfield)
{
    this->m_Version = version;
    this->m_Flags = flags;
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


/*==============================
    AbstractPacket (Destructor)
    Cleans up the class before deletion
==============================*/

AbstractPacket::~AbstractPacket()
{
    if (this->m_Size > 0)
        free(this->m_Data);
}


/*==============================
    AbstractPacket::GetVersion
    Retrieves the packet's version
    @return The packet's version
==============================*/

uint8_t AbstractPacket::GetVersion()
{
    return this->m_Version;
}


/*==============================
    AbstractPacket::GetFlags
    Retrieves the packet's flags
    @return The packet's flags
==============================*/

uint8_t AbstractPacket::GetFlags()
{
    return this->m_Flags;
}


/*==============================
    AbstractPacket::GetSize
    Retrieves the size of the packet's data
    @return The size of the packet's data
==============================*/

uint16_t AbstractPacket::GetSize()
{
    return this->m_Size;
}


/*==============================
    AbstractPacket::GetData
    Retrieves the packet's data
    @return The packet's data
==============================*/

uint8_t* AbstractPacket::GetData()
{
    return this->m_Data;
}


/*==============================
    AbstractPacket::GetSequenceNumber
    Retrieves the packet's sequence number
    @return The packet's sequence number
==============================*/

uint16_t AbstractPacket::GetSequenceNumber()
{
    return this->m_SequenceNum;
}


/*==============================
    AbstractPacket::GetSendTime
    Retrieves the time elapsed (in milliseconds) since this 
    packet was last sent
    @return The time since the packet was sent (in milliseconds)
==============================*/

wxLongLong AbstractPacket::GetSendTime()
{
    return wxGetLocalTimeMillis() - this->m_SendTime;
}


/*==============================
    AbstractPacket::GetSendAttempts
    Retrieves the number of times this packet was attempted
    to be sent
    @return The number of send attempts
==============================*/

uint8_t AbstractPacket::GetSendAttempts()
{
    return this->m_SendAttempts;
}


/*==============================
    AbstractPacket::IsAcked
    Checks if this packet is acking a specific sequence number
    @param  The sequence number to check
    @return Whether the given number is acked by this packet
==============================*/

bool AbstractPacket::IsAcked(uint16_t number)
{
    if (this->m_Ack == number)
        return true;
    return ((this->m_AckBitField & (1 << (sequence_delta(this->m_Ack, number) - 1))) != 0);
}


/*==============================
    AbstractPacket::EnableFlags
    Enables a set of flags on the packet
    @param The flags to enable
==============================*/

void AbstractPacket::EnableFlags(uint8_t flags)
{
    this->m_Flags |= flags;
}


/*==============================
    AbstractPacket::SetSequenceNumber
    Sets this packet's sequence number
    @param The sequence number to set
==============================*/

void AbstractPacket::SetSequenceNumber(uint16_t seqnum)
{
    this->m_SequenceNum = seqnum;
}


/*==============================
    AbstractPacket::SetAck
    Sets this packet's last ack number
    @param The last ack number
==============================*/

void AbstractPacket::SetAck(uint16_t acknum)
{
    this->m_Ack = acknum;
}


/*==============================
    AbstractPacket::SetAckBitfield
    Sets this packet's ack bitfield
    @param The ack bitfield
==============================*/

void AbstractPacket::SetAckBitfield(uint16_t bitfield)
{
    this->m_AckBitField = bitfield;
}


/*==============================
    AbstractPacket::UpdateSendAttempt
    Marks an attempt to send the packet
==============================*/

void AbstractPacket::UpdateSendAttempt()
{
    this->m_SendAttempts++;
    this->m_SendTime = wxGetLocalTimeMillis();
}


/*=============================================================
                          S64 Packet
=============================================================*/

/*==============================
    S64Packet (Constructor)
    Initializes the class
    @param The packet version
    @param The packet type
    @param The size of the packet data
    @param The data itself
    @param The flags to append to the packet
    @param The sequence number of the packet
    @param The seqeuence number of the last received packet
    @param The ack bitfield of the last received packets
==============================*/

S64Packet::S64Packet(uint8_t version, wxString type, uint16_t size, uint8_t* data, uint8_t flags, uint16_t seqnum, uint16_t acknum, uint16_t ackbitfield) : AbstractPacket(version, size, data, flags, seqnum, acknum, ackbitfield)
{
    this->m_Type = type;
}


/*==============================
    S64Packet (Destructor)
    Cleans up the class before deletion
==============================*/

S64Packet::~S64Packet()
{
    // Does nothing
}


/*==============================
    S64Packet::IsS64Packet
    Checks if the data contains an S64Packet
    @param  The data to check
    @return Whether the given data is an S64Packet
==============================*/

bool S64Packet::IsS64Packet(uint8_t* bytes)
{
    return !strncmp((const char*)bytes, S64PACKET_HEADER, strlen(S64PACKET_HEADER));
}


/*==============================
    S64Packet::FromBytes
    Retrieves an S64Packet from the given bytes
    @param  The raw bytes
    @return The S64Packet contained in the bytes, or NULL
==============================*/

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
    ret = new S64Packet(version, wxString(typestr), data_len, data, flags, seqnum, acknum, ackbitfield);
    free(typestr);
    if (data_len > 0)
        free(data);
    return ret;
}


/*==============================
    S64Packet::GetAsBytes
    Converts the S64 packet into a set of raw bytes
    @return The S64 packet converted into raw bytes, or NULL
==============================*/

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


/*==============================
    S64Packet::GetAsBytes_Size
    Gets the size of the S64 packet if it were converted to raw bytes
    @return The number of bytes this S64 packet can be represented with
==============================*/

uint16_t S64Packet::GetAsBytes_Size()
{
    return sizeof(S64PACKET_HEADER) + sizeof(this->m_Version) + sizeof(this->m_Flags) + 
        sizeof(this->m_SequenceNum) + sizeof(this->m_Ack) + sizeof(this->m_AckBitField) +
        1 + this->m_Type.Length() +
        sizeof(this->m_Size) + this->m_Size;
}


/*==============================
    S64Packet::IsAckBeat
    Checks if this packet is an Ack/Heartbeat packet
    @return Whether this packet is an Ack/Heartbeat packet
==============================*/

bool S64Packet::IsAckBeat()
{
    return this->m_Type == "ACK";
}


/*==============================
    S64Packet::GetType
    Gets the type of this packet
    @return The type of the packet
==============================*/

wxString S64Packet::GetType()
{
    return this->m_Type;
}


/*==============================
    S64Packet::AsString
    Gets a string representation of this packet
    @return The string representation of this packet
==============================*/

wxString S64Packet::AsString()
{
    wxString mystr = "S64Packet\n";
    mystr += wxString("    Version: ") + wxString::Format("%d", this->m_Version) + wxString("\n");
    mystr += wxString("    Type: ") + this->m_Type + wxString("\n");
    mystr += wxString("    Sequence Number: ") + wxString::Format("%d", this->m_SequenceNum) + wxString("\n");
    mystr += wxString("    Ack: ") + wxString::Format("%d", this->m_Ack) + wxString("\n");
    mystr += wxString("    AckField: ") + getbits(2, &this->m_AckBitField) + wxString("\n");
    mystr += wxString("    Data size: ") + wxString::Format("%d", this->m_Size) + wxString("\n");
    if (this->m_Size > 0)
    {
        mystr += wxString("    Data: \n");
        mystr += wxString("        ");
        for (int i=0; i<this->m_Size; i++)
            mystr += wxString::Format("%02x ", this->m_Data[i]);
    }
    return mystr;
}

/*=============================================================

=============================================================*/

/*==============================
    NetLibPacket (Constructor)
    Initializes the class
    @param The packet version
    @param The packet type
    @param The size of the packet data
    @param The data itself
    @param The flags to append to the packet
    @param The recipients field for the packet
    @param The sequence number of the packet
    @param The seqeuence number of the last received packet
    @param The ack bitfield of the last received packets
==============================*/

NetLibPacket::NetLibPacket(uint8_t version, uint8_t type, uint16_t size, uint8_t* data, uint8_t flags, uint32_t recipients, uint16_t seqnum, uint16_t acknum, uint16_t ackbitfield) : AbstractPacket(version, size, data, flags, seqnum, acknum, ackbitfield)
{
    this->m_Type = type;
    this->m_Recipients = recipients;
}


/*==============================
    NetLibPacket (Destructor)
    Cleans up the class before deletion
==============================*/

NetLibPacket::~NetLibPacket()
{
    // Does nothing
}


/*==============================
    NetLibPacket::IsNetLibPacket
    Checks if the data contains a NetLib packet
    @param  The data to check
    @return Whether the given data is a NetLib packet
==============================*/

bool NetLibPacket::IsNetLibPacket(uint8_t* bytes)
{
    return !strncmp((const char*)bytes, NETLIBPACKET_HEADER, strlen(NETLIBPACKET_HEADER));
}


/*==============================
    NetLibPacket::FromBytes
    Retrieves an NetLibPacket from the given bytes
    @param  The raw bytes
    @return The NetLibPacket contained in the bytes, or NULL
==============================*/

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
    ret = new NetLibPacket(version, type, data_len, data, flags, recipients, seqnum, acknum, ackbitfield);
    if (data_len > 0)
        free(data);
    return ret;
}


/*==============================
    NetLibPacket::GetAsBytes
    Converts the NetLib packet into a set of raw bytes
    @return The NetLib packet converted into raw bytes, or NULL
==============================*/

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


/*==============================
    NetLibPacket::GetAsBytes_Size
    Gets the size of the NetLib packet if it were converted to raw bytes
    @return The number of bytes this NetLib packet can be represented with
==============================*/

uint16_t NetLibPacket::GetAsBytes_Size()
{
    return sizeof(NETLIBPACKET_HEADER) + sizeof(this->m_Version) +
        sizeof(this->m_Type) + sizeof(this->m_Flags) +
        sizeof(this->m_SequenceNum) + sizeof(this->m_Ack) + sizeof(this->m_AckBitField) +
        sizeof(this->m_Recipients) +
        sizeof(this->m_Size) + this->m_Size;
}


/*==============================
    S64Packet::IsAckBeat
    Checks if this packet is an Ack/Heartbeat packet
    @return Whether this packet is an Ack/Heartbeat packet
==============================*/

bool NetLibPacket::IsAckBeat()
{
    return this->m_Type == 0;
}


/*==============================
    NetLibPacket::GetType
    Gets the type of this packet
    @return The type of the packet
==============================*/

uint8_t NetLibPacket::GetType()
{
    return this->m_Type;
}


/*==============================
    NetLibPacket::GetRecipients
    Gets the recipients field of this packet
    @return The recipients field of the packet
==============================*/

uint32_t NetLibPacket::GetRecipients()
{
    return this->m_Recipients;
}


/*==============================
    NetLibPacket::AsString
    Gets a string representation of this packet
    @return The string representation of this packet
==============================*/

wxString NetLibPacket::AsString()
{
    wxString mystr = "NetLib Packet\n";
    mystr += wxString("    Version: ") + wxString::Format("%d", this->m_Version) + wxString("\n");
    mystr += wxString("    Type: ") + wxString::Format("%d", this->m_Type) + wxString("\n");
    mystr += wxString("    Sequence Number: ") + wxString::Format("%d", this->m_SequenceNum) + wxString("\n");
    mystr += wxString("    Ack: ") + wxString::Format("%d", this->m_Ack) + wxString("\n");
    mystr += wxString("    AckField: ") + getbits(2, &this->m_AckBitField) + wxString("\n");
    mystr += wxString("    Recipients: ") + getbits(4, &this->m_Recipients) + wxString("\n");
    mystr += wxString("    Data size: ") + wxString::Format("%d", this->m_Size) + wxString("\n");
    if (this->m_Size > 0)
    {
        mystr += wxString("    Data: \n");
        mystr += wxString("        ");
        for (int i=0; i<this->m_Size; i++)
            mystr += wxString::Format("%02x ", this->m_Data[i]);
    }
    return mystr;
}