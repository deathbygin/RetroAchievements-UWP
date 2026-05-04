// Profile.xaml.h
// Declaration of the Profile class.
//

#pragma once

#include "Profile.g.h"
#include "CredentialManager.h"

namespace RetroAchievements_UWP
{
    [Windows::UI::Xaml::Data::Bindable]
    public ref class GameItem sealed
    {
    public:
        property Platform::String^ GameID;
        property Platform::String^ ImageUrl;
        property Platform::String^ Title;
        property Platform::String^ ConsoleName;
        property Platform::String^ AchievementProgress;
        property double NumAchieved;
        property double NumPossibleAchievements;
    };

    [Windows::UI::Xaml::Data::Bindable]
    public ref class RecentAchievementItem sealed
    {
    public:
        property Platform::String^ ImageUrl;
        property Platform::String^ Title;
        property Platform::String^ Description;
        property Platform::String^ GameTitle;
        property Platform::String^ GameID;
        property Platform::String^ Points;
        property Platform::String^ DateEarnedText;
    };

    [Windows::UI::Xaml::Data::Bindable]
    public ref class UserProfileHeaderData sealed
    {
    public:
        property Windows::UI::Xaml::Media::ImageSource^ ProfileImage;
        property Platform::String^ Username;
        property Platform::String^ MemberSince;
        property Platform::String^ Motto;
        property Platform::String^ TotalPoints;
    };

    [Windows::UI::Xaml::Data::Bindable]
    public ref class GameInfoNavParam sealed
    {
    public:
        property GameItem^ Game;
        property UserProfileHeaderData^ Header;
    };

    /// <summary>
    /// A simple profile page.
    /// </summary>
    public ref class Profile sealed
    {
    public:
        Profile();

        property Platform::String^ ApiKey;
        property Platform::String^ Username;
        property Platform::String^ UserPicUrl;

    protected:
        virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;

    private:
        UserProfileHeaderData^ GetProfileHeaderData();
        void FetchUserProfile();
        void FetchRecentlyPlayedGames();
        void FetchRecentAchievements();
        void FetchWantToPlayGames();
        void Page_KeyDown(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e);
        void ClearCredentials_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void NavigateToTab(int tabIndex);
        void ViewAllGames_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void RecentGamesGridView_ItemClick(Platform::Object^ sender, Windows::UI::Xaml::Controls::ItemClickEventArgs^ e);
        void RecentAchievementsGridView_ItemClick(Platform::Object^ sender, Windows::UI::Xaml::Controls::ItemClickEventArgs^ e);
        void WantToPlayGridView_ItemClick(Platform::Object^ sender, Windows::UI::Xaml::Controls::ItemClickEventArgs^ e);
        void HardcoreModeToggle_Toggled(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void BackgroundThemeToggle_Toggled(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void AccentColorGridView_ItemClick(Platform::Object^ sender, Windows::UI::Xaml::Controls::ItemClickEventArgs^ e);
        void AccentColorGridView_ContainerContentChanging(Windows::UI::Xaml::Controls::ListViewBase^ sender, Windows::UI::Xaml::Controls::ContainerContentChangingEventArgs^ args);
        void ChooseBackgroundImage_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void ClearBackgroundImage_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void ApplyThemeSettings();
        void OnContainerContentChanging(Windows::UI::Xaml::Controls::ListViewBase^ sender, Windows::UI::Xaml::Controls::ContainerContentChangingEventArgs^ args);
        void Item_GotFocus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void Item_LostFocus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    };
}

