# 编译器和编译选项
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2 -g
INCLUDES = -I./include

# 目录定义
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin
INCLUDE_DIR = include

# 目标可执行文件
TARGET = $(BIN_DIR)/webserver

# 查找所有源文件
SOURCES = $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/*/*.cpp)

# 生成对应的目标文件列表
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SOURCES))

# 默认目标
all: $(TARGET)

# 链接生成可执行文件
$(TARGET): $(OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lpthread
	@echo "编译完成: $(TARGET)"

# 编译源文件为目标文件
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@
	@echo "编译: $<"

# 清理编译生成的文件
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "清理完成"

# 运行程序
run: $(TARGET)
	./$(TARGET)

# 调试模式编译
debug: CXXFLAGS += -DDEBUG -g
debug: clean all

# 显示帮助信息
help:
	@echo "可用的 make 目标："
	@echo "  make- 编译项目"
	@echo "  make clean    - 清理编译文件"
	@echo "  make run      - 编译并运行"
	@echo "  make debug    - 调试模式编译"
	@echo "  make help     - 显示此帮助信息"

# 声明伪目标
.PHONY: all clean run debug help
