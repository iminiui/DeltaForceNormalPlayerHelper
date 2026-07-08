// -----------------------------------------------------------------------------
// 文件：global.h
// 功能：全局头文件，声明项目中所有模块共用的工具函数和类型定义
// 说明：该文件包含了整个项目的公共依赖和函数声明，所有源文件都应包含此头文件
// -----------------------------------------------------------------------------

#pragma once
#pragma warning(disable:4996)

// Windows API 头文件
#include "windows.h"
// 标准输入输出流
#include "iostream"
// EasyX 图形库（用于UI界面绘制）
#include "graphics.h"
// 进程和线程相关API
#include "process.h"
// 标准字符串库
#include "string"

/**
 * @brief 弹出消息框（const char*版本）
 * 
 * 显示一个模态消息框，用于向用户展示提示信息或错误信息。
 * 该函数是对MessageBoxA的封装，提供统一的消息框标题。
 * 
 * @param str 要显示的消息内容
 */
void MBX(const char* str);

/**
 * @brief 弹出消息框（std::string版本）
 * 
 * 显示一个模态消息框，用于向用户展示提示信息或错误信息。
 * 该函数是对MessageBoxA的封装，提供统一的消息框标题。
 * 
 * @param str 要显示的消息内容（std::string类型）
 */
void MBX(std::string str);

/**
 * @brief 透明图片绘制函数
 * 
 * 将源图片以透明方式绘制到目标图片上，支持Alpha通道混合。
 * 该函数实现了逐像素的Alpha混合算法，确保图片边缘平滑过渡。
 * 
 * @param dstimg 目标图片指针，NULL表示绘制到当前绘图窗口
 * @param x 目标位置的X坐标
 * @param y 目标位置的Y坐标
 * @param srcimg 源图片指针，包含要绘制的透明图片
 */
void transparentimage(IMAGE* dstimg, int x, int y, IMAGE* srcimg);

/**
 * @brief 将char*字符串转换为std::wstring（宽字符字符串）
 * 
 * 将ANSI编码的char*字符串转换为Unicode编码的std::wstring字符串。
 * 使用std::wstring自动管理内存，无需调用者手动释放。
 * 
 * @param old 原始的ANSI字符串指针
 * @return std::wstring 转换后的Unicode字符串
 */
std::wstring pCharToLPWSTR(char* old);

/**
 * @brief 模拟按键按下和释放
 * 
 * 使用keybd_event API模拟指定按键的按下和释放操作，实现一次完整的按键触发。
 * 该函数用于游戏中的按键模拟，如快速近战、切枪等操作。
 * 
 * @param Key 要模拟的虚拟键码（如VK_OEM_3表示~键，'W'表示W键）
 */
void PressKey(UINT Key);
