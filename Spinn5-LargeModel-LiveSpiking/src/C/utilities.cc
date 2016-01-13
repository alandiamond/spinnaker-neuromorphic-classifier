#ifndef utilities_cc
#define utilities_cc
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

typedef unsigned int UINT;
enum data_type {data_type_int, data_type_uint, data_type_float,data_type_double};
const string COMMA = ",";
const string SPACE = " ";
const string TAB = "\t";
const string NL = "\n";
const string SLASH = "/";
const string DOT = ".";
const string EMPTY_STRING = "";


template<class T>
std::string toString(T t)
{
	std::stringstream s;
	//  s << std::showpoint;
	s << t;
	return s.str();
}

template<>
std::string toString(float t)
{
	std::stringstream s;
	//  s << std::showpoint;
	s << t;
	if (floor(t) == t) { // it's a whole number
		s << ".0";
	}
	return s.str();
}

template<>
std::string toString(double t)
{
	std::stringstream s;
	//  s << std::showpoint;
	s << t;
	if (floor(t) == t) { // it's a whole number
		s << ".0";
	}
	return s.str();
}


/*
 * return largest of 2 numbers
 */
template <class DATATYPE>
DATATYPE largest(DATATYPE val1, DATATYPE val2)
{
	return val1 > val2 ? val1 : val2;
}

/*
 * return largest of 3 numbers
 */
template <class DATATYPE>
DATATYPE largest(DATATYPE val1, DATATYPE val2, DATATYPE val3)
{
	DATATYPE x;
	return (x=largest(val1,val2)) > val3 ? x : val3;
}

/*
 * return largest of 4 numbers
 */
template <class DATATYPE>
DATATYPE largest(DATATYPE val1, DATATYPE val2, DATATYPE val3, DATATYPE val4)
{
	DATATYPE x,y;
	return (x=largest(val1,val2)) > (y=largest(val3,val4)) ? x : y;
}

/*--------------------------------------------------------------------------
invoke a shell command in Linux
-------------------------------------------------------------------------- */
bool invokeLinuxShellCommand(const char* cmdLine, string &result) {
	char buf[512];
	FILE *output = popen(cmdLine, "r");
	if (!output) {
		cerr << "Failed to invoke command " << cmdLine;
		return false;
	}
	while(fgets(buf, sizeof(buf), output)!=NULL) {
		result.append(buf);
	}

	return true;
}

//count occurrences of find in src
int countOccurences(const string & src,const string &find)
{
	int len = src.length();
	if (len==0 || len < find.length()) {
		return 0;
	} else {
		int count = 0;
		int pos  = src.find(find);
		while (pos!=string::npos) {
			count ++;
			int startFrom = pos + 1;
			pos  = src.find(find, startFrom);
		}
		return count;
	}
}

//replace every occurrence of find in src with replacement
string replace(string src,const string &find, const string &replacement)
{
	UINT pos  = src.find(find);
	while ((pos!=string::npos)) {
		src = src.replace(pos,find.length(),replacement);
		int startFrom = pos + replacement.length();
		pos  = src.find(find, startFrom); //we need to provide startFrom else if a string is replaced by another containing the same find string causes endless loop (e.g replace "xxx" with "YYYxxx")
	}
	return src;
}

/*-----------------------------------------------------------------
Utilities to get the average and stdDev from a vector of floats
-----------------------------------------------------------------*/
float getAverage(vector<float> &v)
{
	float total = 0.0f;
	for (vector<float>::iterator it = v.begin(); it != v.end(); ++it)
		total += *it;
	return total / v.size();
}

float getStdDev(vector<float> &v, float avg)
{
	float totalDiffSquared = 0.0f;
	for (vector<float>::iterator it = v.begin(); it != v.end(); ++it) {
		float diff = (avg - *it);
		totalDiffSquared += diff*diff;
	}
	float variance  = totalDiffSquared / v.size();
	return sqrtf(variance);
}

/*-----------------------------------------------------------------
Utility to write any text file to console
-----------------------------------------------------------------*/
bool printTextFile(string path)
{
	ifstream file(path.c_str());
	if(!file.is_open()) return false;
	while (!file.eof()) {
		string line;
		file >> line;
		printf("%s\n",line.c_str());
	}
	file.close();
	return true;
}

