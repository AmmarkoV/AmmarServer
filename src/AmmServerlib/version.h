#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "20";
	static const char MONTH[] = "02";
	static const char YEAR[] = "2015";
	static const char UBUNTU_VERSION_STYLE[] =  "15.02";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 28;
	static const long BUILD  = 234;
	static const long REVISION  = 1168;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 231;
	#define RC_FILEVERSION 0,28,234,1168
	#define RC_FILEVERSION_STRING "0, 28, 234, 1168\0"
	static const char FULLVERSION_STRING [] = "0.28.234.1168";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 99;
	

#endif //VERSION_H
