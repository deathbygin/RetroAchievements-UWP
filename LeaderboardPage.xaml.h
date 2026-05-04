#pragma once

#include "LeaderboardPage.g.h"

namespace RetroAchievements_UWP
{
    [Windows::UI::Xaml::Data::Bindable]
    public ref class LeaderboardItem sealed
    {
    public:
        property Platform::String^ Title;
        property Platform::String^ Description;
        property Platform::String^ TopEntryUser;
        property Platform::String^ TopEntryScore;
        
        property Platform::String^ LeaderboardID;
        property Windows::UI::Xaml::Visibility UserStatsVisibility;
        property Windows::UI::Xaml::Visibility UserNoStatsVisibility;
        property Platform::String^ UserFormattedScore;
        property Platform::String^ UserRank;
    };

    public ref class LeaderboardPage sealed
    {
    public:
        LeaderboardPage();

    protected:
        virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;

    private:
        void BackButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void FetchGameLeaderboards(Platform::String^ gameId, Platform::String^ gameImageUrl);
        void Page_KeyDown(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e);
        void OnContainerContentChanging(Windows::UI::Xaml::Controls::ListViewBase^ sender, Windows::UI::Xaml::Controls::ContainerContentChangingEventArgs^ args);
        void Item_GotFocus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void Item_LostFocus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    };
}
