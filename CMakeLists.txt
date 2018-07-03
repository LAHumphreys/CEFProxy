#
# Project Definition
#
cmake_minimum_required(VERSION 3.8.2)
project(CEFProxy)

#
# Find dependencies
#
find_package(ThreadComms 0.1 REQUIRED)
find_package(FixedJSON 0.1 REQUIRED)
find_package(DevToolsLog 0.1 REQUIRED)
find_package(OSDevTools 0.1 REQUIRED)
find_package(UtilTime 0.1 REQUIRED)
find_package(GTKUtils 0.1 REQUIRED)
find_package(CPPWebSocketResponseRequest 0.1 REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(GTK REQUIRED gtk)
find_package(WEBSOCKETPP 0.6.0 REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CEF REQUIRED)
find_package(CEFBaseApp REQUIRED)


add_library(CEFProxyLib STATIC
    include/ProxyBrowserApp.h
    include/ProxyRESTEndPoint.h
    include/ReqNavigate.h
    include/ReqResource.h
    include/SubOnResourceLoaded.h
    ProxyLib/ProxyBrowserApp.cpp
    ProxyLib/ProxyRESTEndPoint.cpp
    ProxyLib/ReqNavigate.cpp
    ProxyLib/ReqResource.cpp
    ProxyLib/SubOnResourceLoaded.cpp
)

target_link_libraries(CEFProxyLib PUBLIC
    FixedJSON::FixedJSON
    ThreadComms::ThreadComms
    DevToolsLog::Log
    OSDevTools::OSDevTools
    Boost::system
    Boost::thread
    Boost::boost
    OpenSSL::SSL
    OpenSSL::Crypto
    CEFBaseApp::CEFBaseAppUtils
    CEFBaseApp::CEFBaseAppCore
)

target_include_directories(CEFProxyLib PUBLIC
    $<BUILD_INTERFACE:${CEFProxy_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

set_property(TARGET CEFProxyLib PROPERTY PUBLIC_HEADER
    ${CEFProxy_SOURCE_DIR}/include/ProxyBrowserApp.h
    ${CEFProxy_SOURCE_DIR}/include/ProxyRESTEndPoint.h
    ${CEFProxy_SOURCE_DIR}/include/ReqNavigate.h
    ${CEFProxy_SOURCE_DIR}/include/ReqResource.h
    ${CEFProxy_SOURCE_DIR}/include/SubOnResourceLoaded.h
)



add_executable(proxy proxy/proxy.cpp)
#TODO: This config need cleaning up - see how much we can drag back into the BaseApp project

# Determine the target output directory.
SET_CEF_TARGET_OUT_DIR()

target_link_libraries(proxy
        CEFProxyLib
        CEFBaseApp::CEFBaseAppDefaults
        GTKUtils::GTKUtil
        ${CEF_LIB_RELEASE}
        ${CEF_BINARY_DIR_RELEASE}/libcef_dll_wrapper.a
        ${CEF_STANDARD_LIBS}
        CPPWebSocketResponseRequest::CPPWebSocketResponseRequest
)

#TODO: Now this links we've done a lot of the hard work. We still need to do
#      the file copy etc to allow the binary to run + the above todo around
#       makin it easier for client libraries to link...
