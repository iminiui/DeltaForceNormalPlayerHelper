// -----------------------------------------------------------------------------
// 文件：DeltaForceNormalPlayerHelper.cpp
// 功能：项目主程序入口，包含UI界面、线程管理、六大功能实现
// 说明：该文件是项目的核心文件，负责：
//       1. 程序初始化和单例检测
//       2. GUI界面绘制和用户交互处理
//       3. 游戏状态检测和功能线程生命周期管理
//       4. 六大游戏辅助功能的具体实现
// -----------------------------------------------------------------------------

#include "global.h"
#include "SimpleThread.h"
#include "SCManager.h"
#include "MuteWindow.h"
#include "tlhelp32.h"

#pragma region 全局变量

/** 主窗口句柄 */
HWND hwnd;

/** 当前页面索引（0-4） */
int Page = 0;
/** 上一次页面索引，用于检测页面切换 */
int Page_last = 0;
/** 总页面数 */
int Page_count = 5;
/** 六大功能开关状态数组（0-5对应各功能） */
bool FuncSw[6] = { 0,0,0,0,0,0 };
/** UI绘制状态数组（0-5对应功能开关，6对应主武器模式） */
bool Drawed[7] = { 1,1,1,1,1,1,1 };
/** 游戏运行状态（true表示游戏已启动） */
bool GameState = false;
/** 上一次游戏状态，用于检测状态变化 */
bool GameState_last = false;
/** 六大功能线程句柄数组 */
HANDLE Threads[6] = { NULL };
/** 互斥锁句柄（用于单例检测） */
HANDLE mutex = NULL;
/** 取消刀法动画时长（毫秒），默认165ms */
int t_knife = 165;
/** 取消刀法武器模式（true=主武器，false=手枪） */
bool b_MainWeapon = true;
/** 威慑爆闪时长（秒），默认5秒 */
int t_flash = 5;

/** 图片资源：开关关闭状态 */
IMAGE i_switch_off;
/** 图片资源：开关打开状态 */
IMAGE i_switch_on;
/** 图片资源：功能条背景 */
IMAGE i_bar_func;
/** 图片资源：页面条背景 */
IMAGE i_bar_page;
/** 图片资源：输入框背景 */
IMAGE i_bar_input;
/** 图片资源：主配置图标 */
IMAGE i_icon_mainconfig;
/** 图片资源：取消刀法图标 */
IMAGE i_icon_autoknife;
/** 图片资源：爆闪图标 */
IMAGE i_icon_flash;
/** 图片资源：保存图标 */
IMAGE i_icon_save;
/** 图片资源：信息图标 */
IMAGE i_icon_info;
/** 图片资源：更多功能图标 */
IMAGE i_icon_work;
/** 图片资源：改键示例图标 */
IMAGE i_icon_sample;
/** 图片资源：游戏在线状态图标 */
IMAGE i_icon_online;
/** 图片资源：游戏离线状态图标 */
IMAGE i_icon_offline;
/** 图片资源：改键图示 */
IMAGE i_pic_keysetting;
/** 图片资源：游戏设置图示 */
IMAGE i_pic_gamesetting;

/**
 * @brief 配置数据结构
 * 
 * 用于保存和读取用户配置，包括功能开关状态和参数设置
 */
typedef struct Data {
    bool SW[7];   // 6个功能开关 + 主武器模式
    int tk;       // 取消刀动画时长（ms）
    int tf;       // 爆闪时长（s）
}DATA;

/** 功能开关位置数组（0-5为主页面功能开关，6为取消刀武器模式开关） */
RECT SwitchsPos[7] = {
    {860,100,920,130},  // 功能0：自动关闭ACE
    {860,200,920,230},  // 功能1：切屏静音
    {860,300,920,330},  // 功能2：取消刀法
    {860,400,920,430},  // 功能3：自动屏息
    {860,500,920,530},  // 功能4：无声滑步
    {860,600,920,630},  // 功能5：威慑爆闪
    {860,100,920,130}   // 取消刀武器模式开关（页面1）
};

/** 页面切换按钮位置数组（0-4对应5个页面） */
RECT PagePos[5] = {
    {20,80,220,130},    // 页面0：功能总开关
    {20,140,220,190},   // 页面1：取消刀法设置
    {20,200,220,250},   // 页面2：威慑爆闪设置
    {20,260,220,310},   // 页面3：改键图示
    {20,320,220,370}    // 页面4：更多功能
};

/** 输入框位置数组 */
RECT InputPos[1] = {
    {280,200,930,250}   // 参数输入框位置
};

/** 底部按钮位置数组 */
RECT ButtonPos[2] = {
    {40,640,70,670},    // 保存按钮
    {135,638,165,668}   // 更多按钮
};

/** ACE反作弊服务名称 */
const wchar_t* serviceName = L"AntiCheatExpert Service";
/** 游戏窗口类名 */
const char* WindowClass = "UnrealWindow";
/** 游戏启动器窗口类名 */
const char* LauncherClass = "TWINCONTROL";
/** 游戏窗口标题 */
const char* WindowName = "三角洲行动  ";
/** 游戏启动器窗口标题 */
const char* LauncherName = "三角洲行动";

/** 游戏进程ID */
DWORD g_pid = 0;
/** 游戏窗口句柄 */
HWND g_hwnd = NULL;

#pragma endregion

// 函数声明
/** @brief 保存用户配置到文件 */
void SaveConfig();
/** @brief 从文件读取用户配置 */
void ReadConfig();
/** @brief 检测指定按键是否按下 */
bool IsPressed(int Key);
/** @brief 检测点是否在矩形区域内 */
bool IsPointInRect(POINT& pos, RECT& rec);
/** @brief 重绘整个UI界面 */
void ReDraw();
/** @brief 绘制开关控件 */
void DrawSwitch(RECT& rec, IMAGE& img);
/** @brief UI交互监听线程函数 */
void MainUIListen(void* args);
/** @brief 根据进程名称查找进程ID */
DWORD FindProcessId(const std::wstring& processName);
/** @brief 终止指定进程 */
bool TerminateTargetProcess(DWORD processId);

