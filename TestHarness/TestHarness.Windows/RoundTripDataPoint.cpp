
#include "pch.h"

#include <RoundTripDataPoint.h>

using namespace TestHarness;

float32 RoundTripDataPoint::RoundTripTrialPoint::get()
{
	return roundTripTrialPoint;
}

void RoundTripDataPoint::RoundTripTrialPoint::set(float32 newTrialPoint)
{
	roundTripTrialPoint = newTrialPoint;
}

