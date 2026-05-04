// Profile.xaml.cpp
// Implementation of the Profile class.
//

#include "pch.h"
#include "Profile.xaml.h"
#include "MainPage.xaml.h"
#include "GameInfoPage.xaml.h"
#include "AllGamesPage.xaml.h"
#include <ppltasks.h>
#include <string>
#include <sstream>
#include <vector>
#include <functional>
#include "ThemeSettings.h"

using namespace RetroAchievements_UWP;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Media::Imaging;
using namespace Windows::Web::Http;
using namespace Windows::UI::Core;
using namespace concurrency;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::Storage;

// The Profile page

Profile::Profile()
{
    InitializeComponent();
}

void Profile::OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e)
{
    // Get the MainPage reference passed as parameter
    MainPage^ mainPage = dynamic_cast<MainPage^>(e->Parameter);
    if (mainPage != nullptr)
    {
        this->ApiKey = mainPage->ApiKey;
        this->Username = mainPage->Username;
        FetchUserProfile();
        FetchRecentlyPlayedGames();
        FetchRecentAchievements();
        FetchWantToPlayGames();
    }

    auto localSettings = ApplicationData::Current->LocalSettings;
    if (localSettings->Values->HasKey("DisplayHardcoreStats"))
    {
        HardcoreModeToggle->IsOn = safe_cast<bool>(localSettings->Values->Lookup("DisplayHardcoreStats"));
    }

    // Load theme setting
    BackgroundThemeToggle->IsOn = ThemeSettings::GetIsLightTheme();
    
    ApplyThemeSettings();

    // Set focus to the ProfilePivot to enable controller navigation immediately
    ProfilePivot->Focus(Windows::UI::Xaml::FocusState::Programmatic);
}