// 功能线程函数声明
/** @brief 切屏静音功能线程：游戏窗口不在前台时自动静音 */
unsigned int __stdcall MuteWindow(void* args);
/** @brief 关闭ACE反作弊服务线程：停止ACE服务提升帧率 */
unsigned int __stdcall CloseAntiCheat(void* args);
/** @brief 取消刀法功能线程：快速近战切枪取消动画 */
unsigned int __stdcall AutoKnife(void* args);
/** @brief 开火自动屏息线程：开镜射击时自动屏息 */
unsigned int __stdcall StopBreatheWhileFire(void* args);
/** @brief 无声滑步线程：按住侧键快速点按W键实现滑步 */
unsigned int __stdcall Slide(void* args);
/** @brief 威慑性无限爆闪线程：按住中键快速开关手电 */
unsigned int __stdcall UnlimitedFlash(void* args);

/**
 * @brief 主程序入口函数
 * 
 * 程序执行流程：
 * 1. 检测程序是否已运行（单例模式）
 * 2. 读取配置文件
 * 3. 初始化图形界面窗口
 * 4. 设置窗口标题和图标
 * 5. 创建UI交互监听线程
 * 6. 加载所有图片资源
 * 7. 检测游戏状态
 * 8. 进入主循环：
 *    - 检测游戏状态变化
 *    - 更新功能开关UI显示
 *    - 管理功能线程生命周期
 * 9. 清理资源并退出
 * 
 * @return int 程序退出码
 */
