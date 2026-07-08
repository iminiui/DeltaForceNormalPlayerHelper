// -----------------------------------------------------------------------------
// 文件：SCManager.h
// 功能：Windows服务管理模块
// 说明：提供停止Windows系统服务的功能，主要用于关闭ACE反作弊服务
// -----------------------------------------------------------------------------

#pragma once

// Windows API 基础头文件
#include <windows.h>
// Windows服务控制管理器API
#include <winsvc.h>
// 标准输入输出流（用于错误输出）
#include <iostream>
// 链接advapi32库，提供服务管理相关函数
#pragma comment(lib, "advapi32.lib")

/**
 * @brief 停止指定的Windows系统服务
 * 
 * 通过Windows服务控制管理器（SCM）API停止指定名称的系统服务。
 * 该函数实现了完整的服务停止流程：打开服务管理器→打开目标服务→
 * 发送停止命令→等待服务完全停止→清理资源。
 * 
 * @param serviceName 要停止的服务名称（宽字符格式）
 * @return bool 成功停止返回true，失败返回false
 * @note 需要管理员权限才能停止某些系统服务（如ACE反作弊服务）
 * @warning 停止系统服务可能影响系统稳定性，仅应停止确认安全的服务
 */
bool StopWindowsService(const wchar_t* serviceName);
