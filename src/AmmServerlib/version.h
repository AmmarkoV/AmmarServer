#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "16";
	static const char MONTH[] = "06";
	static const char YEAR[] = "2013";
	static const char UBUNTU_VERSION_STYLE[] = "13.06";
	
	//Software Status
	static const char STATUS[] = "Alpha";
	static const char STATUS_SHORT[] = "a";
	
	//Standard Version Type
	static const long MAJOR = 0;
	static const long MINOR = 27;
	static const long BUILD = 71;
	static const long REVISION = 196;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT = 57;
	#define RC_FILEVERSION 0,27,71,196
	#define RC_FILEVERSION_STRING "0, 27, 71, 196\0"
	static const char FULLVERSION_STRING[] = "0.27.71.196";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY = 36;
	

#endif //VERSION_H
