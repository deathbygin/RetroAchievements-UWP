#include "pch.h"
#include "GameInfoPage.xaml.h"
#include "Profile.xaml.h" // For GameItem
#include "LeaderboardPage.xaml.h"
#include "CredentialManager.h"
#include <ppltasks.h>
#include <string>
#include <vector>
#include <functional>
#include "ThemeSettings.h"

using namespace RetroAchievements_UWP;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::Web::Http;
using namespace Windows::UI::Core;
using namespace concurrency;

GameInfoPage::GameInfoPage()
{
    InitializeComponent();
}

void GameInfoPage::OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e)
{
    GameItem^ game = nullptr;
    UserProfileHeaderData^ headerData = nullptr;

    auto param = dynamic_cast<GameInfoNavParam^>(e->Parameter);
    if (param != nullptr)
    {
        game = param->Game;
        headerData = param->Header;
    }
    else
    {
        game = dynamic_cast<GameItem^>(e->Parameter);
    }

    if (game != nullptr)
    {
        GameTitleTextBlock->Text = game->Title;
        GameIdTextBlock->Text = "Game ID: " + game->GameID;
        AchievementProgressTextBlock->Text = game->AchievementProgress;
        AchievementProgressBar->Maximum = game->NumPossibleAchievements;
        AchievementProgressBar->Value = game->NumAchieved;
        FetchGameProgress(game->GameID);
    }

    m_navParam = e->Parameter;

    if (headerData != nullptr)
    {
        UserProfileImage->Source = headerData->ProfileImage;
        UserTextBlock->Text = headerData->Username;
        MemberSinceTextBlock->Text = headerData->MemberSince;
        TotalPointsTextBlock->Text = headerData->TotalPoints;
    }

    ThemeSettings::ApplyAccentToResources(this->Resources);
    this->RequestedTheme = ThemeSettings::GetElementTheme();
    ThemeSettings::ApplyThemeToTitle(AppTitleTextBlock);
    ThemeSettings::ApplyThemeToBackButton(BackButton);
    ThemeSettings::ApplyAccentToButton(LeaderboardButton);
    ThemeSettings::ApplyAccentToProgressBar(AchievementProgressBar);

    BackButton->Focus(Windows::UI::Xaml::FocusState::Programmatic);
}

void GameInfoPage::BackButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    if (this->Frame->CanGoBack)
    {
        this->Frame->GoBack();
    }
}

void GameInfoPage::LeaderboardButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    this->Frame->Navigate(Windows::UI::Xaml::Interop::TypeName(LeaderboardPage::typeid), m_navParam);
}

