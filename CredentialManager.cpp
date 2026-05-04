// CredentialManager.cpp
// Implementation of the CredentialManager class
//

#include "pch.h"
#include "CredentialManager.h"

using namespace RetroAchievements_UWP;
using namespace Platform;
using namespace Windows::Storage;

CredentialManager^ CredentialManager::instance = nullptr;
const wchar_t* CredentialManager::USERNAME_KEY = L"SavedUsername";
const wchar_t* CredentialManager::APIKEY_KEY = L"SavedApiKey";

CredentialManager::CredentialManager()
{
}

CredentialManager^ CredentialManager::GetInstance()
{
    if (instance == nullptr)
    {
        instance = ref new CredentialManager();
    }
    return instance;
}

void CredentialManager::SaveCredentials(Platform::String^ username, Platform::String^ apiKey)
{
    try
    {
        auto localSettings = ApplicationData::Current->LocalSettings;
        localSettings->Values->Insert(ref new String(USERNAME_KEY), username);
        localSettings->Values->Insert(ref new String(APIKEY_KEY), apiKey);
    }
    catch (...)
    {
        // Silent fail if storage fails
    }
}

bool CredentialManager::LoadCredentials(Platform::String^* username, Platform::String^* apiKey)
{
    try
    {
        auto localSettings = ApplicationData::Current->LocalSettings;
        auto values = localSettings->Values;

        // Check if both keys exist
        if (values->HasKey(ref new String(USERNAME_KEY)) && values->HasKey(ref new String(APIKEY_KEY)))
        {
            *username = safe_cast<Platform::String^>(values->Lookup(ref new String(USERNAME_KEY)));
            *apiKey = safe_cast<Platform::String^>(values->Lookup(ref new String(APIKEY_KEY)));
            return true;
        }
    }
    catch (...)
    {
        // Silent fail if retrieval fails
    }

    return false;
}

bool CredentialManager::HasSavedCredentials()
{
    try
    {
        auto localSettings = ApplicationData::Current->LocalSettings;
        auto values = localSettings->Values;
        return values->HasKey(ref new String(USERNAME_KEY)) && values->HasKey(ref new String(APIKEY_KEY));
    }
    catch (...)
    {
        return false;
    }
}

void CredentialManager::ClearCredentials()
{
    try
    {
        auto localSettings = ApplicationData::Current->LocalSettings;
        auto values = localSettings->Values;

        if (values->HasKey(ref new String(USERNAME_KEY)))
        {
            values->Remove(ref new String(USERNAME_KEY));
        }
        if (values->HasKey(ref new String(APIKEY_KEY)))
        {
            values->Remove(ref new String(APIKEY_KEY));
        }
    }
    catch (...)
    {
        // Silent fail
    }
}
