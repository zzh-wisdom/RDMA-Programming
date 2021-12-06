# 适合单元测试

# 自动每个.cc 文件生成对应名称的可执行文件
# 自动创建bin文件夹
.PHONY: all clean

SOURCE = $(wildcard *.cc)                 # 当前目录的所有 .cc 文件
TARGETS = $(patsubst %.cc,%,$(SOURCE))    # 将源文件的.cc后缀名去掉

CC = gcc
CXX = g++
CXXFLAGS += --std=c++11 -g -Iinclude -Isrc -DNDEBUG              # 这里可以添加头文件寻找的路径
LDFLAGS = -lpthread -lpmem2 -lgtest -libverbs       # 链接库，指定搜索路径 -rpath=. -L..
# 关于依赖库，参考 https://blog.csdn.net/q1302182594/article/details/42102961/

# DEPENCES = ../cpu.h ../chunk_allocator.h ../pm_allocator.h    # 依赖头文件，只用来检测更新关系
# OBJ_SOURCE = ../chunk_allocator.cc ../pm_allocator.cc         # 被测试的源文件

BUILD_DIR := ./build

all: ${BUILD_DIR} $(TARGETS)

%:%.cc # $(OBJ_SOURCE) $(DEPENCES)
	$(CXX) $(CXXFLAGS) -o $(BUILD_DIR)/$@ $< $(LDFLAGS)

# 创建build目录
${BUILD_DIR}:
	$(shell if [ ! -e $(BUILD_DIR) ];then mkdir -p $(BUILD_DIR); fi)

clean:
	rm -rf $(BUILD_DIR) *.o