void GameInfoPage::FetchGameProgress(Platform::String^ gameId)
{
    Platform::String^ username;
    Platform::String^ apiKey;

    if (!CredentialManager::GetInstance()->LoadCredentials(&username, &apiKey))
    {
        ConsoleNameTextBlock->Text = "Error: Missing credentials";
        return;
    }

    // https://retroachievements.org/API/API_GetGameInfoAndUserProgress.php
    std::wstring baseUrl = L"https://retroachievements.org/API/API_GetGameInfoAndUserProgress.php?";
    std::wstring fullUrl = baseUrl;
    fullUrl += L"z=" + std::wstring(username->Data());
    fullUrl += L"&y=" + std::wstring(apiKey->Data());
    fullUrl += L"&u=" + std::wstring(username->Data());
    fullUrl += L"&g=" + std::wstring(gameId->Data());

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

                    std::wstring consoleName = extractJsonString(responseBody, L"ConsoleName");
                    std::wstring imageIcon = extractJsonString(responseBody, L"ImageIcon");

                    // Replace escaped forward slashes "\/" with "/"
                    size_t pos = 0;
                    while ((pos = consoleName.find(L"\\/", pos)) != std::wstring::npos) {
                        consoleName.replace(pos, 2, L"/");
                        pos += 1;
                    }

                    // Parse achievement badges
                    std::vector<AchievementBadge^> badges;
                    size_t badgeSearchPos = 0;
                    
                    int calculatedNumAwarded = 0;
                    int calculatedNumPossible = 0;
                    
                    while (true)
                    {
                        size_t badgeKeyPos = responseBody.find(L"\"BadgeName\"", badgeSearchPos);
                        if (badgeKeyPos == std::wstring::npos)
                            break;

                        size_t colonPos = responseBody.find(L":", badgeKeyPos);
                        size_t valStart = responseBody.find_first_not_of(L" \t", colonPos + 1);
                        
                        if (valStart != std::wstring::npos)
                        {
                            std::wstring badgeName;
                            if (responseBody[valStart] == L'"')
                            {
                                size_t endQuote = responseBody.find(L"\"", valStart + 1);
                                if (endQuote != std::wstring::npos)
                                    badgeName = responseBody.substr(valStart + 1, endQuote - valStart - 1);
                                badgeSearchPos = endQuote + 1;
                            }
                            else
                            {
                                size_t valEnd = valStart;
                                while (valEnd < responseBody.length() && iswdigit(responseBody[valEnd]))
                                    valEnd++;
                                if (valEnd > valStart)
                                    badgeName = responseBody.substr(valStart, valEnd - valStart);
                                badgeSearchPos = valEnd + 1;
                            }

                            if (!badgeName.empty())
                            {
                                AchievementBadge^ badge = ref new AchievementBadge();
                                
                                bool isEarned = false;
                                std::wstring displayDate = L"";
                                std::wstring title = L"";
                                std::wstring desc = L"";
                                std::wstring points = L"";

                                size_t objStart = responseBody.rfind(L"{", badgeKeyPos);
                                size_t objEnd = responseBody.find(L"}", badgeKeyPos);
                                if (objStart != std::wstring::npos && objEnd != std::wstring::npos && objStart < badgeKeyPos)
                                {
                                    std::wstring achievementObj = responseBody.substr(objStart, objEnd - objStart);
                                    
                                    title = extractJsonString(achievementObj, L"Title");
                                    desc = extractJsonString(achievementObj, L"Description");
                                    points = extractJsonNumber(achievementObj, L"Points");
                                    if (points.empty()) points = extractJsonString(achievementObj, L"Points");
                                    
                                    std::wstring dateHardcore = extractJsonString(achievementObj, L"DateEarnedHardcore");
                                    std::wstring dateEarned = extractJsonString(achievementObj, L"DateEarned");

                                    bool useHardcore = false;
                                    auto localSettings = Windows::Storage::ApplicationData::Current->LocalSettings;
                                    if (localSettings->Values->HasKey("DisplayHardcoreStats"))
                                    {
                                        useHardcore = safe_cast<bool>(localSettings->Values->Lookup("DisplayHardcoreStats"));
                                    }

                                    std::wstring targetDate = useHardcore ? dateHardcore : dateEarned;

                                    if (!targetDate.empty() && targetDate != L"null")
                                    {
                                        displayDate = L"Date Earned: " + targetDate;
                                        isEarned = true;
                                    }

                                    if (!isEarned)
                                    {
                                        displayDate = L"Achievement locked.";
                                    }
                                }

                                std::wstring suffix = isEarned ? L"" : L"_lock";
                                badge->ImageUrl = ref new Platform::String((L"https://retroachievements.org/Badge/" + badgeName + suffix + L".png").c_str());
                                badge->Title = ref new Platform::String(title.c_str());
                                badge->Description = ref new Platform::String(desc.c_str());
                                badge->Points = ref new Platform::String((points + L" Points").c_str());
                                badge->DateEarnedText = ref new Platform::String(displayDate.c_str());

                                badges.push_back(badge);
                                
                                calculatedNumPossible++;
                                if (isEarned) {
                                    calculatedNumAwarded++;
                                }
                            }
                        }
                        else
                        {
                            break;
                        }
                    }

                    auto dispatcher = this->Dispatcher;
                    dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this, consoleName, imageIcon, calculatedNumAwarded, calculatedNumPossible, badges]() {
                        if (!consoleName.empty())
                        {
                            ConsoleNameTextBlock->Text = ref new Platform::String(consoleName.c_str());
                        }
                        else
                        {
                            ConsoleNameTextBlock->Text = "Console: Unknown";
                        }

                        AchievementProgressBar->Maximum = calculatedNumPossible > 0 ? calculatedNumPossible : 1;
                        AchievementProgressBar->Value = calculatedNumAwarded;
                        AchievementProgressTextBlock->Text = ref new Platform::String((std::to_wstring(calculatedNumAwarded) + L"/" + std::to_wstring(calculatedNumPossible) + L" earned").c_str());

                        if (!imageIcon.empty())
                        {
                            auto uri = ref new Windows::Foundation::Uri(
                                ref new Platform::String((L"https://retroachievements.org" + imageIcon).c_str()));
                            GameIconImage->Source = ref new Windows::UI::Xaml::Media::Imaging::BitmapImage(uri);
                        }

                        auto items = AchievementsGridView->Items;
                        items->Clear();
                        for (auto badge : badges)
                        {
                            items->Append(badge);
                        }
                    }));
                });
            }
            else
            {
                auto dispatcher = this->Dispatcher;
                dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this]() {
                    ConsoleNameTextBlock->Text = "Error: Failed to fetch data";
                }));
            }
        }
        catch (...)
        {
            auto dispatcher = this->Dispatcher;
            dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this]() {
                ConsoleNameTextBlock->Text = "Error: Exception occurred";
            }));
        }
    });
}

void GameInfoPage::Page_KeyDown(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
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

void GameInfoPage::OnContainerContentChanging(ListViewBase^ sender, ContainerContentChangingEventArgs^ args)
{
    if (!args->InRecycleQueue)
    {
        auto container = args->ItemContainer;
        if (container != nullptr)
        {
            container->GotFocus += ref new RoutedEventHandler(this, &GameInfoPage::Item_GotFocus);
            container->LostFocus += ref new RoutedEventHandler(this, &GameInfoPage::Item_LostFocus);
        }
    }
}

void GameInfoPage::Item_GotFocus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
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
                if (tt != nullptr) tt->IsOpen = true;
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

void GameInfoPage::Item_LostFocus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
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
                if (tt != nullptr) tt->IsOpen = false;
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