/*-----------------------------------------------------------------
Utility to check if a file already exists
-----------------------------------------------------------------*/
bool fileExists(string path)
{
	struct stat buffer;
	return (stat (path.c_str(), &buffer) == 0);

}

//export an array of floats as a delimted string
template <class DATATYPE>
string toString(DATATYPE * data, int count, string delim) {
	string result;
	for (int i = 0; i < count; i++) {
		result.append(toString(data[i]));
		if (i<(count-1)) result.append(delim);
	}
	return result;
}

//export a vector as a delimted string
template <class DATATYPE>
string toString(vector<DATATYPE> &vec, string delim) {
	string result = EMPTY_STRING;
	int count = vec.size();
	if (count > 0) {
	for (int i = 0; i < count; i++) {
		result.append(toString(vec[i]));
		if (i<(count-1)) result.append(delim);
	}
	}
	return result;
}

template <class DATATYPE>
void deleteHeapObjects(vector<DATATYPE *> & vec) {
	int count = vec.size();
	if (count > 0) {
		for (int i = 0; i < count; i++) {
			delete vec[i];
		}
	}

}

bool ping(string ipAddr) {
	stringstream cmd;
	cmd << "ping -c 1 " << ipAddr;
	//cout << cmd.str() << endl;
	string result;
	invokeLinuxShellCommand(cmd.str().c_str(),result);
	//cout << result << endl;
	UINT pos  = result.find("1 received");
	//cout << "pos: " << pos;
	if (pos > 0 && pos < result.length()) {
		cerr << "Ping OK: 1 packet returned at " << ipAddr <<endl;
		return true;
	} else {
		return false;
	}

}

template <class DATATYPE>
bool writeToPythonList(vector<DATATYPE> &vec, string filepath) {
	ofstream out(filepath.c_str());
	if (!out.is_open()) {
		cerr << __FUNCTION__  << ": Unable to open file '" << filepath << "'" << endl;
		exit(1);
	}
	//note this currently won't work with string probably - need to be ['a','b'] not [a,b]
	out << "[" << toString(vec,COMMA) << "]";

	out.close();
}

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

//add elements provided as a space delimeted string to the vector
template <class DATATYPE>
bool addAllToVector(vector<DATATYPE> &vec, const char * line ) {
	stringstream elements(line);
	float element;
	while(!elements.eof()) {
		elements >> element;
		vec.push_back(element);
	}
	return true;
}

string trimWhitespace(string str) {
	std::size_t first = str.find_first_not_of(" \n\t");
	if (first==string::npos) first = 0;
	std::size_t last  = str.find_last_not_of(" \n\t");
	if (last==string::npos) last = str.size();
	return str.substr(first, last-first+1);
}

bool directoryExists(string const &path) {
	struct stat st;
	if (stat(path.c_str(), &st) == -1) {
		return false;
	}
	return  S_ISDIR(st.st_mode);
}


bool isFile(string const &path) {
	struct stat st;
	if (stat(path.c_str(), &st) == -1) {
		return false;
	}
	return  !S_ISDIR(st.st_mode);
}

bool deleteDirectory(string const &path) {
	if (directoryExists(path)) {
		string cmd  = "rm -rf '" + path + "'";
		int ret = system(cmd.c_str());
		return true;
	};
	return true;
}

bool createDirectory(string const &path) {
	if (directoryExists(path)) {
		cerr << "WARNING: instructed to create directory that already exists. Ignoring.." << endl;
		return false;
	} else {
		string cmd  = "mkdir '" + path + "'";
		int ret = system(cmd.c_str());
		return true;
	}
}

bool clearDirectory(string const &path) {
	bool ok = deleteDirectory(path);
	if (ok) return createDirectory(path);
	return false;
}

void pauseForKeyPress() {
	cout << "---------------------PAUSE---------------------" << endl;
	cout << "Press RETURN to continue.." << endl;
	int input;
	cin >> input;
}

bool promptYN(string prompt) {
	string input;
	cout << prompt  << "[y/n]" << endl;
	cin >> input;
	return (input=="y" || input=="Y");
}

