#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "03";
	static const char MONTH[] = "10";
	static const char YEAR[] = "2015";
	static const char UBUNTU_VERSION_STYLE[] =  "15.10";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 30;
	static const long BUILD  = 420;
	static const long REVISION  = 2091;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 459;
	#define RC_FILEVERSION 0,30,420,2091
	#define RC_FILEVERSION_STRING "0, 30, 420, 2091\0"
	static const char FULLVERSION_STRING [] = "0.30.420.2091";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 85;
	

#endif //VERSION_H
