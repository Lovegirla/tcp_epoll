cmake_minimum_required(VERSION 3.10)

project(HelloWorld)  


aux_source_directory(. SRCS) # 源文件

add_executable(server ${SRCS}) # 生成可执行文件，这里程序名即为功能名

