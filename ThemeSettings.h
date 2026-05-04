// ThemeSettings.h
// Shared helper for reading and applying user-customizable theme settings.
// Settings are persisted in LocalSettings so they survive app restarts.

#pragma once

#include <string>

namespace RetroAchievements_UWP
{
    class ThemeSettings
    {
    public:
        // Returns the permanent accent color (Purple).
        static Platform::String^ GetAccentColor()
        {
            return "#7B2FF7";
        }

        // Reads the saved background theme from LocalSettings.
        // Returns false (Dark) if not set.
        static bool GetIsLightTheme()
        {
            auto localSettings = Windows::Storage::ApplicationData::Current->LocalSettings;
            if (localSettings->Values->HasKey("IsLightTheme"))
            {
                return safe_cast<bool>(localSettings->Values->Lookup("IsLightTheme"));
            }
            return false;
        }

        // Creates a SolidColorBrush from a hex string like "#107C10"
        static Windows::UI::Xaml::Media::SolidColorBrush^ BrushFromHex(Platform::String^ hex)
        {
            std::wstring h(hex->Data());
            // Remove leading #
            if (!h.empty() && h[0] == L'#') h = h.substr(1);

            unsigned int r = 0, g = 0, b = 0;
            if (h.length() == 6)
            {
                try {
                    r = std::stoul(h.substr(0, 2), nullptr, 16);
                    g = std::stoul(h.substr(2, 2), nullptr, 16);
                    b = std::stoul(h.substr(4, 2), nullptr, 16);
                } catch (...) {
                    // Fallback to Xbox Green if parsing fails
                    r = 0x10; g = 0x7C; b = 0x10;
                }
            }

            Windows::UI::Color color;
            color.A = 255;
            color.R = (unsigned char)r;
            color.G = (unsigned char)g;
            color.B = (unsigned char)b;
            return ref new Windows::UI::Xaml::Media::SolidColorBrush(color);
        }

        // Applies the saved accent color to a Button
        static void ApplyAccentToButton(Windows::UI::Xaml::Controls::Button^ btn)
        {
            if (btn == nullptr) return;
            auto accent = GetAccentColor();
            btn->Background = BrushFromHex(accent);
            // Accent buttons always have white text for contrast
            Windows::UI::Color white;
            white.A = 255; white.R = 255; white.G = 255; white.B = 255;
            btn->Foreground = ref new Windows::UI::Xaml::Media::SolidColorBrush(white);
        }

        // Applies theme-aware styling to a "Back" button
        static void ApplyThemeToBackButton(Windows::UI::Xaml::Controls::Button^ btn)
        {
            if (btn == nullptr) return;
            bool isLight = GetIsLightTheme();
            btn->Background = BrushFromHex(isLight ? "#CCCCCC" : "#333333");
            Windows::UI::Color fg;
            fg.A = 255;
            if (isLight) { fg.R = 0; fg.G = 0; fg.B = 0; }
            else { fg.R = 255; fg.G = 255; fg.B = 255; }
            btn->Foreground = ref new Windows::UI::Xaml::Media::SolidColorBrush(fg);
        }

        // Applies theme-aware foreground to a TextBlock (for hardcoded-white title text)
        static void ApplyThemeToTitle(Windows::UI::Xaml::Controls::TextBlock^ tb)
        {
            if (tb == nullptr) return;
            bool isLight = GetIsLightTheme();
            Windows::UI::Color fg;
            fg.A = 255;
            if (isLight) { fg.R = 0; fg.G = 0; fg.B = 0; }
            else { fg.R = 255; fg.G = 255; fg.B = 255; }
            tb->Foreground = ref new Windows::UI::Xaml::Media::SolidColorBrush(fg);
        }

        // Applies accent color to a ProgressBar
        static void ApplyAccentToProgressBar(Windows::UI::Xaml::Controls::ProgressBar^ pb)
        {
            if (pb == nullptr) return;
            pb->Foreground = BrushFromHex(GetAccentColor());
        }

        // Gets the RequestedTheme value based on settings
        static Windows::UI::Xaml::ElementTheme GetElementTheme()
        {
            return GetIsLightTheme() 
                ? Windows::UI::Xaml::ElementTheme::Light 
                : Windows::UI::Xaml::ElementTheme::Dark;
        }

        // Applies the accent color to a ResourceDictionary to override system brushes
        static void ApplyAccentToResources(Windows::UI::Xaml::ResourceDictionary^ resources)
        {
            if (resources == nullptr) return;

            auto accentColorStr = GetAccentColor();
            auto accentBrush = BrushFromHex(accentColorStr);
            auto accentColor = accentBrush->Color;

            // Update Color resources
            resources->Insert("SystemAccentColor", accentColor);

            // Update Brush resources
            resources->Insert("SystemControlHighlightAccentBrush", accentBrush);
            resources->Insert("SystemControlHighlightAltAccentBrush", accentBrush);
            
            // ToggleSwitch resources
            resources->Insert("ToggleSwitchFillOn", accentBrush);
            resources->Insert("ToggleSwitchFillOnPointerOver", accentBrush);
            resources->Insert("ToggleSwitchFillOnPressed", accentBrush);
            resources->Insert("ToggleSwitchKnobFillOn", ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::White));
            
            // Pivot resources
            resources->Insert("PivotHeaderItemSelectedPipeBrush", accentBrush);
            resources->Insert("PivotHeaderItemSelectedForegroundBrush", accentBrush);
            
            // GridView / ListView selection and hover resources
            resources->Insert("ListViewItemSelectedBackground", accentBrush);
            resources->Insert("ListViewItemSelectedPointerOverBackground", accentBrush);
            resources->Insert("ListViewItemSelectedPressedBackground", accentBrush);
            resources->Insert("SystemControlHighlightListAccentLowBrush", accentBrush);
            resources->Insert("SystemControlHighlightListAccentMediumBrush", accentBrush);
            resources->Insert("SystemControlHighlightListAccentHighBrush", accentBrush);
            
            // For ProgressBar in DataTemplates (if they use ThemeResource)
            resources->Insert("ProgressBarForegroundThemeBrush", accentBrush);
            resources->Insert("SystemControlProgressBarForegroundBrush", accentBrush);
            resources->Insert("ProgressBarTrackForegroundThemeBrush", accentBrush);
        }
    };
}
