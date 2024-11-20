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
        ~UDPHandler();
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
        wxString m_Type;
        uint32_t m_Size;
        uint16_t m_SequenceNum;
        uint16_t m_Ack;
        uint16_t m_AckBitField;
        uint8_t* m_Data;
        S64Packet(int version, wxString type, int size, uint8_t* data);

    protected:

    public:
        S64Packet(wxString type, int size, uint8_t* data);
        ~S64Packet();

        static bool IsS64Packet(uint8_t* data);
        static S64Packet* FromBytes(uint8_t* data);

        int      GetVersion();
        wxString GetType();
        int      GetSize();
        uint8_t* GetData();
        uint8_t* GetAsBytes();
        int      GetAsBytes_Size();
        void SetSequenceNumber(uint16_t seqnum);
        void SetAck(uint16_t seqnum);
        void SetAckBitfield(uint16_t seqnum);
        uint16_t GetSequenceNumber();
};

// Netlib packet
class NetLibPacket
{
    private:
        uint8_t  m_Version;
        uint32_t m_Recipients;
        uint8_t  m_ID;
        uint8_t  m_Flags;
        uint16_t m_Size;
        uint16_t m_SequenceNum;
        uint16_t m_Ack;
        uint8_t* m_Data;
        NetLibPacket(int version, int id, int recipients, int size, uint8_t* data);

    protected:

    public:
        ~NetLibPacket();

        static bool IsNetLibPacket(uint8_t* data);
        static NetLibPacket* FromBytes(uint8_t* data);

        int      GetVersion();
        int      GetID();
        int      GetSize();
        int      GetRecipients();
        uint8_t* GetData();
        uint8_t* GetAsBytes();
        int      GetAsBytes_Size();
        void SetSequenceNumber(uint16_t seqnum);
        void SetAck(uint16_t seqnum);
        void SetAckBitfield(uint16_t seqnum);
        uint16_t GetSequenceNumber();
};