void Profile::FetchUserProfile()
{
    if (this->ApiKey == nullptr || this->Username == nullptr)
    {
        UserTextBlock->Text = "Error: Missing credentials";
        return;
    }

    // Build the API URL for get-user-profile
    // https://api-docs.retroachievements.org/v1/get-user-profile.html
    std::wstring baseUrl = L"https://retroachievements.org/API/API_GetUserProfile.php?";
    std::wstring fullUrl = baseUrl;
    fullUrl += L"z=" + std::wstring(this->ApiKey->Data());
    fullUrl += L"&y=" + std::wstring(this->ApiKey->Data());
    fullUrl += L"&u=" + std::wstring(this->Username->Data());

    auto uri = ref new Windows::Foundation::Uri(ref new Platform::String(fullUrl.c_str()));
    HttpClient^ client = ref new HttpClient();

    create_task(client->GetAsync(uri)).then([this](task<HttpResponseMessage^> prevTask)
    {
        try
        {
            HttpResponseMessage^ response = prevTask.get();
            if (response->IsSuccessStatusCode)
            {
                create_task(response->Content->ReadAsStringAsync()).then([this](Platform::String^ body)
                {
                    // Parse the JSON response to extract fields
                    std::wstring responseBody(body->Data());

                    // Helper lambda to extract string value from JSON
                    auto extractJsonString = [](const std::wstring& json, const std::wstring& key) -> std::wstring
                    {
                        size_t keyPos = json.find(L"\"" + key + L"\"");
                        if (keyPos != std::wstring::npos)
                        {
                            size_t colonPos = json.find(L":", keyPos);
                            size_t quotePos = json.find(L"\"", colonPos);
                            if (quotePos != std::wstring::npos)
                            {
                                std::wstring result;
                                size_t i = quotePos + 1;
                                while (i < json.length() && json[i] != L'\"')
                                {
                                    if (json[i] == L'\\' && i + 1 < json.length())
                                    {
                                        wchar_t next = json[i + 1];
                                        if (next == L'\"') result += L'\"';
                                        else if (next == L'\\') result += L'\\';
                                        else if (next == L'/') result += L'/';
                                        else if (next == L'b') result += L'\b';
                                        else if (next == L'f') result += L'\f';
                                        else if (next == L'n') result += L'\n';
                                        else if (next == L'r') result += L'\r';
                                        else if (next == L't') result += L'\t';
                                        else if (next == L'u' && i + 5 < json.length())
                                        {
                                            std::wstring hexStr = json.substr(i + 2, 4);
                                            try {
                                                wchar_t wc = (wchar_t)std::stoi(hexStr, nullptr, 16);
                                                result += wc;
                                            } catch (...) {}
                                            i += 4;
                                        } else {
                                            result += next;
                                        }
                                        i += 2;
                                    }
                                    else
                                    {
                                        result += json[i];
                                        i++;
                                    }
                                }

                                // Sometimes the API literally contains "\/" even after unescaping
                                size_t pos = 0;
                                while ((pos = result.find(L"\\/", pos)) != std::wstring::npos) {
                                    result.replace(pos, 2, L"/");
                                }

                                return result;
                            }
                        }
                        return L"";
                    };

                    // Helper lambda to extract numeric value from JSON
                    auto extractJsonNumber = [](const std::wstring& json, const std::wstring& key) -> std::wstring
                    {
                        size_t keyPos = json.find(L"\"" + key + L"\"");
                        if (keyPos != std::wstring::npos)
                        {
                            size_t colonPos = json.find(L":", keyPos);
                            size_t valueStart = json.find_first_not_of(L" \t", colonPos + 1);
                            size_t valueEnd = valueStart;
                            while (valueEnd < json.length() && (iswdigit(json[valueEnd]) || json[valueEnd] == L'.'))
                            {
                                valueEnd++;
                            }
                            if (valueStart != std::wstring::npos && valueEnd > valueStart)
                            {
                                return json.substr(valueStart, valueEnd - valueStart);
                            }
                        }
                        return L"";
                    };

                    std::wstring username = extractJsonString(responseBody, L"User");
                    std::wstring memberSince = extractJsonString(responseBody, L"MemberSince");
                    std::wstring motto = extractJsonString(responseBody, L"Motto");
                    std::wstring totalPoints = extractJsonNumber(responseBody, L"TotalPoints");
                    std::wstring userPic = extractJsonString(responseBody, L"UserPic");

                    auto dispatcher = this->Dispatcher;
                    dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this, username, memberSince, motto, totalPoints, userPic]() {
                        UserTextBlock->Text = ref new Platform::String(L"User: ") + ref new Platform::String(username.c_str());
                        MemberSinceTextBlock->Text = ref new Platform::String(L"Member Since: ") + ref new Platform::String(memberSince.c_str());
                        MottoTextBlock->Text = ref new Platform::String(L"Motto: ") + ref new Platform::String(motto.c_str());
                        TotalPointsTextBlock->Text = ref new Platform::String(L"Total Points: ") + ref new Platform::String(totalPoints.c_str());

                        if (!userPic.empty())
                        {
                            std::wstring imageUrl = L"https://retroachievements.org" + userPic;
                            auto uri = ref new Windows::Foundation::Uri(ref new Platform::String(imageUrl.c_str()));
                            UserProfileImage->Source = ref new Windows::UI::Xaml::Media::Imaging::BitmapImage(uri);
                        }
                    }));
                });
            }
            else
            {
                auto dispatcher = this->Dispatcher;
                dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this]() {
                    UserTextBlock->Text = "Error: Failed to fetch profile";
                }));
            }
        }
        catch (...)
        {
            auto dispatcher = this->Dispatcher;
            dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this]() {
                UserTextBlock->Text = "Error: Exception occurred";
            }));
        }
    });
}



void Profile::ClearCredentials_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    CredentialManager::GetInstance()->ClearCredentials();

    // Navigate back to MainPage
    this->Frame->Navigate(Windows::UI::Xaml::Interop::TypeName(MainPage::typeid), nullptr);
}

