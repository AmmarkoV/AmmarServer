#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "13";
	static const char MONTH[] = "08";
	static const char YEAR[] = "2015";
	static const char UBUNTU_VERSION_STYLE[] =  "15.08";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 29;
	static const long BUILD  = 280;
	static const long REVISION  = 1400;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 285;
	#define RC_FILEVERSION 0,29,280,1400
	#define RC_FILEVERSION_STRING "0, 29, 280, 1400\0"
	static const char FULLVERSION_STRING [] = "0.29.280.1400";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 45;
	

#endif //VERSION_H
