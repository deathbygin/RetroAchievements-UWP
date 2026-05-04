// CredentialManager.h
// Manages persistent storage of API credentials
//

#pragma once

namespace RetroAchievements_UWP
{
    public ref class CredentialManager sealed
    {
    public:
        static CredentialManager^ GetInstance();

        // Save credentials to persistent storage
        void SaveCredentials(Platform::String^ username, Platform::String^ apiKey);

        // Load credentials from persistent storage
        bool LoadCredentials(Platform::String^* username, Platform::String^* apiKey);

        // Clear saved credentials
        void ClearCredentials();

        // Check if credentials are saved
        bool HasSavedCredentials();

    private:
        CredentialManager();
        static CredentialManager^ instance;

        static const wchar_t* USERNAME_KEY;
        static const wchar_t* APIKEY_KEY;
    };
}