int main()
{
    // 步骤1：检测程序是否已运行（单例模式）
    if (CheckSelfExists(mutex, "DeltaForceNormalPlayerHelper"))
    {
        return 0;  // 已存在实例，直接退出
    }

    // 步骤2：创建线程管理器
    SimpleThread ST;

    // 步骤3：读取用户配置
    ReadConfig();

    // 步骤4：初始化图形界面（960x720像素）
    hwnd = initgraph(960, 720);
    // 设置窗口标题
    SetWindowTextA(hwnd, "我真是八宝粥绿玩");
    // 加载并设置窗口图标
    HICON hIcon = (HICON)LoadImage(NULL, L"PicRes\\O5-11.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

    // 步骤5：创建UI交互监听线程
    ST.StartThread(MainUIListen, (void*)&FuncSw);

    // 步骤6：加载所有图片资源
    loadimage(&i_switch_off, L"PicRes\\switch-off.png", 60, 30, true);
    loadimage(&i_switch_on, L"PicRes\\switch-on.png", 60, 30, true);
    loadimage(&i_bar_func, L"PicRes\\func-bar.png", 660, 80, true);
    loadimage(&i_bar_page, L"PicRes\\page-bar.png", 200, 50, true);
    loadimage(&i_bar_input, L"PicRes\\input-bar.png", 650, 50, true);
    loadimage(&i_icon_mainconfig, L"PicRes\\config.png", 20, 20, true);
    loadimage(&i_icon_autoknife, L"PicRes\\sword.png", 20, 20, true);
    loadimage(&i_icon_flash, L"PicRes\\flash.png", 20, 20, true);
    loadimage(&i_icon_work, L"PicRes\\Working.png", 20, 20, true);
    loadimage(&i_icon_sample, L"PicRes\\sample.png", 20, 20, true);
    loadimage(&i_icon_save, L"PicRes\\save.png", 30, 30, true);
    loadimage(&i_icon_info, L"PicRes\\info.png", 30, 30, true);
    loadimage(&i_icon_online, L"PicRes\\online.png", 30, 30, true);
    loadimage(&i_icon_offline, L"PicRes\\offline.png", 30, 30, true);
    loadimage(&i_pic_keysetting, L"PicRes\\keysetting.png");
    loadimage(&i_pic_gamesetting, L"PicRes\\gamesetting.png");

    // 步骤7：检测游戏状态（查找游戏窗口）
    g_hwnd = FindWindowA(WindowClass, WindowName);
    if (g_hwnd != NULL)
        GameState = true;

    // 步骤8：首次绘制UI界面
    ReDraw();

    // 步骤9：主循环（程序核心逻辑）
    while (1) {
        // 9.1 检测游戏状态变化
        // 通过查找游戏窗口句柄判断游戏是否正在运行
        HWND new_hwnd = FindWindowA(WindowClass, WindowName);
        bool new_GameState = (new_hwnd != NULL);
        
        // 如果游戏状态发生变化（启动或退出）
        if (new_GameState != GameState) {
            GameState = new_GameState;        // 更新游戏状态标志
            g_hwnd = new_hwnd;                // 更新游戏窗口句柄
            ReDraw();                         // 重新绘制界面（更新游戏状态显示）
            GameState_last = GameState;       // 记录上一次状态用于比较
        }

        // 9.2 更新功能开关UI显示（主页面，Page==0）
        // 仅当功能开关状态变化且当前在主页面时，才重新绘制开关
        for (int i = 0; i < 6; i++) {
            if (FuncSw[i] != Drawed[i] && Page == 0) {
                DrawSwitch(SwitchsPos[i], FuncSw[i] ? i_switch_on : i_switch_off);
                Drawed[i] = FuncSw[i];  // 更新绘制状态，避免重复绘制
            }
        }

        // 9.3 更新取消刀武器模式开关显示（页面1，Page==1）
        if (b_MainWeapon != Drawed[6] && Page == 1) {
            DrawSwitch(SwitchsPos[6], b_MainWeapon ? i_switch_on : i_switch_off);
            Drawed[6] = b_MainWeapon;
        }

        // 9.4 检测页面切换
        // 页面变化时需要重绘整个界面，因为不同页面布局完全不同
        if (Page != Page_last) {
            ReDraw();      // 重新绘制整个界面
            Page_last = Page;
        }

        // 9.5 管理功能线程生命周期（仅当游戏运行时）
        // 游戏未运行时不创建功能线程，避免无效资源占用
        if (GameState) {
            for (int i = 0; i < 6; i++) {
                DWORD exitCode = STILL_ACTIVE;  // 线程退出码，初始化为"仍活动"
                bool isThreadActive = false;    // 线程活动状态标志

                // 检查线程是否活动
                if (Threads[i] != NULL) {
                    if (GetExitCodeThread(Threads[i], &exitCode)) {
                        // 退出码为STILL_ACTIVE表示线程仍在运行
                        isThreadActive = (exitCode == STILL_ACTIVE);
                    }
                    else {
                        // 获取退出码失败，说明线程句柄已失效，清理资源
                        CloseHandle(Threads[i]);
                        Threads[i] = NULL;
                    }
                }

                // 功能关闭时：等待线程结束并清理资源
                if (!FuncSw[i] && Threads[i] != NULL) {
                    if (isThreadActive) {
                        // 等待线程结束（最多100ms，避免阻塞主循环）
                        if (WaitForSingleObject(Threads[i], 100) == WAIT_OBJECT_0) {
                            CloseHandle(Threads[i]);
                            Threads[i] = NULL;
                        }
                    }
                    else {
                        // 线程已结束，直接清理句柄
                        CloseHandle(Threads[i]);
                        Threads[i] = NULL;
                    }
                    continue;  // 跳过后续创建逻辑
                }

                // 功能开启时：创建新线程（如果线程不存在或已结束）
                if (FuncSw[i] && (Threads[i] == NULL || !isThreadActive)) {
                    // 清理旧的线程句柄（如果存在）
                    if (Threads[i] != NULL) {
                        CloseHandle(Threads[i]);
                        Threads[i] = NULL;
                    }
                    // 根据功能索引创建对应的线程
                    // i=0: CloseAntiCheat 自动关闭ACE反作弊
                    // i=1: MuteWindow    切屏静音
                    // i=2: AutoKnife     取消刀法
                    // i=3: StopBreatheWhileFire 开火自动屏息
                    // i=4: Slide         无声滑步
                    // i=5: UnlimitedFlash 威慑爆闪
                    switch (i) {
                    case 0: Threads[i] = ST.StartThreadEx(CloseAntiCheat, NULL); break;
                    case 1: Threads[i] = ST.StartThreadEx(MuteWindow, NULL); break;
                    case 2: Threads[i] = ST.StartThreadEx(AutoKnife, NULL); break;
                    case 3: Threads[i] = ST.StartThreadEx(StopBreatheWhileFire, NULL); break;
                    case 4: Threads[i] = ST.StartThreadEx(Slide, NULL); break;
                    case 5: Threads[i] = ST.StartThreadEx(UnlimitedFlash, NULL); break;
                    }
                }
            }
        }

        // 降低CPU占用（每10ms轮询一次，平衡响应速度和资源消耗）
        Sleep(10);
    }

    // 清理图形界面资源（理论上不会执行到这里，因为主循环是无限循环）
    closegraph();
    // 释放互斥锁
    CloseMutex(mutex);
    return 0;
}

/**
 * @brief 重绘整个UI界面
 * 
 * 根据当前页面索引绘制不同的界面内容，包括：
 * - 背景色和布局
 * - 页面标题和功能描述
 * - 功能开关状态
 * - 游戏状态显示
 * - 底部按钮和图标
 * 
 * 使用批量绘制（BeginBatchDraw/EndBatchDraw）提高绘制效率
 */
void ReDraw() {
    // 设置字体样式
    LOGFONT f;
    gettextstyle(&f);
    f.lfWeight = FW_BOLD;
    _tcscpy(f.lfFaceName, _T("宋体"));
    f.lfQuality = PROOF_QUALITY;
    f.lfHeight = 26;
    settextstyle(&f);

    // 开始批量绘制（提高效率）
    BeginBatchDraw();
    
    // 清空画布
    cleardevice();
    
    // 绘制左侧导航栏背景（深色）
    setfillcolor(RGB(17, 24, 39));
    solidrectangle(0, 0, 250, 720);
    
    // 绘制右侧内容区背景（深蓝色）
    setfillcolor(RGB(3, 7, 28));
    solidrectangle(251, 0, 960, 720);

    // 设置文字背景透明
    setbkmode(TRANSPARENT);

    // 绘制标题
    outtextxy(30, 20, L"八宝粥绿玩辅助");

    // 根据当前页面绘制不同内容
    if (Page == 0)  // 页面0：功能总开关
    {
        // 绘制页面选中状态条
        transparentimage(NULL, 20, 80, &i_bar_page);

        // 绘制六个功能条和开关
        for (int i = 0; i < 6; i++)
        {
            transparentimage(NULL, 280, 75 + 100 * i, &i_bar_func);
            transparentimage(NULL, SwitchsPos[i].left, SwitchsPos[i].top, &(FuncSw[i] ? i_switch_on : i_switch_off));
        }

        // 设置字体大小并绘制功能标题
        f.lfHeight = 26;
        settextstyle(&f);
        outtextxy(280, 30, L"功能总开关");

        // 绘制各功能名称
        f.lfHeight = 18;
        settextstyle(&f);
        outtextxy(320, 95, L"自动关闭ACE");
        outtextxy(320, 195, L"切屏静音");
        outtextxy(320, 295, L"取消刀法");
        outtextxy(320, 395, L"自动屏息");
        outtextxy(320, 495, L"无声滑步");
        outtextxy(320, 595, L"威慑爆闪");

        // 绘制各功能描述（灰色小字）
        f.lfHeight = 14;
        f.lfWeight = FW_NORMAL;
        settextcolor(RGB(92, 94, 102));
        settextstyle(&f);
        outtextxy(320, 120, L"游戏启动时自动关闭扫盘掉帧的ACE");
        outtextxy(320, 220, L"游戏窗口不在最前时将其静音，给你一段纯净的听觉体验");
        outtextxy(320, 320, L"刀人机一等一的好用，谨慎刀人，吃举报有封号可能");
        outtextxy(320, 420, L"开镜的时候开枪才有用，且效果与枪的据枪属性强相关");
        outtextxy(320, 520, L"旧版本伪装人机步还行，现在一般当做节省体力的移动手段");
        outtextxy(320, 620, L"爆闪大拉专克高手，菜鸡不会上当");
    }
    else if (Page == 1)  // 页面1：取消刀法设置
    {
        // 绘制功能条和武器模式开关
        transparentimage(NULL, 280, 75, &i_bar_func);
        transparentimage(NULL, SwitchsPos[6].left, SwitchsPos[6].top, &(b_MainWeapon ? i_switch_on : i_switch_off));
        
        // 绘制页面选中状态条和输入框背景
        transparentimage(NULL, 20, 140, &i_bar_page);
        transparentimage(NULL, 280, 200, &i_bar_input);

        // 绘制标题
        f.lfHeight = 26;
        settextstyle(&f);
        outtextxy(280, 30, L"取消刀动画时长");

        // 绘制武器模式选项
        f.lfHeight = 18;
        settextstyle(&f);
        outtextxy(320, 95, L"当局主武器不为手枪");
        
        // 绘制武器模式说明
        f.lfHeight = 14;
        f.lfWeight = FW_NORMAL;
        settextcolor(RGB(92, 94, 102));
        settextstyle(&f);
        outtextxy(320, 120, L"只带G18夺舍时也能用出取消刀了");

        // 绘制自定义时长输入区
        f.lfHeight = 14;
        f.lfWeight = FW_NORMAL;
        settextstyle(&f);
        settextcolor(WHITE);
        outtextxy(280, 170, L"设置自定义时长(ms):");
        
        char* buff = new char[10]; 
        memset(buff, 0, 10);
        itoa(t_knife, buff, 10);
        std::wstring show = pCharToLPWSTR(buff);
        f.lfHeight = 18;
        settextstyle(&f);
        outtextxy(300, 215, show.c_str());
        
        delete[] buff;
    }
    else if (Page == 2)  // 页面2：威慑爆闪设置
    {
        // 绘制页面选中状态条和输入框背景
        transparentimage(NULL, 20, 200, &i_bar_page);
        transparentimage(NULL, 280, 200, &i_bar_input);

        // 绘制标题
        f.lfHeight = 26;
        settextstyle(&f);
        outtextxy(280, 30, L"威慑爆闪时长");

        // 绘制自定义时长输入区
        f.lfHeight = 14;
        f.lfWeight = FW_NORMAL;
        settextstyle(&f);
        outtextxy(280, 170, L"设置自定义时长(s):");
        
        char* buff = new char[10]; 
        memset(buff, 0, 10);
        itoa(t_flash, buff, 10);
        std::wstring show = pCharToLPWSTR(buff);
        f.lfHeight = 18;
        settextstyle(&f);
        outtextxy(300, 215, show.c_str());
        
        delete[] buff;
    }
    else if (Page == 3)  // 页面3：改键图示
    {
        // 绘制页面选中状态条
        transparentimage(NULL, 20, 260, &i_bar_page);

        // 绘制标题
        f.lfHeight = 26;
        settextstyle(&f);
        outtextxy(280, 30, L"改键图示");

        // 绘制改键图示图片
        transparentimage(NULL, 280, 75, &i_pic_keysetting);
        transparentimage(NULL, 280, 585, &i_pic_gamesetting);

        // 设置字体大小
        f.lfHeight = 14;
        f.lfWeight = FW_NORMAL;
        settextstyle(&f);

        // 绘制改键说明文字
        outtextxy(320, 140, L"取消刀法");
        outtextxy(320, 160, L"游戏内设置 快速近战:");
        outtextxy(540, 160, L"功能激活键:");
        outtextxy(320, 195, L"无声滑步");
        outtextxy(320, 215, L"游戏内设置 奔跑/快游:");
        outtextxy(540, 215, L"功能激活键:");
        outtextxy(320, 250, L"自动屏息");
        outtextxy(320, 270, L"游戏内设置 屏息:");
        outtextxy(540, 270, L"功能激活: 开镜(鼠标右键)后射击(鼠标左键)");
    }
    else if (Page == 4)  // 页面4：更多功能
    {
        // 绘制页面选中状态条
        transparentimage(NULL, 20, 320, &i_bar_page);

        // 绘制标题
        f.lfHeight = 26;
        settextstyle(&f);
        outtextxy(280, 30, L"后续开发计划");

        // 绘制开发计划内容
        f.lfHeight = 14;
        f.lfWeight = FW_NORMAL;
        settextstyle(&f);
        outtextxy(280, 170, L"一键从裤裆里掏出子弹到口袋的功能");
        outtextxy(280, 200, L"自定义游戏内/功能键位：比如取消刀不再是游戏内绑~键，也不再是鼠标侧键激活，按你自己习惯的来");
        outtextxy(280, 230, L"检查版本更新的功能......");
        outtextxy(280, 290, L"但是谁又知道这游戏还能活多久，又还有多少人需要我这东西呢.....");
        outtextxy(280, 320, L"人心若是烧没了，修好一座破庙又有什么用呢");

        // 绘制作者信息
        f.lfHeight = 26;
        settextstyle(&f);
        outtextxy(280, 400, L"总之关注");
        outtextxy(280, 440, L"Github:O5-11 Bilbili UID:9427514");
        outtextxy(280, 480, L"谢谢喵~");
    }

    // 绘制左侧导航栏图标
    transparentimage(NULL, 40, 95, &i_icon_mainconfig);
    transparentimage(NULL, 40, 155, &i_icon_autoknife);
    transparentimage(NULL, 40, 215, &i_icon_flash);
    transparentimage(NULL, 40, 275, &i_icon_sample);
    transparentimage(NULL, 40, 335, &i_icon_work);

    // 绘制底部按钮图标
    transparentimage(NULL, 40, 640, &i_icon_save);
    transparentimage(NULL, 135, 638, &i_icon_info);

    // 绘制左侧导航栏文字
    f.lfHeight = 18;
    f.lfWeight = FW_NORMAL;
    settextstyle(&f);
    settextcolor(WHITE);
    outtextxy(100, 95, L"功能开关");
    outtextxy(100, 155, L"取消刀法");
    outtextxy(100, 215, L"威慑爆闪");
    outtextxy(100, 275, L"改键示例");
    outtextxy(100, 335, L"更多功能");

    // 绘制游戏状态
    if (GameState)
    {
        transparentimage(NULL, 40, 560, &i_icon_online);
        outtextxy(100, 565, L"游戏已开");
    }
    else
    {
        transparentimage(NULL, 40, 560, &i_icon_offline);
        outtextxy(100, 565, L"游戏未开");
    }

    // 结束批量绘制（刷新显示）
    EndBatchDraw();
}

/**
 * @brief 绘制开关控件
 * 
 * 在指定位置绘制开关的当前状态图片（开/关）
 * 
 * @param rec 开关位置矩形区域
 * @param img 开关状态图片（i_switch_on 或 i_switch_off）
 */
void DrawSwitch(RECT& rec, IMAGE& img)
{
    BeginBatchDraw();
    transparentimage(NULL, rec.left, rec.top, &img);
    EndBatchDraw();
};

/**
 * @brief UI交互监听线程函数
 * 
 * 监听用户鼠标点击事件，处理：
 * - 页面切换
 * - 功能开关切换
 * - 参数输入
 * - 保存配置和打开链接
 * 
 * 使用上升沿检测（仅在点击状态从松开变为按下时触发），避免重复触发
 * 
 * @param args 传入的参数（功能开关数组指针）
 */
void MainUIListen(void* args)
{
    // 获取功能开关数组指针（通过void*传递，需要强制类型转换）
    bool (*FuncUISw)[3] = (bool(*)[3])args;
    
    POINT pos;              // 鼠标位置（屏幕坐标转窗口坐标后）
    bool last_clicked = false;  // 上一次点击状态，用于上升沿检测

    // 无限循环监听用户交互
    while (1)
    {
        // 仅当辅助窗口为前台窗口时处理交互（避免干扰其他窗口操作）
        if (GetForegroundWindow() == hwnd)
        {
            // 获取鼠标位置并转换为窗口客户区坐标
            GetCursorPos(&pos);           // 获取屏幕坐标
            ScreenToClient(hwnd, &pos);   // 转换为窗口坐标
            
            // 获取当前鼠标左键状态（高16位为1表示按下）
            bool current_clicked = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

            // 上升沿检测：仅在点击状态从松开变为按下时触发一次
            // 避免按住鼠标时重复触发事件
            if (current_clicked && !last_clicked) {
                // 处理页面切换（点击左侧导航栏）
                for (int i = 0; i < Page_count; i++)
                {
                    if (IsPointInRect(pos, PagePos[i])) {
                        Page = i;  // 设置当前页面索引
                    }
                }

                // 处理底部按钮（保存和更多）
                for (int i = 0; i < 2; i++)
                {
                    if (IsPointInRect(pos, ButtonPos[i])) {
                        switch (i)
                        {
                        case 0:
                            SaveConfig();  // 保存当前配置到文件
                            break;
                        case 1:
                            // 打开作者B站主页（外部链接）
                            ShellExecuteA(NULL, "open", "https://space.bilibili.com/9427514", 0, 0, 0);
                            break;
                        }
                    }
                }

                // 根据当前页面处理不同的交互逻辑
                if (Page == 0)  // 页面0：功能总开关
                {
                    // 遍历6个功能开关，检测点击并切换状态
                    for (int i = 0; i < 6; i++)
                    {
                        if (IsPointInRect(pos, SwitchsPos[i])) {
                            (*FuncUISw)[i] = !(*FuncUISw)[i];  // 取反切换开关状态
                        }
                    }
                }
                else if (Page == 1)  // 页面1：取消刀法设置
                {
                    // 切换武器模式开关（主武器/手枪）
                    if (IsPointInRect(pos, SwitchsPos[6])) {
                        b_MainWeapon = !b_MainWeapon;
                    }
                    
                    // 点击输入框区域，弹出输入对话框
                    if (IsPointInRect(pos, InputPos[0])) {
                        wchar_t num[10];  // 输入缓冲区
                        // 弹出输入框，提示建议范围
                        InputBox(num, 10, L"建议时长 150ms 到 190ms，电锯大约在 400ms", L"取消刀动画时长");
                        int input = _wtoi(num);  // 转换为整数
                        
                        // 输入验证：确保数值有效且在合理范围内
                        if (input == 0)
                        {
                            t_knife = 165;  // 无效输入，使用默认值
                            MessageBoxA(NULL, "输入中有无法解析的字符，当前数值重置为165ms", "警告", MB_OK | MB_ICONERROR | MB_DEFAULT_DESKTOP_ONLY);
                        }
                        else if (input > 1000 || input < 0)
                        {
                            t_knife = 165;  // 超出范围，使用默认值
                            MessageBoxA(NULL, "输入数值超过阈值，当前数值重置为165ms", "警告", MB_OK | MB_ICONERROR | MB_DEFAULT_DESKTOP_ONLY);
                        }
                        else
                        {
                            t_knife = input;  // 有效输入，更新配置
                        }
                        ReDraw();  // 更新界面显示新的数值
                    }
                }
                else if (Page == 2)  // 页面2：威慑爆闪设置
                {
                    // 点击输入框区域，弹出输入对话框
                    if (IsPointInRect(pos, InputPos[0])) {
                        wchar_t num[10];
                        InputBox(num, 10, L"建议时长 3s 到 5s，太长自己也受不了", L"爆闪手电时长");
                        int input = _wtoi(num);
                        
                        // 输入验证：确保数值有效且在合理范围内（0-10秒）
                        if (input == 0)
                        {
                            t_flash = 5;
                            MessageBoxA(NULL, "输入中有无法解析的字符，当前数值重置为5s", "警告", MB_OK | MB_ICONERROR | MB_DEFAULT_DESKTOP_ONLY);
                        }
                        if (input < 0 || input > 10)
                        {
                            t_flash = 5;
                            MessageBoxA(NULL, "输入超过阈值，当前数值重置为5s", "警告", MB_OK | MB_ICONERROR | MB_DEFAULT_DESKTOP_ONLY);
                        }
                        else
                        {
                            t_flash = input;
                        }
                        ReDraw();  // 更新界面显示新的数值
                    }
                }
            }
            
            // 更新上一次点击状态，用于下一次上升沿检测
            last_clicked = current_clicked;

            // 降低CPU占用（窗口在前台时10ms轮询一次）
            Sleep(10);
        }
        // 窗口不在前台时降低轮询频率，减少资源消耗
        Sleep(10);
    }
}

/**
 * @brief 保存用户配置到文件
 * 
 * 将当前功能开关状态和参数设置保存到DFUserConfig.dat文件（二进制格式）
 */
void SaveConfig()
{
    // 打开配置文件（写入模式）
    FILE* f = fopen("DFUserConfig.dat", "wb+");

    DATA dat;

    // 填充配置数据
    for (int i = 0; i < 6; i++)
    {
        dat.SW[i] = FuncSw[i];
    }
    dat.SW[6] = b_MainWeapon;  // 武器模式
    dat.tf = t_flash;           // 爆闪时长
    dat.tk = t_knife;           // 取消刀动画时长

    // 写入文件
    fwrite(&dat, sizeof(DATA), 1, f);
    fclose(f);

    // 提示用户保存成功
    MBX("已保存当前配置文件");
};

/**
 * @brief 从文件读取用户配置
 * 
 * 从DFUserConfig.dat文件读取之前保存的配置，恢复功能开关状态和参数设置
 * 如果配置文件不存在，则使用默认值
 */
void ReadConfig()
{
    // 打开配置文件（读取模式）
    FILE* f = fopen("DFUserConfig.dat", "rb+");
    
    // 如果文件不存在，使用默认值
    if (f == nullptr)
        return;

    DATA dat; 
    memset(&dat, 0, sizeof(DATA));

    // 读取配置数据
    fread(&dat, sizeof(DATA), 1, f);

    // 恢复功能开关状态
    for (int i = 0; i < 6; i++)
    {
        FuncSw[i] = dat.SW[i];
    }
    // 恢复武器模式和参数
    b_MainWeapon = dat.SW[6];
    t_flash = dat.tf;
    t_knife = dat.tk;

    // 关闭文件
    fclose(f);
};

/**
 * @brief 检测指定按键是否按下
 * 
 * 使用GetAsyncKeyState检测按键状态，支持虚拟键码
 * 
 * @param Key 虚拟键码（如VK_XBUTTON1表示鼠标侧键）
 * @return bool 按下返回true，未按下返回false
 */
bool IsPressed(int Key) {
    return (GetAsyncKeyState(Key) & 0x8000) != 0;
};

/**
 * @brief 检测点是否在矩形区域内
 * 
 * 判断指定坐标点是否位于给定的矩形区域内
 * 
 * @param pos 要检测的点坐标
 * @param rec 矩形区域
 * @return bool 点在区域内返回true，否则返回false
 */
bool IsPointInRect(POINT& pos, RECT& rec)
{
    if (pos.x >= rec.left && pos.x <= rec.right && pos.y >= rec.top && pos.y <= rec.bottom)
        return true;
    return false;
};

/**
 * @brief 根据进程名称查找进程ID
 * 
 * 使用ToolHelp32 API枚举系统进程，找到匹配的进程并返回其ID
 * 
 * @param processName 进程名称（宽字符格式，如L"Game.exe"）
 * @return DWORD 进程ID，未找到返回0
 */
DWORD FindProcessId(const std::wstring& processName) {
    DWORD processId = 0;
    
    // 创建进程快照
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 processEntry;
        processEntry.dwSize = sizeof(PROCESSENTRY32);

        // 遍历进程列表
        if (Process32First(snapshot, &processEntry)) {
            do {
                // 比较进程名称（不区分大小写）
                if (_wcsicmp(processEntry.szExeFile, processName.c_str()) == 0) {
                    processId = processEntry.th32ProcessID;
                    break;
                }
            } while (Process32Next(snapshot, &processEntry));
        }
        CloseHandle(snapshot);
    }
    return processId;
}

