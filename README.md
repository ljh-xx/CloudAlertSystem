# CloudAlertSystem

一个基于 C++ 的期货行情预警系统，支持实时行情推送和邮件通知功能。

## 📋 目录

- [项目简介](#项目简介)
- [功能特性](#功能特性)
- [技术栈](#技术栈)
- [快速开始](#快速开始)
- [项目结构](#项目结构)
- [开发指南](#开发指南)
- [协作开发](#协作开发)

## 项目简介

CloudAlertSystem 是一个云端期货预警系统，主要功能包括：

- 实时连接期货行情服务器（CTP 接口）
- 支持价格预警和时间预警
- 用户在线时通过 TCP 实时推送，离线时发送邮件通知
- 基于 SQLite 的本地数据库存储

## 功能特性

- ✅ CTP 期货行情接口对接
- ✅ 价格预警（>= / <=）
- ✅ 时间预警
- ✅ 实时 TCP 推送
- ✅ 邮件通知（支持 SSL/TLS）
- ✅ 多用户支持
- ✅ SQLite 本地数据库

## 技术栈

- **语言**: C++17
- **IDE**: Visual Studio 2022
- **数据库**: SQLite3
- **网络**: Winsock2
- **邮件**: EmailNotify.dll

## 快速开始

### 环境要求

- Windows 10/11
- Visual Studio 2022 (v145 工具集)
- Git

### 克隆项目

```bash
git clone https://github.com/你的用户名/CloudAlertSystem.git
cd CloudAlertSystem
```

### 编译项目

1. 使用 Visual Studio 2022 打开 `CloudAlertSystem.sln`
2. 选择配置：**Debug | x64**
3. 右键点击解决方案 → **生成解决方案**

### 运行项目

编译成功后，可执行文件位于 `bin/x64/Debug/` 目录：

```bash
cd bin/x64/Debug
# 启动服务器
Server.exe
```

## 项目结构

```
CloudAlertSystem/
├── Server/              # 服务器端
│   ├── main.cpp         # 程序入口
│   ├── Notifier.cpp     # 邮件通知模块
│   ├── AlertEngine.cpp  # 预警引擎
│   ├── CTPMdHandler.cpp # CTP 行情处理
│   ├── ClientSession.cpp # 客户端会话
│   ├── TcpServer.cpp    # TCP 服务器
│   ├── AlertDB.cpp      # 数据库操作
│   └── ctp/             # CTP 接口头文件（存根）
├── Client/              # 控制台客户端
├── MFCClient/           # MFC 图形界面客户端
├── EmailProbe/          # 邮件测试工具
├── bin/x64/Debug/       # 编译输出目录
└── CloudAlertSystem.sln # Visual Studio 解决方案
```

## 开发指南

详细的开发指南请参考 [CONTRIBUTING.md](./CONTRIBUTING.md)

### 配置邮件发送

编辑 `Server/Notifier.cpp`：

```cpp
static const char* kEmailAccount  = "你的126邮箱账号";
static const char* kEmailPassword = "你的授权码";
static const char* kEmailSmtp     = "smtp.126.com:465";
static const char* kEmailFrom     = "你的126邮箱";
```

注意：
- 需要在 126 邮箱设置中开启 SMTP 服务
- 使用授权码而非登录密码
- 确保使用 SSL/TLS（端口 465）

## 协作开发

欢迎贡献代码！请先阅读 [CONTRIBUTING.md](./CONTRIBUTING.md) 了解协作流程。

### 快速参考

| 操作 | 命令 |
|------|------|
| 克隆仓库 | `git clone https://github.com/你的用户名/CloudAlertSystem.git` |
| 创建分支 | `git checkout -b feature/你的功能` |
| 提交更改 | `git add . && git commit -m "描述"` |
| 推送分支 | `git push origin feature/你的功能` |
| 拉取更新 | `git pull origin main` |

## 许可证

[LICENSE](./LICENSE.txt)

## 联系方式

如有问题，请提交 Issue 或联系项目维护者。
