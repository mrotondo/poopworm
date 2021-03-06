if(NOT APPLE)
	message(SEND_ERROR "mac folder invoked by cmake, but not APPLE platform")
endif()

# determines the app name and app install location (scappbundlename, scappdir):
include (${CMAKE_SOURCE_DIR}/cmake_modules/MacAppFolder.cmake)

include_directories(
	${CMAKE_SOURCE_DIR}/lang/LangSource/Bison
	${CMAKE_SOURCE_DIR}/include/common
	${CMAKE_SOURCE_DIR}/include/lang
	${CMAKE_SOURCE_DIR}/include/plugin_interface
	${CMAKE_SOURCE_DIR}/editors/scapp/include
	${CMAKE_SOURCE_DIR}/editors/scapp/include/SMLAdvancedFind
	${CMAKE_SOURCE_DIR}/editors/scapp/include/SMLAdvancedFind/ICU
	${CMAKE_SOURCE_DIR}/external_libraries/icu/
	${CMAKE_SOURCE_DIR}/external_libraries/icu/unicode/
	${CMAKE_SOURCE_DIR}/lang/LangPrimSource/${HID_UTIL_DIR}
	)

find_package(Sndfile)

set(scapp_sources
	${CMAKE_SOURCE_DIR}/lang/LangPrimSource/SC_Wii.cpp
	${CMAKE_SOURCE_DIR}/lang/LangPrimSource/PyrSignalPrim.cpp
	${CMAKE_SOURCE_DIR}/lang/LangPrimSource/PyrSched.cpp
	${CMAKE_SOURCE_DIR}/lang/LangPrimSource/PyrPrimitive.cpp
	${CMAKE_SOURCE_DIR}/lang/LangPrimSource/PyrMathPrim.cpp
	${CMAKE_SOURCE_DIR}/lang/LangPrimSource/SC_ComPort.cpp
	${CMAKE_SOURCE_DIR}/lang/LangPrimSource/OSCData.cpp
	${CMAKE_SOURCE_DIR}/lang/LangPrimSource/PyrArchiver.cpp
	${CMAKE_SOURCE_DIR}/lang/LangPrimSource/PyrArrayPrimitives.cpp
	${CMAKE_SOURCE_DIR}/lang/LangPrimSource/PyrBitPrim.cpp
	${CMAKE_SOURCE_DIR}/lang/LangPrimSource/PyrCharPrim.cpp
	${CMAKE_SOURCE_DIR}/lang/LangPrimSource/PyrFilePrim.cpp
	${CMAKE_SOURCE_DIR}/lang/LangPrimSource/PyrListPrim.cpp
	${CMAKE_SOURCE_DIR}/lang/LangPrimSource/PyrPlatformPrim.cpp
	${CMAKE_SOURCE_DIR}/lang/LangPrimSource/PyrSerialPrim.cpp
	${CMAKE_SOURCE_DIR}/lang/LangPrimSource/PyrStringPrim.cpp
	${CMAKE_SOURCE_DIR}/lang/LangPrimSource/PyrUStringPrim.cpp
	${CMAKE_SOURCE_DIR}/lang/LangPrimSource/PyrSymbolPrim.cpp
	${CMAKE_SOURCE_DIR}/lang/LangPrimSource/PyrUnixPrim.cpp
	${CMAKE_SOURCE_DIR}/lang/LangPrimSource/SC_Speech.M
	${CMAKE_SOURCE_DIR}/lang/LangSource/AdvancingAllocPool.cpp
	${CMAKE_SOURCE_DIR}/lang/LangSource/ByteCodeArray.cpp
	${CMAKE_SOURCE_DIR}/lang/LangSource/DumpParseNode.cpp
	${CMAKE_SOURCE_DIR}/lang/LangSource/GC.cpp
	${CMAKE_SOURCE_DIR}/lang/LangSource/InitAlloc.cpp
	${CMAKE_SOURCE_DIR}/lang/LangSource/PyrFileUtils.cpp
	${CMAKE_SOURCE_DIR}/lang/LangSource/PyrInterpreter3.cpp
	${CMAKE_SOURCE_DIR}/lang/LangSource/PyrLexer.cpp
	${CMAKE_SOURCE_DIR}/lang/LangSource/PyrMathOps.cpp
	${CMAKE_SOURCE_DIR}/lang/LangSource/PyrMathSupport.cpp
	${CMAKE_SOURCE_DIR}/lang/LangSource/PyrMessage.cpp
	${CMAKE_SOURCE_DIR}/lang/LangSource/PyrObject.cpp
	${CMAKE_SOURCE_DIR}/lang/LangSource/PyrParseNode.cpp
	${CMAKE_SOURCE_DIR}/lang/LangSource/PyrSignal.cpp
	${CMAKE_SOURCE_DIR}/lang/LangSource/PyrSymbolTable.cpp
	${CMAKE_SOURCE_DIR}/lang/LangSource/SC_LibraryConfig.cpp
	${CMAKE_SOURCE_DIR}/lang/LangSource/Samp.cpp
	${CMAKE_SOURCE_DIR}/lang/LangSource/SimpleStack.cpp
	${CMAKE_SOURCE_DIR}/lang/LangSource/VMGlobals.cpp
	${CMAKE_SOURCE_DIR}/lang/LangSource/dumpByteCodes.cpp
	${CMAKE_SOURCE_DIR}/lang/LangPrimSource/WiiMote_OSX/wiiremote.c
	${CMAKE_SOURCE_DIR}/lang/LangPrimSource/SC_HID.cpp
	${CMAKE_SOURCE_DIR}/lang/LangPrimSource/${HID_UTIL_DIR}/HID_Utilities.c
	${CMAKE_SOURCE_DIR}/lang/LangPrimSource/${HID_UTIL_DIR}/HID_Name_Lookup.c
	${CMAKE_SOURCE_DIR}/lang/LangPrimSource/${HID_UTIL_DIR}/HID_Queue_Utilities.c
	${CMAKE_SOURCE_DIR}/lang/LangPrimSource/${HID_UTIL_DIR}/HID_Error_Handler.c
	${CMAKE_SOURCE_DIR}/lang/LangPrimSource/SC_CoreMIDI.cpp
	${CMAKE_SOURCE_DIR}/lang/LangPrimSource/SC_CoreAudioPrim.cpp

	${CMAKE_SOURCE_DIR}/common/fftlib.c
	${CMAKE_SOURCE_DIR}/common/SC_fftlib.cpp
	${CMAKE_SOURCE_DIR}/common/SC_AllocPool.cpp
	${CMAKE_SOURCE_DIR}/common/SC_DirUtils.cpp
	${CMAKE_SOURCE_DIR}/common/SC_Sem.cpp
	${CMAKE_SOURCE_DIR}/common/SC_StandAloneInfo_Darwin.cpp
	${CMAKE_SOURCE_DIR}/common/SC_StringBuffer.cpp
	${CMAKE_SOURCE_DIR}/common/SC_StringParser.cpp
	${CMAKE_SOURCE_DIR}/common/scsynthsend.cpp
	${CMAKE_SOURCE_DIR}/common/sc_popen.cpp
	${CMAKE_SOURCE_DIR}/common/strtod.c

	${CMAKE_SOURCE_DIR}/editors/scapp/CocoaBridgePrimitives.M
	${CMAKE_SOURCE_DIR}/editors/scapp/CocoaFilePrimitives.M
	${CMAKE_SOURCE_DIR}/editors/scapp/DrawBackground.M
	${CMAKE_SOURCE_DIR}/editors/scapp/GUIPrimitives.M
	${CMAKE_SOURCE_DIR}/editors/scapp/GetStringFromUser.m
	${CMAKE_SOURCE_DIR}/editors/scapp/GoToPanel.M
	${CMAKE_SOURCE_DIR}/editors/scapp/HTMLRenderer.m
	${CMAKE_SOURCE_DIR}/editors/scapp/MyDocument.M
	${CMAKE_SOURCE_DIR}/editors/scapp/RendezvousClient.M
	${CMAKE_SOURCE_DIR}/editors/scapp/SCCocoaView.M
	${CMAKE_SOURCE_DIR}/editors/scapp/SCDialog.mm
	${CMAKE_SOURCE_DIR}/editors/scapp/SCGraphView.M
	${CMAKE_SOURCE_DIR}/editors/scapp/SCImage.M
	${CMAKE_SOURCE_DIR}/editors/scapp/SCNSWindow.m
	${CMAKE_SOURCE_DIR}/editors/scapp/SCService.mm
	${CMAKE_SOURCE_DIR}/editors/scapp/SCSoundFileView.M
	${CMAKE_SOURCE_DIR}/editors/scapp/SCTextView.M
	${CMAKE_SOURCE_DIR}/editors/scapp/SCView.M
	${CMAKE_SOURCE_DIR}/editors/scapp/SCVirtualMachine.M
	${CMAKE_SOURCE_DIR}/editors/scapp/TabletEvents.m
	${CMAKE_SOURCE_DIR}/editors/scapp/TextFinder.m
	${CMAKE_SOURCE_DIR}/editors/scapp/UserPanel.M
	${CMAKE_SOURCE_DIR}/editors/scapp/main.m
	${CMAKE_SOURCE_DIR}/editors/scapp/post_queue.M
	${CMAKE_SOURCE_DIR}/editors/scapp/syntaxColorize.M
	${CMAKE_SOURCE_DIR}/editors/scapp/SMLAdvancedFind/SMLAdvancedFindController.m
	${CMAKE_SOURCE_DIR}/editors/scapp/SMLAdvancedFind/SMLStatusBarTextFieldCell.m
	${CMAKE_SOURCE_DIR}/editors/scapp/SMLAdvancedFind/ICU/ICUMatcher.m
	${CMAKE_SOURCE_DIR}/editors/scapp/SMLAdvancedFind/ICU/ICUPattern.m
	${CMAKE_SOURCE_DIR}/editors/scapp/SMLAdvancedFind/ICU/NSStringICUAdditions.m
	)