/**
 * @brief 终止指定进程
 * 
 * 打开指定进程并终止它，需要PROCESS_TERMINATE权限
 * 
 * @param processId 要终止的进程ID
 * @return bool 成功终止返回true，失败返回false
 */
bool TerminateTargetProcess(DWORD processId) {
    // 打开进程（获取终止权限）
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processId);
    if (hProcess == NULL) {
        return false;
    }

    // 终止进程
    BOOL result = TerminateProcess(hProcess, 0);
    CloseHandle(hProcess);

    if (!result) {
        return false;
    }

    return true;
}

/**
 * @brief 切屏静音功能线程
 * 
 * 检测游戏窗口是否为前台窗口，当游戏窗口不在前台时自动静音，
 * 返回前台时取消静音。使用Windows Core Audio API控制进程音量。
 * 
 * 实现流程：
 * 1. 获取游戏进程ID
 * 2. 获取游戏进程的音频会话控制接口
 * 3. 循环检测当前活动窗口
 * 4. 根据窗口状态切换静音/取消静音
 * 
 * @param args 线程参数（未使用）
 * @return unsigned int 线程退出码
 */
unsigned int __stdcall MuteWindow(void* args) {
    ISimpleAudioVolume* SV = nullptr;  // 音频会话音量控制接口
    SingleVolume pSingV;                // 音量控制器实例
    HWND hw = NULL;                     // 当前活动窗口句柄

    // 获取游戏进程ID
    GetWindowThreadProcessId(g_hwnd, &g_pid);

    // 主循环（功能开启且游戏运行时）
    while (FuncSw[1] && GameState)
    {
        // 等待获取音频会话控制接口
        while (SV == nullptr && FuncSw[1])
        {
            SV = pSingV.GetControl(g_pid);
            Sleep(100);
        }

        // 获取当前活动窗口
        hw = GetForegroundWindow();
        
        // 如果游戏窗口是前台窗口，取消静音
        if (hw == g_hwnd)
        {
            if (pSingV.IsMuted(SV))
                pSingV.UnMute(SV);
        }
        // 否则静音游戏
        else
        {
            if (!pSingV.IsMuted(SV))
                pSingV.SetMute(SV);
        }
    }

    // 线程结束前确保取消静音
    pSingV.UnMute(SV);
    if(SV != nullptr)
        SV->Release();
    SV = nullptr;
    return 0;
};