bool wildcardFileDelete(string dir, string fileWildcard, bool doubleCheck, bool recursive) {
	stringstream cmd;
	string result;
	if (doubleCheck) {
		if (!promptYN("Files will be deleted from " + dir +  ". Proceed?")) return false;
	}

	string flag = recursive ? "-rf" : "-f";
	cmd << "rm " << flag << " " << dir << "/" << fileWildcard;

	return invokeLinuxShellCommand(cmd.str().c_str(),result);
}

bool wildcardFileDelete(string dir, string fileWildcard, bool doubleCheck) {
	return wildcardFileDelete(dir,fileWildcard,doubleCheck,false);
}

bool fileDelete(string path) {
	if (isFile(path)) {
		stringstream cmd;
		string result;
		cmd << "rm " << path;
		return invokeLinuxShellCommand(cmd.str().c_str(),result);
	}
}

bool writeArrayToTextFile(FILE * out, void * array, UINT rows, UINT cols, data_type dataType, UINT decimalPoints, bool includeRowNum, string delim, bool closeAfterWrite, bool preprendNL, bool appendNL)
{

	if (out==NULL) {
		cerr << "writeArrayToTextFile: specified file not open or valid" << endl;
		return false;
	}

	if (preprendNL) fprintf(out, "%s", NL.c_str()); //start with a newline

	float * floatArray = (float*)array;
	UINT * 	uintArray = (UINT*)array;
	int * intArray = (int*)array;
	double * doubleArray = (double*)array;

	UINT rowNum = 0;
	if (includeRowNum) fprintf(out, "%d%s",rowNum++, delim.c_str());

	for (UINT i = 0; i < rows*cols; ++i) {

		switch (dataType) {
		case(data_type_float):
															fprintf(out, "%*.*f", 1,decimalPoints, floatArray[i]);
		break;
		case(data_type_uint):
															fprintf(out, "%d", uintArray[i]);
		break;
		case(data_type_double):
															fprintf(out, "%*.*f", 1,decimalPoints, doubleArray[i]);
		break;
		case(data_type_int):
															fprintf(out, "%d", intArray[i]);
		break;
		default:
			fprintf(stderr, "ERROR: Unknown data type:%d\n", dataType);
			return false;

		}
		bool lastOneInColumn = i % cols == cols -1;
		bool lastDataItem = i== rows*cols - 1;

		if (!lastDataItem) {
			if (lastOneInColumn ) {
				fprintf(out, "\n");
				if (includeRowNum) fprintf(out, "%d%s",rowNum++, delim.c_str());
			} else {
				const char* cStr = delim.c_str();
				fprintf(out, "%s", cStr);
			}
		}

	}
	if (appendNL) fprintf(out, "%s", NL.c_str()); //end with a newline
	if (closeAfterWrite) fclose(out);
	return true;
}

bool writeArrayToTextFile(FILE * out, void * array, UINT rows, UINT cols, data_type dataType, UINT decimalPoints, bool includeRowNum, string delim, bool closeAfterWrite) {
	return writeArrayToTextFile(out,array,rows,cols,dataType,decimalPoints,includeRowNum,delim, closeAfterWrite, false, false);
}