set(objcpp_sources
	${CMAKE_SOURCE_DIR}/lang/LangPrimSource/SC_Speech.M
	${CMAKE_SOURCE_DIR}/editors/scapp/CocoaBridgePrimitives.M
	${CMAKE_SOURCE_DIR}/editors/scapp/CocoaFilePrimitives.M
	${CMAKE_SOURCE_DIR}/editors/scapp/DrawBackground.M
	${CMAKE_SOURCE_DIR}/editors/scapp/GUIPrimitives.M
	${CMAKE_SOURCE_DIR}/editors/scapp/GoToPanel.M
	${CMAKE_SOURCE_DIR}/editors/scapp/MyDocument.M
	${CMAKE_SOURCE_DIR}/editors/scapp/RendezvousClient.M
	${CMAKE_SOURCE_DIR}/editors/scapp/SCCocoaView.M
	${CMAKE_SOURCE_DIR}/editors/scapp/SCGraphView.M
	${CMAKE_SOURCE_DIR}/editors/scapp/SCImage.M
	${CMAKE_SOURCE_DIR}/editors/scapp/SCService.mm
	${CMAKE_SOURCE_DIR}/editors/scapp/SCSoundFileView.M
	${CMAKE_SOURCE_DIR}/editors/scapp/SCTextView.M
	${CMAKE_SOURCE_DIR}/editors/scapp/SCView.M
	${CMAKE_SOURCE_DIR}/editors/scapp/SCVirtualMachine.M
	${CMAKE_SOURCE_DIR}/editors/scapp/UserPanel.M
	${CMAKE_SOURCE_DIR}/editors/scapp/post_queue.M
	${CMAKE_SOURCE_DIR}/editors/scapp/syntaxColorize.M
	)

