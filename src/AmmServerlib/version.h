#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "02";
	static const char MONTH[] = "07";
	static const char YEAR[] = "2013";
	static const char UBUNTU_VERSION_STYLE[] = "13.07";
	
	//Software Status
	static const char STATUS[] = "Alpha";
	static const char STATUS_SHORT[] = "a";
	
	//Standard Version Type
	static const long MAJOR = 0;
	static const long MINOR = 27;
	static const long BUILD = 91;
	static const long REVISION = 318;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT = 83;
	#define RC_FILEVERSION 0,27,91,318
	#define RC_FILEVERSION_STRING "0, 27, 91, 318\0"
	static const char FULLVERSION_STRING[] = "0.27.91.318";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY = 56;
	

#endif //VERSION_H
