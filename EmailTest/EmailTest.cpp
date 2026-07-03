#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "../Server/EmailSendUnitInterFace.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

typedef CEmailSendUnitInterFace* (*FN_CreateEmailUnit)(
    char*, char*, char*, char*, bool, EN_CallBackNotify, uint32_t);
typedef void (*FN_FreeEmailUnit)(CEmailSendUnitInterFace*);

static std::atomic<bool> g_done(false);

static void EmailCallback(uint32_t code,
                          const char* info,
                          size_t infoLen,
                          uint64_t param) {
    std::string msg;
    if (info && infoLen > 0) msg.assign(info, infoLen);
    std::cout << "=========================================" << std::endl;
    std::cout << "回调结果：" << std::endl;
    std::cout << "  ID:     " << param << std::endl;
    std::cout << "  状态码: " << code << std::endl;
    std::cout << "  信息:   " << (msg.empty() ? "(空)" : msg) << std::endl;
    std::cout << "  结果:   " << (code == 0 ? "✅ 发送成功！" : "❌ 发送失败！") << std::endl;
    std::cout << "=========================================" << std::endl;
    g_done = true;
}

int main(int argc, char** argv) {
    std::cout << "=== Cloud Alert System 邮件测试程序 ===" << std::endl;
    std::cout << std::endl;

    // 配置参数
    const char* recvEmail = argc > 1 ? argv[1] : "2747756751@qq.com";
    const char* smtpServer = argc > 2 ? argv[2] : "smtp.126.com:465";
    bool useSsl = argc > 3 ? (atoi(argv[3]) != 0) : true;
    const char* emailUser = argc > 4 ? argv[4] : "jinan20";
    const char* emailFrom = argc > 5 ? argv[5] : "jinan20@126.com";
    const char* emailPassword = argc > 6 ? argv[6] : "KGCSHVRQMZEETIKX";

    std::cout << "配置信息：" << std::endl;
    std::cout << "  收件人:   " << recvEmail << std::endl;
    std::cout << "  SMTP服务器: " << smtpServer << std::endl;
    std::cout << "  SSL加密:  " << (useSsl ? "开启" : "关闭") << std::endl;
    std::cout << "  发件人:   " << emailFrom << std::endl;
    std::cout << "  用户名:   " << emailUser << std::endl;
    std::cout << std::endl;

    // 加载 DLL
    HMODULE dll = LoadLibraryA("EmailNotify.dll");
    if (!dll) {
        std::cerr << "❌ 加载 EmailNotify.dll 失败，错误码: " << GetLastError() << std::endl;
        return 1;
    }
    std::cout << "✅ EmailNotify.dll 加载成功" << std::endl;

    // 获取函数指针
    FN_CreateEmailUnit createFn =
        (FN_CreateEmailUnit)GetProcAddress(dll, "ENDLL_CreateEmailNotifyUnit");
    FN_FreeEmailUnit freeFn =
        (FN_FreeEmailUnit)GetProcAddress(dll, "ENDLL_FreeEmailNotifyUnit");
    
    if (!createFn || !freeFn) {
        std::cerr << "❌ 获取函数指针失败" << std::endl;
        FreeLibrary(dll);
        return 2;
    }
    std::cout << "✅ 函数指针获取成功" << std::endl;

    // 创建邮件单元
    EN_CallBackNotify cb = EmailCallback;
    CEmailSendUnitInterFace* unit = createFn(
        const_cast<char*>(emailUser),
        const_cast<char*>(emailPassword),
        const_cast<char*>(smtpServer),
        const_cast<char*>(emailFrom),
        useSsl,
        cb,
        1);
    
    if (!unit) {
        std::cerr << "❌ 创建邮件单元失败" << std::endl;
        FreeLibrary(dll);
        return 3;
    }
    std::cout << "✅ 邮件单元创建成功" << std::endl;

    // 准备邮件
    EMSend ems;
    ems.dRecvEmails = recvEmail;
    ems.dSubject = "CloudAlertSystem 测试邮件";
    ems.dInfo = "这是一封来自 CloudAlertSystem 的测试邮件！\n\n"
                "如果你收到了这封邮件，说明邮件发送功能正常工作。\n\n"
                "发送时间: " + std::string(__DATE__) + " " + std::string(__TIME__);
    ems.dwCallBackFuncParam = GetTickCount64();

    std::cout << std::endl;
    std::cout << "📧 正在发送邮件..." << std::endl;
    std::cout << "   邮件ID: " << ems.dwCallBackFuncParam << std::endl;
    std::cout << "   主题:   " << ems.dSubject.m_pszData << std::endl;
    std::cout << std::endl;

    // 发送邮件
    unit->SendEmail(ems);

    // 等待回调（最多30秒）
    std::cout << "⏳ 等待回调结果（最多30秒）..." << std::endl;
    for (int i = 0; i < 300 && !g_done; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (!g_done) {
        std::cout << std::endl;
        std::cout << "⚠️  超时：未收到回调" << std::endl;
    }

    // 清理
    freeFn(unit);
    FreeLibrary(dll);

    std::cout << std::endl;
    std::cout << "程序结束" << std::endl;

    return g_done ? 0 : 4;
}
