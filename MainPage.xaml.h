//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"
#include "CredentialManager.h"

namespace RetroAchievements_UWP
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	public ref class MainPage sealed
	{
	public:
		MainPage();

		void ConfirmButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

		property Platform::String^ ApiKey;
		property Platform::String^ Username;

	protected:
		virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;

	private:
		void ValidateAndNavigate(Platform::String^ username, Platform::String^ apiKey);
		void ShowApiKeyCheckBox_Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void ShowApiKeyCheckBox_Unchecked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void Page_KeyDown(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e);
	};
}
