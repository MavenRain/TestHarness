//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"

using namespace TestHarness;

using namespace Platform;
using namespace Platform::Collections;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Networking;
using namespace Windows::Networking::Sockets;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

using namespace pplpp;
using namespace Concurrency;
using namespace std;



// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

MainPage::MainPage()
{
	InitializeComponent();
}

void TestHarness::MainPage::testButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	wstring _numberOfTrials(numberOfTrials->Text->Data());
	wstringstream stringToIntegerConverter;
	stringToIntegerConverter << _numberOfTrials;
	unsigned int __numberOfTrials = 1;
	stringToIntegerConverter >> __numberOfTrials;

	
	auto _listOfRoundTripTimes = ref new Vector<RoundTripDataPoint^>();
	listOfRoundTripTimes->ItemsSource = _listOfRoundTripTimes;
	create_task([_listOfRoundTripTimes, __numberOfTrials, this]
	{ 
		auto roundTripTotal = 0.0;
		for (unsigned int trialIndex = 0; trialIndex < __numberOfTrials; trialIndex++)
		{
			auto serverHost = ref new Array<HostName^>{ ref new HostName(ref new String(L"google.com")), ref new HostName(ref new String(L"bing.com")), ref new HostName(ref new String(L"facebook.com")), ref new HostName(ref new String(L"yahoo.com")) };
			auto streamSocket = ref new Array<StreamSocket^>{ ref new StreamSocket(), ref new StreamSocket(), ref new StreamSocket(), ref new StreamSocket() };
			float64 currentSpeed = 0.0;
			auto socketConnectTasks = new vector<task<void>>();
			for (unsigned int connectTaskIndex = 0; connectTaskIndex < serverHost->Length; connectTaskIndex++)
			{
				create_task(streamSocket[connectTaskIndex]->ConnectAsync(serverHost[connectTaskIndex], ref new String(L"80"), SocketProtectionLevel::PlainSocket));
			}
			for (unsigned int connectTaskIndex = 0; connectTaskIndex < serverHost->Length; connectTaskIndex++)
			{
				create_task([connectTaskIndex, &currentSpeed, streamSocket, serverHost]
				{ currentSpeed += streamSocket[connectTaskIndex]->Information->RoundTripTimeStatistics.Min / 1000000.0;
				});
			}

			float32 trialSpeed = currentSpeed / (float32)serverHost->Length;
			_listOfRoundTripTimes->Append(ref new RoundTripDataPoint(trialSpeed));
			roundTripTotal += currentSpeed / serverHost->Length;
		}
		meanRoundTripTime->Text = (roundTripTotal / __numberOfTrials).ToString();
	});
}
