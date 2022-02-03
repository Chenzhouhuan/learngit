#最低版本
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
 
#项目信息
# PROJECT(EventLoop_test5)
PROJECT(TimerQueueCancel_test)
 
#头文件目录
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/lib)
LINK_DIRECTORIES(${PROJECT_SOURCE_DIR}/lib)
# LINK_DIRECTORIES(/home/czh/build/release-install-cpp11/lib)
 
#查找当前目录下所有源文件
#并将文件名保存在DIR_SOURECE
# AUX_SOURCE_DIRECTORY(. DIR_SOURCE)
 
#添加math子目录
# ADD_SUBDIRECTORY()
 
 
# #指定生成目标
# # ADD_EXECUTABLE(test1 ${DIR_SOURCE})
# ADD_EXECUTABLE(test5 test5.cc src/Channel.cc src/Poller.cc src/EventLoop.cc src/Timer.cc src/TimerQueue.cc)
 
# #添加链接库
# TARGET_LINK_LIBRARIES(test5 muduo_net.a)
# TARGET_LINK_LIBRARIES(test5 muduo_base.a)
# target_link_libraries(test5 pthread rt)

# #获取目标文件
file(GLOB_RECURSE mains RELATIVE "${PROJECT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/test*.cc")
foreach(mainfile IN LISTS mains)
      get_filename_component(mainname ${mainfile} NAME_WE)
      #指定生成目标
      ADD_EXECUTABLE(
                     ${mainname} ${mainfile}
                     src/Channel.cc 
                     src/EPoller.cc 
                     src/EventLoop.cc 
                     src/Timer.cc 
                     src/TimerQueue.cc 
                     src/EventLoopThread.cc
                     src/Acceptor.cc
                     src/InetAddress.cc
                     src/Socket.cc
                     src/SocketsOps.cc
                     src/TcpConnection.cc
                     src/TcpServer.cc
                     src/Buffer.cc
                     src/EventLoopThreadPool.cc
                     src/Connector.cc
                     src/TcpClient.cc
                    )  
      #添加链接库
      TARGET_LINK_LIBRARIES(${mainname} muduo_net.a)
      TARGET_LINK_LIBRARIES(${mainname} muduo_base.a)
      target_link_libraries(${mainname} pthread rt)
endforeach(mainfile IN LISTS mains)



#指定生成目标
# ADD_EXECUTABLE(test1 ${DIR_SOURCE})
# ADD_EXECUTABLE(
#                test8 test8.cc 
#                      src/Channel.cc 
#                      src/Poller.cc 
#                      src/EventLoop.cc 
#                      src/Timer.cc 
#                      src/TimerQueue.cc 
#                      src/EventLoopThread.cc
#                      src/Acceptor.cc
#                      src/InetAddress.cc
#                      src/Socket.cc
#                      src/SocketsOps.cc
#                      src/TcpConnection.cc
#                      src/TcpServer.cc
#                      src/Buffer.cc
#                      src/EventLoopThreadPool.cc
#               )
 
#添加链接库
# TARGET_LINK_LIBRARIES(test8 muduo_net.a)
# TARGET_LINK_LIBRARIES(test8 muduo_base.a)
# target_link_libraries(test8 pthread rt)