void Profile::FetchRecentAchievements()
{
    if (this->ApiKey == nullptr || this->Username == nullptr)
    {
        return;
    }

    // https://retroachievements.org/API/API_GetUserRecentAchievements.php
    std::wstring baseUrl = L"https://retroachievements.org/API/API_GetUserRecentAchievements.php?";
    std::wstring fullUrl = baseUrl;
    fullUrl += L"z=" + std::wstring(this->ApiKey->Data());
    fullUrl += L"&y=" + std::wstring(this->ApiKey->Data());
    fullUrl += L"&u=" + std::wstring(this->Username->Data());
    fullUrl += L"&m=525600"; // up to 1 year of recent achievements

    auto uri = ref new Windows::Foundation::Uri(ref new Platform::String(fullUrl.c_str()));
    HttpClient^ client = ref new HttpClient();

    create_task(client->GetAsync(uri)).then([this](task<HttpResponseMessage^> prevTask)
    {
        try
        {
            HttpResponseMessage^ response = prevTask.get();
            if (response->IsSuccessStatusCode)
            {
                create_task(response->Content->ReadAsStringAsync()).then([this](Platform::String^ body)
                {
                    std::wstring responseBody(body->Data());
                    
                    // Helper lambda to extract string value from JSON
                    auto extractJsonString = [](const std::wstring& json, const std::wstring& key) -> std::wstring {
                        size_t keyPos = json.find(L"\"" + key + L"\"");
                        if (keyPos != std::wstring::npos) {
                            size_t colonPos = json.find(L":", keyPos);
                            size_t quotePos = json.find(L"\"", colonPos);
                            if (quotePos != std::wstring::npos) {
                                std::wstring result;
                                size_t i = quotePos + 1;
                                while (i < json.length() && json[i] != L'\"') {
                                    if (json[i] == L'\\' && i + 1 < json.length()) {
                                        wchar_t next = json[i + 1];
                                        if (next == L'\"') result += L'\"';
                                        else if (next == L'\\') result += L'\\';
                                        else if (next == L'/') result += L'/';
                                        else if (next == L'b') result += L'\b';
                                        else if (next == L'f') result += L'\f';
                                        else if (next == L'n') result += L'\n';
                                        else if (next == L'r') result += L'\r';
                                        else if (next == L't') result += L'\t';
                                        else if (next == L'u' && i + 5 < json.length()) {
                                            std::wstring hexStr = json.substr(i + 2, 4);
                                            try {
                                                wchar_t wc = (wchar_t)std::stoi(hexStr, nullptr, 16);
                                                result += wc;
                                            } catch (...) {}
                                            i += 4;
                                        } else {
                                            result += next;
                                        }
                                        i += 2;
                                    } else {
                                        result += json[i];
                                        i++;
                                    }
                                }

                                // Sometimes the API literally contains "\/" even after unescaping
                                size_t pos = 0;
                                while ((pos = result.find(L"\\/", pos)) != std::wstring::npos) {
                                    result.replace(pos, 2, L"/");
                                }

                                return result;
                            }
                        }
                        return L"";
                    };

                    // Helper lambda to extract numeric value from JSON
                    auto extractJsonNumber = [](const std::wstring& json, const std::wstring& key) -> std::wstring {
                        size_t keyPos = json.find(L"\"" + key + L"\"");
                        if (keyPos != std::wstring::npos) {
                            size_t colonPos = json.find(L":", keyPos);
                            size_t valueStart = json.find_first_not_of(L" \t", colonPos + 1);
                            size_t valueEnd = valueStart;
                            while (valueEnd < json.length() && (iswdigit(json[valueEnd]) || json[valueEnd] == L'.')) {
                                valueEnd++;
                            }
                            if (valueStart != std::wstring::npos && valueEnd > valueStart) {
                                return json.substr(valueStart, valueEnd - valueStart);
                            }
                        }
                        return L"";
                    };

                    // Get the hardcore setting once
                    bool useHardcoreSetting = false;
                    auto localSettings = Windows::Storage::ApplicationData::Current->LocalSettings;
                    if (localSettings->Values->HasKey("DisplayHardcoreStats"))
                    {
                        useHardcoreSetting = safe_cast<bool>(localSettings->Values->Lookup("DisplayHardcoreStats"));
                    }

                    std::vector<RecentAchievementItem^> itemsList;
                    size_t currentPos = 0;

                    // Parse achievement objects in the JSON array
                    while (currentPos < responseBody.length() && itemsList.size() < 8)
                    {
                        size_t objectStart = responseBody.find(L"{", currentPos);
                        if (objectStart == std::wstring::npos)
                            break;

                        size_t objectEnd = responseBody.find(L"}", objectStart);
                        if (objectEnd == std::wstring::npos)
                            break;

                        std::wstring achObj = responseBody.substr(objectStart, objectEnd - objectStart + 1);
                        
                        // Check if this achievement matches the hardcore setting
                        std::wstring hardcoreVal = extractJsonNumber(achObj, L"HardcoreMode");
                        if (hardcoreVal.empty()) hardcoreVal = extractJsonString(achObj, L"HardcoreMode");
                        bool isHardcore = (hardcoreVal == L"1" || hardcoreVal == L"true");

                        if (useHardcoreSetting && !isHardcore)
                        {
                            currentPos = objectEnd + 1;
                            continue;
                        }

                        std::wstring badgeName = extractJsonString(achObj, L"BadgeName");
                        std::wstring title = extractJsonString(achObj, L"Title");
                        std::wstring description = extractJsonString(achObj, L"Description");
                        std::wstring gameTitle = extractJsonString(achObj, L"GameTitle");
                        std::wstring gameId = extractJsonNumber(achObj, L"GameID");
                        if (gameId.empty()) gameId = extractJsonString(achObj, L"GameID");
                        std::wstring points = extractJsonNumber(achObj, L"Points");
                        if (points.empty()) points = extractJsonString(achObj, L"Points");
                        std::wstring date = extractJsonString(achObj, L"Date");

                        if (!badgeName.empty())
                        {
                            RecentAchievementItem^ item = ref new RecentAchievementItem();
                            item->ImageUrl = ref new Platform::String((L"https://retroachievements.org/Badge/" + badgeName + L".png").c_str());
                            item->Title = ref new Platform::String(title.c_str());
                            item->Description = ref new Platform::String(description.c_str());
                            item->GameTitle = ref new Platform::String(gameTitle.c_str());
                            item->GameID = ref new Platform::String(gameId.c_str());
                            item->Points = ref new Platform::String((points + L" Points").c_str());
                            item->DateEarnedText = ref new Platform::String((L"Date Earned: " + date).c_str());
                            itemsList.push_back(item);
                        }

                        currentPos = objectEnd + 1;
                    }

                    auto dispatcher = this->Dispatcher;
                    dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this, itemsList]() {
                        auto items = RecentAchievementsGridView->Items;
                        items->Clear();
                        for (auto item : itemsList)
                        {
                            items->Append(item);
                        }
                    }));
                });
            }
        }
        catch (...)
        {
            // Fail gracefully
        }
    });
}

