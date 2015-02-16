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
