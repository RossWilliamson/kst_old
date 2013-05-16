
if(NOT NETCDF4_INCLUDEDIR)

if(NOT kst_cross)
	include(FindPkgConfig)
	pkg_check_modules(NETCDF4 QUIET netcdf4)
endif()

if(NETCDF4_INCLUDEDIR AND NETCDF4_LIBRARIES)
	FIND_LIBRARY(NETCDF4_LIBRARY_CPP netcdf_c++4 
		HINTS ${NETCDF4_LIBRARY_DIRS})	
	set(NETCDF4_LIBRARY_C -L${NETCDF4_LIBRARY_DIRS} ${NETCDF4_LIBRARIES} CACHE STRING "" FORCE)
else()
	set(NETCDF4_INCLUDEDIR NETCDF4_INCLUDEDIR-NOTFOUND CACHE STRING "" FORCE)
	FIND_PATH(NETCDF4_INCLUDEDIR netcdf
		HINTS
		ENV NETCDF4_DIR
		PATH_SUFFIXES include
		PATHS 
		${kst_3rdparty_dir}
		~/Library/Frameworks
		/Library/Frameworks
		)
		
	macro(find_netcdf4_lib var libname)
		FIND_LIBRARY(${var} ${libname} 
			HINTS
			ENV NETCDF4_DIR
			PATH_SUFFIXES lib
			PATHS ${kst_3rdparty_dir})
	endmacro()
	
	find_netcdf4_lib(netcdf4_c         netcdf)
	find_netcdf4_lib(netcdf4_c_debug   netcdfd)
	find_netcdf4_lib(netcdf4_cpp       netcdf_c++4)
	find_netcdf4_lib(netcdf4_cpp_debug netcdf_c++4d)
	
	if(netcdf4_c AND netcdf4_c_debug)
		set(NETCDF4_LIBRARY_C optimized ${netcdf4_c} debug ${netcdf4_c_debug} CACHE STRING "" FORCE)
	endif()
	if(netcdf4_cpp AND netcdf4_cpp_debug)
	   set(NETCDF4_LIBRARY_CPP optimized ${netcdf4_cpp} debug ${netcdf4_cpp_debug} CACHE STRING "" FORCE)
	endif()
	
	if(NOT MSVC)
		# only msvc needs debug and release
		set(NETCDF4_LIBRARY_C   ${netcdf4_c}   CACHE STRING "" FORCE)
		set(NETCDF4_LIBRARY_CPP ${netcdf4_cpp} CACHE STRING "" FORCE)
	endif()
endif()
endif()

#message(STATUS "NETCDF4: ${NETCDF4_INCLUDEDIR}")
#message(STATUS "NETCDF4: ${NETCDF4_LIBRARY_C}")
#message(STATUS "NETCDF4: ${NETCDF4_LIBRARY_CPP}")
IF(NETCDF4_INCLUDEDIR AND NETCDF4_LIBRARY_C AND NETCDF4_LIBRARY_CPP)
	SET(NETCDF4_LIBRARIES ${NETCDF4_LIBRARY_CPP} ${NETCDF4_LIBRARY_C})
	SET(NETCDF4_INCLUDE_DIR ${NETCDF4_INCLUDEDIR})
	SET(netcdf4 1)
	message(STATUS "Found Netcdf4:")
	message(STATUS "     includes : ${NETCDF4_INCLUDE_DIR}")
	message(STATUS "     libraries: ${NETCDF4_LIBRARIES}")
ELSE()
	MESSAGE(STATUS "Not found: Netcdf4, set NETCDF4_DIR")
ENDIF()

message(STATUS "")