/* --------------------------------------------------------------------------
utility to load an array of the designated data_type from a delimited text file
-------------------------------------------------------------------------- */
bool loadArrayFromTextFile(string path, void * array, string delim, UINT arrayLen, data_type dataType)
{
	using namespace std;

	float * floatArray = (float*)array;
	UINT * 	uintArray = (UINT*)array;
	int * intArray = (int*)array;
	double * doubleArray = (double*)array;

	ifstream file(path.c_str());
	if(!file.is_open()) {
		cerr << "loadArrayFromTextFile: Unable to open file to read (" << path << ")" << endl;
		return false;
	}

	string line;
	UINT index = 0;

	while (!file.eof()) {
		string line;
		file >> line;
		size_t pos1 = 0;
		size_t pos2;
		do {
			pos2 = line.find(delim, pos1);
			string datum = line.substr(pos1, (pos2-pos1));

			if (index<arrayLen) {
				switch (dataType) {
				case(data_type_float):
																	floatArray[index] = atof(datum.c_str());
				break;
				case(data_type_uint):
																	uintArray[index] = atoi(datum.c_str());;
				break;
				case(data_type_double):
																	doubleArray[index] = atof(datum.c_str());;
				break;
				case(data_type_int):
																	intArray[index] = atoi(datum.c_str());;
				break;
				default:
					fprintf(stderr, "ERROR: Unknown data type:%d\n", dataType);
					exit(1);
				}

				pos1 = pos2+1;
				index ++;
			}
		} while(pos2!=string::npos);
	}
	file.close();
	if (index>arrayLen) {
		cerr << "loadArrayFromTextFile: WARNING: This file (" << path <<  ") contained "<< index << " values, but the array provided allowed only " << arrayLen << " elements."<< endl;
		cerr << "Import was truncated at array length." << endl;
	}

	if (index<arrayLen){
		cerr << "loadArrayFromTextFile: WARNING: This file (" + path + ") is smaller than the array provided (" << arrayLen << " elements)."<< endl;
		cerr << "Import stopped at end of file, remainder of array will be empty." << endl;
	}

	return true;
}


/* --------------------------------------------------------------------------
debug utility to output a line of dashes
-------------------------------------------------------------------------- */
void printSeparator()
{
	printf("--------------------------------------------------------------\n");
}


/* --------------------------------------------------------------------------
debug utility to check contents of an array
-------------------------------------------------------------------------- */
void checkContents(string title, void * array, UINT howMany, UINT displayPerLine, data_type dataType, UINT decimalPoints, string delim, bool includeRowNum )
{
	printSeparator();
	printf("\n%s\n", title.c_str());
	writeArrayToTextFile(stdout,array,howMany/displayPerLine,displayPerLine,dataType,decimalPoints,includeRowNum,delim,false);
	printSeparator();
}

void checkContents(string title, void * array, UINT howMany, UINT displayPerLine, data_type dataType, UINT decimalPoints ) {
	checkContents( title, array,howMany, displayPerLine,dataType,decimalPoints,SPACE,true );
}


/*--------------------------------------------------------------------------
 Utility function to join (append together) a set of text files named by file path in vector, to create the specified result file
 -------------------------------------------------------------------------- */
bool joinTextFiles(vector<string> &filePaths, string outputPath) {
	bool result = true;
	ofstream out(outputPath.c_str());
	if (!out.is_open()) {
		cerr << "joinTextFiles: WARNING, failed to open specified output file : " << outputPath<< endl;
		return false;
	}

	for (UINT i = 0; i < filePaths.size(); ++i) {
		ifstream in(filePaths[i].c_str());
		if (in.is_open()) {
			while(!in.eof()) {
				string line;
				getline(in,line);
				out << line;
				if (!in.eof()) out << endl; //don't put NL at the end of each file
			}
			in.close();
		} else {
			cerr << "joinTextFiles: WARNING, failed to open specified file : " << filePaths[i] << endl;
			result = false;
		}
	}
	out.close();
	cout << "Created merged file : " << outputPath << endl;
	return result;
}

/*--------------------------------------------------------------------------
 Utility function to determine if a passed vector of ints contains the specified value
 -------------------------------------------------------------------------- */
bool vectorContains(vector<int> &vec ,int lookingFor)
{
	vector<int>::iterator it = find(vec.begin(), vec.end(), lookingFor);
	return (it != vec.end());
}

template <class DATATYPE>
void initArray(DATATYPE * array,UINT size, DATATYPE initValue) {
	for (UINT i = 0; i < size; ++i) {
		array[i] = initValue;
	}
}


template <class DATATYPE>
void zeroArray(DATATYPE * array,UINT size) {
	initArray(array,size,(DATATYPE)0.0);
}

bool createDirIfNotExists(string path) {
	if(!directoryExists(path)) {
		return createDirectory(path);
	}
	return true;
}

bool renameFile(string path, string  newPath) {
	string cmd = "mv '" + path + "' '" + newPath + "'";
	cout << cmd << endl;
	int ret  = system(cmd.c_str());
	return true;
}