void Profile::RecentAchievementsGridView_ItemClick(Platform::Object^ sender, Windows::UI::Xaml::Controls::ItemClickEventArgs^ e)
{
    RecentAchievementItem^ clickedAch = dynamic_cast<RecentAchievementItem^>(e->ClickedItem);
    if (clickedAch != nullptr)
    {
        // Create a GameItem to pass to GameInfoPage
        GameItem^ game = ref new GameItem();
        game->GameID = clickedAch->GameID;
        game->Title = clickedAch->GameTitle;
        // The GameInfoPage will fetch progress itself, but we can set some defaults if we want
        game->AchievementProgress = ""; 
        game->NumAchieved = 0;
        game->NumPossibleAchievements = 0;

        GameInfoNavParam^ param = ref new GameInfoNavParam();
        param->Game = game;
        param->Header = GetProfileHeaderData();

        this->Frame->Navigate(Windows::UI::Xaml::Interop::TypeName(GameInfoPage::typeid), param);
    }
}

void Profile::FetchRecentlyPlayedGames()
{
    if (this->ApiKey == nullptr || this->Username == nullptr)
    {
        return;
    }

    // Build the API URL for get-user-recently-played-games
    // https://api-docs.retroachievements.org/v1/get-user-recently-played-games.html
    std::wstring baseUrl = L"https://retroachievements.org/API/API_GetUserRecentlyPlayedGames.php?";
    std::wstring fullUrl = baseUrl;
    fullUrl += L"z=" + std::wstring(this->ApiKey->Data());
    fullUrl += L"&y=" + std::wstring(this->ApiKey->Data());
    fullUrl += L"&u=" + std::wstring(this->Username->Data());
    fullUrl += L"&c=16"; // Get the last 16 recently played games

    auto uri = ref new Windows::Foundation::Uri(ref new Platform::String(fullUrl.c_str()));
    HttpClient^ client = ref new HttpClient();

    create_task(client->GetAsync(uri)).then([this](task<HttpResponseMessage^> prevTask)
    {
        try
        {
            HttpResponseMessage^ response = prevTask.get();
            if (response->IsSuccessStatusCode)
            {
                create_task(response->Content->ReadAsStringAsync()).then([this](Platform::String^ body)
                {
                    std::wstring responseBody(body->Data());

                    // Helper lambda to extract string value from JSON
                    auto extractJsonString = [](const std::wstring& json, const std::wstring& key) -> std::wstring
                    {
                        size_t keyPos = json.find(L"\"" + key + L"\"");
                        if (keyPos != std::wstring::npos)
                        {
                            size_t colonPos = json.find(L":", keyPos);
                            size_t quotePos = json.find(L"\"", colonPos);
                            if (quotePos != std::wstring::npos)
                            {
                                std::wstring result;
                                size_t i = quotePos + 1;
                                while (i < json.length() && json[i] != L'\"')
                                {
                                    if (json[i] == L'\\' && i + 1 < json.length())
                                    {
                                        wchar_t next = json[i + 1];
                                        if (next == L'\"') result += L'\"';
                                        else if (next == L'\\') result += L'\\';
                                        else if (next == L'/') result += L'/';
                                        else if (next == L'b') result += L'\b';
                                        else if (next == L'f') result += L'\f';
                                        else if (next == L'n') result += L'\n';
                                        else if (next == L'r') result += L'\r';
                                        else if (next == L't') result += L'\t';
                                        else if (next == L'u' && i + 5 < json.length())
                                        {
                                            std::wstring hexStr = json.substr(i + 2, 4);
                                            try {
                                                wchar_t wc = (wchar_t)std::stoi(hexStr, nullptr, 16);
                                                result += wc;
                                            } catch (...) {}
                                            i += 4;
                                        } else {
                                            result += next;
                                        }
                                        i += 2;
                                    }
                                    else
                                    {
                                        result += json[i];
                                        i++;
                                    }
                                }

                                // Sometimes the API literally contains "\/" even after unescaping
                                size_t pos = 0;
                                while ((pos = result.find(L"\\/", pos)) != std::wstring::npos) {
                                    result.replace(pos, 2, L"/");
                                }

                                return result;
                            }
                        }
                        return L"";
                    };

                    // Helper lambda to extract numeric value from JSON
                    auto extractJsonNumber = [](const std::wstring& json, const std::wstring& key) -> std::wstring
                    {
                        size_t keyPos = json.find(L"\"" + key + L"\"");
                        if (keyPos != std::wstring::npos)
                        {
                            size_t colonPos = json.find(L":", keyPos);
                            size_t valueStart = json.find_first_not_of(L" \t", colonPos + 1);
                            size_t valueEnd = valueStart;
                            while (valueEnd < json.length() && (iswdigit(json[valueEnd]) || json[valueEnd] == L'.'))
                            {
                                valueEnd++;
                            }
                            if (valueStart != std::wstring::npos && valueEnd > valueStart)
                            {
                                return json.substr(valueStart, valueEnd - valueStart);
                            }
                        }
                        return L"";
                    };

                    // Parse the JSON array of games
                    // The API returns an array of game objects
                    std::vector<GameItem^> gameItems;
                    size_t currentPos = 0;

                    // Find all game objects in the array
                    while (currentPos < responseBody.length())
                    {
                        size_t objectStart = responseBody.find(L"{", currentPos);
                        if (objectStart == std::wstring::npos)
                            break;

                        size_t objectEnd = responseBody.find(L"}", objectStart);
                        if (objectEnd == std::wstring::npos)
                            break;

                        std::wstring gameObject = responseBody.substr(objectStart, objectEnd - objectStart + 1);
                        std::wstring gameID = extractJsonNumber(gameObject, L"GameID");
                        std::wstring imageIcon = extractJsonString(gameObject, L"ImageIcon");
                        std::wstring title = extractJsonString(gameObject, L"Title");
                        std::wstring consoleName = extractJsonString(gameObject, L"ConsoleName");
                        std::wstring numAchieved = extractJsonNumber(gameObject, L"NumAchieved");
                        std::wstring numPossible = extractJsonNumber(gameObject, L"NumPossibleAchievements");

                        if (!imageIcon.empty())
                        {
                            GameItem^ item = ref new GameItem();
                            item->GameID = ref new Platform::String(gameID.c_str());
                            item->ImageUrl = ref new Platform::String((L"https://retroachievements.org" + imageIcon).c_str());
                            item->Title = ref new Platform::String(title.c_str());
                            item->ConsoleName = ref new Platform::String(consoleName.c_str());
                            std::wstring progress = numAchieved + L"/" + numPossible + L" earned";
                            item->AchievementProgress = ref new Platform::String(progress.c_str());
                            
                            double achievedVal = 0.0;
                            if (!numAchieved.empty()) {
                                achievedVal = _wtof(numAchieved.c_str());
                            }
                            
                            double possibleVal = 1.0;
                            if (!numPossible.empty()) {
                                possibleVal = _wtof(numPossible.c_str());
                                if (possibleVal <= 0) possibleVal = 1.0;
                            }
                            
                            item->NumAchieved = achievedVal;
                            item->NumPossibleAchievements = possibleVal;
                            
                            gameItems.push_back(item);
                        }

                        currentPos = objectEnd + 1;
                    }

                    auto dispatcher = this->Dispatcher;
                    dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this, gameItems]() {
                        auto items = RecentGamesGridView->Items;
                        items->Clear();

                        for (auto item : gameItems)
                        {
                            items->Append(item);
                        }
                    }));
                });
            }
            else
            {
                auto dispatcher = this->Dispatcher;
                dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this]() {
                    auto items = RecentGamesGridView->Items;
                    items->Clear();
                }));
            }
        }
        catch (...)
        {
            auto dispatcher = this->Dispatcher;
            dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this]() {
                auto items = RecentGamesGridView->Items;
                items->Clear();
            }));
        }
    });
}

