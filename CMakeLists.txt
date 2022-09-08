# antipidorbot is a telegram bot that looks for messages sent by pidorbot and replies to them in a bad manner
cmake_minimum_required (VERSION 3.14)
project ("antipidorbot" VERSION 1.0.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# in windows for dependency management vcpkg is used
# however it's not working sometimes so paths provided manually
if (DEFINED WIN32)

	set(VCPKG_INSTALLED_DIR "C:/github/vcpkg/installed/x64-windows")

	# this line should be enough (in ideal world) to manage dependecies
	set(CMAKE_TOOLCHAIN_FILE "C:/github/vcpkg/scripts/buildsystems/vcpkg.cmake")

	set(CMAKE_PREFIX_PATH ${VCPKG_INSTALLED_DIR})

	# if toolchain file or prefix path didn't help to find openssl
	# set(OPENSSL_ROOT_DIR ${VCPKG_INSTALLED_DIR})

	list(APPEND CMAKE_PREFIX_PATH "C:/tdlib/tdlib-x64/tdlib")

endif()

# Telegram api
find_package(Td 1.8.5 REQUIRED)

# OpenSSL is a TDlib dependecy
find_package(OpenSSL REQUIRED)

# Add source to this project's executable.
add_executable (antipidorbot "main.cpp")

target_link_libraries(antipidorbot PRIVATE OpenSSL::SSL OpenSSL::Crypto Td::TdStatic)