/**
 * @brief 关闭ACE反作弊服务功能线程
 * 
 * 游戏启动后等待10秒，然后停止ACE反作弊服务，避免其扫描磁盘导致帧率下降。
 * 
 * @param args 线程参数（未使用）
 * @return unsigned int 线程退出码
 */
unsigned int __stdcall CloseAntiCheat(void* args)
{
    // 等待10秒（确保游戏完全启动）
    Wait(10);
    
    // 停止ACE反作弊服务
    StopWindowsService(serviceName);
    return 0;
};

/**
 * @brief 取消刀法功能线程
 * 
 * 利用游戏内置快速近战功能，在第一段伤害判定后快速切枪取消后续动画，
 * 达到取消动画但保留第一段伤害的效果。
 * 
 * 实现流程：
 * 1. 检测鼠标侧键XButton1按下
 * 2. 按下快速近战键（~键）
 * 3. 等待指定时长（可自定义，默认165ms）+ 随机偏移
 * 4. 切枪（主武器按1，手枪按4）
 * 5. 循环执行直到侧键释放
 * 
 * @param args 线程参数（未使用）
 * @return unsigned int 线程退出码
 */
unsigned int __stdcall AutoKnife(void* args)
{
    // 主循环（功能开启且游戏运行时）
    // FuncSw[2] 对应取消刀法功能开关，GameState 表示游戏是否运行
    while (FuncSw[2] && GameState) {
        // 仅当游戏窗口为前台时处理（避免在其他窗口误触）
        if (GetForegroundWindow() == g_hwnd) {
            // 检测鼠标侧键XButton1是否按下（功能激活键）
            // 按住侧键期间持续执行取消刀操作
            while (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) {
                // 按下快速近战键（~键，游戏内绑定的快速近战）
                PressKey(VK_OEM_3);
                
                // 等待出刀动画时长（+随机偏移±10ms，增加模拟真实性）
                // t_knife 为用户自定义的动画时长，默认165ms
                // 随机偏移使操作更自然，避免被检测为机械操作
                Sleep(t_knife + (rand() % 21 - 10));
                
                // 根据武器模式切枪，取消后续动画
                // b_MainWeapon=true: 切主武器（1键），适用于携带主武器时
                // b_MainWeapon=false: 切手枪（4键），适用于只带手枪时（如G18夺舍）
                if (b_MainWeapon)
                    PressKey('1');  // 切主武器
                else
                    PressKey('4');  // 切手枪
            }
            Sleep(100);  // 侧键释放后短暂等待，避免过于频繁检测
        }
        Sleep(100);  // 游戏窗口不在前台时降低检测频率
    }
    return 0;
}

