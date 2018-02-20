#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "20";
	static const char MONTH[] = "02";
	static const char YEAR[] = "2018";
	static const char UBUNTU_VERSION_STYLE[] =  "18.02";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 33;
	static const long BUILD  = 696;
	static const long REVISION  = 3508;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 853;
	#define RC_FILEVERSION 0,33,696,3508
	#define RC_FILEVERSION_STRING "0, 33, 696, 3508\0"
	static const char FULLVERSION_STRING [] = "0.33.696.3508";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 61;
	

#endif //VERSION_H
