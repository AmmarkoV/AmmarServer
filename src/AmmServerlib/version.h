#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "04";
	static const char MONTH[] = "06";
	static const char YEAR[] = "2017";
	static const char UBUNTU_VERSION_STYLE[] =  "17.06";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 32;
	static const long BUILD  = 584;
	static const long REVISION  = 2971;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 732;
	#define RC_FILEVERSION 0,32,584,2971
	#define RC_FILEVERSION_STRING "0, 32, 584, 2971\0"
	static const char FULLVERSION_STRING [] = "0.32.584.2971";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 49;
	

#endif //VERSION_H
