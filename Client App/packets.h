#pragma once

#include <stdint.h>
#include <wx/string.h>
#include <wx/socket.h>

// UNFLoader USB packet
class USBPacket
{
    private:
        int m_Type;
        int m_Size;
        char* m_Data;

    protected:

    public:
        USBPacket(int type, int size, char* data);
        ~USBPacket();

        static USBPacket* ReadPacket(wxSocketClient* socket);
        void SendPacket(wxSocketClient* socket);

        int GetType();
        int GetSize();
        char* GetData();
};

// Server browser packet
class S64Packet
{
    private:
        uint8_t m_Version;
        wxString m_Type;
        uint32_t m_Size;
        uint16_t m_SequenceNum;
        uint16_t m_Ack;
        uint16_t m_AckBitField;
        char* m_Data;
        S64Packet(int version, wxString type, int size, char* data);

    protected:

    public:
        S64Packet(wxString type, int size, char* data);
        ~S64Packet();

        static S64Packet* ReadPacket(wxDatagramSocket* socket);
        void SendPacket(wxDatagramSocket* socket, wxIPV4address address);

        int GetVersion();
        wxString GetType();
        int GetSize();
        char* GetData();
};

// Netlib packet
class NetLibPacket
{
    private:
        uint8_t m_Version;
        uint32_t m_Recipients;
        uint8_t  m_ID;
        uint8_t  m_Flags;
        uint16_t m_Size;
        uint16_t m_SequenceNum;
        uint16_t m_Ack;
        char* m_Data;
        NetLibPacket(int version, int id, int recipients, int size, char* data);

    protected:

    public:
        ~NetLibPacket();

        static NetLibPacket* FromBytes(char* data);
        static NetLibPacket* ReadPacket(wxSocketClient* socket);
        void SendPacket(wxSocketClient* socket);

        int GetVersion();
        int GetID();
        int GetSize();
        int GetRecipients();
        char* GetData();
        char* GetAsBytes();
        int GetAsBytes_Size();
};