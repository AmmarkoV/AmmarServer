#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "15";
	static const char MONTH[] = "10";
	static const char YEAR[] = "2017";
	static const char UBUNTU_VERSION_STYLE[] =  "17.10";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 32;
	static const long BUILD  = 609;
	static const long REVISION  = 3090;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 772;
	#define RC_FILEVERSION 0,32,609,3090
	#define RC_FILEVERSION_STRING "0, 32, 609, 3090\0"
	static const char FULLVERSION_STRING [] = "0.32.609.3090";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 74;
	

#endif //VERSION_H
