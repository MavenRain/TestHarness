//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"

using namespace TestHarness;

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

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

MainPage::MainPage()
{
	InitializeComponent();
}

/// <summary>
/// Invoked when this page is about to be displayed in a Frame.
/// </summary>
/// <param name="e">Event data that describes how this page was reached.  The Parameter
/// property is typically used to configure the page.</param>
void MainPage::OnNavigatedTo(NavigationEventArgs^ e)
{
	(void) e;	// Unused parameter

	// TODO: Prepare page for display here.

	// TODO: If your application contains multiple pages, ensure that you are
	// handling the hardware Back button by registering for the
	// Windows::Phone::UI::Input::HardwareButtons.BackPressed event.
	// If you are using the NavigationHelper provided by some templates,
	// this event is handled for you.
}


void TestHarness::MainPage::testButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	//Convert number of trials entered as text to an unsigned integer
	wstring _numberOfTrials(numberOfTrials->Text->Data());
	wstringstream stringToIntegerConverter;
	stringToIntegerConverter << _numberOfTrials;
	unsigned int __numberOfTrials = 1;
	stringToIntegerConverter >> __numberOfTrials;

	//Bind a vector of round trip pings to a listview in UI
	auto _listOfRoundTripTimes = ref new Vector<RoundTripDataPoint^>();
	listOfRoundTripTimes->ItemsSource = _listOfRoundTripTimes;

	//set test domains and test array of sockets
	auto serverHostNames = ref new Array<String^>{ "google.com", "bing.com", "facebook.com", "yahoo.com" };
	auto streamSocket = ref new Array<StreamSocket^>(__numberOfTrials * serverHostNames->Length);

	//create trial tasks
	vector<task<void>> connectTask;
	for (unsigned int trialIndex = 0; trialIndex < __numberOfTrials * serverHostNames->Length; trialIndex++)
	{
		streamSocket[trialIndex] = ref new StreamSocket();
		connectTask.push_back(create_task(streamSocket[trialIndex]->ConnectAsync(ref new HostName(serverHostNames[(trialIndex + 1) % serverHostNames->Length]), ref new String(L"80"))));
	}

	//execute ping tasks, then calculate the mean and report results
	when_all(begin(connectTask), end(connectTask))
		.then([streamSocket, _listOfRoundTripTimes, __numberOfTrials, serverHostNames, this]() mutable
	{
		float32 speedTotal = 0.0;
		for (unsigned int trialIndex = 0; trialIndex < __numberOfTrials * serverHostNames->Length; trialIndex++)
		{
			speedTotal += streamSocket[trialIndex]->Information->RoundTripTimeStatistics.Min / 1000000.0F;
			_listOfRoundTripTimes->Append(ref new RoundTripDataPoint(streamSocket[trialIndex]->Information->RoundTripTimeStatistics.Min / 1000000.0F));
		}
		meanRoundTripTime->Text = (speedTotal / (float32)__numberOfTrials).ToString();
	});
}
