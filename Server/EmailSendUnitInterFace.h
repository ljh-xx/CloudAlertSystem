#pragma once

#include <functional>
#include <string>
using namespace std;

#pragma region 库外接口,库内不实现
//动态库是否已加载
bool ENDLL_IsLoadDone();

//加载动态库
//参数: 动态库路径. 如果为空,从GetModuleFileName(NULL,,)中获取名为 EmailNotify.dll 的库进行加载
bool ENDLL_Load(char* pszDllPath = nullptr);

//释放动态库
bool ENDLL_Free();
#pragma endregion

#pragma region 邮件通知单元接口
typedef struct tagDCopy
{
	char* m_pszData = nullptr;
	size_t m_nDataLen = 0;

	tagDCopy() = default;
	tagDCopy(char* pszData, size_t nLen)
	{
		if (!pszData || nLen == 0) return;
		m_nDataLen = nLen; m_pszData = new char[m_nDataLen]; memcpy(m_pszData, pszData, m_nDataLen);
	}
	tagDCopy(char* pszData) { tagDCopy(pszData, strlen(pszData) + 1); }
	tagDCopy(const tagDCopy& Other)
	{
		if (Other.m_nDataLen == 0 || !Other.m_pszData) return;
		m_nDataLen = Other.m_nDataLen; m_pszData = new char[m_nDataLen]; memcpy(m_pszData, Other.m_pszData, m_nDataLen);
	}
	tagDCopy& operator=(const tagDCopy& Other)
	{
		clear();
		if (Other.m_nDataLen == 0 || !Other.m_pszData) return *this;
		m_nDataLen = Other.m_nDataLen; m_pszData = new char[m_nDataLen]; memcpy(m_pszData, Other.m_pszData, m_nDataLen);
		return *this;
	}
	tagDCopy& operator=(const char* pszData)
	{
		clear();
		if (!pszData || strlen(pszData) == 0) return *this;
		m_nDataLen = strlen(pszData) + 1; m_pszData = new char[m_nDataLen]; memcpy(m_pszData, pszData, m_nDataLen);
		return *this;
	}
	bool SetData(const char* pszData, size_t nLen)
	{
		clear();
		if (!pszData || nLen == 0) return false;
		m_nDataLen = nLen; m_pszData = new char[m_nDataLen]; memcpy(m_pszData, pszData, m_nDataLen);
		return true;
	}
	~tagDCopy() { clear(); }
	bool IsEmpty() { return !m_pszData || m_nDataLen == 0; }
	void clear() { if (m_pszData) delete[] m_pszData; m_pszData = nullptr; m_nDataLen = 0; }
}DCopy;

typedef struct tagEMSend
{
	DCopy dRecvEmails;					//接受者邮箱, 可设置多个,用','分割 例: xxx1@qq.com,xx2@163.com
	DCopy dSubject;						//邮件主题
	DCopy dInfo;						//邮件内容
	uint64_t dwCallBackFuncParam = 0;	//回调自定义参数, 由创建发送单元时指定的EN_CallBackNotify回调传出

	tagEMSend() {}
	tagEMSend(const tagEMSend& Other)
	{
		dRecvEmails = Other.dRecvEmails; dSubject = Other.dSubject; dInfo = Other.dInfo; dwCallBackFuncParam = Other.dwCallBackFuncParam;
	}
	tagEMSend& operator=(const tagEMSend& Other)
	{
		dRecvEmails = Other.dRecvEmails; dSubject = Other.dSubject; dInfo = Other.dInfo; dwCallBackFuncParam = Other.dwCallBackFuncParam; return *this;
	}
}EMSend;

class CEmailSendUnitInterFace
{
public:
	virtual void SendEmail(const EMSend& ems) = 0;
};
#pragma endregion

#ifdef ENDLL_Inside
	#ifdef _MSC_VER
		#define ENDLL_Exprot extern "C" __declspec(dllexport)
	#else
		#define ENDLL_Exprot extern "C" 
	#endif
#else
	#define ENDLL_Exprot 
#endif

#pragma region 库导出接口

//邮件发送过程回调
//参数1: 通知类型. 0,成功; > 0失败
//参数2: 通知内容
//参数3: 通知内容长度
//参数4: 自定义参数, 指定回调函数时传入, 回调时原值返回
//注意,此回调调用于库内发送线程中, 在回调中按需考虑线程同步问题
using EN_CallBackNotify = std::function<void(uint32_t nNotifyType, const char *pszInfo, size_t nInfoLen, uint64_t dwParam)>;

//创建一个邮件发送单元
//参数1: 发送邮箱账号
//参数2: 发送邮箱密码
//参数3: 发送邮箱服务器及端口, 样式举例: smtp.126.com:25 或 smtp.126.com:465. 端口不是必须, 可以写成smtp.126.com
//参数4: 是否使用SSL加密发送.
//参数5: 可选,发送邮件过程回调函数指针
//参数6: 可选,发送线程数,最小1,最大10. 库内采用多线程发送邮件
ENDLL_Exprot CEmailSendUnitInterFace* ENDLL_CreateEmailNotifyUnit(
	char* pszEmName, 
	char* pszEmPass, 
	char* pszServerHost,
	char* pszSendEmail,
	bool  bIsUserSSL,
	EN_CallBackNotify CallBackFunc = nullptr, 
	uint32_t nSendThreadCount = 1);

//释放一个邮件发送单元
ENDLL_Exprot void ENDLL_FreeEmailNotifyUnit(CEmailSendUnitInterFace* pUnit);

#pragma endregion