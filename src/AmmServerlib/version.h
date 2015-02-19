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
	static const long BUILD  = 233;
	static const long REVISION  = 1159;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 230;
	#define RC_FILEVERSION 0,28,233,1159
	#define RC_FILEVERSION_STRING "0, 28, 233, 1159\0"
	static const char FULLVERSION_STRING [] = "0.28.233.1159";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 98;
	

#endif //VERSION_H
