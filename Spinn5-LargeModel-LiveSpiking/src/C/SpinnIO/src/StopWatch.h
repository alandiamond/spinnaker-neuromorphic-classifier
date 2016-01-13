//--------------------------------------------------------------------------
/*! \file hr_time.h

\brief This header file contains the definition of the StopWatch class that implements a simple timing tool using the system clock.
 */
//--------------------------------------------------------------------------

#ifndef SPINNIO_STOPWATCH_H
#define SPINNIO_STOPWATCH_H


#include <sys/time.h>

namespace spinnio {
	typedef struct {
		timeval start;
		timeval stop;
	} stopWatch;

	class StopWatch {

	private:
		stopWatch timer;
	public:
		StopWatch() {};
		void startTimer( );
		void stopTimer( );
		double getElapsedTime();
		int getSubSecondElapsedTimeInMicroSeconds();
	};
}
#endif
