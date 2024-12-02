#pragma once

#include <stdint.h>
#include <wx/string.h>
#include <wx/socket.h>
#include <wx/msgqueue.h>
#include <deque>

class UDPHandler;
class USBPacket;
class S64Packet;
class NetLibPacket;

typedef enum {
    FLAG_UNRELIABLE  = 0x01,
    FLAG_EXPLICITACK = 0x02,
} PacketFlag;

class ClientTimeoutException : public std::exception
{
    private:
        wxString m_Address;
    public:
        ClientTimeoutException(wxString address) {this->m_Address = address;}
        wxString what() {return this->m_Address;}
};

class UDPHandler
{
    private:
        wxString m_Address;
        int      m_Port;
        wxDatagramSocket* m_Socket;
        uint16_t m_LocalSeqNum_S64;
        uint16_t m_RemoteSeqNum_S64;
        uint16_t m_AckBitfield_S64;
        std::deque<S64Packet*> m_AcksLeft_RX_S64;
        std::deque<S64Packet*> m_AcksLeft_TX_S64;
        uint16_t m_LocalSeqNum_NLP;
        uint16_t m_RemoteSeqNum_NLP;
        uint16_t m_AckBitfield_NLP;
        std::deque<NetLibPacket*> m_AcksLeft_RX_NLP;
        std::deque<NetLibPacket*> m_AcksLeft_TX_NLP;

    protected:

    public:
        UDPHandler(wxDatagramSocket* socket, wxString address, int port);
        UDPHandler(wxDatagramSocket* socket, wxString fulladdress);
        ~UDPHandler();
        wxDatagramSocket* GetSocket();
        wxString GetAddress();
        int      GetPort();
        void SendPacket(S64Packet* pkt);
        S64Packet* ReadS64Packet(uint8_t* data);
        void SendPacket(NetLibPacket* pkt);
        NetLibPacket* ReadNetLibPacket(uint8_t* data);
        void ResendMissingPackets();
};

// Server browser packet
class S64Packet
{
    private:
        uint8_t  m_Version;
        uint8_t  m_Flags;
        uint16_t m_SequenceNum;
        uint16_t m_Ack;
        uint16_t m_AckBitField;
        wxString m_Type;
        uint16_t m_Size;
        uint8_t* m_Data;
        wxLongLong m_SendTime;
        uint8_t    m_SendAttempts;
        S64Packet(uint8_t version, uint8_t flags, wxString type, uint16_t size, uint8_t* data, uint16_t seqnum, uint16_t acknum, uint16_t ackbitfield);

    protected:

    public:
        S64Packet(wxString type, uint16_t size, uint8_t* data, uint8_t flags);
        S64Packet(wxString type, uint16_t size, uint8_t* data);
        ~S64Packet();

        static bool IsS64Packet(uint8_t* bytes);
        static S64Packet* FromBytes(uint8_t* bytes);

        uint8_t    GetVersion();
        uint8_t    GetFlags();
        wxString   GetType();
        uint16_t   GetSize();
        uint8_t*   GetData();
        uint16_t   GetSequenceNumber();
        bool       IsAcked(uint16_t number);
        uint8_t*   GetAsBytes();
        uint16_t   GetAsBytes_Size();
        wxLongLong GetSendTime();
        uint8_t    GetSendAttempts();
        void EnableFlags(uint8_t flags);
        void SetSequenceNumber(uint16_t seqnum);
        void SetAck(uint16_t acknum);
        void SetAckBitfield(uint16_t bitfield);
        void UpdateSendAttempt();
};

// Netlib packet
class NetLibPacket
{
    private:
        uint8_t  m_Version;
        uint8_t  m_Type;
        uint8_t  m_Flags;
        uint16_t m_SequenceNum;
        uint16_t m_Ack;
        uint16_t m_AckBitField;
        uint32_t m_Recipients;
        uint16_t m_Size;
        uint8_t* m_Data;
        wxLongLong m_SendTime;
        uint8_t  m_SendAttempts;
        NetLibPacket(uint8_t version, uint8_t type, uint8_t flags, uint32_t recipients, uint16_t size, uint8_t* data, uint16_t seqnum, uint16_t acknum, uint16_t ackbitfield);

    protected:

    public:
        NetLibPacket(uint8_t type, uint16_t size, uint8_t* data, uint8_t flags);
        ~NetLibPacket();

        static bool IsNetLibPacket(uint8_t* bytes);
        static NetLibPacket* FromBytes(uint8_t* bytes);

        uint8_t    GetVersion();
        uint8_t    GetType();
        uint8_t    GetFlags();
        uint32_t   GetRecipients();
        uint16_t   GetSequenceNumber();
        bool       IsAcked(uint16_t number);
        uint16_t   GetSize();
        uint8_t*   GetData();
        uint8_t*   GetAsBytes();
        uint16_t   GetAsBytes_Size();
        wxLongLong GetSendTime();
        uint8_t    GetSendAttempts();
        void EnableFlags(uint8_t flags);
        void SetSequenceNumber(uint16_t seqnum);
        void SetAck(uint16_t acknum);
        void SetAckBitfield(uint16_t bitfield);
        void UpdateSendAttempt();
        wxString AsString();
};