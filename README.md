# CloudAlertSystem

期货云端预警系统，基于 C++ / MFC / Winsock2 / SQLite / CTP 行情接口实现。系统采用 C/S 架构，将行情接入、预警判断、状态持久化和通知分发放到服务端完成，客户端主要负责登录、配置邮箱、管理预警和接收提醒。

## 功能概览

- 用户注册、登录、退出
- 登录后显示当前绑定邮箱
- 输入密码后修改邮箱
- 添加、修改、删除价格预警
- 添加、修改、删除时间预警
- 普通列表默认隐藏已删除预警，可单独查看删除记录
- 预警列表显示当前价格，价格统一保留两位小数
- 当前价格每 30 秒自动刷新
- 常用期货合约快捷选择
- 在线触发时客户端弹窗提醒
- 离线触发时邮件提醒
- 服务端 SQLite 持久化用户、邮箱和预警数据
- 服务端启动后自动恢复未触发预警
- 服务端连接处理使用固定线程池
- 价格预警规则和最新行情使用内存缓存加速判断
- 支持 Debug 和 Release 一键构建

## 项目结构

```text
CloudAlertSystem/
  Server/                 服务端源码
    main.cpp              服务端入口
    TcpServer.*           TCP 监听与连接线程池
    ClientSession.*       单客户端会话与命令处理
    AlertDB.*             SQLite 数据库访问
    AlertEngine.*         价格/时间预警判断
    CTPMdHandler.*        CTP 行情接入
    Notifier.*            在线推送与离线邮件通知
    Protocol.h            协议常量
    ctp/                  CTP 头文件

  MFCClient/              MFC 图形客户端
    MFCClient.cpp         MFC 应用入口
    MainFrame.*           主界面
    LoginDlg.*            登录弹窗
    RegisterDlg.*         注册弹窗
    ChangeEmailDlg.*      修改邮箱弹窗
    StyledButton.*        自定义按钮

  Client/                 控制台客户端，主要用于调试
  EmailProbe/             邮件库探测/调试工具
  EmailTest/              邮件测试工具
  build.bat               一键构建脚本
  使用教程.md             Debug 版和精简测试版使用说明
```

## 环境要求

- Windows 10/11
- Visual Studio 2022 或 Visual Studio 18
- 安装 C++ 桌面开发组件
- 安装 MFC 组件
- MSBuild 可在命令行访问，或 Visual Studio 已正确安装

运行服务端时还需要以下运行库文件与 `Server.exe` 在同一目录：

- `thostmduserapi_se.dll`
- `EmailNotify.dll`
- `server.conf`
- `ctpflow/` 目录

## 编译方式

进入项目目录：

```bat
cd /d C:\Users\27477\Desktop\trae\CloudAlertSystem
```

编译 Debug 版：

```bat
build.bat
```

编译 Release 版：

```bat
build.bat Release
```

如果遇到工具集版本问题，可以指定工具集：

```bat
build.bat Debug v143
build.bat Release v143
```

输出目录：

```text
bin\x64\Debug
bin\x64\Release
```

## 运行方式

### Debug 开发版

```bat
cd /d C:\Users\27477\Desktop\trae\CloudAlertSystem\bin\x64\Debug
Server.exe
```

保持服务端窗口打开，再启动图形客户端：

```bat
MFCClient.exe
```

### Release / 精简测试版

精简测试包建议包含：

```text
CloudAlertSystem-Test-v0.1/
  Server.exe
  MFCClient.exe
  thostmduserapi_se.dll
  EmailNotify.dll
  server.conf
  ctpflow/
  README-测试说明.txt
```

启动顺序：

```text
1. 双击 Server.exe
2. 等待出现 Listening on port 8888
3. 保持服务端窗口打开
4. 双击 MFCClient.exe
5. 注册或登录账号
6. 设置邮箱
7. 添加价格预警或时间预警
```

更详细的 Debug 版和精简版使用说明见：

[使用教程.md](./使用教程.md)

## 配置说明

服务端默认监听：

```text
127.0.0.1:8888
```

邮件配置文件为：

```text
server.conf
```

示例字段：

```ini
email_account=your_account
email_password=your_smtp_authorization_code
email_smtp=smtp.example.com
email_from=your_account@example.com
email_ssl=0
```

注意：`server.conf` 中可能包含邮箱授权码，不建议提交到公开仓库，也不建议随意转发给无关人员。

## 常见问题

### 1. 客户端显示 Offline

请确认 `Server.exe` 已启动，并且服务端窗口出现：

```text
Listening on port 8888
```

### 2. 当前价格显示为 `-`

可能原因：

- 当前不是交易时段
- CTP 行情前置连接失败
- 合约代码不正确
- 当前行情源不支持该合约
- 服务端尚未收到该合约最新 tick

### 3. CTP 报 can not open CFlow file

请在 `Server.exe` 所在目录下新建：

```text
ctpflow
```

然后重新启动服务端。

### 4. 离线邮件没有收到

请检查：

- `EmailNotify.dll` 是否和 `Server.exe` 在同一目录
- `server.conf` 是否存在
- SMTP 账号、授权码和服务器地址是否正确
- 用户是否已经绑定邮箱
- 客户端在线时默认走弹窗，不会优先发送邮件

## 当前已知限制

- CTP 合约订阅仍需进一步去重，避免频繁重复订阅。
- 测试包需要手动保留 `ctpflow/` 目录。
- 密码字段后续建议升级为加盐哈希存储。
- 当前通知渠道为客户端弹窗和邮件，暂未接入短信、企业微信或移动端推送。
- 当前没有独立运维后台，服务状态主要通过控制台日志查看。

## 版本管理

本项目使用 Git 管理。常用命令：

```bat
git status
git add .
git commit -m "提交说明"
git push origin master
```

提交说明建议使用中文，便于回溯每一步做了什么。

