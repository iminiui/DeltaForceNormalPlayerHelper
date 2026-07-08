// -----------------------------------------------------------------------------
// 文件：MuteWindow.cpp
// 功能：音频会话管理模块实现
// 说明：实现对指定进程的音频会话进行静音/取消静音操作，
//       支持"切屏静音"功能的核心逻辑
// -----------------------------------------------------------------------------

#include "MuteWindow.h"

/**
 * @brief 初始化资源
 * 
 * 初始化步骤：
 * 1. 分配设备名称缓冲区并清零
 * 2. 初始化PROPVARIANT结构体
 * 
 * @return bool 初始化成功返回true，失败返回false
 */
bool SingleVolume::Init()
{
    wszDeviceName = new wchar_t[MAX_PATH]; 
    memset(wszDeviceName, 0, MAX_PATH * sizeof(wchar_t));
    
    PropVariantInit(&pv);

    return TRUE;
}

/**
 * @brief 构造函数
 * 
 * 自动调用Init()方法初始化资源
 */
SingleVolume::SingleVolume()
{
    Initialized = Init();
};

/**
 * @brief 析构函数
 * 
 * 资源释放步骤：
 * 1. 释放设备名称缓冲区（delete[]）
 * 2. 释放所有COM接口（Release()）
 * 3. 清理PROPVARIANT结构体
 * 
 * @note 必须确保所有COM接口都被正确释放，避免内存泄漏
 */
SingleVolume::~SingleVolume()
{
    if (wszDeviceName != NULL)
        delete[]wszDeviceName;

    if (m_pEnumerator != nullptr)
        m_pEnumerator->Release();

    if (pPropertyStore != nullptr)
        pPropertyStore->Release();

    PropVariantClear(&pv);
};

/**
 * @brief 获取指定进程的音频会话控制接口
 * 
 * 实现步骤：
 * 1. 初始化COM库（确保线程安全）
 * 2. 创建音频设备枚举器
 * 3. 获取默认音频输出设备
 * 4. 获取音频会话管理器
 * 5. 获取音频会话枚举器
 * 6. 枚举所有音频会话，找到匹配PID的会话
 * 7. 查询ISimpleAudioVolume接口
 * 8. 释放所有中间COM接口
 * 9. 返回ISimpleAudioVolume接口指针
 * 
 * @param PID 目标进程的进程ID
 * @return ISimpleAudioVolume* 音频会话的音量控制接口指针，失败返回nullptr
 * @note 返回的接口由调用者负责Release()
 * @warning 该函数内部会多次调用CoInitialize/CoUninitialize，可能影响其他COM操作
 */