bool copyFile(string path, string  newPath) {
	string cmd = "cp '" + path + "' '" + newPath + "'";
	cout << cmd << endl;
	int ret  = system(cmd.c_str());
	return true;
}

/* --------------------------------------------------------------------------
generate a random number between 0 and 1 inclusive
-------------------------------------------------------------------------- */
float getRand0to1()
{
	return ((float)rand()) / ((float)RAND_MAX) ;
}

/* --------------------------------------------------------------------------
create a pseudorandom event with a specified probability
-------------------------------------------------------------------------- */
bool randomEventOccurred(float probability)
{
	if (probability <0.0 || probability > 1.0)  {
		fprintf(stderr, "randomEventOccurred() fn. ERROR! INVALID PROBABILITY SPECIFIED AS %f.\n", probability);
		exit(1);
	} else {
		return getRand0to1() <= probability;
	}
}

/*--------------------------------------------------------------------------
total the entries in the passed int array
-------------------------------------------------------------------------- */
int sumEntries(int * data, int size)
{
	int sum = 0;
	for(int i=0; i<size; i++) {
		sum += data[i];
	}
	return sum;
}

/*--------------------------------------------------------------------------
total the entries in the passed UINT array
-------------------------------------------------------------------------- */
UINT sumEntries(UINT * data, int size)
{
	UINT sum = 0;
	for(int i=0; i<size; i++) {
		sum += data[i];
	}
	return sum;
}

/*--------------------------------------------------------------------------
total the entries in the passed float array
-------------------------------------------------------------------------- */
float sumEntries(float * data, int size)
{
	float sum = 0;
	for(int i=0; i<size; i++) {
		sum += data[i];
	}
	return sum;
}

/*--------------------------------------------------------------------------
find the highest entry in the passed int array and return its index
if there are two or more highest then choose randomly between them
-------------------------------------------------------------------------- */
int getIndexOfHighestEntry(UINT * data, UINT size)
{
	UINT winner = 0;
	UINT max = 0;

	for(UINT i=0; i<size; i++) {
		if (data[i] > max) {
			max = data[i];
			winner = i;
		} else if (data[i] == max) { //draw
			//toss a coin (we don't want to always favour the same index)
			if (randomEventOccurred(0.5)) winner = i;
		}
	}
	return winner;
}

/*--------------------------------------------------------------------------
find the highest entry in the passed int vecotr and return its index
if there are two or more highest then choose randomly between them
-------------------------------------------------------------------------- */
int getIndexOfHighestEntry(const vector<int> data)
{
	UINT winner = 0;
	int max = 0;

	for(UINT i=0; i<data.size(); i++) {
		if (data[i] > max) {
			max = data[i];
			winner = i;
		} else if (data[i] == max) { //draw
			//toss a coin (we don't want to always favour the same index)
			if (randomEventOccurred(0.5)) winner = i;
		}
	}
	return winner;
}

/*--------------------------------------------------------------------------
find the highest entry in the passed float array and return its index
if there are two or more highest then choose randomly between them
-------------------------------------------------------------------------- */
int getIndexOfHighestEntry(float * data, int size)
{
	UINT winner = 0;
	float max = 0;

	for(int i=0; i<size; i++) {
		if (data[i] > max) {
			max = data[i];
			winner = i;
		} else if (data[i] == max) { //draw
			//toss a coin (we don't want to always favour the same index)
			if (randomEventOccurred(0.5)) winner = i;
		}
	}
	return winner;
}

int reverseInt (int i)
{
	unsigned char c1, c2, c3, c4;

	c1 = i & 255;
	c2 = (i >> 8) & 255;
	c3 = (i >> 16) & 255;
	c4 = (i >> 24) & 255;

	return ((int)c1 << 24) + ((int)c2 << 16) + ((int)c3 << 8) + c4;
}


