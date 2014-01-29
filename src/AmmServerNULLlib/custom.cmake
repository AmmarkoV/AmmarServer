execute_process(COMMAND "pwd && ln -s ../AmmServerlib/AmmServerlib.h"
    OUTPUT_VARIABLE _output OUTPUT_STRIP_TRAILING_WHITESPACE)

#file(WRITE custom.h "#define COMPILE_TIME_VALUE \"${_output}\"")
