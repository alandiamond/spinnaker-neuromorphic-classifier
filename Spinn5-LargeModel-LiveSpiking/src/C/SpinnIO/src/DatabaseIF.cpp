//####################################################################//
//SpinnIO - An Open-Source library for spike io to/from a spinnaker board																													
//
//																																																				//
//Copyright (c) <2015>, BABEL project, Plymouth University in collaboration with Manchester University																																								


#include <stdio.h>
#include <iostream>
#include <string.h>
#include <map>
#include <sqlite3.h>
#include <algorithm>
#include <deque>
#include <cstdlib>
#include <unistd.h>
#include "DatabaseIF.h"

using namespace std;

struct database_data{
	int no_columns;
	char ** fields;
	char ** attributes;
};

namespace spinnio
{

DatabaseIF::~DatabaseIF() {
	delete this->sock;
}

DatabaseIF::DatabaseIF(int listenPort, char* dbPath) {
	ready = false;
	cout << "Creating database handshaker with port " << listenPort << endl;
	this->sock = new SocketIF(listenPort);
	this->sockfd_input = sock->getSocket();
	waitForDatabaseReadyNotification();
	cout << "Attempting to open database " << dbPath << endl;
	openDatabase(dbPath);
	cout << "Database ready to query" << endl;

}

//use this for sending (injection) populations
std::map<int, int> * DatabaseIF::getIdKeyMap(string sender_pop) {
	if (ready) {
		std::map<int, int> * idKeyMap = new std::map<int, int>();
		readDatabase((char*) sender_pop.c_str(), false, idKeyMap);
		return idKeyMap;
	} else {
		cerr << "DatabaseIF::getIdKeyMap() failed; Database connection was not made succesfully" << endl;
		return NULL;
	}
}

//use this for receiving (broadcasting) populations
std::map<int, int> * DatabaseIF::getKeyIdMap(string sender_pop) {
	if (ready) {
		std::map<int, int> * keyIDMap = new std::map<int, int>();
		readDatabase((char*) sender_pop.c_str(), true, keyIDMap);
		return keyIDMap;
	} else {
		cerr << "DatabaseIF::getkeyIdMap() failed; Database connection was not made succesfully" << endl;
		return NULL;
	}
}

void DatabaseIF::finish() {
	if (!ready) return;
	//sendReadyNotification();
	//cout << "Sent ready notification to Spinnaker board." << endl;
	closeDatabase();
	// send message to toolchain to confirm read
	// close the connection as we've finished with the database
	sock->closeSocket();
	cout << "Closed database connection." << endl;
	ready = false;

}

void DatabaseIF::openDatabase(char* dbPath){

	int rc;
	rc = sqlite3_open(dbPath, &this->db);
	if(rc){
		//fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(this->db));
		cerr << "Can't open database: " << sqlite3_errmsg(this->db) << endl;
		exit(-1);
	}
	else{
		cout << "Opened database successfully" << endl;
	}
	ready = true;

}



void DatabaseIF::readDatabase(char* pop, bool keyToID, std::map<int, int> * _keyMap) {

	cout << "Reading key id mapping from database for population '" << pop << "'" << endl;
	std::map<int, int> & keyMap = (*_keyMap);
	/*
	//This version works for Little Rascal database schema
	char *sql = sqlite3_mprintf(
			"SELECT n.neuron_id as n_id, n.key as key"
			" FROM key_to_neuron_mapping as n"
			" JOIN Partitionable_vertices as p ON n.vertex_id = p.vertex_id"
			" WHERE p.vertex_label=\"%q\"", pop);
	*/

	//updated for revised schema in Spynnaker release "2015.005.Arbitrary"
	char *sql = sqlite3_mprintf(
			"SELECT n.atom_id as n_id, n.event_id as key"
			" FROM event_to_atom_mapping as n"
			" JOIN Partitionable_vertices as p ON n.vertex_id = p.vertex_id"
			" WHERE p.vertex_label=\"%q\"", pop);

	sqlite3_stmt *compiled_statment;
	if (sqlite3_prepare_v2(this->db, sql, -1,
			&compiled_statment, NULL) == SQLITE_OK){
		while (sqlite3_step(compiled_statment) == SQLITE_ROW) {
			int neuron_id = sqlite3_column_int(compiled_statment, 0);
			int key = sqlite3_column_int(compiled_statment, 1);
			if (keyToID == true){

				keyMap[key] = neuron_id;
			} else {
				keyMap[neuron_id] = key;
			}
		}
		sqlite3_free(sql);
	} else {
		cerr << "Error reading database: " << sqlite3_errcode(this->db) << " " << sqlite3_errmsg(this->db) << endl;
		exit(-1);
	}
}


void DatabaseIF::closeDatabase(){
	sqlite3_close(this->db);
}



void DatabaseIF::waitForDatabaseReadyNotification(){
	socklen_t addr_len_input = sizeof(struct sockaddr_in);
	char sdp_header_len = 26;
	unsigned char buffer_input[1515];
	bool received = false;

	cout << "Waiting for database-ready notification.." << endl;
	while (!received){
		int numbytes_input = recvfrom(
				this->sockfd_input, (char *) buffer_input, sizeof(buffer_input), 0,
				(sockaddr*) &this->response_address, (socklen_t*) &addr_len_input);
		if (numbytes_input == -1) {
			cerr << "DatabaseIF - Error reported reading from database socket" << endl;

			// will only get here if there's an error getting the input frame
			// off the Ethernet
			exit(-1);
		} else {
			cout << "Message received on database port." << endl;
			if (numbytes_input < 2) {
				cerr << "DatabaseIF::waitForDatabaseReadyNotification - WARNING: packet received too short" << endl;
				continue;
			}
		}
		received = true;
		cout << "Database notification received." << endl;
	}
}

void DatabaseIF::sendReadyNotification(){

	char message[2];
	message[1] = (1 << 6);
	message[0] = 1;
	int length = 2;

	int response = sendto(this->sockfd_input, message, length, 0,
			(struct sockaddr *) &this->response_address,
			sizeof(this->response_address));

	//cout << "Response from sending ready notification " << response << endl;
	if (response <0) {
		cerr << "Failed to send ready notification to spinnaker";
		exit(-1);
	} else {
		cout << "Sent ready notification to spinnaker" << endl;
	}
}

bool DatabaseIF::isReady(){
	return this->ready;
}

}// namespace spinnio
