#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "22";
	static const char MONTH[] = "10";
	static const char YEAR[] = "2017";
	static const char UBUNTU_VERSION_STYLE[] =  "17.10";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 33;
	static const long BUILD  = 643;
	static const long REVISION  = 3263;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 806;
	#define RC_FILEVERSION 0,33,643,3263
	#define RC_FILEVERSION_STRING "0, 33, 643, 3263\0"
	static const char FULLVERSION_STRING [] = "0.33.643.3263";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 8;
	

#endif //VERSION_H