set(objc_sources
	${CMAKE_SOURCE_DIR}/editors/scapp/GetStringFromUser.m
	${CMAKE_SOURCE_DIR}/editors/scapp/HTMLRenderer.m
	${CMAKE_SOURCE_DIR}/editors/scapp/SCNSWindow.m
	${CMAKE_SOURCE_DIR}/editors/scapp/TabletEvents.m
	${CMAKE_SOURCE_DIR}/editors/scapp/TextFinder.m
	${CMAKE_SOURCE_DIR}/editors/scapp/main.m
	${CMAKE_SOURCE_DIR}/editors/scapp/SMLAdvancedFind/SMLAdvancedFindController.m
	${CMAKE_SOURCE_DIR}/editors/scapp/SMLAdvancedFind/SMLStatusBarTextFieldCell.m
	${CMAKE_SOURCE_DIR}/editors/scapp/SMLAdvancedFind/ICU/ICUMatcher.m
	${CMAKE_SOURCE_DIR}/editors/scapp/SMLAdvancedFind/ICU/ICUPattern.m
	${CMAKE_SOURCE_DIR}/editors/scapp/SMLAdvancedFind/ICU/NSStringICUAdditions.m
)

if (CMAKE_COMPILER_IS_GNUCXX)
	set_source_files_properties(${objcpp_sources}
								PROPERTIES COMPILE_FLAGS "-x objective-c++ -fobjc-exceptions")
	set_source_files_properties(${objc_sources}
								PROPERTIES COMPILE_FLAGS "-fobjc-exceptions")
endif()

if(NOT (HID_UTIL_DIR MATCHES "HID_Utilities_10_4"))
	list(APPEND scapp_sources
		"${CMAKE_SOURCE_DIR}/lang/LangPrimSource/${HID_UTIL_DIR}/IOHIDElement_.c"
		"${CMAKE_SOURCE_DIR}/lang/LangPrimSource/${HID_UTIL_DIR}/IOHIDDevice_.c"
	)
endif()

set(scapp_parser_source ${CMAKE_SOURCE_DIR}/lang/LangSource/Bison/lang11d_tab.cpp)

add_definitions(-DYYSTACK_USE_ALLOCA -DHAVE_SPEECH)

