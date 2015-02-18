//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#pragma once
#include "pch.h"
#include "MainPage.xaml.h"

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

MainPage::MainPage()
{
	InitializeComponent();
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

	//Set test domains and test array of sockets
	auto serverHostNames = new array<String^, 4> { {"google.com", "bing.com", "facebook.com", "yahoo.com" } };
	auto streamSocket = ref new Array<StreamSocket^>(__numberOfTrials * serverHostNames->size());

	//Use a decrementable number for the trial count
	auto trialCount = __numberOfTrials * serverHostNames->size();

	//set time duration for task cancels
	TimeSpan delay;
	delay.Duration = 400000;

	//Prepare cancellation token information for tasks
	map<unsigned int,cancellation_token_source*> cts;
	vector<ThreadPoolTimer^> delayTimer;

	//create trial tasks
	vector<task<void>> connectTask;
	for (unsigned int trialIndex = 0; trialIndex < trialCount; trialIndex++)
	{
		streamSocket[trialIndex] = ref new StreamSocket();
		cts.insert(pair<unsigned int, cancellation_token_source*>(trialIndex, new cancellation_token_source()));
		connectTask.push_back(create_task(streamSocket[trialIndex]->ConnectAsync(ref new HostName(serverHostNames->at((trialIndex + 1) % serverHostNames->size())), ref new String(L"80"), SocketProtectionLevel::PlainSocket), cts[trialIndex]->get_token())
			.then([cts, trialCount, trialIndex, delay, delayTimer]() mutable
		{
			delayTimer.push_back(ThreadPoolTimer::CreateTimer(ref new TimerElapsedHandler([cts, trialIndex](ThreadPoolTimer^ threadPoolTimer)
			{
				cts.at(trialIndex)->cancel();
			}), delay));
		}, task_continuation_context::use_current()).then([trialCount](task<void> previousTask) mutable
		{
			try { previousTask.get(); }
			catch (COMException^){ trialCount--; }
			catch (task_canceled&){ trialCount--; }
		}));
	}

	//execute ping tasks, then calculate the mean and report results
	when_any(begin(connectTask), end(connectTask))
		.then([streamSocket, _listOfRoundTripTimes, trialCount, this](size_t result) mutable 
	{
		float32 speedTotal = 0.0;
		for (unsigned int trialIndex = 0; trialIndex < trialCount; trialIndex++)
		{
			speedTotal += streamSocket[trialIndex]->Information->RoundTripTimeStatistics.Min / 1000000.0F;
			if (streamSocket[trialIndex]->Information->RoundTripTimeStatistics.Min == 0.000000F) trialCount--;
			_listOfRoundTripTimes->Append(ref new RoundTripDataPoint(streamSocket[trialIndex]->Information->RoundTripTimeStatistics.Min / 1000000.0F));
		}
		meanRoundTripTime->Text = (speedTotal / (float32)(trialCount)).ToString();
	});
}