UserProfileHeaderData^ Profile::GetProfileHeaderData()
{
    UserProfileHeaderData^ headerData = ref new UserProfileHeaderData();
    headerData->ProfileImage = UserProfileImage->Source;
    headerData->Username = UserTextBlock->Text;
    headerData->MemberSince = MemberSinceTextBlock->Text;
    headerData->Motto = MottoTextBlock->Text;
    headerData->TotalPoints = TotalPointsTextBlock->Text;
    return headerData;
}

void Profile::ViewAllGames_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    this->Frame->Navigate(Windows::UI::Xaml::Interop::TypeName(AllGamesPage::typeid), GetProfileHeaderData());
}

void Profile::RecentGamesGridView_ItemClick(Platform::Object^ sender, Windows::UI::Xaml::Controls::ItemClickEventArgs^ e)
{
    GameItem^ clickedItem = dynamic_cast<GameItem^>(e->ClickedItem);
    if (clickedItem != nullptr)
    {
        GameInfoNavParam^ param = ref new GameInfoNavParam();
        param->Game = clickedItem;
        param->Header = GetProfileHeaderData();

        this->Frame->Navigate(Windows::UI::Xaml::Interop::TypeName(GameInfoPage::typeid), param);
    }
}

void Profile::FetchWantToPlayGames()
{
    if (this->ApiKey == nullptr || this->Username == nullptr)
    {
        return;
    }

    std::wstring baseUrl = L"https://retroachievements.org/API/API_GetUserWantToPlayList.php?";
    std::wstring fullUrl = baseUrl;
    fullUrl += L"z=" + std::wstring(this->ApiKey->Data());
    fullUrl += L"&y=" + std::wstring(this->ApiKey->Data());
    fullUrl += L"&u=" + std::wstring(this->Username->Data());

    auto uri = ref new Windows::Foundation::Uri(ref new Platform::String(fullUrl.c_str()));
    HttpClient^ client = ref new HttpClient();

    create_task(client->GetAsync(uri)).then([this](task<HttpResponseMessage^> prevTask)
    {
        try
        {
            HttpResponseMessage^ response = prevTask.get();
            if (response->IsSuccessStatusCode)
            {
                create_task(response->Content->ReadAsStringAsync()).then([this](Platform::String^ body)
                {
                    try
                    {
                        std::wstring responseBody(body->Data());
                        std::vector<GameItem^> games;

                        size_t currentPos = 0;
                        while (currentPos < responseBody.length())
                        {
                            size_t objectStart = responseBody.find(L"{", currentPos);
                            if (objectStart == std::wstring::npos) break;

                            size_t objectEnd = responseBody.find(L"}", objectStart);
                            if (objectEnd == std::wstring::npos) break;

                            std::wstring gameObj = responseBody.substr(objectStart, objectEnd - objectStart + 1);
                            
                            auto extractJsonString = [](const std::wstring& json, const std::wstring& key) -> std::wstring
                            {
                                size_t keyPos = json.find(L"\"" + key + L"\"");
                                if (keyPos != std::wstring::npos)
                                {
                                    size_t colonPos = json.find(L":", keyPos);
                                    size_t quotePos = json.find(L"\"", colonPos);
                                    if (quotePos != std::wstring::npos)
                                    {
                                        std::wstring result;
                                        size_t i = quotePos + 1;
                                        while (i < json.length() && json[i] != L'\"')
                                        {
                                            if (json[i] == L'\\' && i + 1 < json.length())
                                            {
                                                wchar_t next = json[i + 1];
                                                if (next == L'\"') result += L'\"';
                                                else if (next == L'\\') result += L'\\';
                                                else if (next == L'/') result += L'/';
                                                else if (next == L'b') result += L'\b';
                                                else if (next == L'f') result += L'\f';
                                                else if (next == L'n') result += L'\n';
                                                else if (next == L'r') result += L'\r';
                                                else if (next == L't') result += L'\t';
                                                else if (next == L'u' && i + 5 < json.length())
                                                {
                                                    std::wstring hexStr = json.substr(i + 2, 4);
                                                    try {
                                                        wchar_t wc = (wchar_t)std::stoi(hexStr, nullptr, 16);
                                                        result += wc;
                                                    } catch (...) {}
                                                    i += 4;
                                                } else {
                                                    result += next;
                                                }
                                                i += 2;
                                            }
                                            else
                                            {
                                                result += json[i];
                                                i++;
                                            }
                                        }
                                        
                                        // Sometimes the API literally contains "\/" even after unescaping
                                        size_t pos = 0;
                                        while ((pos = result.find(L"\\/", pos)) != std::wstring::npos) {
                                            result.replace(pos, 2, L"/");
                                        }
                                        
                                        return result;
                                    }
                                }
                                return L"";
                            };
                            
                            auto extractJsonNumber = [](const std::wstring& json, const std::wstring& key) -> std::wstring
                            {
                                size_t keyPos = json.find(L"\"" + key + L"\"");
                                if (keyPos != std::wstring::npos)
                                {
                                    size_t colonPos = json.find(L":", keyPos);
                                    size_t valueStart = json.find_first_of(L"0123456789-.", colonPos);
                                    if (valueStart != std::wstring::npos)
                                    {
                                        size_t valueEnd = json.find_first_not_of(L"0123456789-.", valueStart);
                                        if (valueEnd != std::wstring::npos)
                                        {
                                            return json.substr(valueStart, valueEnd - valueStart);
                                        }
                                        return json.substr(valueStart);
                                    }
                                }
                                return L"";
                            };

                            std::wstring idStr = extractJsonNumber(gameObj, L"ID");
                            if (idStr.empty()) idStr = extractJsonString(gameObj, L"ID");
                            std::wstring title = extractJsonString(gameObj, L"Title");
                            std::wstring consoleName = extractJsonString(gameObj, L"ConsoleName");
                            std::wstring imageIcon = extractJsonString(gameObj, L"ImageIcon");

                            if (!idStr.empty() && !title.empty())
                            {
                                GameItem^ item = ref new GameItem();
                                item->GameID = ref new Platform::String(idStr.c_str());
                                item->Title = ref new Platform::String(title.c_str());
                                item->ConsoleName = ref new Platform::String(consoleName.c_str());
                                
                                std::wstring imageUrl = L"https://retroachievements.org" + imageIcon;
                                item->ImageUrl = ref new Platform::String(imageUrl.c_str());

                                games.push_back(item);
                            }

                            currentPos = objectEnd + 1;
                        }

                        auto dispatcher = this->Dispatcher;
                        dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this, games]() {
                            auto items = WantToPlayGridView->Items;
                            items->Clear();
                            for (auto game : games)
                            {
                                items->Append(game);
                            }
                        }));
                    }
                    catch (...) {}
                });
            }
        }
        catch (...)
        {
            // Fail gracefully
        }
    });
}

