cmake_policy(SET CMP0057 NEW) # For IN_LIST to work as operator

function(ensure_is_one_of VAR_NAME OPTIONS DEFAULT_VALUE)
    if(NOT ${VAR_NAME} IN_LIST OPTIONS)
      set(${VAR_NAME} ${DEFAULT_VALUE} CACHE STRING "" FORCE)
      message("${VAR_NAME} was not specified. Options are: ${OPTIONS}. Using default: ${DEFAULT_VALUE}")
    endif()
endfunction()

function(ensure_is_specified VAR_NAME DEFAULT_VALUE)
    if(NOT ${VAR_NAME})
      set(${VAR_NAME} ${DEFAULT_VALUE} CACHE STRING "" FORCE)
      message("${VAR_NAME} was not specified. Using default: ${DEFAULT_VALUE}")
    endif()
endfunction()
