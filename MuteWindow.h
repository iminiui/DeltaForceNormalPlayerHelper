// -----------------------------------------------------------------------------
// 文件：MuteWindow.h
// 功能：音频会话管理模块接口定义
// 说明：使用Windows Core Audio API实现进程级别的音量控制，
//       主要用于"切屏静音"功能，当游戏窗口不在前台时自动静音
// -----------------------------------------------------------------------------

#pragma once

// Windows Core Audio API 头文件
#include <mmdeviceapi.h>       // 音频设备枚举接口
#include <endpointvolume.h>    // 端点音量接口
#include <audioclient.h>       // 音频客户端接口
#include <audiopolicy.h>       // 音频策略接口

/**
 * @brief 单进程音量控制类
 * 
 * 该类封装了Windows Core Audio API，实现对指定进程的音频会话进行静音/取消静音操作。
 * 通过枚举系统中所有音频会话，找到对应进程ID的会话，并获取其音量控制接口。
 * 
 * 使用流程：
 * 1. 创建SingleVolume对象（自动初始化资源）
 * 2. 调用GetControl(PID)获取指定进程的音频会话控制接口
 * 3. 使用IsMuted/SetMute/UnMute操作音频会话
 * 4. 对象销毁时自动清理资源
 */
class SingleVolume
{
private:
    /** 初始化状态标志，标记COM环境是否已初始化 */
    bool Initialized = false;
    /** 是否保留原始音量标志，为false时使用音量0替代静音 */
    bool KeepOriginVolume = true;

    /** COM操作返回码 */
    HRESULT hr = S_OK;
    /** 默认音频设备接口 */
    IMMDevice* pDevice = nullptr;
    /** 音频设备枚举器接口 */
    IMMDeviceEnumerator* m_pEnumerator = nullptr;
    /** 音频会话枚举器接口 */
    IAudioSessionEnumerator* pSessionEnum = nullptr;
    /** 音频会话管理器接口 */
    IAudioSessionManager2* pASManager = nullptr;
    /** 音频会话控制接口 */
    IAudioSessionControl* pASControl = nullptr;
    /** 音频会话控制扩展接口（支持获取进程ID） */
    IAudioSessionControl2* pASControl2 = nullptr;

    /** 属性存储接口 */
    IPropertyStore* pPropertyStore = nullptr;
    /** 属性值结构体 */
    PROPVARIANT pv;

    /** ISimpleAudioVolume接口的GUID */
    const IID IID_ISimpleAudioVolume = __uuidof(ISimpleAudioVolume);
    /** IAudioSessionControl2接口的GUID */
    const IID IID_IAudioSessionControl2 = __uuidof(IAudioSessionControl2);
    /** 简单音频音量控制接口 */
    ISimpleAudioVolume* pSimplevol = nullptr;

    /** 设备名称缓冲区 */
    wchar_t* wszDeviceName;

public:

    /**
     * @brief 初始化COM环境和资源
     * 
     * @return bool 初始化成功返回true，失败返回false
     */
    bool Init();

    /**
     * @brief 构造函数
     * 
     * 自动调用Init()初始化COM环境和资源
     */
    SingleVolume();

    /**
     * @brief 析构函数
     * 
     * 释放所有已分配的COM接口和内存资源
     */
    ~SingleVolume();

    /**
     * @brief 获取指定进程的音频会话控制接口
     * 
     * 通过枚举系统中所有音频会话，找到与指定进程ID匹配的会话，
     * 并返回其ISimpleAudioVolume接口指针，用于后续的音量控制操作。
     * 
     * @param PID 目标进程的进程ID
     * @return ISimpleAudioVolume* 音频会话的音量控制接口指针，失败返回nullptr
     * @note 返回的接口需要调用者在使用完毕后手动Release()
     */
    ISimpleAudioVolume* GetControl(ULONG PID);

    /**
     * @brief 检查音频会话是否处于静音状态
     * 
     * @param ISAV 音频会话的ISimpleAudioVolume接口指针
     * @return bool 静音状态返回true，非静音返回false
     */
    bool IsMuted(ISimpleAudioVolume* ISAV);

    /**
     * @brief 设置音频会话为静音状态
     * 
     * @param ISAV 音频会话的ISimpleAudioVolume接口指针
     * @return bool 操作成功返回true，失败返回false
     */
    bool SetMute(ISimpleAudioVolume* ISAV);

    /**
     * @brief 取消音频会话的静音状态
     * 
     * @param ISAV 音频会话的ISimpleAudioVolume接口指针
     * @return bool 操作成功返回true，失败返回false
     */
    bool UnMute(ISimpleAudioVolume* ISAV);
};
