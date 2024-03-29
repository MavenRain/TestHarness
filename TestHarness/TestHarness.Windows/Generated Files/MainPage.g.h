﻿

#pragma once
//------------------------------------------------------------------------------
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//------------------------------------------------------------------------------

namespace Windows {
    namespace UI {
        namespace Xaml {
            namespace Controls {
                ref class Button;
                ref class TextBox;
                ref class ListView;
                ref class TextBlock;
            }
        }
    }
}

namespace TestHarness
{
    partial ref class MainPage : public ::Windows::UI::Xaml::Controls::Page, 
        public ::Windows::UI::Xaml::Markup::IComponentConnector
    {
    public:
        void InitializeComponent();
        virtual void Connect(int connectionId, ::Platform::Object^ target);
    
    private:
        bool _contentLoaded;
    
        private: ::Windows::UI::Xaml::Controls::Button^ testButton;
        private: ::Windows::UI::Xaml::Controls::TextBox^ numberOfTrials;
        private: ::Windows::UI::Xaml::Controls::ListView^ listOfRoundTripTimes;
        private: ::Windows::UI::Xaml::Controls::TextBlock^ meanRoundTripTime;
        private: ::Windows::UI::Xaml::Controls::TextBlock^ maxRoundTripTime;
    };
}

