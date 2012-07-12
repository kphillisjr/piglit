cmake_minimum_required(VERSION 2.6)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")

INCLUDE (CheckCCompilerFlag)
INCLUDE (CheckCXXCompilerFlag)
INCLUDE (CheckFunctionExists)
INCLUDE (CheckIncludeFile)
INCLUDE (FindPkgConfig)
INCLUDE (FindGit)

INCLUDE (GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
message( "Building piglit from Git Repository:\n\t Git Refspec: ${GIT_REFSPEC} \n\t Git Revision: ${GIT_SHA1}" )

project (piglit)
set(CMAKE_INSTALL_PREFIX "/opt/piglit")
find_package(OpenGL REQUIRED)
find_package(TIFF)
find_package(GLUT REQUIRED)
find_package(PNG REQUIRED)
find_package(X11)


option(BUILD_GLES1_TESTS "Build tests for OpenGL ES1" OFF)
option(BUILD_GLES2_TESTS "Build tests for OpenGL ES2" OFF)

option(USE_WAFFLE "Use Waffle in place of GLUT" OFF)
if(USE_WAFFLE)
	# FIXME: Specify version requirements for Waffle.
	find_package(Waffle REQUIRED)
	add_definitions(-DUSE_WAFFLE)
	include_directories("${WAFFLE_INCLUDE_DIR}")
endif(USE_WAFFLE)

if(BUILD_GLES1_TESTS AND NOT USE_WAFFLE)
	message(FATAL_ERROR "Option BUILD_GLES1_TESTS requires USE_WAFFLE")
endif(BUILD_GLES1_TESTS AND NOT USE_WAFFLE)

if(BUILD_GLES2_TESTS AND NOT USE_WAFFLE)
	message(FATAL_ERROR "Option BUILD_GLES2_TESTS requires USE_WAFFLE")
endif(BUILD_GLES2_TESTS AND NOT USE_WAFFLE)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	option(BUILD_GLX_TESTS "Build tests that require GLX" ON)
ELSE()
	option(BUILD_GLX_TESTS "Build tests that require GLX" OFF)
ENDIF()
IF(BUILD_GLX_TESTS)
	pkg_check_modules(GLPROTO REQUIRED glproto)
ENDIF()

# The 'REQUIRED' above correctly produces an error for
# OpenGL and PNG, but there's a bug involving FindGLUT.cmake
# that fails to produce the error as of CMake 2.8.5.
#
# Instead, CMake keeps going and eventually spams
# the console with a message for every target that used
# e.g. the ${GLUT_INCLUDE_DIR} variable. So it
# prints a line for basically every single test in piglit.
#
# Work around the bug and error out quickly here instead.
if (NOT GLUT_FOUND)
	message(FATAL_ERROR "GLUT library not found")
endif()

# Check for presence of Python 2.6 or greater.
foreach(python_cmd python2 python)
	execute_process(
		COMMAND ${python_cmd} -c "import sys; assert '2.6' <= sys.version < '3'"
		OUTPUT_QUIET
		ERROR_QUIET
		RESULT_VARIABLE python_version_check_error_code)
	if(python_version_check_error_code EQUAL 0)
		set(python ${python_cmd})
		break()
	endif(python_version_check_error_code EQUAL 0)
endforeach(python_cmd)

if(NOT DEFINED python)
	message(FATAL_ERROR "python version 2.x (where x >= 6) required")
endif(NOT DEFINED python)

# Check for the presence of several python packages, which are needed to build
# generated tests.
execute_process(
	COMMAND ${python} -c "import numpy"
	OUTPUT_QUIET
	ERROR_QUIET
	RESULT_VARIABLE import_numpy_error_code)
if(NOT import_numpy_error_code EQUAL 0)
	message(FATAL_ERROR "numpy python module not found")
endif(NOT import_numpy_error_code EQUAL 0)
execute_process(
	COMMAND ${python} -c "from mako.template import Template"
	OUTPUT_QUIET
	ERROR_QUIET
	RESULT_VARIABLE import_mako_error_code)
if(NOT import_mako_error_code EQUAL 0)
	message(FATAL_ERROR "mako.template python module not found")
endif(NOT import_mako_error_code EQUAL 0)

# Default to compiling with debug information (`gcc -g`):
if(NOT CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE Debug CACHE STRING
	"May be one of: None Debug RelWithDebInfo Release MinSizeRel" FORCE)
endif(NOT CMAKE_BUILD_TYPE)

if (NOT MSVC)
	CHECK_C_COMPILER_FLAG("-Wall" C_COMPILER_FLAG_WALL)
	IF (C_COMPILER_FLAG_WALL)
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
	ENDIF (C_COMPILER_FLAG_WALL)
	CHECK_CXX_COMPILER_FLAG("-Wall" CXX_COMPILER_FLAG_WALL)
	IF (CXX_COMPILER_FLAG_WALL)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
	ENDIF (CXX_COMPILER_FLAG_WALL)

	# Unfortunately MSVC does not support C99.  Among all features enabled
	# by C99, declarations after statements is the most frequently used.
	# For portability sake, we request gcc to warn when this is used.
	CHECK_C_COMPILER_FLAG("-Wall" C_COMPILER_FLAG_WDECL_AFTER_STMT)
	IF (C_COMPILER_FLAG_WDECL_AFTER_STMT)
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wdeclaration-after-statement")
	ENDIF (C_COMPILER_FLAG_WDECL_AFTER_STMT)
else ()
	include_directories("include/msvc/c99")

	# -Wall or (/Wall) is actually supported by MSVC and would be detected
	# by CHECK_C_COMPILER_FLAG above, but is very pedantic, causing
	# thousand of warnings when including windows.h.
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -W4")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W4")

	add_definitions (-D_CRT_SECURE_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_WARNINGS)
	add_definitions (-D_SCL_SECURE_NO_DEPRECATE -D_SCL_SECURE_NO_WARNINGS)
