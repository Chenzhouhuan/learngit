# cmake_minimum_required(VERSION 3.0)

# project(EventLoop_test1)

# include_directories(lib)

# link_directories(/home/czh/build/release-install-cpp11/lib)
# link_directories(/home/czh/build/release-install-cpp11/lib/libmuduo_base.a)

# add_executable(test1 test1.cc src/EventLoop.cc)

# target_link_libraries(test1 /home/czh/build/release-install-cpp11/lib/libmuduo_net.a)
# target_link_libraries(test1 /home/czh/build/release-install-cpp11/lib/libmuduo_base.a)
# target_link_libraries(test1 pthread rt)

#最低版本
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
 
#项目信息
PROJECT(EventLoop_test1)
 
#头文件目录
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/lib)
# LINK_DIRECTORIES(${PROJECT_SOURCE_DIR}/lib)
LINK_DIRECTORIES(/home/czh/build/release-install-cpp11/lib)
 
#查找当前目录下所有源文件
#并将文件名保存在DIR_SOURECE
# AUX_SOURCE_DIRECTORY(. DIR_SOURCE)
 
#添加math子目录
# ADD_SUBDIRECTORY()
 
 
#指定生成目标
# ADD_EXECUTABLE(test1 ${DIR_SOURCE})
ADD_EXECUTABLE(test2 test2.cc src/EventLoop.cc)
 
#添加链接库
TARGET_LINK_LIBRARIES(test2 muduo_net.a)
TARGET_LINK_LIBRARIES(test2 muduo_base.a)
target_link_libraries(test2 pthread rt)