cmake_minimum_required(VERSION 3.8)
enable_language(CXX)
add_library(libember_slim
    SHARED
    ember_consumer.c
    ember_consumer.h
	SocketEx.cpp
	SocketEx.h
	Utilities.cpp
	Utilities.h
	Output.cpp
	Output.h
	ClientConfig.cpp
	ClientConfig.h
	EmberConsumer.cpp
	EmberConsumer.h
	APIFormat.h
	ReadIni.cpp
	Client.cpp
	Client.h
	DeviceContents.cpp
    DeviceContents.h
    DeviceAction.cpp
    DeviceAction.h
    EmberInfo.h
	ReadCsv.cpp
)

target_compile_features(libember_slim PUBLIC cxx_std_17)

target_include_directories(libember_slim
	PUBLIC
	${CMAKE_SOURCE_DIR}/libember_slim/include
)

target_link_libraries(libember_slim
	PUBLIC
	${CMAKE_SOURCE_DIR}/libember_slim/lib/ember_slim-static.lib	
)

add_definitions(-DDLL_EXPORT)