endif ()

if (MINGW)
	# Avoid depending on MinGW runtime DLLs
	check_cxx_compiler_flag (-static-libgcc HAVE_STATIC_LIBGCC_FLAG)
	if (HAVE_STATIC_LIBGCC_FLAG)
		set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc")
		set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc")
		set (CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -static-libgcc")
	endif ()
	check_cxx_compiler_flag (-static-libstdc++ HAVE_STATIC_LIBSTDCXX_FLAG)
	if (HAVE_STATIC_LIBSTDCXX_FLAG)
		set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++")
		set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libstdc++")
		set (CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -static-libstdc++")
	endif ()
endif ()

if (WIN32)
	# MSVC & MinGW only define & use APIENTRY
	add_definitions (-DGLAPIENTRY=__stdcall)

        # Avoid namespace pollution when including windows.h
        # http://support.microsoft.com/kb/166474
	add_definitions (-DWIN32_LEAN_AND_MEAN=1)

	# Don't define min/max macros
	add_definitions (-DNOMINMAX)

	# Define M_PI and others
	add_definitions (-D_USE_MATH_DEFINES)
endif (WIN32)

if (APPLE)
	find_path(GLEXT_INCLUDE_DIR
		NAMES OpenGL/glext.h
		PATHS ${OPENGL_INCLUDE_DIR}
		DOC "Include for OpenGL/glext.h on OSX"
	)
else (APPLE)
	find_path(GLEXT_INCLUDE_DIR
		NAMES GL/glext.h
		PATHS ${OPENGL_INCLUDE_DIR}
		DOC "Include for GL/glext.h"
	)
endif (APPLE)

FIND_LIBRARY(OPENGL_egl_LIBRARY
      NAMES EGL
      PATHS /usr/lib
)
find_library(OPENGL_gles1_LIBRARY NAMES GLESv1_CM)
find_library(OPENGL_gles2_LIBRARY NAMES GLESv2)

# Put all executables into the bin subdirectory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${piglit_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${piglit_BINARY_DIR}/lib)

# Do the same for MSVC, regardless of the build type. This only works correctly
# for CMake 2.8.1 and above.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${piglit_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${piglit_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${piglit_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${piglit_BINARY_DIR}/bin)

check_function_exists(strchrnul HAVE_STRCHRNUL)
check_function_exists(fopen_s   HAVE_FOPEN_S)
check_function_exists(setrlimit HAVE_SETRLIMIT)

check_include_file(sys/time.h  HAVE_SYS_TIME_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(sys/resource.h  HAVE_SYS_RESOURCE_H)
check_include_file(sys/stat.h  HAVE_SYS_STAT_H)
check_include_file(unistd.h    HAVE_UNISTD_H)
check_include_file(fcntl.h     HAVE_FCNTL_H)

configure_file(
	"${piglit_SOURCE_DIR}/tests/util/config.h.in"
	"${piglit_BINARY_DIR}/tests/util/config.h"
)



include(cmake/piglit_util.cmake)
include(cmake/piglit_glapi.cmake)
include(cmake/piglit_dispatch.cmake)

IF(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")

   SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Piglit OpenGL Test Suite")
   # Set these two variable If you Aare the Package Maintainer.
   #SET(CPACK_PACKAGE_CONTACT "")
   #SET(CPACK_PACKAGE_VENDOR "Piglit")
   SET(CPACK_PACKAGE_VERSION_MAJOR "0")
   SET(CPACK_PACKAGE_VERSION_MINOR "1")
   SET(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 1)
   SET(CPACK_PACKAGE_VERSION_PATCH "GIT-${GIT_SHA1}")
   SET(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/COPYING")
   SET(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README")
   SET(CPACK_RESOURCE_FILE_WELCOME "${PROJECT_SOURCE_DIR}/HACKING")
   SET(CPACK_SET_DESTDIR "cmake-packing")
   Set(CPACK_PACKAGING_INSTALL_PREFIX /opt/piglit-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR})
   SET(CPACK_PACKAGE_INSTALL_DIRECTORY "piglit-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}")
   SET(CPACK_SOURCE_PACKAGE_FILE_NAME "packages/piglit-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
   SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.3.1-6), libgcc1 (>= 1:3.4.2-12), freeglut3 >= 2.6.0, python-numpy, python-mako >= 0.5.0, libpng12-0 >=1.2.46")
   SET(CPACK_SOURCE_IGNORE_FILES "/^CPack.*/" "/.*\\\\.kdev.*/" "_CPack_Packages*" "/^CMakeCache.txt"
   	 "/^cmake_install.cmake" "/^.git/" "/^CMakeFiles/" "/^CMakeCache.txt" "${CPACK_SOURCE_PACKAGE_FILE_NAME}.*" "/^target_api" 
   	 "/^generated_tests/spec/")
   IF(UNIX)
		SET(CPACK_GENERATOR "TGZ;TBZ2")
   ELSE(WIN32)
		SET(CPACK_GENERATOR "ZIP")
   ELSE(APPLE)
   	# todo.
		SET(CPACK_GENERATOR "TGZ")		
   ENDIF(UNIX)
   INCLUDE(CPack)
   # List executables
   message("CPack Package Support Enabled")
ENDIF(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")


include_directories(src)
add_subdirectory(cmake/target_api)
add_subdirectory(generated_tests)

# Here is where The Installer Magic happens.

# Install All Executable Files.
INSTALL(DIRECTORY ${PROJECT_BINARY_DIR}/bin ${PROJECT_BINARY_DIR}/lib DESTINATION "." USE_SOURCE_PERMISSIONS)
# piglit framework.
INSTALL(DIRECTORY ${PROJECT_SOURCE_DIR}/framework DESTINATION "." USE_SOURCE_PERMISSIONS
	FILES_MATCHING PATTERN "*.py"
	)
file(GLOB Piglet_Runtime_Files ${PROJECT_SOURCE_DIR}/*.py)
INSTALL(FILES ${Piglet_Runtime_Files} DESTINATION "."
	PERMISSIONS
		OWNER_READ OWNER_EXECUTE OWNER_WRITE
		GROUP_READ GROUP_EXECUTE
		WORLD_READ WORLD_EXECUTE
	
	)
# Piglit Executable scripts
INSTALL(DIRECTORY tests DESTINATION "." USE_SOURCE_PERMISSIONS
	FILES_MATCHING PATTERN "*.tests"
		PATTERN "*.frag" PATTERN "*.vert" PATTERN "*.shader_test"
    	PATTERN "*.rgb"
	)	
# Piglit test scripts

# Test Data.
install(DIRECTORY "generated_tests"  DESTINATION "."
        	FILES_MATCHING PATTERN "*.frag"
        	PATTERN "*.vert"
        	PATTERN "*.shader_test")        	
#Various required Documentation:
INSTALL(FILES
	 "${PROJECT_SOURCE_DIR}/COPYING" "${PROJECT_SOURCE_DIR}/HACKING" "${PROJECT_SOURCE_DIR}/RELEASE" "${PROJECT_SOURCE_DIR}/README" DESTINATION ".")

INSTALL(DIRECTORY "templates" DESTINATION ".")
INSTALL(DIRECTORY "examples" DESTINATION ".")
INSTALL(DIRECTORY "documentation" DESTINATION ".")
INSTALL(DIRECTORY "licences" DESTINATION ".")

