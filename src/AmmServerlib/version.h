#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "15";
	static const char MONTH[] = "10";
	static const char YEAR[] = "2017";
	static const char UBUNTU_VERSION_STYLE[] =  "17.10";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 32;
	static const long BUILD  = 605;
	static const long REVISION  = 3072;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 768;
	#define RC_FILEVERSION 0,32,605,3072
	#define RC_FILEVERSION_STRING "0, 32, 605, 3072\0"
	static const char FULLVERSION_STRING [] = "0.32.605.3072";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 70;
	

#endif //VERSION_H
