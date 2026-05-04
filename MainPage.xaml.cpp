//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"
#include "Profile.xaml.h"
#include <ppltasks.h>
#include <string>
#include "ThemeSettings.h"

using namespace RetroAchievements_UWP;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace concurrency;
using namespace Windows::Web::Http;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml::Interop;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

MainPage::MainPage()
{
	InitializeComponent();
}

void MainPage::OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e)
{
	// Clear fields to ensure no stale data is shown
	UsernameTextBox->Text = "";
	ApiKeyPasswordBox->Password = "";
	ResultTextBlock->Text = "";

	// Try to load saved credentials
	Platform::String^ savedUsername = nullptr;
	Platform::String^ savedApiKey = nullptr;

	if (CredentialManager::GetInstance()->LoadCredentials(&savedUsername, &savedApiKey))
	{
		// Automatically validate and navigate if credentials are saved
		ValidateAndNavigate(savedUsername, savedApiKey);
	}
	
	ThemeSettings::ApplyAccentToResources(this->Resources);
	this->RequestedTheme = ThemeSettings::GetElementTheme();
	ThemeSettings::ApplyThemeToTitle(AppTitleTextBlock);
	ThemeSettings::ApplyAccentToButton(ConfirmButton);
	ThemeSettings::ApplyBackground(RootGrid);
}

void MainPage::ValidateAndNavigate(Platform::String^ username, Platform::String^ apiKey)
{
	// Update UI with checking status
	ResultTextBlock->Text = "Checking...";
	UsernameTextBox->Text = username;
	ApiKeyPasswordBox->Password = apiKey;

	// Store credentials as properties
	this->ApiKey = apiKey;
	this->Username = username;

	// Use the documented endpoint and 'y' query parameter for the API key
	std::wstring baseUrl = L"https://retroachievements.org/API/API_GetAchievementOfTheWeek.php?&y=";
	std::wstring fullUrl = baseUrl + std::wstring(apiKey->Data());
	auto uri = ref new Windows::Foundation::Uri(ref new Platform::String(fullUrl.c_str()));

	HttpClient^ client = ref new HttpClient();

	create_task(client->GetAsync(uri)).then([this, client, apiKey, username](task<HttpResponseMessage^> prevTask)
	{
		bool ok = false;
		try
		{
			HttpResponseMessage^ response = prevTask.get();
			if (response->IsSuccessStatusCode)
			{
				// Read the response body to look for any error message indicating a bad API key
				create_task(response->Content->ReadAsStringAsync()).then([this, apiKey, username](Platform::String^ body)
				{
					std::wstring s(body->Data());
					// Lowercase for simple case-insensitive checks
					for (auto &c : s) c = towlower(c);
					bool bad = (s.find(L"bad api key") != std::wstring::npos) || (s.find(L"invalid api key") != std::wstring::npos) || (s.find(L"bad key") != std::wstring::npos);

					auto dispatcher = this->Dispatcher;
					dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this, bad, apiKey, username]() {
						if (bad)
						{
							ResultTextBlock->Text = "Fail";
							// Clear invalid credentials from storage
							CredentialManager::GetInstance()->ClearCredentials();
						}
						else
						{
							ResultTextBlock->Text = "Success";
							// Save credentials on successful validation
							CredentialManager::GetInstance()->SaveCredentials(username, apiKey);
							// Navigate to Profile page on success
							this->Frame->Navigate(TypeName(Profile::typeid), this);
						}
					}));
				});
				return;
			}
			else
			{
				ok = false;
			}
		}
		catch (...)
		{
			ok = false;
		}

		auto dispatcher = this->Dispatcher;
		dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this, ok]() {
			if (ok)
			{
				ResultTextBlock->Text = "Success";
				this->Frame->Navigate(TypeName(Profile::typeid), this);
			}
			else
			{
				ResultTextBlock->Text = "Fail";
			}
		}));
	});
}

void MainPage::ConfirmButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	Platform::String^ key = ApiKeyPasswordBox->Password;
	Platform::String^ username = UsernameTextBox->Text;

	// Validate and navigate with the manually entered credentials
	ValidateAndNavigate(username, key);
}

void MainPage::ShowApiKeyCheckBox_Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	ApiKeyPasswordBox->PasswordRevealMode = PasswordRevealMode::Visible;
}

void MainPage::ShowApiKeyCheckBox_Unchecked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	ApiKeyPasswordBox->PasswordRevealMode = PasswordRevealMode::Hidden;
}

void MainPage::Page_KeyDown(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
{
	// On main page, B button doesn't have a clear "back" destination,
	// so we'll just handle it to prevent unwanted behaviors or just leave it.
	// However, A button on many controls is handled automatically.
}