/**
 * @brief 开火自动屏息功能线程
 * 
 * 监测鼠标右键（开镜）和左键（射击）状态，当开镜同时射击时自动按下屏息键，
 * 达到开镜开火自动屏息、降低后坐力的效果。
 * 
 * 实现流程：
 * 1. 检测鼠标右键按下（开镜）
 * 2. 检测鼠标左键按下（射击）
 * 3. 同时按下时自动按下屏息键（;键）
 * 4. 左键释放时释放屏息键
 * 5. 右键释放或窗口切换时确保释放屏息键
 * 
 * @param args 线程参数（未使用）
 * @return unsigned int 线程退出码
 */
unsigned int __stdcall StopBreatheWhileFire(void* args)
{
    bool bShiftPressed = false;  // 屏息键（;键，VK_OEM_1）按下状态标志

    // 主循环（功能开启且游戏运行时）
    // FuncSw[3] 对应开火自动屏息功能开关
    while (FuncSw[3] && GameState)
    {
        if (GetForegroundWindow() == g_hwnd)
        {
            // 检测右键是否按下（开镜动作）
            // 只有开镜时才启用自动屏息
            if (GetAsyncKeyState(VK_RBUTTON) & 0x8000)
            {
                // 检测左键是否按下（射击动作）
                bool bLeftDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

                // 左键按下且屏息键未按下：自动按下屏息键
                // 实现"开镜+射击=自动屏息"的逻辑
                if (bLeftDown && !bShiftPressed)
                {
                    keybd_event(VK_OEM_1, 0, 0, 0);  // 按下;键（游戏内绑定的屏息）
                    bShiftPressed = true;
                }
                // 左键释放且屏息键已按下：释放屏息键
                // 射击结束后自动取消屏息
                else if (!bLeftDown && bShiftPressed)
                {
                    keybd_event(VK_OEM_1, 0, KEYEVENTF_KEYUP, 0);
                    bShiftPressed = false;
                }
            }
            // 右键释放且屏息键已按下：释放屏息键
            // 开镜结束后确保取消屏息
            else if (bShiftPressed)
            {
                keybd_event(VK_OEM_1, 0, KEYEVENTF_KEYUP, 0);
                bShiftPressed = false;
            }
        }
        // 窗口切换且屏息键已按下：释放屏息键（防止卡在按下状态）
        // 切换窗口时必须释放所有模拟按键，避免影响其他程序
        else if (bShiftPressed)
        {
            keybd_event(VK_OEM_1, 0, KEYEVENTF_KEYUP, 0);
            bShiftPressed = false;
        }
        Sleep(100);  // 100ms检测一次，平衡响应速度和资源消耗
    }
    return 0;
}

