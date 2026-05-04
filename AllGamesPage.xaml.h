#pragma once

#include "AllGamesPage.g.h"

namespace RetroAchievements_UWP
{
    public ref class AllGamesPage sealed
    {
    public:
        AllGamesPage();

    protected:
        virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;

    private:
        void FetchAllGames();
        void BackButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void AllGamesGridView_ItemClick(Platform::Object^ sender, Windows::UI::Xaml::Controls::ItemClickEventArgs^ e);
        void Page_KeyDown(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e);
        void OnContainerContentChanging(Windows::UI::Xaml::Controls::ListViewBase^ sender, Windows::UI::Xaml::Controls::ContainerContentChangingEventArgs^ args);
        void Item_GotFocus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void Item_LostFocus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    };
}