ISimpleAudioVolume* SingleVolume::GetControl(ULONG PID)
{
    bool Find = false;
    ISimpleAudioVolume* pResult = nullptr;

    if (!SUCCEEDED(CoInitialize(NULL)))
    {
        MessageBoxA(NULL, "Audio初始化失败", "提示", MB_OK | MB_SYSTEMMODAL);
        return nullptr;
    }

    hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        nullptr,
        CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator),
        (void**)&m_pEnumerator
    );
    if (!SUCCEEDED(hr))
    {
        MessageBoxA(NULL, "创建音频设备枚举器失败", "提示", MB_OK | MB_SYSTEMMODAL);
        CoUninitialize();
        return nullptr;
    }

    hr = m_pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (!SUCCEEDED(hr))
    {
        MessageBoxA(NULL, "获取默认音频设备失败", "提示", MB_OK | MB_SYSTEMMODAL);
        m_pEnumerator->Release();
        m_pEnumerator = nullptr;
        CoUninitialize();
        return nullptr;
    }

    hr = pDevice->Activate(
        __uuidof(IAudioSessionManager2),
        CLSCTX_ALL,
        nullptr,
        (void**)&pASManager
    );
    if (!SUCCEEDED(hr))
    {
        MessageBoxA(NULL, "激活音频会话管理器失败", "提示", MB_OK | MB_SYSTEMMODAL);
        pDevice->Release();
        pDevice = nullptr;
        m_pEnumerator->Release();
        m_pEnumerator = nullptr;
        CoUninitialize();
        return nullptr;
    }

    hr = pASManager->GetSessionEnumerator(&pSessionEnum);
    if (!SUCCEEDED(hr))
    {
        MessageBoxA(NULL, "获取音频会话枚举器失败", "提示", MB_OK | MB_SYSTEMMODAL);
        pASManager->Release();
        pASManager = nullptr;
        pDevice->Release();
        pDevice = nullptr;
        m_pEnumerator->Release();
        m_pEnumerator = nullptr;
        CoUninitialize();
        return nullptr;
    }

    int num = 0;
    hr = pSessionEnum->GetCount(&num);
    if (!SUCCEEDED(hr))
    {
        MessageBoxA(NULL, "获取会话数量失败", "提示", MB_OK | MB_SYSTEMMODAL);
        pSessionEnum->Release();
        pSessionEnum = nullptr;
        pASManager->Release();
        pASManager = nullptr;
        pDevice->Release();
        pDevice = nullptr;
        m_pEnumerator->Release();
        m_pEnumerator = nullptr;
        CoUninitialize();
        return nullptr;
    }

    for (int i = 0; i < num && !Find; i++)
    {
        hr = pSessionEnum->GetSession(i, &pASControl);
        if (SUCCEEDED(hr))
        {
            hr = pASControl->QueryInterface(
                __uuidof(IAudioSessionControl2),
                (void**)&pASControl2
            );

            if (SUCCEEDED(hr))
            {
                DWORD processId = 0;
                hr = pASControl2->GetProcessId(&processId);
                if (SUCCEEDED(hr) && processId == PID)
                {
                    hr = pASControl2->QueryInterface(
                        IID_ISimpleAudioVolume,
                        (void**)&pResult
                    );
                    if (SUCCEEDED(hr))
                    {
                        Find = true;
                    }
                }
                pASControl2->Release();
                pASControl2 = nullptr;
            }
            pASControl->Release();
            pASControl = nullptr;
        }
    }

    if (pSessionEnum != nullptr)
    {
        pSessionEnum->Release();
        pSessionEnum = nullptr;
    }
    
    if (pASManager != nullptr)
    {
        pASManager->Release();
        pASManager = nullptr;
    }
    
    if (pDevice != nullptr)
    {
        pDevice->Release();
        pDevice = nullptr;
    }
    
    if (m_pEnumerator != nullptr)
    {
        m_pEnumerator->Release();
        m_pEnumerator = nullptr;
    }
    
    CoUninitialize();
    
    return pResult;
}

/**
 * @brief 检查音频会话是否处于静音状态
 * 
 * @param ISAV 音频会话的ISimpleAudioVolume接口指针
 * @return bool 静音状态返回true，非静音返回false
 * @note 如果ISAV为nullptr，默认返回false
 */
bool SingleVolume::IsMuted(ISimpleAudioVolume* ISAV)
{
    BOOL state = false;
    if (ISAV != nullptr)
        ISAV->GetMute(&state);
    return state;
};

/**
 * @brief 设置音频会话为静音状态
 * 
 * @param ISAV 音频会话的ISimpleAudioVolume接口指针
 * @return bool 操作成功返回true，失败返回false
 * @note 如果KeepOriginVolume为false，会将音量设置为0而非使用静音标志
 */
bool SingleVolume::SetMute(ISimpleAudioVolume* ISAV)
{
    if (ISAV != nullptr)
    {
        // 如果不保留原始音量，直接将音量设为0
        if (KeepOriginVolume == false)
            ISAV->SetMasterVolume(0.0f, NULL);
        
        // 设置静音标志
        ISAV->SetMute(true, NULL);
        return true;
    }
    return false;
}

/**
 * @brief 取消音频会话的静音状态
 * 
 * @param ISAV 音频会话的ISimpleAudioVolume接口指针
 * @return bool 操作成功返回true，失败返回false
 * @note 如果KeepOriginVolume为false，会将音量恢复为1.0（最大音量）
 */
bool SingleVolume::UnMute(ISimpleAudioVolume* ISAV)
{
    if (ISAV != nullptr)
    {
        // 如果不保留原始音量，直接将音量设为最大
        if (KeepOriginVolume == false)
            ISAV->SetMasterVolume(1.0f, NULL);
        
        // 取消静音标志
        ISAV->SetMute(false, NULL);
        return true;
    }
    return false;
}
