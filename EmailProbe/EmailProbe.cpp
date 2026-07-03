#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "../Server/EmailSendUnitInterFace.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>

typedef CEmailSendUnitInterFace* (*FN_CreateEmailUnit)(
    char*, char*, char*, char*, bool, EN_CallBackNotify, uint32_t);
typedef void (*FN_FreeEmailUnit)(CEmailSendUnitInterFace*);

static std::atomic<bool> g_done(false);
static std::atomic<int> g_resultCode(-1);

static void EmailCallback(uint32_t code,
                          const char* info,
                          size_t infoLen,
                          uint64_t param) {
    std::string msg;
    if (info && infoLen > 0) msg.assign(info, infoLen);
    std::cout << "CALLBACK id=" << param
              << " code=" << code
              << " info=" << (msg.empty() ? "(empty)" : msg)
              << std::endl;
    g_resultCode = static_cast<int>(code);
    g_done = true;
}

int main(int argc, char** argv) {
    const char* recv = argc > 1 ? argv[1] : "2747756751@qq.com";
    const char* smtp = argc > 2 ? argv[2] : "smtp.126.com";
    bool useSsl = argc > 3 ? atoi(argv[3]) != 0 : false;
    const char* user = argc > 4 ? argv[4] : "jinan20";
    const char* from = argc > 5 ? argv[5] : "jinan20@126.com";

    std::cout << "Usage: EmailProbe.exe [recv] [smtp] [useSsl 0|1] [user] [from]"
              << std::endl;

    HMODULE dll = LoadLibraryA("EmailNotify.dll");
    if (!dll) {
        std::cerr << "LoadLibrary failed: " << GetLastError() << std::endl;
        return 1;
    }

    FN_CreateEmailUnit createFn =
        (FN_CreateEmailUnit)GetProcAddress(dll, "ENDLL_CreateEmailNotifyUnit");
    FN_FreeEmailUnit freeFn =
        (FN_FreeEmailUnit)GetProcAddress(dll, "ENDLL_FreeEmailNotifyUnit");
    if (!createFn || !freeFn) {
        std::cerr << "GetProcAddress failed" << std::endl;
        return 2;
    }

    EN_CallBackNotify cb = EmailCallback;
    CEmailSendUnitInterFace* unit = createFn(
        const_cast<char*>(user),
        const_cast<char*>("KGCSHVRQMZEETIKX"),
        const_cast<char*>(smtp),
        const_cast<char*>(from),
        useSsl,
        cb,
        1);
    if (!unit) {
        std::cerr << "CreateEmailNotifyUnit returned null" << std::endl;
        return 3;
    }

    EMSend ems;
    ems.dRecvEmails = recv;
    ems.dSubject = "CloudAlertSystem email probe";
    ems.dInfo = "This is a direct EmailNotify.dll probe message.";
    ems.dwCallBackFuncParam = GetTickCount64();

    std::cout << "SEND recv=" << recv
              << " smtp=" << smtp
              << " ssl=" << (useSsl ? "true" : "false")
              << " user=" << user
              << " from=" << from
              << " id=" << ems.dwCallBackFuncParam
              << std::endl;
    unit->SendEmail(ems);

    for (int i = 0; i < 600 && !g_done; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (!g_done) std::cout << "CALLBACK timeout" << std::endl;
    freeFn(unit);
    FreeLibrary(dll);
    if (!g_done) return 4;
    if (g_resultCode == 0) {
        std::cout << "RESULT success" << std::endl;
        return 0;
    }
    std::cout << "RESULT failed" << std::endl;
    return 10 + g_resultCode;
}
