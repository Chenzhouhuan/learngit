#最低版本
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
 
#项目信息
# PROJECT(EventLoop_test5)
PROJECT(MySQL_Connect_C++_test0)
 
#头文件目录
INCLUDE_DIRECTORIES(/usr/include)
LINK_LIBRARIES(/usr/lib64)

add_definitions(-D _GLIBCXX_USE_CXX11_ABI=0)

#指定生成目标
ADD_EXECUTABLE(
               demo demo.cc 
              )
 
#添加链接库
TARGET_LINK_LIBRARIES(demo mysqlcppconn)