if(__packing_piglit_description)
	return()
endif()
set(__packing_piglit_description YES)

function(Configure_PIGLIT_CPACK)
	IF(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")
	   #SET(CPACK_SET_DESTDIR "off")
	   SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Piglit OpenGL Test Suite")
	   # Set this to the Folder where Piglit is installed at.
	   #SET(CPACK_PACKAGE_CONTACT "")
	   SET(CPACK_PACKAGE_VENDOR "Piglit")
	   SET(CPACK_PACKAGE_VERSION_MAJOR "0")
	   SET(CPACK_PACKAGE_VERSION_MINOR "1")
	   SET(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 1)
	   SET(CPACK_PACKAGE_VERSION_PATCH "GIT-${GIT_SHA1}")
	   SET(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/COPYING")
	   SET(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README")
	   SET(CPACK_RESOURCE_FILE_WELCOME "${PROJECT_SOURCE_DIR}/HACKING")
	   SET(CPACK_PACKAGING_INSTALL_PREFIX "/opt/piglit-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}")
	   SET(CPACK_PACKAGING_PREFIX "/tmp/CPACK-Piglit")
	   #SET(CPACK_PACKAGE_INSTALL_DIRECTORY "piglit-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}")
   	   SET(CPACK_PACKAGE_FILE_NAME "Packages/piglit-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.999")
	   SET(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_SOURCE_DIR}/Packages/piglit-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.999-src")
	   SET(CPACK_DEBIAN_PACKAGE_DEPENDS "python, python-numpy, python-mako, freeglut3 (>=2.6.0), zlib1g, libtiff4 ( >= 3.9.5 ), libgl1-mesa-glx (>=8.0.2)")
	   SET(DEBIAN_PACKAGE_BUILDS_DEPENDS  "python-numpy, python-mako, freeglut3, zlib1g-dev, libtiff4-dev, libgl1-mesa-dev, x11proto-gl-dev, libpng12-dev")
	   # set the Package Output type.
	   IF(UNIX)
			SET(CPACK_GENERATOR "TBZ2;TGZ")
	   ELSE(WIN32)
			SET(CPACK_GENERATOR "ZIP")
	   ELSE(APPLE)
	   		# This should be a mac osx bundle.
			SET(CPACK_GENERATOR "TGZ")		
	   ENDIF(UNIX)
	   INCLUDE(CPack)
	   # List executables
	   message("CPack Package Support Enabled")
	ELSE()
	   message("CPack Package Support Disabled")
	ENDIF()
endfunction()


function(PIGLIT_INSTALL_FILES)

	# use, i.e. don't skip the full RPATH for the build tree
	SET(CMAKE_SKIP_BUILD_RPATH  FALSE)

	# when building, don't use the install RPATH already
	# (but later on when installing)
	SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) 

	# the RPATH to be used when installing
	SET(CMAKE_INSTALL_RPATH "../lib")

	# don't add the automatically determined parts of the RPATH
	# which point to directories outside the build tree to the install RPATH
	SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)

	# Here is where The Installer Magic happens.
	# Install All Executable Files.
	INSTALL(DIRECTORY ${PROJECT_BINARY_DIR}/lib DESTINATION "./lib/piglit"
		FILE_PERMISSIONS
			OWNER_READ OWNER_WRITE
			GROUP_READ 
			WORLD_READ 
		)
	INSTALL(DIRECTORY ${PROJECT_BINARY_DIR}/bin DESTINATION "./share/piglit"
		FILE_PERMISSIONS
			OWNER_READ OWNER_EXECUTE
			GROUP_READ GROUP_EXECUTE 
			WORLD_READ GROUP_EXECUTE 
	)
	INSTALL(DIRECTORY ${PROJECT_SOURCE_DIR}/framework DESTINATION "./lib/piglit"
	)
	# piglit framework.
	file(GLOB Piglet_REDIST_Runtime_Files "${PROJECT_SOURCE_DIR}/redistributable/*.py")
	INSTALL(FILES ${Piglet_REDIST_Runtime_Files} DESTINATION "./bin"
		PERMISSIONS
			OWNER_READ OWNER_EXECUTE
			GROUP_READ GROUP_EXECUTE
			WORLD_READ WORLD_EXECUTE		
		)
	# Piglit python runtime.
	file(GLOB Piglet_Runtime_Files "${PROJECT_SOURCE_DIR}/piglit*.py")
	INSTALL(FILES ${Piglet_Runtime_Files} DESTINATION "./lib/piglit/"
		PERMISSIONS
			OWNER_READ OWNER_EXECUTE
			GROUP_READ GROUP_EXECUTE
			WORLD_READ WORLD_EXECUTE
		)
	# Piglit Executable scripts

	INSTALL(DIRECTORY ${PROJECT_SOURCE_DIR}/tests DESTINATION "./share/piglit/"
		FILES_MATCHING PATTERN "*.tests" PATTERN "*.vpfp" PATTERN "*.vert"
			PATTERN "*.frag" PATTERN "*.vert" PATTERN "*.shader_test"
			PATTERN "*.rgb"
		)
	#add asm parser tests.
	INSTALL(DIRECTORY ${PROJECT_SOURCE_DIR}/tests/asmparsertest/shaders/ARBfp1.0
					  ${PROJECT_SOURCE_DIR}/tests/asmparsertest/shaders/ARBvp1.0
		DESTINATION "./share/piglit/tests/asmparsertest/shaders" )
	# Piglit test scripts

	# Test Data.
	install(DIRECTORY "${PROJECT_SOURCE_DIR}/generated_tests"  DESTINATION "./share/piglit"
		    	FILES_MATCHING PATTERN "*.frag"
		    	PATTERN "*.vert"
		    	PATTERN "*.shader_test")
	#Various required Documentation:
	INSTALL(FILES
		 "${PROJECT_SOURCE_DIR}/COPYING"
		 "${PROJECT_SOURCE_DIR}/HACKING"
		 "${PROJECT_SOURCE_DIR}/RELEASE"
		 "${PROJECT_SOURCE_DIR}/README"
		 DESTINATION "./share/doc/piglit" )

	INSTALL(DIRECTORY "${PROJECT_SOURCE_DIR}/templates"
		 "${PROJECT_SOURCE_DIR}/examples"
		 "${PROJECT_SOURCE_DIR}/documentation"
		 "${PROJECT_SOURCE_DIR}/licences"
		DESTINATION "./share/piglit" )
endfunction()
	
