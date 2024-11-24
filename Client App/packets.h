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

class UDPHandler
{
    private:
        wxString m_Address;
        int      m_Port;
        wxDatagramSocket* m_Socket;
        uint16_t m_LocalSeqNum; // Java doesn't support unsigned shorts, so we'll have to mimic them with ints
        uint16_t m_RemoteSeqNum;
        uint16_t m_AckBitfield;
        std::deque<uint16_t> m_AcksLeft;

    protected:

    public:
        UDPHandler(wxDatagramSocket* socket, wxString address, int port);
        UDPHandler(wxDatagramSocket* socket, wxString fulladdress);
        ~UDPHandler();
        wxDatagramSocket* GetSocket();
        wxString GetAddress();
        int      GetPort();
        void SendPacket(S64Packet* pkt);
        void SendPacketWaitAck(S64Packet* pkt);
        S64Packet* ReadS64Packet(uint8_t* data);
};

// Server browser packet
class S64Packet
{
    private:
        uint8_t  m_Version;
        uint16_t m_SequenceNum;
        uint16_t m_Ack;
        uint16_t m_AckBitField;
        wxString m_Type;
        uint16_t m_Size;
        uint8_t* m_Data;
        S64Packet(uint8_t version, wxString type, uint16_t size, uint8_t* data, uint16_t seqnum, uint16_t acknum, uint16_t ackbitfield);

    protected:

    public:
        S64Packet(wxString type, uint16_t size, uint8_t* data);
        ~S64Packet();

        static bool IsS64Packet(uint8_t* bytes);
        static S64Packet* FromBytes(uint8_t* bytes);

        uint8_t  GetVersion();
        wxString GetType();
        uint16_t GetSize();
        uint8_t* GetData();
        uint16_t GetSequenceNumber();
        uint8_t* GetAsBytes();
        uint16_t GetAsBytes_Size();
        void SetSequenceNumber(uint16_t seqnum);
        void SetAck(uint16_t acknum);
        void SetAckBitfield(uint16_t bitfield);
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
        NetLibPacket(uint8_t version, uint8_t type, uint8_t flags, uint32_t recipients, uint16_t size, uint8_t* data, uint16_t seqnum, uint16_t acknum, uint16_t ackbitfield);

    protected:

    public:
        ~NetLibPacket();

        static bool IsNetLibPacket(uint8_t* bytes);
        static NetLibPacket* FromBytes(uint8_t* bytes);

        uint8_t  GetVersion();
        uint8_t  GetType();
        uint32_t GetRecipients();
        uint16_t GetSequenceNumber();
        uint16_t GetSize();
        uint8_t* GetData();
        uint8_t* GetAsBytes();
        uint16_t GetAsBytes_Size();
        void SetFlags(uint8_t flags);
        void SetSequenceNumber(uint16_t seqnum);
        void SetAck(uint16_t acknum);
        void SetAckBitfield(uint16_t bitfield);
};