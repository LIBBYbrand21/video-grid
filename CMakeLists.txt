cmake_minimum_required(VERSION 3.16)
project(VideoGrid)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Multimedia)
set(FFMPEG_DIR "C:/ffmpeg")

include_directories(${FFMPEG_DIR}/include)
link_directories(${FFMPEG_DIR}/lib)

add_executable(VideoGrid
    main.cpp
    mainwindow.cpp
    mainwindow.h
    videowidget.cpp
    videowidget.h
    videodecoder.cpp
    videodecoder.h
    audioplayer.h
)

target_link_libraries(VideoGrid PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
    avcodec
    avformat
    avutil
    swscale
    swresample
    avdevice
)
