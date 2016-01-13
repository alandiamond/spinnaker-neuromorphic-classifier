//####################################################################//
//SpinnIO - An Open-Source library for spike io to/from a spinnaker board																													
//
//																																																				//
//Copyright (c) <2015>, BABEL project, Plymouth University in collaboration with Manchester University																																								


#include <pthread.h>

#ifndef C_VIS_DATABASE_THREADABLE_H_
#define C_VIS_DATABASE_THREADABLE_H_

class Threadable {
public:
	Threadable();
	virtual ~Threadable();
	bool start(){
		return (pthread_create(&_thread, NULL, InternalThreadEntryFunc, this) == 0);
    }
	void exit_thread(){
		(void) pthread_join(_thread, NULL);
	}
        void stop(){
                (void) pthread_cancel(_thread);
        }
protected:
   /** Implement this method in your subclass with the code you want your thread to run. */
   virtual void InternalThreadEntry() = 0;
private:
   static void * InternalThreadEntryFunc(void * This) {((Threadable *)This)->InternalThreadEntry(); return NULL;}
   pthread_t _thread;
};

#endif /* C_VIS_DATABASE_THREADABLE_H_ */
