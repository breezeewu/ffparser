cmake_minimum_required (VERSION 2.8)
set(PROJECT_NAME "ffparser")
project (${PROJECT_NAME})
#set(THIRD_PARTY_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/third_party/aarch64)
#set(FFMPEG_DIRS /mnt/d/code/mediaserver/third_party/x86_64/ffmpeg/jellyfin-ffmpeg)
set(FFMPEG_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/3rd/linux/ffmpeg)
LINK_DIRECTORIES(/usr/lib
		${FFMPEG_DIRS}/lib)

include_directories(/usr/include 
	${CMAKE_CURRENT_SOURCE_DIR}
	${FFMPEG_DIRS}/include)

#set(DEPENCIES_LIB_LINUX avcodec swscale avformat avfilter swresample avutil fdk-aac)

SET(SRC_TRANSCODE
  src/FFDemuxer.cpp
	src/FFmpegUtils.cpp
  src/AACBsfFilter.cpp
  src/H264BsfFilter.cpp
  src/H265BsfFilter.cpp
  src/UGBsfFilter.cpp
  src/main.cpp
)

set(FFMPEG_LIBRARIES
        avcodec
        avfilter
        avformat
        avutil
        swresample
		swscale
    )
#add_library(${PROJECT_NAME} SHARED ${SRC_TRANSCODE} src/transcode/gstvideopipeline.cpp)
#target_link_libraries(${PROJECT_NAME} ${DEPENCIES_LIB})
#target_compile_options(${PROJECT_NAME} PRIVATE -g)
#target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_GSTREAMER_ENCODER=1)

add_executable(${PROJECT_NAME} ${SRC_TRANSCODE})
target_link_libraries(${PROJECT_NAME} ${FFMPEG_LIBRARIES} z lzma m)
target_compile_options(${PROJECT_NAME} PRIVATE -g)
#file(COPY build/${PROJECT_NAME} DESTINATION /mnt/d/code/debug/jellyfin-ffmpeg/build/build/bin/)

#add_executable(${APP_EXE_NAME} ${SRC_TRANSCODE} src/transcode/appmain.cpp)
#target_link_libraries(${APP_EXE_NAME} ${DEPENCIES_LIB} transcode)
#target_compile_options(${APP_EXE_NAME} PRIVATE -g)

#add_executable(${X86_EXE_NAME} ${SRC_TRANSCODE} src/transcode/appmain.cpp)
#target_link_libraries(${X86_EXE_NAME} ${DEPENCIES_LIB_LINUX})
#target_compile_options(${X86_EXE_NAME} PRIVATE -g)
#target_compile_definitions(${X86_EXE_NAME} PRIVATE ENABLE_GSTREAMER_ENCODER=0)