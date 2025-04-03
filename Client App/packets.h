#pragma once

#include <stdint.h>
#include "Include/asio.hpp"
#include <wx/string.h>
#include <wx/msgqueue.h>
#include <deque>

using asio::ip::udp;


/******************************
             Macros
******************************/

#define S64PACKET_HEADER "S64"
#define S64PACKET_VERSION 1

#define NETLIBPACKET_HEADER "NLP"
#define NETLIBPACKET_VERSION 1


/******************************
             Types
******************************/

typedef enum {
    FLAG_UNRELIABLE  = 0x01,
    FLAG_EXPLICITACK = 0x02,
} PacketFlag;

class UDPHandler;
class AbstractPacket;
class S64Packet;
class NetLibPacket;


/******************************
            Globals
******************************/

extern asio::io_context* global_asiocontext;


/*********************************
             Classes
*********************************/

// Wrapper class for ASIO
class ASIOSocket
{
    private:
        udp::resolver* m_Resolver;
        udp::socket* m_Socket;
        size_t m_LastReadCount;
        wxString m_Address;
        int m_Port;

    protected:

    public:
        static void InitASIO();

        ASIOSocket(wxString fulladdress);
        ASIOSocket(wxString address, int port);
        ~ASIOSocket();
        void Read(uint8_t* buff, size_t size);
        void Send(wxString address, int port, uint8_t* buff, size_t size);
        size_t LastReadCount();
};

// Main UDP Handler class
class UDPHandler
{
    private:
        wxString m_Address;
        int      m_Port;
        ASIOSocket* m_Socket;
        uint16_t m_LocalSeqNum;
        uint16_t m_RemoteSeqNum;
        uint16_t m_AckBitfield;
        std::deque<AbstractPacket*> m_AcksLeft_RX;
        std::deque<AbstractPacket*> m_AcksLeft_TX;

        bool HandlePacketSequence(AbstractPacket* pkt, AbstractPacket* (*ackmaker)());

    protected:

    public:
        UDPHandler(ASIOSocket* socket, wxString address, int port);
        UDPHandler(ASIOSocket* socket, wxString fulladdress);
        ~UDPHandler();
        ASIOSocket* GetSocket();
        wxString GetAddress();
        int      GetPort();
        void SendPacket(AbstractPacket* pkt);
        S64Packet* ReadS64Packet(uint8_t* data);
        NetLibPacket* ReadNetLibPacket(uint8_t* data);
        void ResendMissingPackets();
};

// An abstract packet class to reduce code repetition
// Do not use directly
class AbstractPacket
{
    private:

    protected:
        uint8_t  m_Version;
        uint8_t  m_Flags;
        uint16_t m_SequenceNum;
        uint16_t m_Ack;
        uint16_t m_AckBitField;
        uint16_t m_Size;
        uint8_t* m_Data;
        wxLongLong m_SendTime;
        uint8_t    m_SendAttempts;

        AbstractPacket(uint8_t version, uint16_t size, uint8_t* data, uint8_t flags, uint16_t seqnum, uint16_t acknum, uint16_t ackbitfield);
        AbstractPacket(uint8_t version, uint16_t size, uint8_t* data, uint8_t flags) : AbstractPacket(version, size, data, flags, 0, 0, 0) {};
        AbstractPacket(uint8_t version, uint16_t size, uint8_t* data) : AbstractPacket(version, size, data, 0, 0, 0, 0) {};

    public:
        virtual ~AbstractPacket();

        uint8_t    GetVersion();
        uint8_t    GetFlags();
        uint16_t   GetSize();
        uint8_t*   GetData();
        uint16_t   GetSequenceNumber();
        bool       IsAcked(uint16_t number);
        wxLongLong GetSendTime();
        uint8_t    GetSendAttempts();
        virtual uint8_t* GetAsBytes() {return NULL;};
        virtual uint16_t GetAsBytes_Size() {return 0;};

        virtual bool IsAckBeat() {return false;};
        void EnableFlags(uint8_t flags);
        void SetSequenceNumber(uint16_t seqnum);
        void SetAck(uint16_t acknum);
        void SetAckBitfield(uint16_t bitfield);
        void UpdateSendAttempt();
        virtual wxString AsString() {return "";};
};

// Server browser packet
class S64Packet : public AbstractPacket
{
    private:
        wxString m_Type;

        S64Packet(uint8_t version, wxString type, uint16_t size, uint8_t* data, uint8_t flags, uint16_t seqnum, uint16_t acknum, uint16_t ackbitfield);

    protected:

    public:
        S64Packet(wxString type, uint16_t size, uint8_t* data, uint8_t flags) : S64Packet(S64PACKET_VERSION, type, size, data, flags, 0, 0, 0) {};
        S64Packet(wxString type, uint16_t size, uint8_t* data) : S64Packet(S64PACKET_VERSION, type, size, data, 0, 0, 0, 0) {};
        ~S64Packet();

        static bool IsS64Packet(uint8_t* bytes);
        static S64Packet* FromBytes(uint8_t* bytes);
        uint8_t*   GetAsBytes();
        uint16_t   GetAsBytes_Size();

        bool IsAckBeat();
        wxString GetType();
        wxString AsString();
};

// NetLib packet for N64 games
class NetLibPacket : public AbstractPacket
{
    private:
        uint8_t  m_Type;
        uint32_t m_Recipients;

        NetLibPacket(uint8_t version, uint8_t type, uint16_t size, uint8_t* data, uint8_t flags, uint32_t recipients, uint16_t seqnum, uint16_t acknum, uint16_t ackbitfield);

    protected:

    public:
        NetLibPacket(uint8_t type, uint16_t size, uint8_t* data, uint8_t flags) : NetLibPacket(NETLIBPACKET_VERSION,  type, size, data, flags, 0, 0, 0, 0) {};
        ~NetLibPacket();

        static bool IsNetLibPacket(uint8_t* bytes);
        static NetLibPacket* FromBytes(uint8_t* bytes);
        uint8_t* GetAsBytes();
        uint16_t GetAsBytes_Size();

        bool IsAckBeat();
        uint8_t  GetType();
        uint32_t GetRecipients();
        wxString AsString();
};

// Exceptions thrown by the UDPHandler
class ClientTimeoutException : public std::exception
{
    private:
        wxString m_Address;

    public:
        ClientTimeoutException(wxString address) {this->m_Address = address;}
        wxString what() {return this->m_Address;}
};

class BadPacketVersionException : public std::exception
{
    private:
        int m_Version;

    public:
        BadPacketVersionException(int version) {this->m_Version = version;};
        int what() {return this->m_Version;}
};