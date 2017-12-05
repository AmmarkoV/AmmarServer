#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "05";
	static const char MONTH[] = "12";
	static const char YEAR[] = "2017";
	static const char UBUNTU_VERSION_STYLE[] =  "17.12";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 33;
	static const long BUILD  = 657;
	static const long REVISION  = 3322;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 817;
	#define RC_FILEVERSION 0,33,657,3322
	#define RC_FILEVERSION_STRING "0, 33, 657, 3322\0"
	static const char FULLVERSION_STRING [] = "0.33.657.3322";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 22;
	

#endif //VERSION_H
