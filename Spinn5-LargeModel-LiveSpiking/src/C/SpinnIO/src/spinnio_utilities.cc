#ifndef spinnio_utilities_cc
#define spinnio_utilities_cc
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <fstream>
#include <math.h>
#include <cmath>
#include <sstream>
#include <algorithm> //for std:find
//#include <random>

//for stat command (file info interrgation
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;

namespace spinnio {

typedef unsigned int UINT;
enum data_type {data_type_int, data_type_uint, data_type_float,data_type_double};
const string COMMA = ",";
const string SPACE = " ";
const string TAB = "\t";
const string NL = "\n";
const string SLASH = "/";
const string DOT = ".";
const string EMPTY_STRING = "";


//export a map as a delimted string
template <class DATATYPE>
string toString(map<DATATYPE,DATATYPE> &m, string delim) {
	stringstream result;
	for(typename map<DATATYPE,DATATYPE>::iterator it = m.begin(); it != m.end(); ++it) {
		DATATYPE key = it->first;
		result << key << ":" << m[key] << delim;
	}
	return result.str();
}

}
#endif
