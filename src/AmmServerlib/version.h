#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "05";
	static const char MONTH[] = "08";
	static const char YEAR[] = "2012";
	static const char UBUNTU_VERSION_STYLE[] = "12.08";
	
	//Software Status
	static const char STATUS[] = "Alpha";
	static const char STATUS_SHORT[] = "a";
	
	//Standard Version Type
	static const long MAJOR = 0;
	static const long MINOR = 27;
	static const long BUILD = 37;
	static const long REVISION = 11;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT = 16;
	#define RC_FILEVERSION 0,27,37,11
	#define RC_FILEVERSION_STRING "0, 27, 37, 11\0"
	static const char FULLVERSION_STRING[] = "0.27.37.11";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY = 2;
	

#endif //VERSION_H