# set how it shows up in the Info.plist file
SET(MACOSX_BUNDLE_ICON_FILE SCcube.icns)
# set where in the bundle to put the icns file
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/Resources/SCcube.icns
	PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
# include the icns file in the target
SET(scapp_resources ${CMAKE_CURRENT_SOURCE_DIR}/Resources/SCcube.icns)

file(COPY Resources
	DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/${scappbundlename}.app/Contents
	PATTERN Info.plist EXCLUDE
)

add_executable(${scappbundlename} MACOSX_BUNDLE
	${scapp_sources} ${scapp_parser_source}
	${scapp_resources}
	)

if (SCLANG_SERVER)
	target_link_libraries(${scappbundlename} libscsynth)
else()
	set_property(TARGET ${scappbundlename}
		APPEND
		PROPERTY COMPILE_DEFINITIONS NO_INTERNAL_SERVER
		)
endif()

if (SC_QT)
	include_directories(${CMAKE_SOURCE_DIR}/include/QtCollider)
	target_link_libraries(${scappbundlename} QtCollider)
	set_property(TARGET ${scappbundlename}
		APPEND
		PROPERTY COMPILE_DEFINITIONS SC_QT
		)
endif()

# for processing into the plist:
set_property(TARGET ${scappbundlename}
	PROPERTY MACOSX_BUNDLE_BUNDLE_NAME ${scappbundlename})
# also TODO, separate gui name? (currently using MACOSX_BUNDLE_BUNDLE_NAME twice)

target_link_libraries(${scappbundlename} icucore)

target_link_libraries(${scappbundlename} "-framework ApplicationServices")
target_link_libraries(${scappbundlename} "-framework Carbon")
target_link_libraries(${scappbundlename} "-framework Cocoa")
target_link_libraries(${scappbundlename} "-framework CoreAudio")
target_link_libraries(${scappbundlename} "-framework CoreMidi")
target_link_libraries(${scappbundlename} "-framework CoreServices")
target_link_libraries(${scappbundlename} "-framework IOBluetooth")
target_link_libraries(${scappbundlename} "-framework IOKit")
target_link_libraries(${scappbundlename} "-framework Quartz")
target_link_libraries(${scappbundlename} "-framework QuartzCore")
target_link_libraries(${scappbundlename} "-framework QuickTime")
target_link_libraries(${scappbundlename} "-framework QTKit")
target_link_libraries(${scappbundlename} "-framework WebKit")

if(SNDFILE_FOUND)
	include_directories(${SNDFILE_INCLUDE_DIR})
	target_link_libraries(${scappbundlename} ${SNDFILE_LIBRARIES})
	add_definitions("-DLIBSNDFILE_1018")
elseif(NOT NO_LIBSNDFILE)
	message(SEND_ERROR "Cannot find libsndfile")
endif(SNDFILE_FOUND)

target_link_libraries(${scappbundlename} ${PTHREADS_LIBRARY})

install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${scappbundlename}.app"
	DESTINATION ${scappdir}
	USE_SOURCE_PERMISSIONS)

if (NOT ("${standalone}" STREQUAL ""))
	# A synthdefs folder is expected in the bundle
	file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${scappbundlename}.app/Contents/Resources/synthdefs")

	# Standalones are also given a chance to overwrite things from their specially-manufactured resources dir.
	# If "${scappbundlename} Resources" exists then that's used; otherwise it uses "Standalone Resources".
	if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${scappbundlename} Resources")
		set(standaloneresourcesfrom "${scappbundlename} Resources")
	else()
		set(standaloneresourcesfrom "Standalone Resources")
	endif()
	message(STATUS "Including standalone resources from folder named: " ${standaloneresourcesfrom})

	# TODO: This globbing approach means cmake can't autodetect if files have changed and regenerate. Might be good if we can find a way that allows that.
	file(GLOB_RECURSE standaloneresourceitems RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/${standaloneresourcesfrom}" "${standaloneresourcesfrom}/*")

	foreach(onefile ${standaloneresourceitems})
		GET_FILENAME_COMPONENT(onefiledir "${onefile}" PATH)
		file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/${standaloneresourcesfrom}/${onefile}"
			DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/${scappbundlename}.app/Contents/Resources/${onefiledir}"
		)
	endforeach(onefile)
endif()

# fixup_bundle will fix the dll referencing so the .app can be sent to other people:
INSTALL(CODE "
   include(BundleUtilities)
   set(BU_CHMOD_BUNDLE_ITEMS TRUE) # allow cmake>=2.8.3 to chmod libs it's including, rather than choking on readonly
   fixup_bundle(\"${scappdir}/${scappbundlename}.app\"   \"\"   \"${CMAKE_CURRENT_BINARY_DIR}\")
   "
COMPONENT Runtime)
