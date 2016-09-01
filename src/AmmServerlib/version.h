#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "01";
	static const char MONTH[] = "09";
	static const char YEAR[] = "2016";
	static const char UBUNTU_VERSION_STYLE[] =  "16.09";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 32;
	static const long BUILD  = 535;
	static const long REVISION  = 2716;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 663;
	#define RC_FILEVERSION 0,32,535,2716
	#define RC_FILEVERSION_STRING "0, 32, 535, 2716\0"
	static const char FULLVERSION_STRING [] = "0.32.535.2716";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 0;
	

#endif //VERSION_H