/*--------------------------------------------------------------------------
find how many cols are in the indicated text file, given the supplied column delimeter character
NB: This function aasumes that the first line is correctly representative of the whol file
-------------------------------------------------------------------------- */
int countColsInTextFile(string filepath, string delim) {
	if (fileExists(filepath)) {
		ifstream in(filepath.c_str());
		if (in.is_open() && !in.eof()) {
			string line;
			getline(in,line);
			return countOccurences(line,delim) + 1;
		} else {
			cerr <<  __FUNCTION__  << ": failed to count delimeted columns in text file:  '" << filepath << "'" << endl;
			exit(1);
		}
	} else {
		cerr << __FUNCTION__  << ": Unable to open file '" << filepath << "'" << endl;
		exit(1);
	}
}

/*--------------------------------------------------------------------------
find how many rows are in the indicated text file
-------------------------------------------------------------------------- */
int countRowsInTextFile(string filepath, bool includeBlankLines) {
	int rowCount = 0;
	ifstream in(filepath.c_str());
	if (in.is_open()) {
		while(!in.eof()) {
			string line;
			getline(in,line);
			if (includeBlankLines || line.length() > 0) {
				rowCount ++;
			}
		}
		in.close();

	} else {
		cerr << "countRowsInTextFile: Unable to open file '" << filepath << "'" << endl;
		return -1; //failed to open file
	}

	return rowCount;

}

/*
 * This fn appends the specfied string as an additional line
 */
void appendLineToTextFile(string appendToFilePath,string additionalLine) {
	ofstream out(appendToFilePath.c_str(), ifstream::app);
	if (!out.is_open()) {
		cerr <<  __FUNCTION__  << ": Unable to open file '" << appendToFilePath << "'" << endl;
		exit(1);
	}
	out << additionalLine;
	out.close();
}

/*
 * This fn appends the contents of the second file indicated to the first
 */
void appendOneFileToAnother(string appendToFilePath,string additionalContentFilePath,bool includeBlankLines) {
	ofstream out(appendToFilePath.c_str(), ifstream::app);
	if (!out.is_open()) {
		cerr << __FUNCTION__  << ": Unable to open file '" << appendToFilePath << "'" << endl;
		exit(1);
	}
	ifstream in(additionalContentFilePath.c_str());
	if (in.is_open()) {
		while(!in.eof()) {
			string line;
			getline(in,line);
			if (includeBlankLines || line.length() > 0) {
				out << endl;
				out << line;
			}
		}
		in.close();
		out.close();
	} else {
		cerr << __FUNCTION__  << ": Unable to open file '" << additionalContentFilePath << "'" << endl;
		exit(1);
	}

}

/*
 * Calcuate the sum of the passed 2 vectors, return via the supplied mem location
 */
bool vectorAdd(float * result,float * v1, float * v2, int cols) {
	for (int i = 0; i < cols; ++i) {
		result[i] = v1[i] + v2[i];
	}
	return true;
}

/*
 * Calcuate the difference v1 - v2 of the passed 2 vectors, return via the supplied mem location
 */
bool vectorSubtract(float * result,float * v1, float * v2, int cols) {
	for (int i = 0; i < cols; ++i) {
		result[i] = v1[i] - v2[i];
	}
	return true;
}

/*
 * Calcualte the modulus of a vector, optionally using the (faster) Manhattan version
 */
float vectorLength(float * v, int cols, bool useManhattan) {
	float result = 0.0f;
	if (useManhattan) {
		for (int i = 0; i < cols; ++i) {
			float colVal = v[i];
			result += abs(colVal);
		}
		return result;
	} else {
		for (int i = 0; i < cols; ++i) {
			result += powf(v[i],2);
		}
		return sqrt(result);
	}
}

/*
 * Calcuate the sum of the passed float array of vectors, return via the supplied mem location
 */
bool vectorSum(float * result,float * data,UINT rows ,UINT cols ) {
	zeroArray(result,cols);
	for (UINT i = 0; i < rows; ++i) {
		bool ok  = vectorAdd(result,result, & data[i * cols], cols);
		if (!ok) return false;
	}
	return true;
}

/*
 * Calcuate the vector, returned via the supplied mem location, whih is the result of dividing the suppiled vector by a scalar
 */
bool singleVectorDivide(float * result,float * v1, UINT cols, float divideBy) {
	for (UINT i = 0; i < cols; ++i) {
		result[i] = v1[i] / divideBy;
	}
	return true;
}