/**
 * @brief 无声滑步功能线程
 * 
 * 按住鼠标侧键时，按下奔跑键并快速点按W键，打断跑步动画但保持移动速度，
 * 达到不降低移动速度但降低脚步声的效果。
 * 
 * 实现流程：
 * 1. 检测鼠标侧键XButton2按下
 * 2. 按下小键盘0键（奔跑键）
 * 3. 快速点按W键（50ms按下，30ms释放）
 * 4. 侧键释放时释放奔跑键
 * 
 * @param args 线程参数（未使用）
 * @return unsigned int 线程退出码
 */
unsigned int __stdcall Slide(void* args) {
    bool bNumPad0Pressed = false;  // 奔跑键（小键盘0）按下状态标志

    // 初始化按键输入结构（使用SendInput模拟按键，比keybd_event更精确）
    // W键按下事件
    INPUT keyDown = { 0 };
    keyDown.type = INPUT_KEYBOARD;
    keyDown.ki.wVk = 'W';

    // W键释放事件
    INPUT keyUp = { 0 };
    keyUp.type = INPUT_KEYBOARD;
    keyUp.ki.wVk = 'W';
    keyUp.ki.dwFlags = KEYEVENTF_KEYUP;

    // 小键盘0键（奔跑键）按下事件
    INPUT numDown = { 0 };
    numDown.type = INPUT_KEYBOARD;
    numDown.ki.wVk = VK_NUMPAD0;

    // 小键盘0键释放事件
    INPUT numUp = { 0 };
    numUp.type = INPUT_KEYBOARD;
    numUp.ki.wVk = VK_NUMPAD0;
    numUp.ki.dwFlags = KEYEVENTF_KEYUP;

    // 主循环（功能开启且游戏运行时）
    // FuncSw[4] 对应无声滑步功能开关
    while (FuncSw[4] && GameState)
    {
        // 仅当游戏窗口为前台时处理
        if (GetForegroundWindow() == g_hwnd)
        {
            // 检测鼠标侧键XButton2是否按下（功能激活键）
            if (GetAsyncKeyState(VK_XBUTTON2) & 0x8000) {
                // 首次按下时按下奔跑键（小键盘0）
                // 保持奔跑状态，但通过快速点按W键打断跑步动画
                if (!bNumPad0Pressed) {
                    SendInput(1, &numDown, sizeof(INPUT));
                    bNumPad0Pressed = true;
                }

                // 快速点按W键（50ms按下，30ms释放）
                // 这种快速点击可以打断跑步动画，但保持移动速度
                // 达到"无声滑步"的效果（降低脚步声但不降低移动速度）
                SendInput(1, &keyDown, sizeof(INPUT));
                Sleep(50);
                SendInput(1, &keyUp, sizeof(INPUT));
                Sleep(30);
            }
            // 侧键释放时释放奔跑键，恢复正常状态
            else {
                if (bNumPad0Pressed) {
                    SendInput(1, &numUp, sizeof(INPUT));
                    bNumPad0Pressed = false;
                }
            }
        }
        Sleep(10);  // 高频检测，确保响应及时
    }
    return 0;
}

