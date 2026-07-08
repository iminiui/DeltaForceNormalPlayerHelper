// -----------------------------------------------------------------------------
// 文件：SimpleThread.h
// 功能：线程封装类和通用辅助函数
// 说明：提供轻量级的线程创建封装、进程单例检测、延时等待等功能
// -----------------------------------------------------------------------------

#pragma once

// Windows 基础类型定义
#include "windef.h"
// 进程和线程相关API
#include "process.h"

/**
 * @brief 延时等待函数
 * 
 * 阻塞当前线程指定的秒数。使用clock()函数精确计算时间，
 * 每50ms检测一次，避免长时间阻塞导致的响应延迟。
 * 
 * @param sec 等待的秒数
 * @return bool 始终返回true
 * @note 该函数在等待期间会释放CPU时间片（Sleep(50)），不会造成CPU占用过高
 */
bool Wait(int sec)
{
	// 记录开始时间
	clock_t beg = clock(), now = clock();
	// 循环等待直到达到指定秒数
	while (now - beg <= sec * CLOCKS_PER_SEC)
	{
		now = clock();
		// 每50ms检测一次，避免忙等待
		Sleep(50);
	}
	return true;
};

/**
 * @brief 检测程序是否已运行（单例模式）
 * 
 * 通过创建互斥锁（Mutex）来检测程序是否已经在运行。
 * 如果互斥锁已存在，则说明程序已运行，返回true；
 * 否则创建互斥锁并返回false，表示程序可以正常启动。
 * 
 * @param hMutex 输出参数，返回创建的互斥锁句柄
 * @param MutexName 互斥锁的名称，用于标识程序实例
 * @return bool true表示程序已运行，false表示可以启动
 * @warning 调用者需要负责关闭互斥锁句柄以避免资源泄漏
 */
bool CheckSelfExists(HANDLE& hMutex, const char* MutexName) {
	hMutex = CreateMutexA(NULL, true, MutexName);
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		return true;
	}
	return false;
}

/**
 * @brief 释放互斥锁句柄
 * 
 * 安全地关闭之前由CheckSelfExists创建的互斥锁句柄。
 * 在程序退出时调用此函数，确保互斥锁资源被正确释放。
 * 
 * @param hMutex 互斥锁句柄
 * @return bool 释放成功返回true，失败返回false
 */
bool CloseMutex(HANDLE& hMutex) {
	if (hMutex != NULL)
	{
		CloseHandle(hMutex);
		hMutex = NULL;
		return true;
	}
	return false;
}

/**
 * @brief 弹出系统模态消息框
 * 
 * 显示一个系统模态消息框，确保用户必须处理后才能继续操作。
 * 常用于显示重要的错误信息或提示。
 * 
 * @param str 要显示的消息内容
 */
void MBX(const char* str)
{
	MessageBoxA(NULL, str, "Notice", MB_OK | MB_SYSTEMMODAL);
};

/**
 * @brief 简单线程封装类
 * 
 * 提供轻量级的线程创建接口，封装了_beginthreadex和_beginthread函数，
 * 简化了线程创建过程，同时保存线程句柄和线程ID以便后续管理。
 */
class SimpleThread
{
public:
	/** 线程句柄，用于线程管理和同步操作 */
	HANDLE hThread;
	/** 线程ID，用于标识线程 */
	unsigned int  threadId;

	/**
	 * @brief 构造函数
	 * 
	 * 初始化线程句柄和线程ID为NULL，表示尚未创建线程。
	 */
	SimpleThread()
	{
		this->hThread = NULL;
		this->threadId = NULL;
	};

	/**
	 * @brief 创建线程（_beginthreadex版本）
	 * 
	 * 使用_beginthreadex创建线程，支持返回码和更完整的线程控制。
	 * 适用于需要获取线程退出码或进行精细控制的场景。
	 * 
	 * @param Func 线程入口函数，签名为unsigned WINAPI Func(LPVOID p)
	 * @param p 传递给线程函数的参数
	 * @return HANDLE 创建的线程句柄，失败返回NULL
	 * @note 创建的线程需要调用CloseHandle关闭句柄
	 */
	HANDLE StartThreadEx(unsigned WINAPI Func(LPVOID p), LPVOID p)
	{
		this->hThread = (HANDLE)_beginthreadex(NULL, 0, Func, p, 0, &(this->threadId));
		return this->hThread;
	};

	/**
	 * @brief 创建线程（_beginthread版本）
	 * 
	 * 使用_beginthread创建线程，接口更简单但不支持返回码。
	 * 适用于不需要获取线程退出码的简单场景。
	 * 
	 * @param Func 线程入口函数，签名为void(*Func)(void*)
	 * @param p 传递给线程函数的参数
	 * @return HANDLE 创建的线程句柄，失败返回NULL
	 * @note 创建的线程需要调用CloseHandle关闭句柄
	 */
	HANDLE StartThread(void(*Func)(void*), LPVOID p)
	{
		this->hThread = (HANDLE)_beginthread(Func, 0, p);
		return this->hThread;
	};

};
