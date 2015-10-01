#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "01";
	static const char MONTH[] = "10";
	static const char YEAR[] = "2015";
	static const char UBUNTU_VERSION_STYLE[] =  "15.10";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 30;
	static const long BUILD  = 376;
	static const long REVISION  = 1886;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 411;
	#define RC_FILEVERSION 0,30,376,1886
	#define RC_FILEVERSION_STRING "0, 30, 376, 1886\0"
	static const char FULLVERSION_STRING [] = "0.30.376.1886";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 41;
	

#endif //VERSION_H
