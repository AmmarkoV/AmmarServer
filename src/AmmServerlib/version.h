#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "15";
	static const char MONTH[] = "08";
	static const char YEAR[] = "2015";
	static const char UBUNTU_VERSION_STYLE[] =  "15.08";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 29;
	static const long BUILD  = 290;
	static const long REVISION  = 1462;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 297;
	#define RC_FILEVERSION 0,29,290,1462
	#define RC_FILEVERSION_STRING "0, 29, 290, 1462\0"
	static const char FULLVERSION_STRING [] = "0.29.290.1462";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 55;
	

#endif //VERSION_H
