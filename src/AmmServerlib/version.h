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
	static const long BUILD  = 644;
	static const long REVISION  = 3271;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 807;
	#define RC_FILEVERSION 0,33,644,3271
	#define RC_FILEVERSION_STRING "0, 33, 644, 3271\0"
	static const char FULLVERSION_STRING [] = "0.33.644.3271";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 9;
	

#endif //VERSION_H
