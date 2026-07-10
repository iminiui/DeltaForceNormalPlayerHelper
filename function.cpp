// -----------------------------------------------------------------------------
// 文件：function.cpp
// 功能：通用工具函数实现
// 说明：包含项目中所有模块共用的工具函数，包括消息框、透明图片绘制、
//       字符串转换和按键模拟等功能
// -----------------------------------------------------------------------------

#include "global.h"

/**
 * @brief 弹出消息框（const char*版本）
 * 
 * 显示一个标准消息框，用于向用户展示提示信息。
 * 该函数是对Windows API MessageBoxA的封装，提供统一的中文标题。
 * 
 * @param str 要显示的消息内容（ANSI编码）
 */
void MBX(const char* str)
{
    MessageBoxA(NULL, str, "提示", MB_OK);
};

/**
 * @brief 弹出消息框（std::string版本）
 * 
 * 显示一个标准消息框，用于向用户展示提示信息。
 * 该函数是对Windows API MessageBoxA的封装，提供统一的中文标题。
 * 
 * @param str 要显示的消息内容（std::string类型）
 */
void MBX(std::string str)
{
    MessageBoxA(NULL, str.c_str(), "提示", MB_OK);
};

/**
 * @brief 透明图片绘制函数
 * 
 * 将源图片以透明方式绘制到目标图片上，支持Alpha通道混合。
 * 该函数实现了逐像素的Alpha混合算法，确保图片边缘平滑过渡。
 * 
 * Alpha混合算法说明：
 * - 对于完全透明的像素（Alpha=0）：不绘制
 * - 对于完全不透明的像素（Alpha=255）：直接覆盖
 * - 对于半透明像素（0<Alpha<255）：按比例混合源像素和目标像素的RGB值
 * 
 * @param dstimg 目标图片指针，NULL表示绘制到当前绘图窗口
 * @param x 目标位置的X坐标
 * @param y 目标位置的Y坐标
 * @param srcimg 源图片指针，包含要绘制的透明图片
 * @note 该函数使用EasyX图形库的GetImageBuffer获取图像缓冲区
 * @warning 必须确保dstimg和srcimg都已正确初始化
 */
void transparentimage(IMAGE* dstimg, int x, int y, IMAGE* srcimg)
{
    // 步骤1：获取图像缓冲区指针
    DWORD* dst = GetImageBuffer(dstimg);  // 目标图像缓冲区
    DWORD* src = GetImageBuffer(srcimg);  // 源图像缓冲区
    
    // 获取源图像尺寸
    int src_width = srcimg->getwidth();
    int src_height = srcimg->getheight();
    
    // 获取目标图像尺寸（如果dstimg为NULL，使用当前窗口尺寸）
    int dst_width = (dstimg == NULL ? getwidth() : dstimg->getwidth());
    int dst_height = (dstimg == NULL ? getheight() : dstimg->getheight());

    // 步骤2：计算实际绘制区域（处理边界溢出情况）
    int iwidth = (x + src_width > dst_width) ? dst_width - x : src_width;  // 处理右边界
    int iheight = (y + src_height > dst_height) ? dst_height - y : src_height;  // 处理下边界
    
    // 处理左边界（源图像起始偏移）
    if (x < 0) { 
        src += -x;       // 源指针偏移
        iwidth -= -x;    // 宽度减少
        x = 0;           // 目标位置修正为0
    }
    
    // 处理上边界（源图像起始偏移）
    if (y < 0) { 
        src += src_width * -y;  // 源指针偏移（按行计算）
        iheight -= -y;          // 高度减少
        y = 0;                  // 目标位置修正为0
    }

    // 步骤3：定位目标图像起始位置
    dst += dst_width * y + x;

    // 步骤4：实现逐像素Alpha混合绘制
    for (int iy = 0; iy < iheight; ++iy)
    {
        for (int i = 0; i < iwidth; ++i)
        {
            // 获取源像素的Alpha值（高8位）
            int sa = ((src[i] & 0xff000000) >> 24);
            
            // 如果Alpha不为0（非完全透明）
            if (sa != 0)
                // 如果Alpha为255（完全不透明），直接覆盖
                if (sa == 255)
                    dst[i] = src[i];
                // 否则进行Alpha混合计算
                else
                    // 计算公式：目标RGB = 源RGB + 目标RGB * (255 - Alpha) / 255
                    // 分别计算R、G、B三个通道的值
                    dst[i] = ((((src[i] & 0xff0000) >> 16) + ((dst[i] & 0xff0000) >> 16) * (255 - sa) / 255) << 16) 
                           | ((((src[i] & 0xff00) >> 8) + ((dst[i] & 0xff00) >> 8) * (255 - sa) / 255) << 8) 
                           | ((src[i] & 0xff) + (dst[i] & 0xff) * (255 - sa) / 255);
        }
        // 移动到下一行
        dst += dst_width;
        src += src_width;
    }
}

/**
 * @brief 将char*字符串转换为std::wstring（宽字符字符串）
 * 
 * 将ANSI编码的char*字符串转换为Unicode编码的std::wstring字符串。
 * 使用Windows API MultiByteToWideChar进行转换，支持中文等多字节字符。
 * 使用std::wstring自动管理内存，无需调用者手动释放。
 * 
 * @param old 原始的ANSI字符串指针
 * @return std::wstring 转换后的Unicode字符串
 * @note 转换时使用CP_ACP编码页（系统默认ANSI编码）
 */
std::wstring pCharToLPWSTR(char* old)
{
    int num = MultiByteToWideChar(CP_ACP, 0, old, -1, NULL, 0);
    if (num == 0)
        return std::wstring();
    
    std::wstring result(num, L'\0');
    MultiByteToWideChar(CP_ACP, 0, old, -1, &result[0], num);
    return result;
};

/**
 * @brief 模拟按键按下和释放
 * 
 * 使用keybd_event API模拟指定按键的按下和释放操作，实现一次完整的按键触发。
 * 该函数用于游戏中的按键模拟，如快速近战、切枪等操作。
 * 
 * 按键模拟流程：
 * 1. 发送按键按下事件（keybd_event KeyDown）
 * 2. 等待1毫秒（确保游戏能检测到按键状态变化）
 * 3. 发送按键释放事件（keybd_event KeyUp）
 * 
 * @param Key 要模拟的虚拟键码（如VK_OEM_3表示~键，'W'表示W键）
 * @note 1毫秒的等待时间是必要的，确保按键状态变化能被游戏正确识别
 * @warning 频繁调用可能影响系统性能，建议控制调用频率
 */
void PressKey(UINT Key) {
    // 发送按键按下事件
    keybd_event(Key, 0, 0, 0);
    
    // 等待1毫秒（确保按键状态变化能被游戏检测到）
    Sleep(1);
    
    // 发送按键释放事件（KEYEVENTF_KEYUP = 2）
    keybd_event(Key, 0, 2, 0);
}
