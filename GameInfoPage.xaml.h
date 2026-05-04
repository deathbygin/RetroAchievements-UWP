#pragma once

#include "GameInfoPage.g.h"

namespace RetroAchievements_UWP
{
    [Windows::UI::Xaml::Data::Bindable]
    public ref class AchievementBadge sealed
    {
    public:
        property Platform::String^ ImageUrl;
        property Platform::String^ Title;
        property Platform::String^ Description;
        property Platform::String^ Points;
        property Platform::String^ DateEarnedText;
    };

    public ref class GameInfoPage sealed
    {
    public:
        GameInfoPage();

    protected:
        virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;

    private:
        Platform::Object^ m_navParam;
        void BackButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void LeaderboardButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void FetchGameProgress(Platform::String^ gameId);
        void Page_KeyDown(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e);
        void OnContainerContentChanging(Windows::UI::Xaml::Controls::ListViewBase^ sender, Windows::UI::Xaml::Controls::ContainerContentChangingEventArgs^ args);
        void Item_GotFocus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void Item_LostFocus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    };
}
