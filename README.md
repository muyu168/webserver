# WebServer - 高性能C++服务器

## 项目简介

这是一个从零开始手写的高性能 Web 服务器，基于 Linux 平台，使用 C++ 实现。本项目旨在深入学习和理解 Linux 高性能服务器的核心技术。

## 技术特点

- **I/O 多路复用**：使用 Epoll 边缘触发模式
- **并发模型**：Reactor 模式 + 线程池
- **HTTP 协议**：支持 HTTP/1.1 协议解析
- **定时器**：基于小根堆的定时器，处理超时连接
- **日志系统**：异步日志系统，记录服务器运行状态
- **数据库连接池**：（待实现）

## 技术栈

- **语言**：C++11/14
- **平台**：Linux
- **编译器**：g++
- **构建工具**：Makefile
- **版本控制**：Git

## 项目结构

webserver/
├── include/ # 头文件目录
│ ├── socket/ # Socket 封装
│ ├── epoll/ # Epoll 事件驱动
│ ├── http/ # HTTP 协议解析
│ ├── threadpool/ # 线程池
│ ├── timer/ # 定时器
│ ├── log/ # 日志系统
│ └── server/ # 服务器主类
├── src/ # 源文件目录
├── test/ # 测试文件
├── resources/ # 资源文件（HTML、配置等）
├── docs/ # 文档
├── build/ # 编译输出目录
└── bin/ # 可执行文件目录

## 编译与运行

### 编译
```bash
make
运行
./bin/webserver
测试
# 使用浏览器访问
http://localhost:8080

# 或使用 curl 测试
curl http://localhost:8080
清理
make clean
开发计划
 项目框架搭建
 Socket 封装
 Epoll 事件驱动
 HTTP 协议解析
 线程池实现
 定时器管理
 日志系统
 性能测试与优化
学习目标
通过这个项目，我希望掌握：
Linux 网络编程（Socket API）
I/O 多路复用（Epoll）
多线程编程与线程池设计
HTTP 协议原理
高性能服务器架构设计
Git 版本控制实践
参考资料
《Linux高性能服务器编程》
《Unix网络编程》
《C++ Primer》
作者
muyu168
许可证
MIT License
