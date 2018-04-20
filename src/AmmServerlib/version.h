#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "20";
	static const char MONTH[] = "04";
	static const char YEAR[] = "2018";
	static const char UBUNTU_VERSION_STYLE[] =  "18.04";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 36;
	static const long BUILD  = 954;
	static const long REVISION  = 4909;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 1161;
	#define RC_FILEVERSION 0,36,954,4909
	#define RC_FILEVERSION_STRING "0, 36, 954, 4909\0"
	static const char FULLVERSION_STRING [] = "0.36.954.4909";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 19;
	

#endif //VERSION_H
