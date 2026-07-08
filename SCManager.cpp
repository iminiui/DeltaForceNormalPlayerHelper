// -----------------------------------------------------------------------------
// 文件：SCManager.cpp
// 功能：Windows服务管理模块实现
// 说明：实现停止Windows系统服务的具体逻辑，用于关闭ACE反作弊服务以提升游戏帧率
// -----------------------------------------------------------------------------

#include "SCManager.h"

/**
 * @brief 停止指定的Windows系统服务
 * 
 * 实现步骤：
 * 1. 打开服务管理器（SCM）
 * 2. 打开目标服务（获取服务句柄）
 * 3. 向服务发送停止命令（SERVICE_CONTROL_STOP）
 * 4. 轮询等待服务状态变为STOPPED
 * 5. 关闭服务句柄和服务管理器句柄
 * 6. 返回操作结果
 * 
 * @param serviceName 要停止的服务名称（宽字符格式）
 * @return bool 成功停止返回true，失败返回false
 */
bool StopWindowsService(const wchar_t* serviceName) {
    // 步骤1：打开服务管理器
    // 参数：NULL（本地计算机）、NULL（默认数据库）、SC_MANAGER_ALL_ACCESS（完全访问权限）
    SC_HANDLE hSCManager = OpenSCManager(
        NULL, NULL, SC_MANAGER_ALL_ACCESS);

    // 检查服务管理器是否打开成功
    if (hSCManager == NULL) {
        MessageBoxA(NULL, "打开服务管理器失败", "错误", MB_OK | MB_SYSTEMMODAL);
        return false;
    }

    // 步骤2：打开目标服务
    // 参数：服务管理器句柄、服务名称、访问权限（停止+查询状态）
    SC_HANDLE hService = OpenService(
        hSCManager, serviceName, SERVICE_STOP | SERVICE_QUERY_STATUS);

    // 检查服务是否打开成功
    if (hService == NULL) {
        MessageBoxA(NULL, "打开服务失败", "错误", MB_OK | MB_SYSTEMMODAL);
        CloseServiceHandle(hSCManager);  // 清理已打开的服务管理器句柄
        return false;
    }

    // 步骤3：发送停止命令并获取服务状态
    SERVICE_STATUS_PROCESS ssStatus;  // 服务状态结构
    DWORD dwBytesNeeded;              // 输出参数，返回所需字节数

    // 向服务发送停止控制命令
    if (!ControlService(
        hService, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&ssStatus)) {

        MessageBoxA(NULL, "停止服务失败", "错误", MB_OK | MB_SYSTEMMODAL);
        CloseServiceHandle(hService);   // 清理服务句柄
        CloseServiceHandle(hSCManager); // 清理服务管理器句柄
        return false;
    }

    // 步骤4：轮询等待服务完全停止
    // 服务停止可能需要一定时间，需要循环查询状态
    while (ssStatus.dwCurrentState != SERVICE_STOPPED) {
        // 等待服务建议的等待时间（dwWaitHint）
        Sleep(ssStatus.dwWaitHint);

        // 查询服务当前状态
        if (!QueryServiceStatusEx(
            hService, SC_STATUS_PROCESS_INFO,
            (LPBYTE)&ssStatus, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded)) {

            std::cerr << "\n查询服务状态失败，错误码：" << GetLastError() << std::endl;
            break;  // 查询失败，退出等待循环
        }

        // 再次检查状态是否已停止
        if (ssStatus.dwCurrentState == SERVICE_STOPPED)
            break;
    }

    // 步骤5：清理资源
    CloseServiceHandle(hService);   // 关闭服务句柄
    CloseServiceHandle(hSCManager); // 关闭服务管理器句柄

    // 步骤6：返回结果
    return ssStatus.dwCurrentState == SERVICE_STOPPED;
}
