﻿//
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


// Cancels the provided task after the specifed delay, if the task 
// did not complete in timeout milliseconds.
template<typename T>
task<T> cancel_after_timeout(task<T> t, cancellation_token_source cts, unsigned int timeout)
{
	// Create a task that returns true after the specified task completes.
	task<bool> success_task = t.then([](T)
	{
		return true;
	});

	// Creates a task that completes after the specified delay.
	auto complete_after = [](unsigned int timeout)
	{
		// A task completion event that is set when a timer fires.
		task_completion_event<void> tce;

		// Create a non-repeating timer.
		auto fire_once = new timer<int>(timeout, 0, nullptr, false);
		// Create a call object that sets the completion event after the timer fires.
		auto callback = new call<int>([tce](int)
		{
			tce.set();
		});

		// Connect the timer to the callback and start the timer.
		fire_once->link_target(callback);
		fire_once->start();

		// Create a task that completes after the completion event is set.
		task<void> event_set(tce);

		// Create a continuation task that cleans up resources and 
		// and return that continuation task. 
		return event_set.then([callback, fire_once]()
		{
			delete callback;
			delete fire_once;
		});
	};

	// Create a task that returns false after the specified timeout.
	task<bool> failure_task = complete_after(timeout).then([]
	{
		return false;
	});

	// Create a continuation task that cancels the overall task  
	// if the timeout task finishes first. 
	return (failure_task || success_task).then([t, cts](bool success)
	{
		if (!success)
		{
			// Set the cancellation token. The task that is passed as the 
			// t parameter should respond to the cancellation and stop 
			// as soon as it can.
			cts.cancel();
		}

		// Return the original task. 
		return t;
	});
}

void MainPage::testButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
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

	//Set test domains and test array of sockets
	auto serverHostNames = new array<String^, 4> { {"google.com", "bing.com", "facebook.com", "yahoo.com" } };
	auto streamSocket = ref new Array<StreamSocket^>(__numberOfTrials * serverHostNames->size());

	//Use a decrementable number for the trial count
	auto trialCount = __numberOfTrials * serverHostNames->size();

	//set time duration for task cancels
	TimeSpan delay;
	delay.Duration = 400000;

	//Prepare cancellation token information for tasks
	map<unsigned int, cancellation_token_source*> cts;
	vector<ThreadPoolTimer^> delayTimer;

	//create trial tasks
	vector<task<void>> connectTask;
	for (unsigned int trialIndex = 0; trialIndex < trialCount; trialIndex++)
	{
		streamSocket[trialIndex] = ref new StreamSocket();
		cts.insert(pair<unsigned int, cancellation_token_source*>(trialIndex, new cancellation_token_source()));
		connectTask.push_back(cancel_after_timeout(create_task(streamSocket[trialIndex]->ConnectAsync(ref new HostName(serverHostNames->at((trialIndex + 1) % serverHostNames->size())), ref new String(L"80"), SocketProtectionLevel::PlainSocket)), *cts[trialIndex], 400U));
	}

	//execute ping tasks, then calculate the mean and report results
	when_any(begin(connectTask), end(connectTask))
		.then([streamSocket, _listOfRoundTripTimes, trialCount, this, serverHostNames](size_t result) mutable
	{
		float32 speedTotal = 0.0;
		for (unsigned int trialIndex = 0; trialIndex < trialCount; trialIndex++)
		{
			speedTotal += streamSocket[trialIndex]->Information->RoundTripTimeStatistics.Min / 1000000.0F;
			if (streamSocket[trialIndex]->Information->RoundTripTimeStatistics.Min == 0.000000F) trialCount--;
			_listOfRoundTripTimes->Append(ref new RoundTripDataPoint(streamSocket[trialIndex]->Information->RoundTripTimeStatistics.Min / 1000000.0F));
		}
		meanRoundTripTime->Text = (speedTotal / (float32)(trialCount)).ToString();
		delete serverHostNames;
	});
}