/**
 * @brief 威慑性无限爆闪功能线程
 * 
 * 按住鼠标中键时，快速反复按Y键开关手电，产生爆闪效果，持续指定时长。
 * 用于从掩体后突进时威慑敌人。
 * 
 * 实现流程：
 * 1. 检测鼠标中键按下
 * 2. 记录开始时间
 * 3. 在指定时长内快速按Y键
 * 4. 时长结束或窗口切换时停止
 * 
 * @param args 线程参数（未使用）
 * @return unsigned int 线程退出码
 */
unsigned int __stdcall UnlimitedFlash(void* args)
{
    SimpleThread ST;
    clock_t beg = 0;  // 开始时间
    clock_t now = 0;  // 当前时间

    // 主循环（功能开启且游戏运行时）
    while (FuncSw[5] && GameState)
    {
        if (GetForegroundWindow() == g_hwnd)
        {
            // 检测鼠标中键是否按下
            if ((GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0)
            {
                // 记录开始时间
                beg = clock();
                now = clock();
                
                // 在指定时长内快速按Y键（开关手电）
                while (GetForegroundWindow() == g_hwnd && FuncSw[5] && 
                       (((now - beg) / CLOCKS_PER_SEC) < t_flash))
                {
                    PressKey('Y');
                    now = clock();
                }
            }
            Sleep(10);
        }
        Sleep(100);
    }
    return 0;
}
