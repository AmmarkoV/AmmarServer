#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "10";
	static const char MONTH[] = "12";
	static const char YEAR[] = "2017";
	static const char UBUNTU_VERSION_STYLE[] =  "17.12";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 33;
	static const long BUILD  = 660;
	static const long REVISION  = 3334;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 819;
	#define RC_FILEVERSION 0,33,660,3334
	#define RC_FILEVERSION_STRING "0, 33, 660, 3334\0"
	static const char FULLVERSION_STRING [] = "0.33.660.3334";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 25;
	

#endif //VERSION_H
