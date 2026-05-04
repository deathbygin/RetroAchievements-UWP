#include "pch.h"
#include "LeaderboardPage.xaml.h"
#include "Profile.xaml.h"
#include "CredentialManager.h"
#include "ThemeSettings.h"
#include <ppltasks.h>
#include <string>
#include <vector>
#include <functional>

using namespace RetroAchievements_UWP;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::Web::Http;
using namespace Windows::UI::Core;
using namespace concurrency;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::Web::Http;
using namespace Windows::UI::Core;
using namespace concurrency;

// Helper to extract JSON string
static std::wstring extractJsonString2(const std::wstring& json, const std::wstring& key)
{
    std::wstring searchKey = L"\"" + key + L"\":";
    size_t startPos = json.find(searchKey);
    if (startPos == std::wstring::npos) return L"";

    startPos += searchKey.length();
    
    // Skip whitespace
    while (startPos < json.length() && (json[startPos] == L' ' || json[startPos] == L'\t'))
    {
        startPos++;
    }

    if (startPos >= json.length()) return L"";

    bool isString = (json[startPos] == L'"');
    if (isString) startPos++; // Skip opening quote

    std::wstring result = L"";
    bool escaped = false;
    
    while (startPos < json.length())
    {
        wchar_t c = json[startPos];
        if (isString)
        {
            if (escaped)
            {
                if (c == L'u' && startPos + 4 < json.length())
                {
                    std::wstring hexStr = json.substr(startPos + 1, 4);
                    wchar_t wc = (wchar_t)std::wcstol(hexStr.c_str(), nullptr, 16);
                    result += wc;
                    startPos += 4;
                }
                else if (c == L'/') result += L'/';
                else if (c == L'"') result += L'"';
                else if (c == L'\\') result += L'\\';
                else if (c == L'n') result += L'\n';
                else if (c == L'r') result += L'\r';
                else if (c == L't') result += L'\t';
                escaped = false;
            }
            else if (c == L'\\')
            {
                escaped = true;
            }
            else if (c == L'"')
            {
                break; // End of string
            }
            else
            {
                result += c;
            }
        }
        else
        {
            // For unquoted values, stop at comma, brace, or whitespace
            if (c == L',' || c == L'}' || c == L']' || c == L' ' || c == L'\r' || c == L'\n')
            {
                break;
            }
            result += c;
        }
        startPos++;
    }

    return result;
}
LeaderboardPage::LeaderboardPage()
{
    InitializeComponent();
}

void LeaderboardPage::OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e)
{
    GameInfoNavParam^ param = dynamic_cast<GameInfoNavParam^>(e->Parameter);
    if (param != nullptr)
    {
        UserProfileHeaderData^ headerData = param->Header;
        if (headerData != nullptr)
        {
            UserProfileImage->Source = headerData->ProfileImage;
            UserTextBlock->Text = headerData->Username;
            MemberSinceTextBlock->Text = headerData->MemberSince;
            MottoTextBlock->Text = headerData->Motto;
            TotalPointsTextBlock->Text = headerData->TotalPoints;
        }

        ThemeSettings::ApplyAccentToResources(this->Resources);
        this->RequestedTheme = ThemeSettings::GetElementTheme();
        ThemeSettings::ApplyThemeToTitle(AppTitleTextBlock);
        ThemeSettings::ApplyThemeToBackButton(BackButton);
        BackButton->Focus(Windows::UI::Xaml::FocusState::Programmatic);

        GameItem^ game = param->Game;
        if (game != nullptr)
        {
            FetchGameLeaderboards(game->GameID, game->ImageUrl);
        }
    }
}

