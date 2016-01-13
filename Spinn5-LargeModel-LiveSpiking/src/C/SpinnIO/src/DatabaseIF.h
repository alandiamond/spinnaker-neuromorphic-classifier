//####################################################################//
//SpinnIO - An Open-Source library for spike io to/from a spinnaker board																													
//
//																																																				//
//Copyright (c) <2015>, BABEL project, Plymouth University in collaboration with Manchester University																																								

#include "EIEIOMessage.h"
#include "SocketIF.h"
#include <sqlite3.h>
#include <map>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>

#ifndef SPINNIO_DATABASEIF_H_
#define SPINNIO_DATABASEIF_H_

using namespace std;

namespace spinnio
{

class DatabaseIF {
	SocketIF *sock;
	int sockfd_input;
	struct sockaddr_in response_address;
	sqlite3 *db;
public:
	virtual ~DatabaseIF();
	DatabaseIF(int, char*);
	void finish();
	std::map<int, int> * getKeyIdMap(string receiver_pop);
	std::map<int, int> * getIdKeyMap(string sender_pop);
	bool isReady();
	void waitForDatabaseReadyNotification();
	void sendReadyNotification();

private:
	bool ready;

	void openDatabase(char*);
	void readDatabase(char* pop, bool keyToID, std::map<int, int> * keyMap);
	void closeDatabase();

};
}
#endif /* SPINNIO_DATABASEIF_H_ */
