#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "01";
	static const char MONTH[] = "09";
	static const char YEAR[] = "2014";
	static const char UBUNTU_VERSION_STYLE[] =  "14.09";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 28;
	static const long BUILD  = 148;
	static const long REVISION  = 665;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 130;
	#define RC_FILEVERSION 0,28,148,665
	#define RC_FILEVERSION_STRING "0, 28, 148, 665\0"
	static const char FULLVERSION_STRING [] = "0.28.148.665";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 13;
	

#endif //VERSION_H
