#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "17";
	static const char MONTH[] = "06";
	static const char YEAR[] = "2013";
	static const char UBUNTU_VERSION_STYLE[] = "13.06";
	
	//Software Status
	static const char STATUS[] = "Alpha";
	static const char STATUS_SHORT[] = "a";
	
	//Standard Version Type
	static const long MAJOR = 0;
	static const long MINOR = 27;
	static const long BUILD = 88;
	static const long REVISION = 295;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT = 82;
	#define RC_FILEVERSION 0,27,88,295
	#define RC_FILEVERSION_STRING "0, 27, 88, 295\0"
	static const char FULLVERSION_STRING[] = "0.27.88.295";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY = 53;
	

#endif //VERSION_H