/*
 * Calcuate the vector, returned via the supplied mem location, whih is the result of dividing the suppiled vector by a scalar
 */
bool singleVectorMultiply(float * result,float * v1, UINT cols, float multiplyBy) {
	for (UINT i = 0; i < cols; ++i) {
		result[i] = v1[i] * multiplyBy;
	}
	return true;
}

bool singleVectorCopy(float * result,float * src,UINT cols ) {
	for (UINT i = 0; i < cols; ++i) {
		result[i] = src[i];
	}
	return true;
}

/*
 * Calcuate the mean/centroid of the passed float array of vectors, return via the supplied mem location
 */
bool vectorMean(float * result,float * data,UINT rows ,UINT cols ) {
	if (rows==1) {
		return singleVectorCopy(result,data,cols);
	}
	bool ok = vectorSum(result,data,rows,cols);
	if (ok) {
		float divideBy  = rows;
		return singleVectorDivide(result,result,cols,divideBy);
	} else {
		return false;
	}
}

/* --------------------------------------------------------------------------
calculate the Manhattan distance metric between two vectors of floats denoting points in for example, feature space
The "Manhattan" distance is simply the sum of all the co-ordinate differences
-------------------------------------------------------------------------- */
float getManhattanDistance(float * pointA,float * pointB, UINT numElements)
{
	float totalDistance = 0.0;
	for (UINT i=0 ; i < numElements; i++) {
		float a  = (float)pointA[i];
		float b = pointB[i];
		float pointDistance = abs(a - b);
		totalDistance = totalDistance + pointDistance;
	}
	return totalDistance;
}


/**
 * search the toMultiple array, looking for the nearest entry (by Mahattan distance) to fromSingle
 */
UINT getIndexOfNearestVector_Manhattan(float * fromSingle, float * toMultiple, int toRows, int cols) {
	float shortestDistance = getManhattanDistance(fromSingle, toMultiple,cols); //start with distance to the first entry
	int indexOfNearest = 0;
	for (int i = 1; i < toRows; ++i) { //go from 2nd to last
		float distance  = getManhattanDistance(fromSingle, & toMultiple[i * cols],cols);
		if (distance < shortestDistance) {
			shortestDistance = distance;
			indexOfNearest = i;
		}
	}
	return indexOfNearest;
}

/*
 * shorten a text file by the specified number of rows/lines, overwriting the original
 */
bool truncateTextFileByNRows(string path, int rowsToRemove) {

	ifstream in(path.c_str());
	if (!in.is_open()) {
		cerr << __FUNCTION__  << ": Unable to open file '" << path << "'" << endl;
		exit(1);
	}

	int rowsBefore = countRowsInTextFile(path,true);
	int maxLines  = rowsBefore - rowsToRemove;
	if (maxLines < 0 ) {
		cerr << __FUNCTION__  << ": can't remove " << rowsToRemove << " lines from a " << rowsBefore << " line file." << endl;
		return false;
	}

	string tmpFilePath = "tmpFileTruncateTextFileByNRows.txt";
	ofstream out(tmpFilePath.c_str());
	if (!out.is_open()) {
		cerr << __FUNCTION__  << ": Unable to open file '" << tmpFilePath << "'" << endl;
		exit(1);
	}

	string line = "";
	int linesRead = 0;
	while(!in.eof() && linesRead < maxLines) {
		getline(in,line);
		linesRead++;
		out << line;
		if (linesRead < maxLines) {
			out << endl;
		}
	}
	in.close();
	out.close();

	//delete original file, replace with tmp file
	string cmd = "mv " + tmpFilePath + SPACE + path;
	int result = system(cmd.c_str());

	return true;
}

/*
 * reduce a text file by removing the specified rows/lines (indicated in a vector of row indices), overwriting the original
 */
