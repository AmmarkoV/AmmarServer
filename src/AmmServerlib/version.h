#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "07";
	static const char MONTH[] = "09";
	static const char YEAR[] = "2014";
	static const char UBUNTU_VERSION_STYLE[] =  "14.09";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 28;
	static const long BUILD  = 226;
	static const long REVISION  = 1134;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 224;
	#define RC_FILEVERSION 0,28,226,1134
	#define RC_FILEVERSION_STRING "0, 28, 226, 1134\0"
	static const char FULLVERSION_STRING [] = "0.28.226.1134";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 91;
	

#endif //VERSION_H
