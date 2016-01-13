//--------------------------------------------------------------------------
/*! \file hr_time.cpp

\brief This  file contains the implementation of the StopWatch class that provides a simple timing tool based on the system clock.
*/
//--------------------------------------------------------------------------

#ifndef SPINNIO_STOPWATCH_CPP
#define SPINNIO_STOPWATCH_CPP

#include <cstdio> // NULL
#include "StopWatch.h"

namespace spinnio {
//--------------------------------------------------------------------------
/*! \brief This method starts the timer
 */
//--------------------------------------------------------------------------

void StopWatch::startTimer( ) {
	gettimeofday(&(timer.start),NULL);
}

//--------------------------------------------------------------------------
/*! \brief This method stops the timer
 */
//--------------------------------------------------------------------------

void StopWatch::stopTimer( ) {
	gettimeofday(&(timer.stop),NULL);
}

//just return the microsecond part of an elapsed time (use for timings that will be under one second)
int StopWatch::getSubSecondElapsedTimeInMicroSeconds() {
	timeval res;
	timersub(&(timer.stop),&(timer.start),&res);
	return res.tv_usec;
}
//--------------------------------------------------------------------------
/*! \brief This method returns the time elapsed between start and stop of the timer in seconds.
 */
//--------------------------------------------------------------------------

double StopWatch::getElapsedTime() {
	timeval res;
	timersub(&(timer.stop),&(timer.start),&res);
	return res.tv_sec + res.tv_usec/1000000.0; // 10^6 uSec per second
}
}
#endif