bool removeSpecifiedRowsFromTextFile(string path, vector<int> & rowsToRemove) {

	int numRowsToRemove = rowsToRemove.size();

	if (numRowsToRemove==0) return true; // nothing to do

	ifstream in(path.c_str());
	if (!in.is_open()) {
		cerr << __FUNCTION__  << ": Unable to open file '" << path << "'" << endl;
		exit(1);
	}

	int rowsBefore = countRowsInTextFile(path,true);
	int maxLines  = rowsBefore - numRowsToRemove;
	if (maxLines < 0 ) {
		cerr << __FUNCTION__  << ": can't remove " << numRowsToRemove << " lines from a " << rowsBefore << " line file." << endl;
		return false;
	}
	int highestRow = getIndexOfHighestEntry(rowsToRemove);
	if (rowsToRemove[highestRow] >= rowsBefore) {
		cerr << __FUNCTION__  << ": can't remove row " << rowsToRemove[highestRow] << " from a " << rowsBefore << " line file." << endl;
		return false;
	}

	string tmpFilePath = "tmpFileTruncateTextFileByNRows.txt";
	ofstream out(tmpFilePath.c_str());
	if (!out.is_open()) {
		cerr << __FUNCTION__  << ": Unable to open file '" << tmpFilePath << "'" << endl;
		exit(1);
	}

	string line = "";
	int row = 0;
	while(!in.eof()) {
		getline(in,line);
		if (!vectorContains(rowsToRemove,row)) {
			out << line;
			//if last row, don't add a NL
			if (!in.eof() && row < (rowsBefore -1) ) {
				out << endl;
			}
		}
		row++;
	}
	in.close();
	out.close();

	//overwrite original file, replace with tmp file
	string cmd = "mv " + tmpFilePath + SPACE + path;
	int result = system(cmd.c_str());

	//double check result
	int rowsAfter = countRowsInTextFile(path,true);
	if (rowsAfter != rowsBefore - numRowsToRemove) {
		cerr << "removeSpecifiedRowsFromTextFile: The final file " << path  << " should have " << (rowsBefore - numRowsToRemove) << " rows, but has " << rowsAfter << endl;
	}
	return true;
}

float dither(float x, float fraction) {
	return x + x+fraction*getRand0to1();
}

template <class DATATYPE>
DATATYPE clip(DATATYPE x, DATATYPE min, DATATYPE max)
{
	if (x < min) return min;
	if (x > max) return max;
	return x;
}

template <class DATATYPE>
void separateAcross(unsigned int total, unsigned int maxEach, vector<DATATYPE> & result ) {
	if (total==0) return ;
    unsigned int size = 0;
    for (int i = 0; i < total; ++i) {
        size++;
        if (size == maxEach) {
            result.push_back(size);
            size = 0;
        }
    }
    if (size > 0) {
        //put remiander in last one
        result.push_back(size);
    }

	return ;
}


int launchPythonScript(string dir,string script,vector<string> & args, int delayStartSecs, int waitForReadySecs, bool asychronous, bool invokeProfiler) {
	stringstream cmd;
	string  sleepCmd  = delayStartSecs > 0 ? " && sleep " + toString(delayStartSecs) : "";
	cmd << "cd " << dir << sleepCmd << " && python ";
	if (invokeProfiler) {
		//python -m cProfile myscript.py
		cmd << "-m cProfile ";
	}
	cmd  <<  script;
	for (int arg = 0; arg < args.size(); ++arg) {
		cmd << SPACE << args[arg];
	}
	if (asychronous) {
		cmd << SPACE << "&"; //run asychronously
	}
	cout << cmd.str() << endl;
	int ret = system(cmd.str().c_str());
	if (waitForReadySecs > 0) {
		cout << "Waiting " << waitForReadySecs << "seconds for Python script to be ready.." << endl;
		sleep(waitForReadySecs); //give python chance to get ready for task
	}
	return ret;

}

//block until the specified file exists (usally a marker or output from another program/process) or times out
bool waitForFileToAppear(UINT timeoutSecs,string path ) {
	cout << "Waiting for file " << path << ".." << endl;
	bool found = false;
	for (int t = 0; t < timeoutSecs; ++t) {
		if (fileExists(path)) {
			found = true;
			break;
		}
		sleep(1);
		cout << t << SPACE;
		cout.flush();
	}
	cout << endl;
	if (!found) {
		cout << "Timed out." << endl;
	}
	return found;
}


#endif
