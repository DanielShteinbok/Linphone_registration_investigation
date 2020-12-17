cmake_minimum_required(VERSION 3.10)

project(Registration)
set(CMAKE_CXX_STANDARD 17)

#add_library(linphone++ SHARED IMPORTED)
add_library(linphone SHARED IMPORTED)
add_library(mediastreamer SHARED IMPORTED)
add_library(ortp SHARED IMPORTED)
add_library(bctoolbox SHARED IMPORTED)

#set_property(TARGET linphone++ PROPERTY IMPORTED_LOCATION /home/daniel/linphone/linphone-sdk/build/linphone-sdk/desktop/lib/liblinphone++.so)
set_property(TARGET linphone PROPERTY IMPORTED_LOCATION /home/daniel/linphone/linphone-sdk/build/linphone-sdk/desktop/lib/liblinphone.so)
set_property(TARGET mediastreamer PROPERTY IMPORTED_LOCATION /home/daniel/linphone/linphone-sdk/build/linphone-sdk/desktop/lib/libmediastreamer.so)
set_property(TARGET ortp PROPERTY IMPORTED_LOCATION /home/daniel/linphone/linphone-sdk/build/linphone-sdk/desktop/lib/libortp.so)
set_property(TARGET bctoolbox PROPERTY IMPORTED_LOCATION /home/daniel/linphone/linphone-sdk/build/linphone-sdk/desktop/lib/libbctoolbox.so)

add_executable(registration registration.c)

#target_link_libraries(registration PUBLIC linphone PUBLIC linphone++)
target_link_libraries(registration PRIVATE linphone PRIVATE mediastreamer PRIVATE ortp PRIVATE bctoolbox)
target_include_directories(registration PRIVATE /home/daniel/linphone/linphone-sdk/build/linphone-sdk/desktop/include/)