void Profile::WantToPlayGridView_ItemClick(Platform::Object^ sender, Windows::UI::Xaml::Controls::ItemClickEventArgs^ e)
{
    GameItem^ clickedItem = dynamic_cast<GameItem^>(e->ClickedItem);
    if (clickedItem != nullptr)
    {
        GameInfoNavParam^ param = ref new GameInfoNavParam();
        param->Game = clickedItem;
        param->Header = GetProfileHeaderData();

        this->Frame->Navigate(Windows::UI::Xaml::Interop::TypeName(GameInfoPage::typeid), param);
    }
}

void Profile::HardcoreModeToggle_Toggled(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto localSettings = ApplicationData::Current->LocalSettings;
    localSettings->Values->Insert("DisplayHardcoreStats", HardcoreModeToggle->IsOn);
}

void Profile::BackgroundThemeToggle_Toggled(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto localSettings = ApplicationData::Current->LocalSettings;
    localSettings->Values->Insert("IsLightTheme", BackgroundThemeToggle->IsOn);
    ApplyThemeSettings();
}

void Profile::Page_KeyDown(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
{
    if (e->Key == Windows::System::VirtualKey::GamepadLeftShoulder)
    {
        int nextIndex = (ProfilePivot->SelectedIndex - 1 + ProfilePivot->Items->Size) % ProfilePivot->Items->Size;
        ProfilePivot->SelectedIndex = nextIndex;
        e->Handled = true;
    }
    else if (e->Key == Windows::System::VirtualKey::GamepadRightShoulder)
    {
        int nextIndex = (ProfilePivot->SelectedIndex + 1) % ProfilePivot->Items->Size;
        ProfilePivot->SelectedIndex = nextIndex;
        e->Handled = true;
    }
    else if (e->Key == Windows::System::VirtualKey::GamepadB || e->Key == Windows::System::VirtualKey::Escape)
    {
        // Go back to login if we're on the profile
        if (this->Frame->CanGoBack)
        {
            this->Frame->GoBack();
        }
        else
        {
            this->Frame->Navigate(Windows::UI::Xaml::Interop::TypeName(MainPage::typeid));
        }
        e->Handled = true;
    }
}

void Profile::ApplyThemeSettings()
{
    ThemeSettings::ApplyAccentToResources(Application::Current->Resources);
    ThemeSettings::ApplyAccentToResources(this->Resources);
    
    auto targetTheme = ThemeSettings::GetElementTheme();
    
    // Force a re-evaluation of ThemeResources by toggling the theme
    this->RequestedTheme = (targetTheme == ElementTheme::Light) ? ElementTheme::Dark : ElementTheme::Light;
    this->RequestedTheme = targetTheme;

    ThemeSettings::ApplyThemeToTitle(AppTitleTextBlock);
    ThemeSettings::ApplyAccentToButton(ViewAllGamesButton);
    ThemeSettings::ApplyAccentToButton(ClearCredentialsButton);
}

void Profile::OnContainerContentChanging(ListViewBase^ sender, ContainerContentChangingEventArgs^ args)
{
    if (!args->InRecycleQueue)
    {
        auto container = args->ItemContainer;
        if (container != nullptr)
        {
            // Attach focus handlers to the container (the GridViewItem)
            container->GotFocus += ref new RoutedEventHandler(this, &Profile::Item_GotFocus);
            container->LostFocus += ref new RoutedEventHandler(this, &Profile::Item_LostFocus);
        }
    }
}

void Profile::Item_GotFocus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto container = dynamic_cast<DependencyObject^>(sender);
    if (container != nullptr)
    {
        // Helper to find an element with a tooltip recursively
        std::function<void(DependencyObject^)> openTooltip = [&](DependencyObject^ obj)
        {
            auto tooltip = ToolTipService::GetToolTip(obj);
            if (tooltip != nullptr)
            {
                auto tt = dynamic_cast<ToolTip^>(tooltip);
                if (tt != nullptr)
                {
                    tt->IsOpen = true;
                }
                return;
            }

            int count = Windows::UI::Xaml::Media::VisualTreeHelper::GetChildrenCount(obj);
            for (int i = 0; i < count; i++)
            {
                openTooltip(Windows::UI::Xaml::Media::VisualTreeHelper::GetChild(obj, i));
            }
        };

        openTooltip(container);
    }
}

void Profile::Item_LostFocus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto container = dynamic_cast<DependencyObject^>(sender);
    if (container != nullptr)
    {
        // Helper to close tooltip recursively
        std::function<void(DependencyObject^)> closeTooltip = [&](DependencyObject^ obj)
        {
            auto tooltip = ToolTipService::GetToolTip(obj);
            if (tooltip != nullptr)
            {
                auto tt = dynamic_cast<ToolTip^>(tooltip);
                if (tt != nullptr)
                {
                    tt->IsOpen = false;
                }
                return;
            }

            int count = Windows::UI::Xaml::Media::VisualTreeHelper::GetChildrenCount(obj);
            for (int i = 0; i < count; i++)
            {
                closeTooltip(Windows::UI::Xaml::Media::VisualTreeHelper::GetChild(obj, i));
            }
        };

        closeTooltip(container);
    }
}
