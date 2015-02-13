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



// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

MainPage::MainPage()
{
	InitializeComponent();
}

double MainPage::InternetConnectSocketAsync()
{
	auto _canceled = false;
	unsigned int retries = 4;
	long long task_timeout_ms = 400;
	auto currentSpeed = 0.0;
	auto _socketTcpWellKnownHostNames = ref new Array <String^>(4){ "google.com", "bing.com", "facebook.com", "yahoo.com" };
	for (unsigned int trialHostIndex = 0; trialHostIndex < retries; ++trialHostIndex)
	{
		auto _serverHost = ref new HostName(_socketTcpWellKnownHostNames[trialHostIndex]);

		auto _clientSocket = ref new StreamSocket();
		_clientSocket->Control->NoDelay = true;
		_clientSocket->Control->QualityOfService = SocketQualityOfService::LowLatency;
		_clientSocket->Control->KeepAlive = false;
		//tasks must complete in a fixed amount of time, cancel otherwise..
		timed_cancellation_token_source tcs;
		std::chrono::milliseconds timeout(task_timeout_ms);
		try
		{
			create_task([&]
			{
				tcs.cancel(timeout);
				return _clientSocket->ConnectAsync(_serverHost, "80", SocketProtectionLevel::PlainSocket);
			}, tcs.get_token()).then([&]
			{
				currentSpeed += _clientSocket->Information->RoundTripTimeStatistics.Min / 1000000.0;
			}).get();
		}
		catch (Platform::COMException^ e)
		{
			currentSpeed = 0.0;
			retries--;
		}
		catch (task_canceled&)
		{
			currentSpeed = 0.0;
			retries--;
		}
		delete _clientSocket;
	}
	//Compute speed...
	return currentSpeed / retries;
	
}

void TestHarness::MainPage::testButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	auto _listOfRoundTripTimes = ref new Vector<double>();
	listOfRoundTripTimes->ItemsSource = _listOfRoundTripTimes;
	auto roundTripTotal = 0.0;
	for (unsigned int trialIndex = 0; trialIndex < (unsigned int)numberOfTrials->Text->Data(); trialIndex++)
	{
		auto trialRoundTripTime = InternetConnectSocketAsync();
		_listOfRoundTripTimes->Append(trialRoundTripTime);
		roundTripTotal += trialRoundTripTime;
	}
	meanRoundTripTime->Text = roundTripTotal.ToString();
}
