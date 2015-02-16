#pragma once
#include <pch.h>


namespace TestHarness
{
	[Windows::UI::Xaml::Data::Bindable]
	public ref class RoundTripDataPoint sealed
	{
		float32 roundTripTrialPoint;
	public:
		RoundTripDataPoint(float32 newTrialPoint) : roundTripTrialPoint(newTrialPoint) {}
		property float32 RoundTripTrialPoint { float32 get(); void set(float32 newTrialPoint); }
	};
}
