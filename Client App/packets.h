#pragma once

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
        int m_Version;
        wxString m_Type;
        int m_Size;
        char* m_Data;
        S64Packet(int version, wxString type, int size, char* data);

    protected:

    public:
        S64Packet(wxString type, int size, char* data);
        ~S64Packet();

        static S64Packet* ReadPacket(wxSocketClient* socket);
        void SendPacket(wxSocketClient* socket);

        int GetVersion();
        wxString GetType();
        int GetSize();
        char* GetData();
};

// Netlib packet
class NetLibPacket
{
    private:
        int m_Version;
        int m_ID;
        int m_Recipients;
        int m_Size;
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