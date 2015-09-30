#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "30";
	static const char MONTH[] = "09";
	static const char YEAR[] = "2015";
	static const char UBUNTU_VERSION_STYLE[] =  "15.09";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 30;
	static const long BUILD  = 374;
	static const long REVISION  = 1873;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 410;
	#define RC_FILEVERSION 0,30,374,1873
	#define RC_FILEVERSION_STRING "0, 30, 374, 1873\0"
	static const char FULLVERSION_STRING [] = "0.30.374.1873";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 39;
	

#endif //VERSION_H