void LeaderboardPage::FetchGameLeaderboards(Platform::String^ gameId, Platform::String^ gameImageUrl)
{
    Platform::String^ username;
    Platform::String^ apiKey;

    if (!CredentialManager::GetInstance()->LoadCredentials(&username, &apiKey))
    {
        return;
    }

    Platform::String^ url = "https://retroachievements.org/API/API_GetGameLeaderboards.php?z=" + username + "&y=" + apiKey + "&i=" + gameId;
    Platform::String^ url2 = "https://retroachievements.org/API/API_GetUserGameLeaderboards.php?z=" + username + "&y=" + apiKey + "&u=" + username + "&i=" + gameId;
    
    Windows::Foundation::Uri^ uri = ref new Windows::Foundation::Uri(url);
    Windows::Foundation::Uri^ uri2 = ref new Windows::Foundation::Uri(url2);
    HttpClient^ httpClient = ref new HttpClient();

    create_task(httpClient->GetAsync(uri)).then([this, httpClient, uri2, gameImageUrl](task<Windows::Web::Http::HttpResponseMessage^> prevTask) {
        try
        {
            auto responseMsg = prevTask.get();
            if (responseMsg->IsSuccessStatusCode)
            {
                create_task(responseMsg->Content->ReadAsStringAsync()).then([this, httpClient, uri2, gameImageUrl](task<Platform::String^> prevStrTask) {
                    try
                    {
                        Platform::String^ response = prevStrTask.get();
                        if (response != nullptr && !response->IsEmpty())
                        {
                            std::wstring responseBody(response->Data());
                            std::vector<LeaderboardItem^> leaderboards;
                            
                            size_t currentPos = 0;
                            while (currentPos < responseBody.length())
                            {
                                size_t objectStart = responseBody.find(L"{", currentPos);
                                if (objectStart == std::wstring::npos) break;

                                size_t objectEnd = responseBody.find(L"}", objectStart);
                                if (objectEnd == std::wstring::npos) break;

                                std::wstring leaderboardObj = responseBody.substr(objectStart, objectEnd - objectStart + 1);
                                std::wstring idStr = extractJsonString2(leaderboardObj, L"ID");
                                std::wstring title = extractJsonString2(leaderboardObj, L"Title");
                                std::wstring desc = extractJsonString2(leaderboardObj, L"Description");
                                
                                std::wstring topUser = extractJsonString2(leaderboardObj, L"User");
                                std::wstring topScore = extractJsonString2(leaderboardObj, L"FormattedScore");

                                if (!title.empty())
                                {
                                    LeaderboardItem^ item = ref new LeaderboardItem();
                                    item->LeaderboardID = ref new Platform::String(idStr.c_str());
                                    item->Title = ref new Platform::String(title.c_str());
                                    item->Description = ref new Platform::String(desc.c_str());
                                    
                                    if (!topUser.empty())
                                    {
                                        item->TopEntryUser = ref new Platform::String((L"Top User: " + topUser).c_str());
                                        item->TopEntryScore = ref new Platform::String((L"Score: " + topScore).c_str());
                                    }
                                    else
                                    {
                                        item->TopEntryUser = "Top User: N/A";
                                        item->TopEntryScore = "Score: N/A";
                                    }
                                    
                                    item->UserStatsVisibility = Windows::UI::Xaml::Visibility::Collapsed;
                                    item->UserNoStatsVisibility = Windows::UI::Xaml::Visibility::Visible;
                                    
                                    leaderboards.push_back(item);
                                }

                                currentPos = objectEnd + 1;
                            }

                            // Second API Call
                            create_task(httpClient->GetAsync(uri2)).then([this, leaderboards](task<Windows::Web::Http::HttpResponseMessage^> prevTask2) {
                                try
                                {
                                    auto responseMsg2 = prevTask2.get();
                                    if (responseMsg2->IsSuccessStatusCode)
                                    {
                                        create_task(responseMsg2->Content->ReadAsStringAsync()).then([this, leaderboards](task<Platform::String^> prevStrTask2) {
                                            try
                                            {
                                                Platform::String^ response2 = prevStrTask2.get();
                                                if (response2 != nullptr && !response2->IsEmpty())
                                                {
                                                    std::wstring rBody(response2->Data());
                                                    size_t cPos = 0;
                                                    while (cPos < rBody.length())
                                                    {
                                                        size_t oStart = rBody.find(L"{", cPos);
                                                        if (oStart == std::wstring::npos) break;
                                                        size_t oEnd = rBody.find(L"}", oStart);
                                                        if (oEnd == std::wstring::npos) break;

                                                        std::wstring lObj = rBody.substr(oStart, oEnd - oStart + 1);
                                                        std::wstring lId = extractJsonString2(lObj, L"LeaderboardID");
                                                        if (lId.empty()) lId = extractJsonString2(lObj, L"ID");

                                                        std::wstring rank = extractJsonString2(lObj, L"Rank");
                                                        std::wstring score = extractJsonString2(lObj, L"FormattedScore");

                                                        if (!lId.empty())
                                                        {
                                                            for (auto item : leaderboards)
                                                            {
                                                                if (item->LeaderboardID != nullptr && std::wstring(item->LeaderboardID->Data()) == lId)
                                                                {
                                                                    item->UserStatsVisibility = Windows::UI::Xaml::Visibility::Visible;
                                                                    item->UserNoStatsVisibility = Windows::UI::Xaml::Visibility::Collapsed;
                                                                    item->UserRank = ref new Platform::String((L"Rank: " + rank).c_str());
                                                                    item->UserFormattedScore = ref new Platform::String((L"Score: " + score).c_str());
                                                                    break;
                                                                }
                                                            }
                                                        }
                                                        cPos = oEnd + 1;
                                                    }
                                                }
                                            }
                                            catch (...) { }

                                            auto dispatcher = this->Dispatcher;
                                            dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this, leaderboards]() {
                                                auto items = LeaderboardsGridView->Items;
                                                items->Clear();
                                                for (auto item : leaderboards)
                                                {
                                                    items->Append(item);
                                                }
                                            }));
                                        });
                                    }
                                    else
                                    {
                                        auto dispatcher = this->Dispatcher;
                                        dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this, leaderboards]() {
                                            auto items = LeaderboardsGridView->Items;
                                            items->Clear();
                                            for (auto item : leaderboards)
                                            {
                                                items->Append(item);
                                            }
                                        }));
                                    }
                                }
                                catch (...)
                                {
                                    auto dispatcher = this->Dispatcher;
                                    dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this, leaderboards]() {
                                        auto items = LeaderboardsGridView->Items;
                                        items->Clear();
                                        for (auto item : leaderboards)
                                        {
                                            items->Append(item);
                                        }
                                    }));
                                }
                            });
                        }
                    }
                    catch (...)
                    {
                        // Fail gracefully
                    }
                });
            }
        }
        catch (...)
        {
            // Fail gracefully
        }
    });
}

void LeaderboardPage::BackButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    if (this->Frame->CanGoBack)
    {
        this->Frame->GoBack();
    }
}

void LeaderboardPage::Page_KeyDown(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
{
    if (e->Key == Windows::System::VirtualKey::GamepadB || e->Key == Windows::System::VirtualKey::Escape)
    {
        if (this->Frame->CanGoBack)
        {
            this->Frame->GoBack();
            e->Handled = true;
        }
    }
}

void LeaderboardPage::OnContainerContentChanging(ListViewBase^ sender, ContainerContentChangingEventArgs^ args)
{
    if (!args->InRecycleQueue)
    {
        auto container = args->ItemContainer;
        if (container != nullptr)
        {
            container->GotFocus += ref new RoutedEventHandler(this, &LeaderboardPage::Item_GotFocus);
            container->LostFocus += ref new RoutedEventHandler(this, &LeaderboardPage::Item_LostFocus);
        }
    }
}

void LeaderboardPage::Item_GotFocus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto container = dynamic_cast<DependencyObject^>(sender);
    if (container != nullptr)
    {
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

void LeaderboardPage::Item_LostFocus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto container = dynamic_cast<DependencyObject^>(sender);
    if (container != nullptr)
    {
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
