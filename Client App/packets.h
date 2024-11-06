#pragma once

#include <wx/string.h>
#include <wx/socket.h>

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