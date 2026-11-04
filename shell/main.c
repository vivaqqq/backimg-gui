/*
 * backimg 图形板 v1.3.0 (C 语言版 / Win32 API)
 * 纯 C 重写 ExplorerBgTool 配置面板，单文件 EXE。
 *
 * 编译 (MinGW-w64):
 *   windres -O coff resource.rc -o resource.res
 *   gcc -O2 -mwindows -municode -std=c11 main.c resource.res -o backimg图形板_v1.3.16.exe -lgdiplus -lcomctl32 -luxtheme -ldwmapi -lshlwapi -lole32
 *
 *   v1.3.25：定时轮换支持秒级 + 自定义图片目录偶发不显示修复（详见
 *            《变更清单_CHANGELOG_v1.3.25.md》）。要点：
 *              - D1（用户红/绿框）：random 行自定义时间输入框收窄到只容数字
 *                （S(60)→S(42)），右侧新增「分钟/秒钟」单位下拉，轮换间隔
 *                可自由选到秒级；内部权威单位改为秒（randomVal），
 *                randomMin 仍兼容写入供旧版 DLL 使用
 *              - D2（用户实机 bug）：自定义图片目录指向程序目录之外
 *                （如 G:\手机备份\Image）时偶发不显示 —— DLL 侧加
 *                枚举瞬时失败重试 + 空列表定时自愈重载（详见 DLL 变更清单）
 *            配套 DLL 升级为 v1.3.25（rotateSec + 自愈重载）。
 *   v1.3.22：配套 DLL 升级为 v1.3.22 复审加固版（界面零改动）。本版外壳
 *            仅递增版本号，全部改动在 DLL 侧（Deepseek 复审 4 条：
 *            跨线程销毁加固 / TOCTOU 不变量注释 / INT_MAX 魔数 /
 *            定时器语义注释）。详见《变更清单_CHANGELOG_v1.3.22.md》。
 *   v1.3.21：修复 Deepseek V4 Pro 审计确认的 3 项外壳问题（详见
 *            《变更清单_CHANGELOG_v1.3.21.md》）。要点：
 *              - G1：「定时轮换」下拉按钮面永远空白（创建按钮后漏了
 *                把句柄回填 g_cb_rot.btn，选档回显整句被跳过）
 *              - G2：自定义分钟输入框实时同步 v_rotmin（EN_CHANGE）+
 *                选档后刷新摘要时间戳
 *              - G3：保存时钳位结果回写输入框（输入 9999 落盘 1440，
 *                框里不再残留 9999）
 *            配套 DLL 升级为 v1.3.21 审计修复版（D1~D5，详见变更清单）。
 *   v1.3.20：随机功能重做（详见《变更清单_CHANGELOG_v1.3.20.md》）。
 *            界面改动：random 行右侧新增「定时轮换」自绘下拉 + 自定义整数
 *            分钟输入框（样式与「界面缩放」一致），其余界面零改动。
 *            配套 DLL 升级：真随机源 / 按窗口记账 / 定时轮换（randomMin）。
 *   v1.3.19：DLL 配套升级为 v1.3.19 审计修复版（详见《变更清单_CHANGELOG_v1.3.19.md》，
 *            修复 Kimi K3.1 Pro 复审确认的 14 项问题）。本版界面零改动。
 *   v1.3.18：DLL 配套升级为 v1.3.18 审计修复版（详见《变更清单_CHANGELOG_v1.3.18.md》）。
 *            界面改动：移除状态栏「已提权/未提权」胶囊 —— 它显示的是外壳进程自身的
 *            管理员状态，静态不变、与 DLL 注册状态无关，紧挨着"DLL not registered"极易
 *            误导用户以为注册/提权出了问题。其余界面零改动。
 *   v1.3.17：配套 DLL 修复 v1.3.16 审计发现的 9 项问题（详见
 *            《变更清单_CHANGELOG_v1.3.17.md》）。要点：
 *              - 修掉每线程源图缓存无上限泄漏（换图一次永久多驻留 177MB）
 *              - 修掉 thread_local 析构崩溃雷（MinGW 在已释放 TLS 内存上跑析构）
 *              - 修掉空目录/解码失败时每帧自触发重绘（CPU 跑满）
 *              - GDI+ 串行锁真正接上（v1.3.16 定义了却零引用）
 *            界面仅更新版本号与行为说明，逻辑零改动。
 *   v1.3.15：配套 DLL 换为修复版 v1.3.15（内置资源同步），界面零逻辑改动，
 *            仅递增版本号；修复版 DLL 的 30 处修订详见交付包《CHANGELOG_v1.3.15.md》
 *
 * 版本号约定：GUI_VERSION 为本程序版本，每次修改必须递增
 *   v1.2.9 首版交付
 *   v1.3.0 按豆包审查反馈重做 UI（生效流程块/摘要两列/开关分层/分段两行/
 *          图片与规则页卡片高度/摘要着色/日志分级着色/窗口可拉伸/缩放即点即用）
 *   + 修复：定位按钮、注册注销退出码判定、taskkill 退出码、60s 看门狗、
 *           复制摘要张数、规则图片存在性校验、开关状态单一数据源
 *   v1.3.2 UI 精修（对照 HTML 设计稿 8 条 P1 + 4 条代码遗留），要点：
 *          - key 芯片 / readout 芯片 / 摘要键省略号 / 开关行虚线 / 日志浅灰头
 *          - posType 七段外加统一容器、hint 右对齐、摘要键值基线对齐
 *          - 工具栏与底栏改白底、状态胶囊改实心圆点 + 光晕、字体随缩放放大
 *          - 修：缩放切换窗口逐步缩小、重建吞编辑框文本、BTN_STYLE_MINI 与
 *                分段 (idx<<4) 位冲突（改 0x100）
 *   v1.3.3 UI 精修第二轮（详见 版本迭代报告.md），要点：
 *          - 三个输入框改轻自绘：圆角 7 + 1px 边框，focus 时蓝边 + 白底
 *            （新增 backimgEditFrame 自绘外框，内部 EDIT 去 WS_BORDER）
 *          - 不透明度滑块自绘：6px 圆角轨道 + 14px 白圆钮 + 蓝色已填段
 *            （新增 backimgSlider，保留 TBM_GETPOS/SETPOS 语义，上层逻辑不改）
 *          - 选项卡 hover 浅灰底（TCM_HITTEST 追踪热点项）
 *          - 按钮统一 32px 高、底栏按剩余宽度均匀分布、主按钮加 ✓
 *          - 摘要值改为自绘省略号（键列 128→108，值列加宽）
 *          - 开关行高 46→40、chip 字号 8→8.5pt
 *          - 修：on_copy_summary 版本号未走 u8tow、remove_folder_section 缓冲 1024→4096
 *   v1.3.4 UI 精修第三轮（用户实机标注 6 处），要点：
 *          - posType 二行居中、流程块弹性铺满卡底、摘要按钮固定 3+2 两行
 *          - 界面缩放改自绘下拉按钮 + TrackPopupMenu、底栏回固定间距
 *          - 修：WM_CTLCOLOREDIT 判焦点恒真（lParam==本控件）致空闲态白底
 *   v1.3.5 UI 精修第四轮（用户实机标注 7 处），要点：
 *          - 输入框文字垂直居中：editframe_layout 按字体实测高度放置内嵌
 *            EDIT（单行 EDIT 默认顶对齐，框一/框四文字贴顶的根因）
 *          - 修：工具栏插件目录输入框宽度多算 8px，与「浏览...」按钮真重叠
 *          - 显示选项卡弹性填满：开关行高 40→44、分段钮 28→30、滑块 26→30，
 *            剩余空间按 35/35/30 分摊到三处间距（框二不再大片留白）
 *          - 选项卡文字改 semibold（设计稿 .tab font-weight:600），
 *            行底加 1px 全宽分隔线（.tabs border-bottom，新增 PANEL_BG_LINE）
 *          - 摘要按钮宽度够时排一行（与设计稿一致），不够回退 3+2 两行
 *          - 生效流程余量改按比例分摊（行距/标题距/提示条高/提示条前距），
 *            不再把全部余量塞进提示条前距造成大空洞（框六）
 *          - 开关精修：圆钮加 1px 下移阴影 + 浅描边，宽 40→38（设计稿 38×22）
 *   v1.3.13 实测拖影修复 + 豆包 v1.3.12 核验报告观察点：
 *          - 滑块拖影/重影（用户红框，疯狂来回拖动时出现）：根因是滑块
 *            WM_ERASEBKGND 返回 1 不擦背景，而 WM_PAINT 只画 6px 高的轨道——
 *            圆钮（14px）与阴影凸出轨道上下的部分从来没人重画，快速拖动时
 *            旧位置圆钮的抗锯齿残环逐帧累积成 hatch 状鬼影。修法：WM_PAINT
 *            先整块铺卡片底色再绘制，并改双缓冲（内存 DC 画完一次 BitBlt），
 *            既擦净历史残影又消除清除+重画两步之间的闪烁
 *          - 摘要 imgAlpha 值乱字（用户蓝框）：同一类根因——SumValProc 画文字用
 *            TRANSPARENT 模式且从不铺底，拖动滑块时 update_summary 高频改文字，
 *            新字叠旧字成乱码。SumKeyProc/SumValProc/ChipProc（圆角四角）/
 *            StepNoProc（圆外四角）四处统一在绘制前铺父容器底色
 *          - 豆包核验观察点 #1/#2：custom 未勾选时 v_folder 不影响生效目录，
 *            EN_CHANGE 与「选择...」不再无条件失效张数缓存/重扫目录
 *          - 豆包核验观察点 #3：允许"no image on this path"的专属规则（img 留空直接写入，
 *            用于清背景），对齐 Python 版 _adv_add 行为
 *   v1.3.14【重大堆越界修复】（用户实测：选目录后点保存配置→卡死退出、
 *          config.ini 写烂——节名丢 ]/键值丢失/中文注释变 mojibake/整块重复）：
 *          - ini_write_value 与 remove_folder_section 的动态拼接缓冲
 *            malloc(32) 只分配 32【字节】(16 个 WCHAR)，out_cap 却按 32【个
 *            字符】用——ini 文本超 16 字符后 memcpy 越界写堆。砸坏堆元数据
 *            →保存时崩溃；砸中相邻堆块→其它缓冲内容（如日志字节）被拼进
 *            宽串一起写进 ini（用户 ini 里的乱码即被污染证据）。v1.3.0 起
 *            潜伏，是否暴露取决于堆布局。修：malloc(64 * sizeof(WCHAR))，
 *            cap 与字节数单位统一，两处同修
 *          - 内嵌默认 config.ini 重制：folder 默认值原为 Python 版遗留的
 *            "D:/backimg_GUI_v1.2.9/Image"，现改为空（用插件目录/Image 或
 *            勾选 custom 指定）；并修复历史文本模式重写累积的 "\r\r" 行尾群
 *   v1.3.12 代码审计修复（豆包 v1.3.11 审计报告 7 项，逐条核实属实后修复）：
 *          - P1 补 DLL 完整性校验（对齐 Python 版 v1.1.2/v1.1.4）：启动时对内置
 *            资源 DLL 与 plugin 目录 DLL 各算一次 SHA256 并比对——不一致弹警告；
 *            点「注册(提权)」前不一致弹确认框，用户选否则中止注册（防本地 DLL
 *            被替换后照常注册）
 *          - P2 自定义图片目录"no reaction after selection"：点「选择...」选完目录后，若
 *            custom 开关未勾选，弹询问"also tick the box"，选是自动勾上并切换列表
 *            （保留 C 版"list = actual active folder"的严格逻辑，只补提示）
 *          - P2 界面日志多行内容挤成一条：log_view_add 按换行拆分逐条入列表
 *            （browse_plugin 的 4 行状态信息不再带 \r\n 控制字符挤在一行）
 *          - P3 手改自定义目录输入框即时生效：EN_CHANGE 使张数缓存失效，
 *            custom 勾选时同步刷新图片列表与规则图片下拉
 *          - P3 大图预览提示：>30MB 的图片解码前记日志并把光标换成等待态，
 *            解码结束恢复（对齐 Python 版 v1.2.3 的"prompt first"行为）
 *          - P3 保存失败不再假报成功：write_text_file/ini_commit 返回 bool，
 *            失败时 log_err + 弹错误框（config.ini 只读/被占用不再静默丢失）
 *          - P3 复制摘要补诊断三件套：DLL SHA256、DLL 大小、插件 CLSID
 *            （对齐 Python 版 v1.2.4，反馈问题时光贴摘要信息就够）
 *   v1.3.11 实机反馈一项：
 *          - 图片管理页预览卡孤零零显示「GDI+ 解码」芯片（用户红框）：它是设计稿
 *            .pmeta 元信息行（文件名+尺寸+解码芯片）的第三件，仅在选中图片后
 *            有意义——空状态时文件名/尺寸为空串，只剩芯片挂着，观感像乱入文字。
 *            修法：元信息行三件套初始隐藏，选中图片成功后按实际文字宽度
 *            （flex gap 排布）显示；文件丢失/列表刷新时一并隐藏
 *   v1.3.10 实机标注两项：
 *          - 圆角锯齿（绿框）：GDI RoundRect/Ellipse 无抗锯齿，开关轨道/圆钮/
 *            分段/芯片/序号圆点/滑块圆钮的圆弧在实机上有明显锯齿感。圆角矩形与
 *            圆形统一改走 GDI+ 路径（SmoothingModeAntiAlias），g_gdiplus_ok
 *            不可用或半径过小时回退 GDI
 *          - posType 选中段整钮着色（蓝框）：之前选中只有文字变蓝，用户要求
 *            像"emergency off"按钮一样按下时整个按钮有颜色——选中段改蓝底白字
 *            （实机反馈优先于设计稿的白底蓝字）
 *   v1.3.9 实机标注三项：
 *          - 右卡内容填满整卡（红框）：v1.3.8 的流程块把 extra 全计入块高却只
 *            消耗封顶后的部分，超出部分沉淀在卡底形成大空白。改为布局前统一
 *            规划：剩余空间 40% 注入摘要网格行距（每行封顶 S(6)），其余给
 *            流程块三处间距（各自封顶），块顶按实际消耗量锚定——提示条正好
 *            落在卡底 S(10) 上，不再有沉底空白
 *          - 「本次刷新」时间尾部被裁（红框）：label 余量 S(10)→S(24)、
 *            右缘 S(16)→S(20)
 *          - 文件夹专属背景页两个原生 COMBOBOX（蓝框）改自绘下拉按钮 +
 *            TrackPopupMenu（与「界面缩放」同一模式）：插入 CLSID、
 *            图片文件名均为纯选择型，行为对齐旧实现（占位项/文本匹配回选）
 *   v1.3.8 实机标注三项：
 *          - 摘要按钮排下移（黄框箭头）：按钮排从"right under the summary grid"移到生效流程块
 *            正上方 S(8) 处，与网格之间的大空档改到按钮排之前；矮窗流程块
 *            跳过时回退紧跟摘要网格
 *          - posType 分段去背景色（红框）：容器 #EDF1F7 灰底改白，未选中段
 *            白底（视觉全透明），只留 1px 边框和选中蓝边
 *          - 开关行距填满绿框：剩余空间 70% 注入开关行距（内容在行内居中），
 *            行距呼吸感对齐下方「图片显示位置」区块；其余 30% 仍分摊三处间距
 *   v1.3.7 实机标注三项：
 *          - 按钮"square shadow outside rounded corners"：主窗口缺 WM_CTLCOLORBTN 分支，owner-draw
 *            按钮擦底落到 DefWindowProc 返回系统灰 COLOR_BTNFACE，圆角四角
 *            露出直角灰块。按按钮父容器底色返回画刷（与 CTLCOLORSTATIC 同规则）
 *          - 生效流程块整体下移贴卡底（红框箭头）：v1.3.6 给三处间距封顶后
 *            未分配的余量悬空，块底距卡底最多差 80px+；剩余量改为全部塞进
 *            块顶前距，块内部保持紧凑、整体贴底
 *          - posType 七段未选中段去掉白色填充底（黑框）：保留 1px 边框的
 *            "button feel"，但底色透出容器灰，只有选中段保持白底蓝边
 *   v1.3.6 实机标注四项（对照原版与设计稿）：
 *          - 底栏「刷新状态/备份配置/恢复配置」右对齐到窗口右缘（设计稿 .actions
 *            中间有 spacer，前 4 个左起、后 3 个靠右；v1.3.4 起一直 7 个连排左起）
 *          - posType 七段每个加 1px 按钮边框（用户要求"button feel like the selector buttons"：
 *            未选中白底灰边灰字、选中白底蓝边蓝字，容器 #EDF1F7 保留）
 *          - 流程提示条固定 26px 不再吃余量（v1.3.5 的 add_h 把它撑到 40px+，
 *            一条又高又灰的大条横在卡底，用户指出"background is gray; the original is not"）
 *          - 选项卡弃用 SysTabControl32（TCS_BUTTONS 系统 3D 边框去不掉，owner-draw
 *            只接管内容区），改 3 个纯自绘按钮：无边框白底、hover 浅灰、
 *            选中蓝字 + 底部 3px 蓝条压住行底分隔线
 *   v1.3.1 代码级缺陷修复（详见 版本迭代报告.md），要点：
 *          - read_text_file 带 BOM 时输出未 NUL 终止（整条 INI 链路的越界读根因）
 *          - path_join 用 _snwprintf 截断不补 NUL
 *          - WideCharToMultiByte 缓冲不足时输出未终止（sec/key/value 多处）
 *          - ini 写入行缓冲仅 1024，长路径被静默截断
 *          - 日志文件行缓冲 2300 字节，中文长日志写不进磁盘
 *          - 预览位图被 STATIC 持有时即删除（改为先 SETIMAGE 取回旧图再删）
 *          - ShellExecuteEx 成功但 hProcess 为空时误判为成功
 *          - GDI+ 初始化/解码返回值未检查；缩略图尺寸可能为负
 *          - _beginthreadex 句柄未关闭（每次注册/注销泄漏一个）
 *          - 规则列表靠反解析显示文本取 section（长路径截断即删错规则）
 *          - 拖动透明度滑块时每次都全盘扫描图片目录（改为缓存）
 *          - 启动校验定时器未 KillTimer，每 400ms 空转
 *          - 窗口位置记忆未做屏幕可见性校验
 *   + 改进：控件统一字体、图片列表显示体积、刷新时记录 DLL SHA256、
 *           备份/打开目录失败有提示、恢复配置后自动刷新列表
 *          [P3]   list_folder_sections realloc 失败处理、load_thumb 后半段 GDI+ 返回值检查、
 *                 ini_read_value section 缓冲放大到 4096、GUI_VERSION 转换改走 u8tow
 *
 * 说明：
 *   - DLL / config.ini / app.ico 以 RCDATA/ICON 资源内嵌，首次运行解包到 exe 同级 plugin\。
 *   - 实现严格遵循《backimg_图形板_C语言版_开发交接报告.md》。
 */

#define _WIN32_WINNT 0x0601
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <shellapi.h>
#include <gdiplus.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <process.h>

#ifdef _MSC_VER
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "shlwapi.lib")
#endif

/* ------------------------------------------------------------------ */
/* 常量与版本                                                          */
/* ------------------------------------------------------------------ */
#define APP_NAME_W      L"backimg Panel"
#define GUI_VERSION     "1.3.28"
#define DLL_VERSION     "1.3.28"
#define CLSID_STR       "{ED15A97D-FE3E-4CDE-98FF-BC46B02896B0}"
#define CLSID_STR_W     L"{ED15A97D-FE3E-4CDE-98FF-BC46B02896B0}"

#define IDR_DLL         101
#define IDR_INI         102
#define IDI_APPICON     1

#define DLL_NAME        L"ExplorerBgTool.dll"
#define INI_NAME        L"config.ini"
#define ICON_NAME       L"app.ico"
#define IMAGE_DIR_NAME  L"Image"
#define GUI_LOG_NAME    L"gui_log.txt"

#define MAX_PATH_W      4096

/* ------------------------------------------------------------------ */
/* 颜色 token（来自 HTML 设计稿）                                      */
/* UNUSED_TOKEN：设计稿里定义了但本版尚未用到的（留给自绘深色标题栏等）  */
/* ------------------------------------------------------------------ */
#define UNUSED_TOKEN __attribute__((unused))
static const COLORREF C_CANVAS       = RGB(0xEE, 0xF1, 0xF6);
static const COLORREF C_PANEL        = RGB(0xFF, 0xFF, 0xFF);
static const COLORREF C_TITLEBAR     UNUSED_TOKEN = RGB(0x1B, 0x23, 0x30);
static const COLORREF C_TITLE_TEXT   UNUSED_TOKEN = RGB(0xE6, 0xEB, 0xF4);
static const COLORREF C_TITLE_SUB    UNUSED_TOKEN = RGB(0x8F, 0xA0, 0xB8);
static const COLORREF C_BORDER       = RGB(0xE3, 0xE7, 0xEF);
static const COLORREF C_BORDER_STRONG UNUSED_TOKEN = RGB(0xCD, 0xD5, 0xE1);
static const COLORREF C_TEXT         = RGB(0x1B, 0x24, 0x30);
static const COLORREF C_TEXT2        = RGB(0x5B, 0x64, 0x72);
static const COLORREF C_TEXT3        = RGB(0x8A, 0x93, 0xA3);
static const COLORREF C_ACCENT       = RGB(0x2F, 0x6B, 0xFF);
static const COLORREF C_ACCENT_HOVER UNUSED_TOKEN = RGB(0x24, 0x56, 0xE0);
static const COLORREF C_ACCENT_SOFT  = RGB(0xEE, 0xF4, 0xFF);
static const COLORREF C_ACCENT_LINE  = RGB(0xD8, 0xE5, 0xFB);
static const COLORREF C_OK           = RGB(0x16, 0xA3, 0x4A);
static const COLORREF C_OK_SOFT      = RGB(0xF0, 0xFD, 0xF4);
static const COLORREF C_OK_LINE      = RGB(0xBB, 0xF7, 0xD0);
static const COLORREF C_WARN         = RGB(0xB4, 0x53, 0x09);
static const COLORREF C_WARN_SOFT    = RGB(0xFF, 0xF7, 0xED);
static const COLORREF C_WARN_LINE    = RGB(0xFC, 0xD9, 0xA8);
static const COLORREF C_DANGER       = RGB(0xDC, 0x26, 0x26);
static const COLORREF C_DANGER_SOFT  = RGB(0xFE, 0xF6, 0xF6);
static const COLORREF C_DANGER_LINE  = RGB(0xF3, 0xC2, 0xC2);
static const COLORREF C_LOG_BG       = RGB(0xFC, 0xFD, 0xFE);
static const COLORREF C_LOG_TEXT     = RGB(0x47, 0x55, 0x69);
static const COLORREF C_LOG_TIME     = RGB(0x9A, 0xA4, 0xB5);
static const COLORREF C_TRACK_OFF    = RGB(0xCB, 0xD5, 0xE1);
/* v1.3.2 新增：设计稿里此前未落到 C 版上的 token */
static const COLORREF C_KEY_TEXT     = RGB(0x33, 0x60, 0xC9);   /* .key 字色 */
static const COLORREF C_SUM_KEY      = RGB(0x6B, 0x72, 0x80);   /* .skey 键名 */
static const COLORREF C_LOG_HEAD_BG  = RGB(0xF6, 0xF8, 0xFB);   /* .log-head */
static const COLORREF C_LOG_HEAD_TXT = RGB(0x3A, 0x43, 0x53);
static const COLORREF C_SEG_BG       = RGB(0xED, 0xF1, 0xF7);   /* .seg 容器 */
static const COLORREF C_DIVIDER      = RGB(0xED, 0xF0, 0xF5);   /* 开关行虚线 */
static const COLORREF C_SHADOW       = RGB(0xE2, 0xE8, 0xF2);   /* 分段选中浅阴影近似 */
static const COLORREF C_STATUS_BG    = RGB(0xFB, 0xFC, 0xFE);   /* .status 条 */
static const COLORREF C_DOT_IDLE     = RGB(0x94, 0xA3, 0xB8);   /* .pill .dot 灰点 */

/* v1.3.3 新增：输入框 / 自绘滑块 / 选项卡 hover */
static const COLORREF C_INPUT_BG     = RGB(0xFB, 0xFC, 0xFE);   /* .dir-input 常态底 */
static const COLORREF C_TRACK_BG     = RGB(0xE5, 0xEA, 0xF3);   /* 滑块未填充轨道 */
static const COLORREF C_HOVER_BG     = RGB(0xF4, 0xF6, 0xFA);   /* .btn:hover / .tab:hover */
static const COLORREF C_HOVER_LINE   = RGB(0xCD, 0xD5, 0xE1);   /* .btn:hover 边框 */

static const WCHAR *POS_NAMES[] = {
    L"Top left", L"Top right", L"Bottom left", L"Bottom right",
    L"Center", L"Fit", L"Fill"
};

static const WCHAR *BUILTIN_CLIDS[] = {
    L"This PC",
    L"Quick access",
    L"Network"
};
static const WCHAR *BUILTIN_GUIDS[] = {
    L"20D04FE0-3AEA-1069-A2D8-08002B30309D",
    L"679F85CB-0220-4080-B29B-5540CC05AAB6",
    L"F02C1A0D-BE21-4350-88B0-7367FC96EF3C"
};

/* ------------------------------------------------------------------ */
/* 控件 ID                                                             */
/* ------------------------------------------------------------------ */
enum {
    ID_EDIT_PLUGIN_DIR = 100,
    ID_BTN_BROWSE_DIR,
    ID_BTN_LOCATE,
    ID_CB_SCALE,
    ID_BTN_SAVE,
    ID_BTN_REG,
    ID_BTN_UNREG,
    ID_BTN_RESTART,
    ID_BTN_REFRESH,
    ID_BTN_BACKUP,
    ID_BTN_RESTORE,
    ID_BTN_LOG_CLEAR,
    ID_TAB_BASIC,
    ID_TAB_IMAGES,
    ID_TAB_FOLDERS,
    /* 基础配置 */
    ID_SW_FOLDEREXT,
    ID_SW_NOERROR,
    ID_SW_RANDOM,
    ID_SW_CUSTOM,
    ID_CB_ROT,              /* v1.3.20(C)：定时轮换下拉按钮 */
    ID_EDIT_ROTMIN,         /* v1.3.20(C)：自定义时间输入框 */
    ID_CB_ROTUNIT,          /* v1.3.25(D1)：单位下拉（分钟/秒钟） */
    ID_SEG_POSTYPE_0,
    ID_SEG_POSTYPE_6 = ID_SEG_POSTYPE_0 + 6,
    ID_SLIDER_ALPHA,
    ID_EDIT_FOLDER,
    ID_BTN_PICK_FOLDER,
    ID_BTN_SUM_REFRESH,
    ID_BTN_OPEN_INI,
    ID_BTN_COPY_SUM,
    ID_BTN_EMERGENCY,
    ID_BTN_RESTORE_PREV,
    /* 图片管理 */
    ID_LV_IMAGES,
    ID_BTN_IMG_REFRESH,
    ID_BTN_OPEN_IMG_DIR,
    ID_ST_PREVIEW,
    /* 专属背景 */
    ID_LV_RULES,
    ID_EDIT_RULE_PATH,
    ID_CB_RULE_IMG,
    ID_CB_CLSID,
    ID_BTN_RULE_ADD,
    ID_BTN_RULE_DEL,
    ID_BTN_RULE_REFRESH,
    /* 界面缩放弹出菜单项（v1.3.4）：IDM_SCALE_FIRST + 0..7 */
    IDM_SCALE_FIRST = 500,
    /* v1.3.9：CLSID/图片文件名自绘下拉菜单项：IDM_CB_FIRST + 0..n */
    IDM_CB_FIRST = 600,
    /* v1.3.20(C)：定时轮换弹出菜单项：IDM_ROT_FIRST + 0..6 */
    IDM_ROT_FIRST = 700,
    /* v1.3.25(D1)：单位下拉弹出菜单项：IDM_ROTUNIT_FIRST + 0..1 */
    IDM_ROTUNIT_FIRST = 710,
};

/* ------------------------------------------------------------------ */
/* 全局状态                                                            */
/* ------------------------------------------------------------------ */
typedef struct {
    WCHAR exe_dir[MAX_PATH_W];
    WCHAR plugin_dir[MAX_PATH_W];
    WCHAR dll_path[MAX_PATH_W];
    WCHAR ini_path[MAX_PATH_W];
    WCHAR image_dir[MAX_PATH_W];

    int scale_idx;          /* -1=auto, 0=70%, ..., 7=150% */
    double scale_factor;
    /* cw/ch：实际客户区像素，布局一律用它（v1.3.2 修复缩放≠100% 时卡片偏窄） */
    int cw, ch;
    /* nominal_w/h：名义逻辑基准（=客户区像素 / scale），只在初始化、用户手动拉伸、
       WM_CLOSE 时更新；缩放切换用它换算窗口尺寸，不再被 WM_SIZE 反复改写 */
    int nominal_w, nominal_h;
    int base_w, base_h;     /* 兼容旧引用：与 cw/ch 同步 */
    int py_panel, ph_panel; /* 面板区在主窗口客户区中的位置（主窗口分区上色用） */
    int win_x, win_y;

    bool v_folderExt;
    bool v_noerror;
    bool v_random;
    bool v_custom;
    int  v_posType;
    int  v_alpha;
    /* v1.3.25(D1)：定时轮换间隔改用【秒】为唯一权威单位（0=不轮换）。
     * v1.3.20~v1.3.24 的 v_rotmin 只能整分钟，用户实测"even 1 minute feels slow"，
     * 需要秒级轮换。界面显示单位（分钟/秒钟）由 v_rotunit 决定，
     * 但内部一律换算成秒落盘（config.ini → randomVal），不吃小数。 */
    int  v_rotsec;          /* 定时轮换间隔（秒，0=不轮换，上限 86400=一天） */
    int  v_rotunit;         /* 自定义态显示单位：0=分钟，1=秒钟（v1.3.25(D1)） */
    WCHAR v_folder[MAX_PATH_W];

    bool admin;
    bool dll_exists;
    bool ini_exists;
    bool registered;
    /* v1.3.12：内置资源 DLL 的 SHA256（启动时算一次缓存），与 plugin 目录 DLL 比对 */
    bool embedded_hash_ok;
    char  embedded_hash[65];
    WCHAR reg_detail[4][512];
    WCHAR reg_dll_paths[4][MAX_PATH_W];
    int  reg_dll_path_count;

    bool emergency_backup_valid;
    WCHAR emergency_folder[MAX_PATH_W];
    bool emergency_random;
    bool emergency_custom;
    bool emergency_noerror;

    HWND hwndMain;
    HWND hwndTabs[3];
    HWND hwndLog;
    HWND hwndStatus;
    HWND hwndPluginDir;
    HWND hwndScaleBtn;      /* v1.3.4：自绘下拉按钮（替代原生 COMBOBOX） */
    HWND hwndBtnReg, hwndBtnUnreg, hwndBtnRestart;
    HWND hwndBtnEmergency, hwndBtnRestorePrev;
    HWND hwndEditFolder;
    HWND hwndRotBtn;        /* v1.3.20(C)：定时轮换自绘下拉按钮 */
    HWND hwndRotMinEdit;    /* v1.3.20(C)：自定义时间输入框（内层 EDIT） */
    HWND hwndRotUnitBtn;    /* v1.3.25(D1)：单位下拉按钮（分钟/秒钟） */
    HWND hwndAlphaReadout, hwndSumTime;
    HWND hwndImgList, hwndPreview, hwndRuleList;
    HWND hwndImgCount, hwndPreviewName, hwndPreviewDim, hwndRuleCount;
    HWND hwndPreviewCodec;              /* v1.3.11：「GDI+ 解码」芯片（有图才显示） */
    HWND hwndRulePath, hwndRuleImg, hwndClsid;
    HWND hwndSummaryLabels[9];

    WCHAR last_refresh_time[32];

    WCHAR preview_path[MAX_PATH_W];
    HBITMAP hbm_preview;

    /* 图片张数缓存：避免拖动滑块/切开关时反复做磁盘扫描 */
    int    img_count_cached;
    bool   img_count_valid;

    HFONT font_ui;
    HFONT font_mono;
    HFONT font_bold;
    HFONT font_chip;        /* 等宽小字（约 10.5px），key 芯片 / 摘要键名 */

    bool busy;
    HANDLE hMutex;
    ULONG_PTR gdiplusToken;
} AppState;

static AppState g_app;

/* 缩放选项 */
static const WCHAR *SCALE_CHOICES[] = { L"Auto", L"70%", L"80%", L"90%", L"100%", L"110%", L"125%", L"150%" };
static const double SCALE_FACTORS[] = { 1.0, 0.70, 0.80, 0.90, 1.0, 1.10, 1.25, 1.50 };

/* ------------------------------------------------------------------ */
/* 基础工具函数                                                        */
/* ------------------------------------------------------------------ */
static int sfmt(WCHAR *buf, size_t n, const WCHAR *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = _vsnwprintf(buf, n, fmt, ap);
    va_end(ap);
    /* _vsnwprintf 截断时不保证 NUL，手动补 */
    if (n > 0) buf[n - 1] = 0;
    return r;
}

static void ensure_dir(const WCHAR *path) {
    if (!CreateDirectoryW(path, NULL)) {
        DWORD e = GetLastError();
        if (e != ERROR_ALREADY_EXISTS) {
            /* ignore */
        }
    }
}

/* 安全 UTF-16 -> UTF-8：缓冲不足时截断并强制补 NUL（WideCharToMultiByte 自身不保证） */
static int wtou8(const WCHAR *src, char *dst, size_t dst_n) {
    if (!dst || dst_n == 0) return 0;
    dst[0] = 0;
    if (!src) return 0;
    int nb = WideCharToMultiByte(CP_UTF8, 0, src, -1, dst, (int)dst_n, NULL, NULL);
    if (nb > 0) {
        if ((size_t)nb < dst_n) return nb - 1;   /* 成功，返回不含 NUL 的字节数 */
        /* 恰好填满：最后一个字节被写成 NUL 了吗？保守处理，强制末尾归零 */
    }
    dst[dst_n - 1] = 0;
    return (int)strlen(dst);
}

/* 安全 UTF-8 -> UTF-16：同上，dst_n 以 WCHAR 计 */
static int u8tow(const char *src, WCHAR *dst, size_t dst_n) {
    if (!dst || dst_n == 0) return 0;
    dst[0] = 0;
    if (!src) return 0;
    int n = MultiByteToWideChar(CP_UTF8, 0, src, -1, dst, (int)dst_n);
    if (n > 0 && (size_t)n < dst_n) return n - 1;
    dst[dst_n - 1] = 0;
    return (int)wcslen(dst);
}

static void path_join(WCHAR *out, size_t n, const WCHAR *a, const WCHAR *b) {
    if (!out || n == 0) return;
    out[0] = 0;
    if (!a || !a[0]) { if (b) sfmt(out, n, L"%s", b); return; }
    if (!b || !b[0]) { sfmt(out, n, L"%s", a); return; }
    size_t al = wcslen(a);
    sfmt(out, n, L"%s%s%s", a,
         (a[al-1] == L'\\' || a[al-1] == L'/') ? L"" : L"\\", b);
}

static bool file_exists(const WCHAR *path) {
    DWORD a = GetFileAttributesW(path);
    return (a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY));
}

static bool dir_exists(const WCHAR *path) {
    DWORD a = GetFileAttributesW(path);
    return (a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY));
}

static int64_t file_size(const WCHAR *path) {
    WIN32_FILE_ATTRIBUTE_DATA fa;
    if (!GetFileAttributesExW(path, GetFileExInfoStandard, &fa)) return -1;
    LARGE_INTEGER li; li.LowPart = fa.nFileSizeLow; li.HighPart = fa.nFileSizeHigh;
    return li.QuadPart;
}

static void get_exe_dir(WCHAR *out, size_t n) {
    GetModuleFileNameW(NULL, out, (DWORD)n);
    WCHAR *p = wcsrchr(out, L'\\');
    if (p) *(p + 1) = L'\0';
}

static void read_pref_file(const WCHAR *name, WCHAR *out, size_t n) {
    out[0] = 0;
    WCHAR path[MAX_PATH_W];
    WCHAR prof[MAX_PATH_W];
    DWORD r = GetEnvironmentVariableW(L"USERPROFILE", prof, MAX_PATH_W);
    if (!r) return;
    path_join(path, MAX_PATH_W, prof, L".backimg_gui");
    ensure_dir(path);
    path_join(path, MAX_PATH_W, path, name);
    HANDLE h = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) return;
    char buf[512];
    DWORD rd = 0;
    if (!ReadFile(h, buf, sizeof(buf)-1, &rd, NULL)) rd = 0;
    CloseHandle(h);
    buf[rd] = 0;
    u8tow(buf, out, n);
}

static void write_pref_file(const WCHAR *name, const WCHAR *value) {
    WCHAR path[MAX_PATH_W];
    WCHAR prof[MAX_PATH_W];
    if (!GetEnvironmentVariableW(L"USERPROFILE", prof, MAX_PATH_W)) return;
    path_join(path, MAX_PATH_W, prof, L".backimg_gui");
    ensure_dir(path);
    path_join(path, MAX_PATH_W, path, name);
    char buf[512];
    int nb = wtou8(value ? value : L"", buf, sizeof(buf));
    HANDLE h = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;
    DWORD wr;
    WriteFile(h, buf, (DWORD)nb, &wr, NULL);
    CloseHandle(h);
}

/* ------------------------------------------------------------------ */
/* SHA256（仅用于 DLL 完整性校验，使用 Windows Crypt API）             */
/* ------------------------------------------------------------------ */
static bool sha256_file(const WCHAR *path, char hex[65]) {
    hex[0] = 0;
    HCRYPTPROV prov = 0; HCRYPTHASH hash = 0; HANDLE h = INVALID_HANDLE_VALUE;
    if (!CryptAcquireContextW(&prov, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) return false;
    if (!CryptCreateHash(prov, CALG_SHA_256, 0, 0, &hash)) goto fail;
    h = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) goto fail;
    BYTE buf[65536]; DWORD rd;
    while (ReadFile(h, buf, sizeof(buf), &rd, NULL) && rd > 0) {
        CryptHashData(hash, buf, rd, 0);
    }
    CloseHandle(h); h = INVALID_HANDLE_VALUE;
    BYTE hashb[32]; DWORD len = 32;
    if (!CryptGetHashParam(hash, HP_HASHVAL, hashb, &len, 0)) goto fail;
    for (int i = 0; i < 32; i++) sprintf(hex + i*2, "%02x", hashb[i]);
    hex[64] = 0;
    CryptDestroyHash(hash);
    CryptReleaseContext(prov, 0);
    return true;
fail:
    if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
    if (hash) CryptDestroyHash(hash);
    if (prov) CryptReleaseContext(prov, 0);
    return false;
}

/* v1.3.12：内存版 SHA256 —— 用于比对 EXE 内置资源里的 DLL 与 plugin 目录 DLL */
static bool sha256_mem(const BYTE *data, size_t n, char hex[65]) {
    hex[0] = 0;
    HCRYPTPROV prov = 0; HCRYPTHASH hash = 0;
    if (!CryptAcquireContextW(&prov, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) return false;
    if (!CryptCreateHash(prov, CALG_SHA_256, 0, 0, &hash)) { CryptReleaseContext(prov, 0); return false; }
    bool ok = true;
    /* 分块哈希，避免超长数据一次提交（CryptHashData 本身无长度限制，保守分块） */
    while (n > 0) {
        DWORD chunk = n > 65536 ? 65536 : (DWORD)n;
        if (!CryptHashData(hash, data, chunk, 0)) { ok = false; break; }
        data += chunk; n -= chunk;
    }
    if (ok) {
        BYTE hashb[32]; DWORD len = 32;
        if (CryptGetHashParam(hash, HP_HASHVAL, hashb, &len, 0)) {
            for (int i = 0; i < 32; i++) sprintf(hex + i*2, "%02x", hashb[i]);
            hex[64] = 0;
        } else ok = false;
    }
    CryptDestroyHash(hash);
    CryptReleaseContext(prov, 0);
    return ok;
}

/* ------------------------------------------------------------------ */
/* 日志                                                                */
/* ------------------------------------------------------------------ */
static void log_append(const WCHAR *msg, int level);
static void on_slider_change(void);
static int  count_images_current(void);          /* 前向声明：状态条绘制也要用 */
static void list_clear_with_param(HWND lv);      /* 前向声明：两个列表页都要用 */

/* 日志缓冲（窗口重建/缩放后用于回填；同时限制界面日志条数） */
#define LOG_BUF_MAX     300
static WCHAR g_log_buf[LOG_BUF_MAX][512];
static int   g_log_lvl[LOG_BUF_MAX];
static int   g_log_count;   /* 当前条数（达到上限后滚动丢弃最早一条） */

static void log_buf_push(const WCHAR *line, int level) {
    if (g_log_count < LOG_BUF_MAX) {
        wcsncpy(g_log_buf[g_log_count], line, 511);
        g_log_buf[g_log_count][511] = 0;
        g_log_lvl[g_log_count] = level;
        g_log_count++;
    } else {
        memmove(g_log_buf, g_log_buf + 1, sizeof(g_log_buf[0]) * (LOG_BUF_MAX - 1));
        memmove(g_log_lvl, g_log_lvl + 1, sizeof(g_log_lvl[0]) * (LOG_BUF_MAX - 1));
        wcsncpy(g_log_buf[LOG_BUF_MAX - 1], line, 511);
        g_log_buf[LOG_BUF_MAX - 1][511] = 0;
        g_log_lvl[LOG_BUF_MAX - 1] = level;
    }
}

static void log_view_add_single(const WCHAR *line, int level) {
    if (!g_app.hwndLog) return;
    int idx = (int)SendMessageW(g_app.hwndLog, LB_ADDSTRING, 0, (LPARAM)line);
    if (idx >= 0) SendMessageW(g_app.hwndLog, LB_SETITEMDATA, idx, (LPARAM)level);
    /* 自动滚到末尾 */
    int cnt = (int)SendMessageW(g_app.hwndLog, LB_GETCOUNT, 0, 0);
    RECT rc; GetClientRect(g_app.hwndLog, &rc);
    int ih = (int)SendMessageW(g_app.hwndLog, LB_GETITEMHEIGHT, 0, 0);
    int vis = ih > 0 ? (rc.bottom - rc.top) / ih : 1;
    int top = cnt - vis; if (top < 0) top = 0;
    SendMessageW(g_app.hwndLog, LB_SETTOPINDEX, top, 0);
}

/* v1.3.12：日志内容里可能带 \r\n（如 browse_plugin 的 4 行状态信息），
   LISTBOX 一项一行、TextOutW 不认控制字符——不拆分会挤成一行还出怪字符。
   这里按 \n 拆分逐条入列表（空行跳过），缓冲/磁盘日志不受影响 */
static void log_view_add(const WCHAR *line, int level) {
    if (!line) return;
    const WCHAR *p = line;
    for (;;) {
        const WCHAR *nl = wcschr(p, L'\n');
        if (!nl) { log_view_add_single(p, level); break; }
        size_t len = (size_t)(nl - p);
        if (len > 0 && p[len-1] == L'\r') len--;      /* 去掉 \r\n 的 \r */
        if (len > 0) {
            WCHAR seg[2048];
            if (len >= 2048) len = 2047;
            memcpy(seg, p, len * sizeof(WCHAR));
            seg[len] = 0;
            log_view_add_single(seg, level);
        }
        p = nl + 1;
    }
}

static void log_view_restore(void) {
    if (!g_app.hwndLog) return;
    SendMessageW(g_app.hwndLog, LB_RESETCONTENT, 0, 0);
    for (int i = 0; i < g_log_count; i++) log_view_add(g_log_buf[i], g_log_lvl[i]);
}

static void log_msg(int level, const WCHAR *msg) {
    SYSTEMTIME st; GetLocalTime(&st);
    WCHAR line[2048];
    WCHAR prefix[32];
    const WCHAR *tag = L"";
    if (level == 1) tag = L"[Error] ";
    else if (level == 2) tag = L"[Warn] ";
    else if (level == 3) tag = L"[Info] ";
    sfmt(prefix, 32, L"[%02d:%02d:%02d]", st.wHour, st.wMinute, st.wSecond);
    sfmt(line, 2048, L"%s %s%s", prefix, tag, msg);
    log_append(line, level);
}

static void log_append(const WCHAR *msg, int level) {
    /* 界面日志（自绘着色列表） */
    log_buf_push(msg, level);
    log_view_add(msg, level);

    /* 持久化日志 */
    WCHAR logpath[MAX_PATH_W];
    path_join(logpath, MAX_PATH_W, g_app.plugin_dir, GUI_LOG_NAME);
    HANDLE h = CreateFileW(logpath, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER sz; GetFileSizeEx(h, &sz);
        if (sz.QuadPart > 200 * 1024) {
            CloseHandle(h);
            WCHAR old[MAX_PATH_W]; sfmt(old, MAX_PATH_W, L"%s.old", logpath);
            DeleteFileW(old);
            MoveFileW(logpath, old);
            h = CreateFileW(logpath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        }
        SetFilePointer(h, 0, NULL, FILE_END);
        SYSTEMTIME st; GetLocalTime(&st);
        char stamp[64];
        sprintf(stamp, "%04d-%02d-%02d %02d:%02d:%02d  ", st.wYear, st.wMonth, st.wDay,
                st.wHour, st.wMinute, st.wSecond);
        int prefix_len = (int)strlen(stamp);

        /* 复制 msg 并压平换行，确保 NUL 结尾；数组整体清零避免越界读取 */
        WCHAR flat[2048] = {0};
        size_t fl = wcslen(msg); if (fl >= 2048) fl = 2047;
        memcpy(flat, msg, fl * sizeof(WCHAR));
        flat[fl] = 0;
        for (size_t i = 0; i < fl; i++) if (flat[i] == L'\r' || flat[i] == L'\n') flat[i] = L' ';

        /* 按实际需要的字节数动态分配：
           旧实现用固定 2300 字节的缓冲，一条中文日志 UTF-8 可达 6KB，
           超长时 WideCharToMultiByte 直接失败，磁盘日志整条丢失（v1.3.1 修复）。 */
        int need = WideCharToMultiByte(CP_UTF8, 0, flat, (int)(fl + 1), NULL, 0, NULL, NULL);
        if (need <= 0) need = (int)fl + 1;
        char *line = (char*)malloc((size_t)prefix_len + (size_t)need + 8);
        if (!line) { CloseHandle(h); return; }
        memcpy(line, stamp, (size_t)prefix_len);
        int nb = WideCharToMultiByte(CP_UTF8, 0, flat, (int)(fl + 1),
                                     line + prefix_len, need, NULL, NULL);
        DWORD wr;
        if (nb > 0) {
            int write_len = prefix_len + nb - 1;   /* 返回值含结尾 NUL */
            if (write_len > prefix_len)
                WriteFile(h, line, (DWORD)write_len, &wr, NULL);
            else
                WriteFile(h, line, (DWORD)prefix_len, &wr, NULL);
        } else {
            WriteFile(h, line, (DWORD)prefix_len, &wr, NULL);
            char dbg[64]; sprintf(dbg, "[WCTMB=%lu]", GetLastError());
            WriteFile(h, dbg, (DWORD)strlen(dbg), &wr, NULL);
        }
        WriteFile(h, "\r\n", 2, &wr, NULL);
        free(line);
        CloseHandle(h);
    }
}

#define log_info(s)  log_msg(0, (s))
#define log_warn(s)  log_msg(2, (s))
#define log_err(s)   log_msg(1, (s))

/* ------------------------------------------------------------------ */
/* INI 读写（保留注释与格式）                                           */
/* ------------------------------------------------------------------ */
/*
 * 读文本文件为 UTF-16。
 * 关键：MultiByteToWideChar 在传入“显式长度”时【不会】补结尾 NUL，
 * 旧实现因此返回一个未终止的缓冲区，后续 wcslen 全部越界读（v1.3.1 修复）。
 */
static WCHAR *read_text_file(const WCHAR *path) {
    HANDLE h = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) return NULL;
    LARGE_INTEGER sz; GetFileSizeEx(h, &sz);
    if (sz.QuadPart <= 0) { CloseHandle(h); return NULL; }
    if (sz.QuadPart > 64 * 1024 * 1024) { CloseHandle(h); return NULL; }  /* 异常大文件保护 */
    char *raw = (char*)malloc((size_t)sz.QuadPart + 4);
    if (!raw) { CloseHandle(h); return NULL; }
    DWORD rd = 0;
    if (!ReadFile(h, raw, (DWORD)sz.QuadPart, &rd, NULL)) rd = 0;
    CloseHandle(h);
    raw[rd] = 0;

    int cp = CP_UTF8;
    const char *src = raw;
    int src_len = -1;
    /* 跳过 UTF-8 BOM */
    if (rd >= 3 && (unsigned char)raw[0] == 0xEF &&
        (unsigned char)raw[1] == 0xBB && (unsigned char)raw[2] == 0xBF) {
        src = raw + 3;
        src_len = (int)rd - 3;
    }
    int wc = MultiByteToWideChar(cp, 0, src, src_len, NULL, 0);
    if (wc <= 0) {
        wc = MultiByteToWideChar(CP_ACP, 0, src, src_len, NULL, 0);
        cp = CP_ACP;
    }
    if (wc <= 0) { free(raw); return NULL; }
    /* 多分配 2 个 WCHAR，显式补结尾 NUL */
    WCHAR *out = (WCHAR*)malloc(((size_t)wc + 2) * sizeof(WCHAR));
    if (!out) { free(raw); return NULL; }
    int written = MultiByteToWideChar(cp, 0, src, src_len, out, wc);
    if (written < 0) written = 0;
    out[written] = 0;
    out[(size_t)wc + 1] = 0;
    free(raw);
    return out;
}

/* v1.3.12：返回 bool——config.ini 只读/被占用时CreateFile/WriteFile 失败不再静默 */
static bool write_text_file(const WCHAR *path, const WCHAR *text) {
    HANDLE h = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return false;
    /* 写 UTF-8 BOM */
    BYTE bom[3] = { 0xEF, 0xBB, 0xBF };
    DWORD wr; bool ok = WriteFile(h, bom, 3, &wr, NULL);
    int nb = ok ? WideCharToMultiByte(CP_UTF8, 0, text, -1, NULL, 0, NULL, NULL) : 0;
    char *buf = nb > 0 ? (char*)malloc(nb) : NULL;
    if (ok && buf) {
        WideCharToMultiByte(CP_UTF8, 0, text, -1, buf, nb, NULL, NULL);
        ok = WriteFile(h, buf, nb - 1, &wr, NULL); /* 不含结尾 NUL */
        if (ok && wr != (DWORD)(nb - 1)) ok = false;   /* 磁盘满等写一半的情况 */
    } else {
        ok = false;
    }
    free(buf);
    CloseHandle(h);
    return ok;
}

/* 去掉一行两端的空白（INI 里常见 "key = value" 写法） */
static WCHAR *trim_w(WCHAR *s) {
    while (*s == L' ' || *s == L'\t' || *s == L'\r' || *s == L'\n') s++;
    WCHAR *end = s + wcslen(s);
    while (end > s && (end[-1] == L' ' || end[-1] == L'\t' || end[-1] == L'\r' || end[-1] == L'\n')) end--;
    *end = 0;
    return s;
}

static void ini_read_value(const WCHAR *text, const char *section, const char *key,
                           char *out, size_t out_n, const char *default_val) {
    if (!out || out_n == 0) return;
    strncpy(out, default_val ? default_val : "", out_n-1); out[out_n-1] = 0;
    if (!text) return;
    const WCHAR *p = text;
    bool in_sec = false;
    WCHAR line[4096];
    while (*p) {
        const WCHAR *e = p;
        while (*e && *e != L'\n' && *e != L'\r') e++;
        size_t ll = e - p; if (ll >= 4095) ll = 4095;
        memcpy(line, p, ll * sizeof(WCHAR)); line[ll] = 0;
        WCHAR *body = trim_w(line);
        /* 注释：# 与 ; 都支持 */
        if (body[0] == L'#' || body[0] == L';') { p = e; if (*p == L'\r') p++; if (*p == L'\n') p++; continue; }
        size_t bl = wcslen(body);
        if (bl >= 2 && body[0] == L'[' && body[bl-1] == L']') {
            body[bl-1] = 0;
            /* v1.3.2：section 名缓冲放大到 4096（长路径区块不再截断误判） */
            char sec[4096];
            wtou8(trim_w(body + 1), sec, sizeof(sec));
            in_sec = (_stricmp(sec, section) == 0);
        } else if (in_sec) {
            WCHAR *eq = wcschr(body, L'=');
            if (eq) {
                *eq = 0;
                char k[256];
                wtou8(trim_w(body), k, sizeof(k));
                if (_stricmp(k, key) == 0) {
                    wtou8(trim_w(eq + 1), out, out_n);
                    return;
                }
            }
        }
        p = e; if (*p == L'\r') p++; if (*p == L'\n') p++;
    }
}

/* 构造 "prefix + key=value + eol" 一行（动态分配，避免长路径在固定缓冲里被截断）
 * v1.3.14：原用 %S 转换 char*——msvcrt 的 %S 按 ACP(GBK) 而非 UTF-8 解释，
 * 中文路径写入即成 mojibake。改显式 u8tow（UTF-8 -> UTF-16）后再拼接 */
static WCHAR *mk_kv_line(const char *key, const char *value, const WCHAR *prefix, const WCHAR *eol) {
    size_t vlen = strlen(value);
    WCHAR *vw = (WCHAR*)malloc((vlen + 1) * 2 * sizeof(WCHAR));
    if (!vw) return NULL;
    u8tow(value, vw, (vlen + 1) * 2);
    WCHAR kw[512];
    u8tow(key, kw, 512);
    size_t cap = wcslen(kw) + wcslen(vw) +
                 (prefix ? wcslen(prefix) : 0) + (eol ? wcslen(eol) : 0) + 16;
    WCHAR *buf = (WCHAR*)malloc(cap * sizeof(WCHAR));
    if (!buf) { free(vw); return NULL; }
    sfmt(buf, cap, L"%s%s=%s%s", prefix ? prefix : L"", kw, vw, eol ? eol : L"");
    free(vw);
    return buf;
}

static void ini_write_value(WCHAR **ptext, const char *section, const char *key, const char *value) {
    WCHAR *text = *ptext;

    /* 构建新文本 */
    /* v1.3.14【重大堆越界修复】：原 malloc(32) 只分配 32【字节】(16 个 WCHAR)，
       但 out_cap=32 按【字符数】用——文本超 16 字符后 memcpy 越界写堆：
       砸坏堆元数据→保存时崩溃("kill on hang")；砸中相邻块→其它数据被拼进
       宽串写进 ini(mojibake/内容重复)。v1.3.0 起潜伏，属字节/字符单位混淆。 */
    WCHAR *out = (WCHAR*)malloc(64 * sizeof(WCHAR));
    if (!out) return;
    out[0] = 0;
    size_t out_cap = 64, out_len = 0;

#define APPENDW(s) do { \
        size_t L = wcslen(s); \
        if (out_len + L + 2 > out_cap) { \
            size_t nc = (out_len + L + 2) * 2; \
            WCHAR *tmp = (WCHAR*)realloc(out, nc * sizeof(WCHAR)); \
            if (!tmp) { free(out); return; } \
            out = tmp; out_cap = nc; \
        } \
        memcpy(out + out_len, s, L * sizeof(WCHAR)); out_len += L; \
    } while(0)

    bool in_sec = false;
    bool replaced = false;
    bool sec_exists = false;
    int sec_start_line = -1;
    /* 行缓冲放宽到 4096：长路径的 [路径] 区块若被截断会整块丢失（v1.3.1 修复） */
    WCHAR line[4096];
    const WCHAR *p = text ? text : L"";

    /* 先扫描是否 section 存在 */
    const WCHAR *q = p;
    while (*q) {
        const WCHAR *e = q;
        while (*e && *e != L'\n' && *e != L'\r') e++;
        size_t ll = e - q; if (ll >= 4095) ll = 4095;
        memcpy(line, q, ll * sizeof(WCHAR)); line[ll] = 0;
        if (ll >= 2 && line[0] == L'[' && line[ll-1] == L']') {
            line[ll-1] = 0;
            char sec[4096]; wtou8(trim_w(line + 1), sec, sizeof(sec));
            if (_stricmp(sec, section) == 0) sec_exists = true;
        }
        q = e; if (*q == L'\r') q++; if (*q == L'\n') q++;
    }

    p = text ? text : L"";
    int line_no = 0;
    while (*p) {
        const WCHAR *e = p;
        const WCHAR *line_start = p;
        while (*e && *e != L'\n' && *e != L'\r') e++;
        size_t ll = e - p; if (ll >= 4095) ll = 4095;
        memcpy(line, p, ll * sizeof(WCHAR)); line[ll] = 0;
        WCHAR eol[4] = {0}; const WCHAR *ep = e;
        if (*ep == L'\r') { wcscat(eol, L"\r"); ep++; }
        if (*ep == L'\n') { wcscat(eol, L"\n"); ep++; }

        if (ll >= 2 && line[0] == L'[' && line[ll-1] == L']') {
            /* 离开 section 时如果还没替换，在 section 末尾插入 */
            if (in_sec && !replaced && sec_start_line >= 0) {
                WCHAR *nl = mk_kv_line(key, value, L"", eol);
                if (nl) { APPENDW(nl); free(nl); }
                replaced = true;
            }
            /* v1.3.14【Bug A 修复】：原代码就地 line[ll-1]=0 截断取节名后，
               落到底部 APPENDW(line) 把截断行（丢了 ']'）写进输出——
               重建一次所有 [section] 变 [section，后续 sec_exists 永远
               判 false，每次保存把 7 个键整组追加到文件尾（ini 越存越大）。
               改为在局部副本上取节名，原行完整 append */
            WCHAR secw[4096];
            memcpy(secw, line + 1, (ll - 2) * sizeof(WCHAR)); secw[ll-2] = 0;
            char sec[4096]; wtou8(trim_w(secw), sec, sizeof(sec));
            in_sec = (_stricmp(sec, section) == 0);
            if (in_sec) sec_start_line = line_no;
        } else if (in_sec && !replaced) {
            /* v1.3.14【Bug B 修复】：原代码 *eq=0 截断后比较键名，键名不匹配时
               从不恢复，落到底部把截断行（丢 '=值'）写进输出——同节内排在
               目标键之前的键全部丢值。改为在局部副本上比较，原行不动 */
            WCHAR tmpw[4096];
            memcpy(tmpw, line, (ll + 1) * sizeof(WCHAR));
            WCHAR *eq = wcschr(tmpw, L'=');
            if (eq) {
                *eq = 0;
                char k[256]; wtou8(trim_w(tmpw), k, sizeof(k));
                if (_stricmp(k, key) == 0) {
                    /* 保留原行缩进 */
                    int indent = 0;
                    while (line_start[indent] == L' ' || line_start[indent] == L'\t') indent++;
                    WCHAR prefix[256];
                    if (indent >= 255) indent = 255;
                    memcpy(prefix, line_start, indent * sizeof(WCHAR)); prefix[indent] = 0;
                    WCHAR *nl = mk_kv_line(key, value, prefix, eol);
                    if (nl) { APPENDW(nl); free(nl); }
                    replaced = true;
                    p = ep; line_no++; continue;
                }
            }
        }
        APPENDW(line); APPENDW(eol);
        p = ep; line_no++;
    }

    if (in_sec && !replaced) {
        /* v1.3.14【Bug D 修复】：追加行尾统一 CRLF（原 L"\n" 产生 LF 混排行尾） */
        WCHAR *nl = mk_kv_line(key, value, L"", L"\r\n");
        if (nl) { APPENDW(nl); free(nl); }
        replaced = true;
    }

    if (!sec_exists) {
        if (out_len > 0 && out[out_len-1] != L'\n') APPENDW(L"\r\n");
        /* section 头 + 首行也动态构造，value 可能是很长的路径（v1.3.14：节名可能
           是中文文件夹路径，同样弃用 %S 改显式 u8tow） */
        WCHAR secw[4096];
        u8tow(section, secw, 4096);
        WCHAR *nl = mk_kv_line(key, value, L"", L"\r\n");
        if (nl) {
            size_t cap = wcslen(secw) + wcslen(nl) + 16;
            WCHAR *secheader = (WCHAR*)malloc(cap * sizeof(WCHAR));
            if (secheader) {
                sfmt(secheader, cap, L"\r\n[%s]\r\n%s", secw, nl);
                APPENDW(secheader);
                free(secheader);
            }
            free(nl);
        }
        replaced = true;
    }

    out[out_len] = 0;
    free(text);
    *ptext = out;
#undef APPENDW
}

static WCHAR *g_ini_text = NULL;

/*
 * 读取键值到调用者缓冲。
 * 旧实现返回 static 缓冲，连续两次调用会让第一个指针失效；v1.3.1 改为调用者提供缓冲。
 */
static void ini_get_buf(const char *section, const char *key, const char *def,
                        char *out, size_t out_n) {
    ini_read_value(g_ini_text, section, key, out, out_n, def);
}

static void ini_set(const char *section, const char *key, const char *value) {
    ini_write_value(&g_ini_text, section, key, value);
}

static void ini_reload(void) {
    if (g_ini_text) { free(g_ini_text); g_ini_text = NULL; }
    g_ini_text = read_text_file(g_app.ini_path);
}

/* v1.3.12：返回写入是否成功（config.ini 只读/被占用时上层要能感知） */
static bool ini_commit(void) {
    if (!g_ini_text) return false;
    return write_text_file(g_app.ini_path, g_ini_text);
}

static WCHAR **list_folder_sections(int *count) {
    *count = 0;
    if (!g_ini_text) return NULL;
    int cap = 16;
    WCHAR **arr = (WCHAR**)malloc(cap * sizeof(WCHAR*));
    if (!arr) return NULL;
    const WCHAR *p = g_ini_text;
    WCHAR line[4096];
    while (*p) {
        const WCHAR *e = p;
        while (*e && *e != L'\n' && *e != L'\r') e++;
        size_t ll = e - p; if (ll >= 4095) ll = 4095;
        memcpy(line, p, ll * sizeof(WCHAR)); line[ll] = 0;
        if (ll >= 2 && line[0] == L'[' && line[ll-1] == L']') {
            line[ll-1] = 0;
            char sec[4096]; wtou8(trim_w(line + 1), sec, sizeof(sec));
            if (_stricmp(sec, "load") != 0 && _stricmp(sec, "image") != 0) {
                /* v1.3.2：realloc 失败时保留已收集部分并退出，不再丢原指针（泄漏） */
                if (*count >= cap) {
                    int ncap = cap * 2;
                    WCHAR **tmp = (WCHAR**)realloc(arr, (size_t)ncap * sizeof(WCHAR*));
                    if (!tmp) break;
                    arr = tmp; cap = ncap;
                }
                WCHAR *dup = _wcsdup(line+1);
                if (!dup) break;
                arr[*count] = dup;
                (*count)++;
            }
        }
        p = e; if (*p == L'\r') p++; if (*p == L'\n') p++;
    }
    return arr;
}

static void remove_folder_section(const char *section) {
    if (!g_ini_text) return;
    /* v1.3.14：同 ini_write_value 的堆越界——malloc(32) 字节配 out_cap=32 字符 */
    /* v1.3.23(K2，Kimi 审计)：初始 malloc 补判空——此前 out[0]=0 在判空前执行，
     * 内存不足时同样空指针写入（与 APPENDW2 同源，报告未单列，一并修） */
    WCHAR *out = (WCHAR*)malloc(64 * sizeof(WCHAR));
    if (!out) return;               /* 失败即放弃：g_ini_text 原样保留，规则不删 */
    out[0] = 0; size_t out_cap = 64, out_len = 0;
#define APPENDW2(s) do { \
        size_t L = wcslen(s); \
        if (out_len + L + 2 > out_cap) { \
            /* v1.3.23(K1，Kimi 审计 P2)：realloc 返回值必须经临时指针判空——
             * 直接 out = realloc(out,...) 失败时原块泄漏且随后 memcpy(NULL) 崩溃。
             * 写法对齐 ini_write_value 的 APPENDW；失败即 free 并返回，
             * 此时 g_ini_text 尚未释放，ini 内容原样保留（规则不删、无半成品） */ \
            size_t nc = (out_len + L + 2) * 2; \
            WCHAR *tmp = (WCHAR*)realloc(out, nc * sizeof(WCHAR)); \
            if (!tmp) { free(out); return; } \
            out = tmp; out_cap = nc; \
        } \
        memcpy(out + out_len, s, L * sizeof(WCHAR)); out_len += L; \
    } while(0)
    const WCHAR *p = g_ini_text;
    bool in_sec = false;
    WCHAR line[4096];
    while (*p) {
        const WCHAR *e = p;
        while (*e && *e != L'\n' && *e != L'\r') e++;
        size_t ll = e - p; if (ll >= 4095) ll = 4095;
        memcpy(line, p, ll * sizeof(WCHAR)); line[ll] = 0;
        WCHAR eol[4] = {0}; const WCHAR *ep = e;
        if (*ep == L'\r') { wcscat(eol, L"\r"); ep++; }
        if (*ep == L'\n') { wcscat(eol, L"\n"); ep++; }
        if (ll >= 2 && line[0] == L'[' && line[ll-1] == L']') {
            /* v1.3.14：同 ini_write_value Bug A——就地截断取节名后，非目标节
               的 ']' 会随 APPENDW2 丢失。改为局部副本上取节名，原行不动 */
            WCHAR secw[4096];
            memcpy(secw, line + 1, (ll - 2) * sizeof(WCHAR)); secw[ll-2] = 0;
            /* v1.3.3：与 list_folder_sections 一致放宽到 4096，超长路径也能精确判 section */
            char sec[4096]; wtou8(trim_w(secw), sec, sizeof(sec));
            in_sec = (_stricmp(sec, section) == 0);
        }
        if (!in_sec) { APPENDW2(line); APPENDW2(eol); }
        p = ep;
    }
    out[out_len] = 0;
    /* 折叠空行 */
    /* v1.3.23(K2，Kimi 审计)：malloc 判空——失败即放弃，g_ini_text 原样保留 */
    WCHAR *folded = (WCHAR*)malloc((out_len + 4) * sizeof(WCHAR));
    if (!folded) { free(out); return; }
    int j = 0, nl = 0;
    for (int i = 0; out[i]; i++) {
        if (out[i] == L'\n') {
            nl++;
            if (nl <= 2) folded[j++] = out[i];
        } else {
            nl = (out[i] == L'\r') ? nl : 0;
            folded[j++] = out[i];
        }
    }
    /* 去掉首尾空行 */
    int s = 0; while (s < j && (folded[s] == L'\n' || folded[s] == L'\r')) s++;
    int e = j - 1; while (e >= s && (folded[e] == L'\n' || folded[e] == L'\r')) e--;
    int k = 0;
    for (int i = s; i <= e; i++) folded[k++] = folded[i];
    folded[k++] = L'\n'; folded[k] = 0;
    free(out);
    free(g_ini_text); g_ini_text = folded;
#undef APPENDW2
}

/* ------------------------------------------------------------------ */
/* 资源解包（仅缺失时写入）                                            */
/* ------------------------------------------------------------------ */
static bool extract_resource(int id, const WCHAR *dst) {
    HRSRC hr = FindResourceW(NULL, MAKEINTRESOURCEW(id), (LPCWSTR)RT_RCDATA);
    if (!hr) return false;
    HGLOBAL hg = LoadResource(NULL, hr);
    if (!hg) return false;
    DWORD sz = SizeofResource(NULL, hr);
    void *data = LockResource(hg);
    if (!data || !sz) return false;
    HANDLE h = CreateFileW(dst, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        if (GetLastError() == ERROR_FILE_EXISTS) return true;
        return false;
    }
    DWORD wr; WriteFile(h, data, sz, &wr, NULL);
    CloseHandle(h);
    return true;
}

static void ensure_self_contained(void) {
    path_join(g_app.plugin_dir, MAX_PATH_W, g_app.exe_dir, L"plugin");
    ensure_dir(g_app.plugin_dir);
    WCHAR dll[MAX_PATH_W], ini[MAX_PATH_W];
    path_join(dll, MAX_PATH_W, g_app.plugin_dir, DLL_NAME);
    path_join(ini, MAX_PATH_W, g_app.plugin_dir, INI_NAME);
    if (!extract_resource(IDR_DLL, dll)) log_warn(L"Failed to unpack ExplorerBgTool.dll (already exists or folder not writable)");
    if (!extract_resource(IDR_INI, ini)) log_warn(L"Failed to unpack config.ini (already exists or folder not writable)");
    /* app.ico 由 EXE 资源直接加载，无需落地 */
}

/* ------------------------------------------------------------------ */
/* 注册表检测（只读）                                                  */
/* ------------------------------------------------------------------ */
static void detect_registered(void) {
    g_app.registered = false;
    g_app.reg_dll_path_count = 0;
    for (int i = 0; i < 4; i++) g_app.reg_detail[i][0] = 0;

    HKEY hKey;
    LONG r;
    WCHAR path[MAX_PATH_W];
    DWORD type, size;

    /* ① HKCR\CLSID -> 实际路径 HKLM/HKCU SOFTWARE\Classes\CLSID */
    bool clsid_found = false;
    const HKEY clsid_hives[] = { HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER };
    const WCHAR *clsid_hive_names[] = { L"HKLM", L"HKCU" };
    for (int i = 0; i < 2 && !clsid_found; i++) {
        sfmt(path, MAX_PATH_W, L"SOFTWARE\\Classes\\CLSID\\%s", CLSID_STR_W);
        r = RegOpenKeyExW(clsid_hives[i], path, 0, KEY_READ, &hKey);
        if (r == ERROR_SUCCESS) {
            clsid_found = true;
            HKEY hInproc;
            r = RegOpenKeyExW(hKey, L"InProcServer32", 0, KEY_READ, &hInproc);
            if (r == ERROR_SUCCESS) {
                /* 预留一个 WCHAR 给结尾 NUL，避免值刚好填满缓冲时无终止符 */
                size = sizeof(path) - sizeof(WCHAR);
                path[0] = 0;
                r = RegQueryValueExW(hInproc, NULL, NULL, &type, (BYTE*)path, &size);
                path[MAX_PATH_W - 1] = 0;
                if (r == ERROR_SUCCESS && (type == REG_SZ || type == REG_EXPAND_SZ)) {
                    g_app.registered = true;
                    if (g_app.reg_dll_path_count < 4)
                        wcsncpy(g_app.reg_dll_paths[g_app.reg_dll_path_count++], path, MAX_PATH_W-1);
                    WCHAR tm[64]; size = sizeof(tm) - sizeof(WCHAR); tm[0] = 0;
                    r = RegQueryValueExW(hInproc, L"ThreadingModel", NULL, &type, (BYTE*)tm, &size);
                    tm[63] = 0;
                    bool tm_ok = (r == ERROR_SUCCESS && type == REG_SZ &&
                                  (_wcsicmp(tm, L"Apartment") == 0));
                    sfmt(g_app.reg_detail[1], 512, L"  %s [%s] InProcServer32: %s  ThreadingModel=%s%s",
                         L"✓", clsid_hive_names[i],                          wcsrchr(path, L'\\') ? wcsrchr(path, L'\\')+1 : path,
                         tm[0] ? tm : L"(not set)",
                         tm_ok ? L"" : L" ⚠");
                } else {
                    sfmt(g_app.reg_detail[1], 512, L"  %s [%s] InProcServer32 exists but no DLL path", L"✗", clsid_hive_names[i]);
                }
                RegCloseKey(hInproc);
                sfmt(g_app.reg_detail[0], 512, L"  %s [%s] CLSID registered", L"✓", clsid_hive_names[i]);
            } else {
                sfmt(g_app.reg_detail[0], 512, L"  %s [%s] CLSID key exists but no InProcServer32", L"✗", clsid_hive_names[i]);
            }
            RegCloseKey(hKey);
        } else if (r == ERROR_ACCESS_DENIED) {
            sfmt(g_app.reg_detail[0], 512, L"  %s [%s] no read permission", L"⚠", clsid_hive_names[i]);
        }
    }
    if (!clsid_found && g_app.reg_detail[0][0] == 0)
        sfmt(g_app.reg_detail[0], 512, L"  %s CLSID key does not exist", L"✗");

    /* ③ BHO */
    bool bho_found = false;
    for (int i = 0; i < 2 && !bho_found; i++) {
        const HKEY bho_hives[] = { HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER };
        const WCHAR *bho_hive_names[] = { L"HKLM", L"HKCU" };
        sfmt(path, MAX_PATH_W, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Browser Helper Objects\\%s", CLSID_STR_W);
        r = RegOpenKeyExW(bho_hives[i], path, 0, KEY_READ, &hKey);
        if (r == ERROR_SUCCESS) {
            bho_found = true; g_app.registered = true;
            DWORD nie; size = sizeof(nie);
            r = RegQueryValueExW(hKey, L"NoInternetExplorer", NULL, &type, (BYTE*)&nie, &size);
            WCHAR niestr[64];
            if (r == ERROR_SUCCESS && type == REG_DWORD)
                sfmt(niestr, 64, L"  NoIE=%lu", nie);
            else
                wcscpy(niestr, L"  NoIE=(not set)");
            sfmt(g_app.reg_detail[2], 512, L"  %s [%s] BHO enabled%s", L"✓", bho_hive_names[i], niestr);
            RegCloseKey(hKey);
        } else if (r == ERROR_ACCESS_DENIED) {
            sfmt(g_app.reg_detail[2], 512, L"  %s [%s] BHO key: no read permission", L"⚠", bho_hive_names[i]);
        }
    }
    if (!bho_found && g_app.reg_detail[2][0] == 0)
        sfmt(g_app.reg_detail[2], 512, L"  %s BHO key does not exist", L"✗");

    /* ④ FolderExtensions */
    bool fe_found = false;
    for (int i = 0; i < 2 && !fe_found; i++) {
        const HKEY fe_hives[] = { HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER };
        const WCHAR *fe_hive_names[] = { L"HKLM", L"HKCU" };
        sfmt(path, MAX_PATH_W, L"SOFTWARE\\Classes\\Drive\\shellex\\FolderExtensions\\%s", CLSID_STR_W);
        r = RegOpenKeyExW(fe_hives[i], path, 0, KEY_READ, &hKey);
        if (r == ERROR_SUCCESS) {
            fe_found = true;
            DWORD dm; size = sizeof(dm);
            r = RegQueryValueExW(hKey, L"DriveMask", NULL, &type, (BYTE*)&dm, &size);
            bool dm_ok = (r == ERROR_SUCCESS && type == REG_DWORD && dm == 255);
            sfmt(g_app.reg_detail[3], 512, L"  %s [%s] File dialog support DriveMask=%lu%s",
                 L"✓", fe_hive_names[i], (r == ERROR_SUCCESS && type == REG_DWORD) ? dm : 0,
                 dm_ok ? L"" : L" ⚠");
            RegCloseKey(hKey);
        } else if (r == ERROR_ACCESS_DENIED) {
            sfmt(g_app.reg_detail[3], 512, L"  %s [%s] File dialog key: no read permission", L"⚠", fe_hive_names[i]);
        }
    }
    if (!fe_found && g_app.reg_detail[3][0] == 0)
        sfmt(g_app.reg_detail[3], 512, L"  %s File dialog key does not exist", L"✗");

    for (int i = 0; i < 4; i++) if (g_app.reg_detail[i][0] == 0)
        sfmt(g_app.reg_detail[i], 512, L"  %s not found in registry", L"✗");
}

/* ------------------------------------------------------------------ */
/* 图片扫描                                                            */
/* ------------------------------------------------------------------ */
typedef struct {
    WCHAR name[MAX_PATH_W];
    WCHAR full[MAX_PATH_W];
    int64_t size;
} ImgEntry;

static int img_compare(const void *a, const void *b) {
    return _wcsicmp(((const ImgEntry*)a)->name, ((const ImgEntry*)b)->name);
}

static ImgEntry *scan_images(const WCHAR *folder, int *count) {
    *count = 0;
    if (!folder || !dir_exists(folder)) return NULL;
    int cap = 16;
    ImgEntry *arr = (ImgEntry*)malloc(cap * sizeof(ImgEntry));
    if (!arr) return NULL;
    WCHAR spec[MAX_PATH_W]; path_join(spec, MAX_PATH_W, folder, L"*.*");
    WIN32_FIND_DATAW fd; HANDLE h = FindFirstFileW(spec, &fd);
    if (h == INVALID_HANDLE_VALUE) { free(arr); return NULL; }
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        const WCHAR *ext = wcsrchr(fd.cFileName, L'.');
        if (!ext) continue;
        WCHAR extl[16]; int i = 0;
        while (ext[i] && i < 15) { extl[i] = towlower(ext[i]); i++; } extl[i] = 0;
        if (wcscmp(extl, L".png") && wcscmp(extl, L".jpg") && wcscmp(extl, L".jpeg") &&
            wcscmp(extl, L".bmp") && wcscmp(extl, L".gif") && wcscmp(extl, L".webp") &&
            wcscmp(extl, L".tiff")) continue;
        if (*count >= cap) {
            int ncap = cap * 2;
            ImgEntry *tmp = (ImgEntry*)realloc(arr, (size_t)ncap * sizeof(ImgEntry));
            if (!tmp) break;            /* 扩容失败：保留已扫描部分 */
            arr = tmp; cap = ncap;
        }
        ImgEntry *e = &arr[*count];
        wcsncpy(e->name, fd.cFileName, MAX_PATH_W-1);
        path_join(e->full, MAX_PATH_W, folder, fd.cFileName);
        LARGE_INTEGER li; li.LowPart = fd.nFileSizeLow; li.HighPart = fd.nFileSizeHigh;
        e->size = li.QuadPart;
        (*count)++;
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    qsort(arr, *count, sizeof(ImgEntry), img_compare);
    return arr;
}

/* ------------------------------------------------------------------ */
/* GDI+ 图片预览                                                       */
/* ------------------------------------------------------------------ */
static bool g_gdiplus_ok = false;

/* ow/oh 可选：回填原图尺寸（预览区下方显示 3840×2160 用） */
static HBITMAP load_thumb(const WCHAR *path, int w, int h, UINT *ow, UINT *oh) {
    if (ow) *ow = 0;
    if (oh) *oh = 0;
    if (!g_gdiplus_ok) return NULL;
    if (w < 8) w = 8;
    if (h < 8) h = 8;
    HBITMAP hbmp = NULL;
    GpImage *img = NULL;
    GpGraphics *g = NULL;
    GpBitmap *bmp = NULL;
    if (GdipLoadImageFromFile(path, &img) != Ok) return NULL;
    UINT iw = 0, ih = 0;
    if (GdipGetImageWidth(img, &iw) != Ok || GdipGetImageHeight(img, &ih) != Ok || iw == 0 || ih == 0) {
        GdipDisposeImage(img);
        return NULL;
    }
    /* 小图不放大，避免插值后糊成一团 */
    double s1 = (double)w / iw, s2 = (double)h / ih;
    double s = (s1 < s2) ? s1 : s2;
    if (s > 1.0) s = 1.0;
    int dw = (int)(iw * s), dh = (int)(ih * s);
    if (dw < 1) dw = 1;
    if (dh < 1) dh = 1;
    int dx = (w - dw) / 2, dy = (h - dh) / 2;
    if (GdipCreateBitmapFromScan0(w, h, 0, PixelFormat32bppARGB, NULL, &bmp) != Ok || !bmp) {
        GdipDisposeImage(img);
        return NULL;
    }
    if (GdipGetImageGraphicsContext((GpImage*)bmp, &g) != Ok || !g) {
        GdipDisposeImage((GpImage*)bmp);
        GdipDisposeImage(img);
        return NULL;
    }
    GdipGraphicsClear(g, 0xFFFFFFFF);
    /* v1.3.2：后半段 GDI+ 调用也检查返回值，失败时不再返回可能为空的位图 */
    if (GdipDrawImageRectRectI(g, img, dx, dy, dw, dh, 0, 0, iw, ih, UnitPixel, NULL, NULL, NULL) != Ok) {
        GdipDeleteGraphics(g);
        GdipDisposeImage((GpImage*)bmp);
        GdipDisposeImage(img);
        return NULL;
    }
    if (GdipCreateHBITMAPFromBitmap(bmp, &hbmp, 0xFFFFFFFF) != Ok) hbmp = NULL;
    GdipDeleteGraphics(g);
    GdipDisposeImage((GpImage*)bmp);
    GdipDisposeImage(img);
    if (hbmp) {
        if (ow) *ow = iw;
        if (oh) *oh = ih;
    }
    return hbmp;
}

static void format_size(int64_t sz, WCHAR *out, size_t n) {
    if (sz < 1024) sfmt(out, n, L"%lld B", sz);
    else if (sz < 1024*1024) sfmt(out, n, L"%.1f KB", sz/1024.0);
    else sfmt(out, n, L"%.1f MB", sz/(1024.0*1024.0));
}

/* ------------------------------------------------------------------ */
/* 绘制辅助                                                            */
/* ------------------------------------------------------------------ */
/* 字号随界面缩放一起放大（v1.3.2：旧版只放大间距不放大字，150% 下字显得偏小）
   v1.3.3：pt 支持小数（chip 用 8.5pt，向设计稿 10px 字号靠拢） */
static HFONT create_font(const WCHAR *name, double pt, bool bold) {
    HDC dc = GetDC(NULL);
    int dpi = GetDeviceCaps(dc, LOGPIXELSY);
    ReleaseDC(NULL, dc);
    if (dpi < 72) dpi = 96;
    double pts = pt * (g_app.scale_factor > 0.1 ? g_app.scale_factor : 1.0);
    int px = -(int)round(pts * dpi / 72.0);
    return CreateFontW(px, 0, 0, 0, bold ? FW_SEMIBOLD : FW_NORMAL,
                       FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                       OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, name);
}

/* 三套字体一起创建（缩放切换后需重建，字号随之变化） */
static void init_fonts(void) {
    g_app.font_ui   = create_font(L"Microsoft YaHei", 9, false);
    g_app.font_mono = create_font(L"Consolas", 9, false);
    g_app.font_bold = create_font(L"Microsoft YaHei", 9, true);
    g_app.font_chip = create_font(L"Consolas", 8.5, false);  /* key 芯片 / 摘要键名（v1.3.3：8→8.5pt） */
}

/* v1.3.10：圆角矩形/圆形统一走 GDI+ 抗锯齿路径——GDI RoundRect/Ellipse 无
   抗锯齿，开关轨道、圆钮、分段按钮的圆弧实机锯齿感明显（用户标注）。
   g_gdiplus_ok 不可用或半径过小时回退 GDI。 */
static DWORD argb_of(COLORREF c) {
    return (0xFF000000u) | ((DWORD)GetRValue(c) << 16) | ((DWORD)GetGValue(c) << 8) | (DWORD)GetBValue(c);
}

static void draw_rounded_rect(HDC dc, const RECT *rc, int r, COLORREF c, bool fill) {
    int w = rc->right - rc->left - 1, h = rc->bottom - rc->top - 1;
    if (g_gdiplus_ok && r >= 2 && w > 0 && h > 0) {
        if (r * 2 > w) r = w / 2;
        if (r * 2 > h) r = h / 2;
        GpGraphics *gfx = NULL;
        if (GdipCreateFromHDC(dc, &gfx) == Ok) {
            GdipSetSmoothingMode(gfx, SmoothingModeAntiAlias);
            GpPath *path = NULL;
            if (GdipCreatePath(FillModeWinding, &path) == Ok) {
                INT d = r * 2, x = rc->left, y = rc->top;
                GdipStartPathFigure(path);
                GdipAddPathArcI(path, x, y, d, d, 180.0f, 90.0f);
                GdipAddPathArcI(path, x + w - d, y, d, d, 270.0f, 90.0f);
                GdipAddPathArcI(path, x + w - d, y + h - d, d, d, 0.0f, 90.0f);
                GdipAddPathArcI(path, x, y + h - d, d, d, 90.0f, 90.0f);
                GdipClosePathFigure(path);
                DWORD argb = argb_of(c);
                if (fill) {
                    GpSolidFill *br = NULL;
                    if (GdipCreateSolidFill(argb, &br) == Ok) {
                        GdipFillPath(gfx, (GpBrush*)br, path);
                        GdipDeleteBrush((GpBrush*)br);
                    }
                } else {
                    GpPen *pen = NULL;
                    if (GdipCreatePen1(argb, 1.0f, UnitPixel, &pen) == Ok) {
                        GdipDrawPath(gfx, pen, path);
                        GdipDeletePen(pen);
                    }
                }
                GdipDeletePath(path);
            }
            GdipDeleteGraphics(gfx);
            return;
        }
    }
    /* GDI 回退：r < 2 的小圆角抗锯齿反而模糊，直接系统绘制 */
    HPEN pen = CreatePen(PS_SOLID, 1, c);
    HBRUSH br = fill ? CreateSolidBrush(c) : (HBRUSH)GetStockObject(NULL_BRUSH);
    HGDIOBJ op = SelectObject(dc, pen);
    HGDIOBJ ob = SelectObject(dc, br);
    RoundRect(dc, rc->left, rc->top, rc->right, rc->bottom, r*2, r*2);
    SelectObject(dc, op);
    SelectObject(dc, ob);
    DeleteObject(pen);
    if (fill) DeleteObject(br);
}

/* v1.3.10：抗锯齿圆形填充（步骤序号圆点 / 滑块钮阴影 / 状态胶囊圆点） */
static void fill_ellipse(HDC dc, const RECT *rc, COLORREF c) {
    int w = rc->right - rc->left - 1, h = rc->bottom - rc->top - 1;
    if (g_gdiplus_ok && w > 0 && h > 0) {
        GpGraphics *gfx = NULL;
        if (GdipCreateFromHDC(dc, &gfx) == Ok) {
            GdipSetSmoothingMode(gfx, SmoothingModeAntiAlias);
            GpSolidFill *br = NULL;
            if (GdipCreateSolidFill(argb_of(c), &br) == Ok) {
                GdipFillEllipseI(gfx, (GpBrush*)br, rc->left, rc->top, w, h);
                GdipDeleteBrush((GpBrush*)br);
            }
            GdipDeleteGraphics(gfx);
            return;
        }
    }
    HPEN pen = CreatePen(PS_SOLID, 1, c);
    HBRUSH br = CreateSolidBrush(c);
    HGDIOBJ op = SelectObject(dc, pen);
    HGDIOBJ ob = SelectObject(dc, br);
    Ellipse(dc, rc->left, rc->top, rc->right, rc->bottom);
    SelectObject(dc, op);
    SelectObject(dc, ob);
    DeleteObject(pen);
    DeleteObject(br);
}

/* v1.3.10：抗锯齿圆形描边（开关圆钮 / 滑块圆钮的 1px 边——贴在蓝轨道上，
   GDI Ellipse 的锯齿最显眼） */
static void stroke_ellipse(HDC dc, const RECT *rc, COLORREF c) {
    int w = rc->right - rc->left - 1, h = rc->bottom - rc->top - 1;
    if (g_gdiplus_ok && w > 0 && h > 0) {
        GpGraphics *gfx = NULL;
        if (GdipCreateFromHDC(dc, &gfx) == Ok) {
            GdipSetSmoothingMode(gfx, SmoothingModeAntiAlias);
            GpPen *pen = NULL;
            if (GdipCreatePen1(argb_of(c), 1.0f, UnitPixel, &pen) == Ok) {
                GdipDrawEllipseI(gfx, pen, rc->left, rc->top, w, h);
                GdipDeletePen(pen);
            }
            GdipDeleteGraphics(gfx);
            return;
        }
    }
    HPEN pen = CreatePen(PS_SOLID, 1, c);
    HGDIOBJ op = SelectObject(dc, pen);
    HGDIOBJ ob = (HBRUSH)SelectObject(dc, GetStockObject(NULL_BRUSH));
    Ellipse(dc, rc->left, rc->top, rc->right, rc->bottom);
    SelectObject(dc, op);
    SelectObject(dc, ob);
    DeleteObject(pen);
}

static void fill_rounded_rect(HDC dc, const RECT *rc, int r, COLORREF c) {
    draw_rounded_rect(dc, rc, r, c, true);
}

static void draw_text_dc(HDC dc, const WCHAR *s, const RECT *rc, COLORREF c, bool center, HFONT f) {
    HGDIOBJ of = SelectObject(dc, f ? f : g_app.font_ui);
    SetTextColor(dc, c);
    SetBkMode(dc, TRANSPARENT);
    UINT fmt = DT_NOPREFIX;
    if (center) fmt |= DT_CENTER | DT_VCENTER | DT_SINGLELINE;
    else fmt |= DT_LEFT | DT_VCENTER | DT_SINGLELINE;
    DrawTextW(dc, s, -1, (RECT*)rc, fmt);
    SelectObject(dc, of);
}

static int pt_scaled(int pt) { return (int)round(pt * g_app.scale_factor); }

/* ------------------------------------------------------------------ */
/* 窗口创建                                                            */
/* ------------------------------------------------------------------ */
/* owner-draw 按钮的 GWLP_USERDATA 位图（同一控件类的位务必整体规划，避免撞位）：
   bit0 primary / bit1 danger / bit2 开关 / bit3 分段 / bit4-7 分段 idx / bit8 mini
   / bit9 主按钮带 ✓ 图标（v1.3.3） / bit10 下拉选择器（v1.3.4，替代原生 COMBOBOX） */
#define BTN_STYLE_MINI 0x100
#define BTN_STYLE_ICON_OK 0x200
#define BTN_STYLE_DROPDOWN 0x400
#define BTN_STYLE_TAB 0x800      /* 选项卡按钮（bit4-7 存页序号） */
static HWND new_btn_styled(HWND parent, int id, const WCHAR *text, int x, int y, int w, int h,
                           LONG_PTR style) {
    HWND hBtn = CreateWindowW(L"BUTTON", text,
                              WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON,
                              x, y, w, h, parent, (HMENU)(INT_PTR)id, NULL, NULL);
    SetWindowLongPtrW(hBtn, GWLP_USERDATA, style);
    return hBtn;
}
static HWND new_btn(HWND parent, int id, const WCHAR *text, int x, int y, int w, int h,
                    bool primary, bool danger) {
    return new_btn_styled(parent, id, text, x, y, w, h, (primary ? 1 : 0) | (danger ? 2 : 0));
}

/* mini 文本按钮（设计稿 .log-head .mini）：无边框，hover 时浅蓝底 + 蓝字 */
static HWND new_mini_btn(HWND parent, int id, const WCHAR *text, int x, int y, int w, int h) {
    return new_btn_styled(parent, id, text, x, y, w, h, BTN_STYLE_MINI);
}

/* 主按钮 + ✓ 图标（设计稿 .btn.primary svg + 文字，这里用字符近似） */
static HWND new_btn_ok(HWND parent, int id, const WCHAR *text, int x, int y, int w, int h) {
    return new_btn_styled(parent, id, text, x, y, w, h, 1 | BTN_STYLE_ICON_OK);
}

/* 下拉选择器按钮（v1.3.4，替代原生 COMBOBOX——原生下拉箭头不居中、外观突兀）：
   白底圆角 7 + 左侧文字 + 右侧居中小三角；点击由上层弹 TrackPopupMenu */
static HWND new_btn_dropdown(HWND parent, int id, const WCHAR *text, int x, int y, int w, int h) {
    return new_btn_styled(parent, id, text, x, y, w, h, BTN_STYLE_DROPDOWN);
}

/* 选项卡按钮（v1.3.6，替代 SysTabControl32）：无边框文本式，选中态画底部蓝条 */
static HWND new_tab_btn(HWND parent, int id, const WCHAR *text, int x, int y, int w, int h, int idx) {
    return new_btn_styled(parent, id, text, x, y, w, h, BTN_STYLE_TAB | ((LONG_PTR)idx << 4));
}

/* 中间容器需要把子控件的消息转发给主窗口（owner-draw / 命令 / 通知） */
static LRESULT forward_to_main(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/* v1.3.3：输入框轻自绘 —— 外框圆角 7 + 1px 边框（常态 #E3E7EF / focus #2F6BFF），
   内部 EDIT 去掉 WS_BORDER，输入逻辑（EN_CHANGE / WM_GETTEXT 等）完全不变。
   外框自己保存内部 EDIT 的 HWND（GWLP_USERDATA），用于判断焦点与布局。 */
static void editframe_layout(HWND frame) {
    HWND ed = (HWND)GetWindowLongPtrW(frame, GWLP_USERDATA);
    if (!ed) return;
    RECT rc; GetClientRect(frame, &rc);
    /* v1.3.5：单行 EDIT 默认顶对齐，直接拉满外框高度时文字贴顶。
       改为按字体实测行高放置 EDIT，在外框内垂直居中（框1/框4 修复） */
    HDC dc = GetDC(frame);
    HFONT fe = (HFONT)SendMessageW(ed, WM_GETFONT, 0, 0);
    HGDIOBJ ofe = fe ? SelectObject(dc, fe) : NULL;
    TEXTMETRICW tm;
    if (!GetTextMetricsW(dc, &tm)) { tm.tmHeight = 16; }
    if (ofe) SelectObject(dc, ofe);
    ReleaseDC(frame, dc);
    int padx = pt_scaled(10);                       /* 设计稿 padding:0 10px */
    int lh = tm.tmHeight + 2;                       /* 行高 + 光标余量 */
    if (lh < 8) lh = 8;
    int fh = rc.bottom - rc.top - 2;                /* 上下各留 1px 给圆角边框 */
    if (fh < 2) fh = 2;
    if (lh > fh) lh = fh;
    int ew = rc.right - rc.left - padx * 2;
    if (ew < 2) ew = 2;
    int ey = 1 + (fh - lh) / 2;
    MoveWindow(ed, padx, ey, ew, lh, TRUE);
}

static bool editframe_focused(HWND frame) {
    HWND ed = (HWND)GetWindowLongPtrW(frame, GWLP_USERDATA);
    return ed && (GetFocus() == ed);
}

static LRESULT CALLBACK EditFrameProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);
        bool focused = editframe_focused(hwnd);
        fill_rounded_rect(dc, &rc, pt_scaled(7), focused ? C_PANEL : C_INPUT_BG);
        draw_rounded_rect(dc, &rc, pt_scaled(7), focused ? C_ACCENT : C_BORDER, false);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_SIZE:
        editframe_layout(hwnd);
        return 0;
    case WM_CTLCOLOREDIT: {
        /* 编辑框文字背景必须跟随焦点态刷同色，否则编辑时出现白块残影。
           v1.3.4 修复：旧判断 "userdata == lParam" 恒为真（这条消息本来就来自
           内嵌 EDIT），导致空闲态也刷白底、设计稿的 #F7F8FA 浅灰填充永远不出现；
           改用 GetFocus 判断，与 WM_PAINT 的焦点判定一致 */
        HDC dc = (HDC)wParam;
        bool focused = editframe_focused(hwnd);
        COLORREF bg = focused ? C_PANEL : C_INPUT_BG;
        static HBRUSH br_idle = NULL, br_focus = NULL;
        if (!br_idle) br_idle = CreateSolidBrush(C_INPUT_BG);
        if (!br_focus) br_focus = CreateSolidBrush(C_PANEL);
        SetBkMode(dc, OPAQUE);
        SetBkColor(dc, bg);
        SetTextColor(dc, C_TEXT);
        return (LRESULT)(focused ? br_focus : br_idle);
    }
    case WM_LBUTTONDOWN: {
        /* 点到内边距上也要把焦点交给 EDIT */
        HWND ed = (HWND)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        if (ed) SetFocus(ed);
        return 0;
    }
    case WM_COMMAND: {
        int code = HIWORD(wParam);
        if (code == EN_SETFOCUS || code == EN_KILLFOCUS) {
            InvalidateRect(hwnd, NULL, TRUE);
            InvalidateRect((HWND)lParam, NULL, TRUE);
        }
        return forward_to_main(hwnd, msg, wParam, lParam);
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORLISTBOX:
    case WM_DRAWITEM:
    case WM_NOTIFY:
    case WM_HSCROLL:
    case WM_VSCROLL:
        return forward_to_main(hwnd, msg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static HWND new_edit(HWND parent, int id, const WCHAR *text, int x, int y, int w, int h, bool readonly) {
    HWND frame = CreateWindowW(L"backimgEditFrame", L"", WS_CHILD | WS_VISIBLE,
                               x, y, w, h, parent, NULL, NULL, NULL);
    HWND hh = CreateWindowW(L"EDIT", text,
                         WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL |
                         (readonly ? ES_READONLY : 0),
                         0, 0, 2, 2, frame, (HMENU)(INT_PTR)id, NULL, NULL);
    /* 设计稿路径输入框是 12px 等宽字（.dir-input font:12px var(--mono)） */
    if (g_app.font_mono) SendMessageW(hh, WM_SETFONT, (WPARAM)g_app.font_mono, 0);
    SetWindowLongPtrW(frame, GWLP_USERDATA, (LONG_PTR)hh);
    editframe_layout(frame);
    return hh;
}

static HWND new_label_ex(HWND parent, const WCHAR *text, int x, int y, int w, int ht,
                         bool bold, COLORREF c, bool right, bool mono) {
    HWND hh = CreateWindowW(L"STATIC", text,
                           WS_CHILD | WS_VISIBLE |
                           (right ? SS_RIGHT : SS_LEFTNOWORDWRAP),
                           x, y, w, ht, parent, NULL, NULL, NULL);
    SetWindowLongPtrW(hh, GWLP_USERDATA, (LONG_PTR)c);
    HFONT f = mono ? g_app.font_mono : (bold ? g_app.font_bold : g_app.font_ui);
    if (!f) f = g_app.font_ui;
    if (f) SendMessageW(hh, WM_SETFONT, (WPARAM)f, 0);
    return hh;
}

static HWND new_label(HWND parent, const WCHAR *text, int x, int y, int w, int ht, bool bold, COLORREF c) {
    return new_label_ex(parent, text, x, y, w, ht, bold, c, false, false);
}

/* 动态改标签颜色（WM_CTLCOLORSTATIC 会读 GWLP_USERDATA 作为前景色） */
static void set_label_color(HWND h, COLORREF c) {
    if (!h) return;
    SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)c);
    InvalidateRect(h, NULL, TRUE);
}

/* 量取文本像素宽度（用于把 key 标签排在标题文字后面；bold/mono 需与实际渲染字体一致） */
static int text_width(const WCHAR *s, bool bold, bool mono) {
    if (!s || !s[0]) return 0;
    HDC dc = GetDC(NULL);
    HFONT f = mono ? g_app.font_mono : (bold ? g_app.font_bold : g_app.font_ui);
    HGDIOBJ of = f ? SelectObject(dc, f) : NULL;
    SIZE sz = {0};
    GetTextExtentPoint32W(dc, s, (int)wcslen(s), &sz);
    if (of) SelectObject(dc, of);
    ReleaseDC(NULL, dc);
    return sz.cx;
}

/* 量取指定字体下的文本宽度（芯片自动宽度用） */
static int text_width_f(const WCHAR *s, HFONT f) {
    if (!s || !s[0]) return 0;
    HDC dc = GetDC(NULL);
    if (!f) f = g_app.font_ui;
    HGDIOBJ of = f ? SelectObject(dc, f) : NULL;
    SIZE sz = {0};
    GetTextExtentPoint32W(dc, s, (int)wcslen(s), &sz);
    if (of) SelectObject(dc, of);
    ReleaseDC(NULL, dc);
    return sz.cx;
}

/* ------------------------------------------------------------------ */
/* v1.3.2 新增自绘小部件：key/readout 芯片、摘要键（省略号）、虚线分隔   */
/* ------------------------------------------------------------------ */

/* 面板背景模式（backimgCardPanel 的 GWLP_USERDATA 编码） */
#define PANEL_BG_MASK   0x0F
#define PANEL_BORDER    0x10
#define PANEL_BG_CANVAS  0   /* 画布灰（面板区） */
#define PANEL_BG_WHITE   1   /* 纯白（工具栏 / 底栏 / 卡片） */
#define PANEL_BG_LOGHEAD 2   /* #F6F8FB 日志头 */
#define PANEL_BG_SEG     3   /* #EDF1F7 分段容器 */
#define PANEL_BG_LINE    4   /* #E3E7EF 1px 实线（选项卡行底分隔线，v1.3.5） */

static COLORREF panel_bg_color(LONG_PTR st) {
    switch (st & PANEL_BG_MASK) {
    case PANEL_BG_WHITE:   return C_PANEL;
    case PANEL_BG_LOGHEAD: return C_LOG_HEAD_BG;
    case PANEL_BG_SEG:     return C_SEG_BG;
    case PANEL_BG_LINE:    return C_BORDER;
    default:               return C_CANVAS;
    }
}

/* key / readout 芯片：等宽小字 + 浅蓝底 + 浅蓝边框 + 圆角（设计稿 .key / .readout） */
static LRESULT CALLBACK ChipProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);
        bool readout = (GetWindowLongPtrW(hwnd, GWLP_USERDATA) & 1) != 0;
        int r = pt_scaled(readout ? 6 : 4);
        /* v1.3.13：先铺父容器底色——圆角四角的 AA 像素混的是上一帧残影而非白卡 */
        HBRUSH bgb = CreateSolidBrush(C_PANEL);
        FillRect(dc, &rc, bgb);
        DeleteObject(bgb);
        fill_rounded_rect(dc, &rc, r, C_ACCENT_SOFT);
        draw_rounded_rect(dc, &rc, r, C_ACCENT_LINE, false);
        WCHAR t[160] = {0};
        GetWindowTextW(hwnd, t, 160);
        draw_text_dc(dc, t, &rc, readout ? C_ACCENT : C_KEY_TEXT, true,
                     readout ? g_app.font_mono : g_app.font_chip);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_SETTEXT: {
        LRESULT lr = DefWindowProcW(hwnd, msg, wParam, lParam);
        InvalidateRect(hwnd, NULL, TRUE);
        return lr;
    }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

/* 摘要键名：等宽小字灰，过宽出省略号（v1.3.2：旧版是 STATIC 硬截断） */
static LRESULT CALLBACK SumKeyProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);
        /* v1.3.13：铺底色——TRANSPARENT 文字模式下不铺底，文字一变新字叠旧字
           （拖动透明度滑块时 imgAlpha 值高频刷新，实测叠成乱码） */
        HBRUSH bgb = CreateSolidBrush(C_PANEL);
        FillRect(dc, &rc, bgb);
        DeleteObject(bgb);
        COLORREF c = (COLORREF)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        if (!c) c = C_SUM_KEY;
        WCHAR t[256] = {0};
        GetWindowTextW(hwnd, t, 256);
        HFONT f = g_app.font_chip ? g_app.font_chip : g_app.font_mono;
        HGDIOBJ of = f ? SelectObject(dc, f) : NULL;
        SetTextColor(dc, c);
        SetBkMode(dc, TRANSPARENT);
        UINT fmt = DT_NOPREFIX | DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS;
        DrawTextW(dc, t, -1, &rc, fmt);
        if (of) SelectObject(dc, of);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_SETTEXT: {
        LRESULT lr = DefWindowProcW(hwnd, msg, wParam, lParam);
        InvalidateRect(hwnd, NULL, TRUE);
        return lr;
    }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

/* 开关行之间的 1px 虚线分隔（最后一行不画） */
static LRESULT CALLBACK DividerProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);
        HBRUSH br = CreateSolidBrush(C_PANEL);
        FillRect(dc, &rc, br); DeleteObject(br);
        HPEN pen = CreatePen(PS_DOT, 1, C_DIVIDER);
        HGDIOBJ op = SelectObject(dc, pen);
        int y = rc.top;
        MoveToEx(dc, rc.left, y, NULL);
        LineTo(dc, rc.right, y);
        SelectObject(dc, op); DeleteObject(pen);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

/* 摘要值：等宽字（设计稿 .sval font:12px var(--mono)），超宽出省略号
   （v1.3.3：旧版是 STATIC 硬截断，"70 / 255(27%)" 会被切掉尾巴） */
static LRESULT CALLBACK SumValProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);
        /* v1.3.13：铺底色（同 SumKeyProc——拖动滑块时摘要值高频刷新叠字） */
        HBRUSH bgb = CreateSolidBrush(C_PANEL);
        FillRect(dc, &rc, bgb);
        DeleteObject(bgb);
        COLORREF c = (COLORREF)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        if (!c) c = C_ACCENT;
        WCHAR t[1024] = {0};
        GetWindowTextW(hwnd, t, 1024);
        HFONT f = g_app.font_mono ? g_app.font_mono : g_app.font_ui;
        HGDIOBJ of = f ? SelectObject(dc, f) : NULL;
        SetTextColor(dc, c);
        SetBkMode(dc, TRANSPARENT);
        UINT fmt = DT_NOPREFIX | DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS;
        DrawTextW(dc, t, -1, &rc, fmt);
        if (of) SelectObject(dc, of);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_SETTEXT: {
        LRESULT lr = DefWindowProcW(hwnd, msg, wParam, lParam);
        InvalidateRect(hwnd, NULL, TRUE);
        return lr;
    }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

/* w<=0 时按文本自动定宽（key 芯片）；readout 由调用方给定宽度 */
static HWND new_chip(HWND parent, const WCHAR *text, int x, int y, bool readout, int w) {
    HFONT f = readout ? g_app.font_mono : g_app.font_chip;
    int pad = pt_scaled(readout ? 9 : 5);
    int cw = w;
    if (cw <= 0) cw = text_width_f(text, f) + pad*2 + 4;
    int chh = pt_scaled(readout ? 22 : 16);
    HWND hh = CreateWindowW(L"backimgChip", text, WS_CHILD | WS_VISIBLE,
                            x, y, cw, chh, parent, NULL, NULL, NULL);
    SetWindowLongPtrW(hh, GWLP_USERDATA, readout ? 1 : 0);
    return hh;
}

static HWND new_sum_key(HWND parent, const WCHAR *text, int x, int y, int w, int h, COLORREF c) {
    HWND hh = CreateWindowW(L"backimgSumKey", text, WS_CHILD | WS_VISIBLE,
                            x, y, w, h, parent, NULL, NULL, NULL);
    SetWindowLongPtrW(hh, GWLP_USERDATA, (LONG_PTR)c);
    return hh;
}

/* 摘要值：自绘 + 省略号（v1.3.3） */
static HWND new_sum_val(HWND parent, const WCHAR *text, int x, int y, int w, int h, COLORREF c) {
    HWND hh = CreateWindowW(L"backimgSumVal", text, WS_CHILD | WS_VISIBLE,
                            x, y, w, h, parent, NULL, NULL, NULL);
    SetWindowLongPtrW(hh, GWLP_USERDATA, (LONG_PTR)c);
    return hh;
}

static HWND new_divider(HWND parent, int x, int y, int w) {
    return CreateWindowW(L"backimgDivider", L"", WS_CHILD | WS_VISIBLE,
                         x, y, w, 1, parent, NULL, NULL, NULL);
}

/* 生效流程：步骤序号圆点 */
static LRESULT CALLBACK StepNoProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);
        int n = (int)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        /* v1.3.13：先铺底色，圆外四角的 AA 像素不再混上一帧残影 */
        HBRUSH bgb = CreateSolidBrush(C_PANEL);
        FillRect(dc, &rc, bgb);
        DeleteObject(bgb);
        fill_ellipse(dc, &rc, C_ACCENT_SOFT);
        stroke_ellipse(dc, &rc, C_ACCENT_LINE);
        WCHAR t[8]; sfmt(t, 8, L"%d", n);
        draw_text_dc(dc, t, &rc, C_ACCENT, true, g_app.font_ui);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

/* 生效流程：底部虚线提示条 */
static LRESULT CALLBACK FlowTipProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);
        fill_rounded_rect(dc, &rc, pt_scaled(7), RGB(0xF6, 0xF8, 0xFB));
        HPEN pen = CreatePen(PS_DOT, 1, RGB(0xD8, 0xDE, 0xE8));
        HGDIOBJ op = SelectObject(dc, pen);
        HGDIOBJ ob = (HBRUSH)SelectObject(dc, GetStockObject(NULL_BRUSH));
        RoundRect(dc, rc.left, rc.top, rc.right - 1, rc.bottom - 1, pt_scaled(14), pt_scaled(14));
        SelectObject(dc, op); SelectObject(dc, ob);
        DeleteObject(pen);
        WCHAR t[256] = {0};
        GetWindowTextW(hwnd, t, 256);
        draw_text_dc(dc, t, &rc, C_TEXT3, true, g_app.font_ui);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static HWND new_step_no(HWND parent, int n, int x, int y, int sz) {
    HWND hh = CreateWindowW(L"backimgStepNo", L"", WS_CHILD | WS_VISIBLE,
                            x, y, sz, sz, parent, NULL, NULL, NULL);
    SetWindowLongPtrW(hh, GWLP_USERDATA, n);
    return hh;
}

static HWND new_flow_tip(HWND parent, const WCHAR *text, int x, int y, int w, int h) {
    return CreateWindowW(L"backimgFlowTip", text, WS_CHILD | WS_VISIBLE,
                         x, y, w, h, parent, NULL, NULL, NULL);
}

/* 开关：BS_OWNERDRAW 与 BS_AUTOCHECKBOX 类型码冲突，不能用 BM_GETCHECK；
   绘制时直接读 g_app.v_*（单一数据源），不自存第二份状态。 */
static bool sw_value(int idx) {
    switch (idx) {
    case 0: return g_app.v_folderExt;
    case 1: return g_app.v_noerror;
    case 2: return g_app.v_random;
    case 3: return g_app.v_custom;
    }
    return false;
}

static HWND new_switch(HWND parent, int id, int x, int y, int w, int ht) {
    HWND hh = CreateWindowW(L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                           x, y, w, ht, parent, (HMENU)(INT_PTR)id, NULL, NULL);
    SetWindowLongPtrW(hh, GWLP_USERDATA, 4 | ((id - ID_SW_FOLDEREXT) << 4));
    return hh;
}

static HWND new_segment(HWND parent, int id, const WCHAR *text, int x, int y, int w, int ht) {
    HWND hh = CreateWindowW(L"BUTTON", text, WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON,
                           x, y, w, ht, parent, (HMENU)(INT_PTR)id, NULL, NULL);
    int idx = id - ID_SEG_POSTYPE_0;
    SetWindowLongPtrW(hh, GWLP_USERDATA, (LONG_PTR)(8 | (idx << 4)));
    SetWindowTextW(hh, text);
    return hh;
}

/* 中间容器需要把子控件的消息转发给主窗口（owner-draw / 命令 / 通知） */
static void on_draw_item(DRAWITEMSTRUCT *di);
static void paint_status(HDC dc);

static LRESULT forward_to_main(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (g_app.hwndMain && g_app.hwndMain != hwnd)
        return SendMessageW(g_app.hwndMain, msg, wParam, lParam);
    return 0;
}

static LRESULT CALLBACK CardPanelProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);
        LONG_PTR st = GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        int bgmode = (int)(st & PANEL_BG_MASK);
        COLORREF bgc = panel_bg_color(st);
        bool border = (st & PANEL_BORDER) != 0;
        int r = pt_scaled(bgmode == PANEL_BG_SEG ? 9 : 10);
        if (bgmode == PANEL_BG_LOGHEAD || !border) {
            HBRUSH br = CreateSolidBrush(bgc);
            FillRect(dc, &rc, br);
            DeleteObject(br);
        } else {
            fill_rounded_rect(dc, &rc, r, bgc);
        }
        if (border) {
            draw_rounded_rect(dc, &rc, r, C_BORDER, false);
        }
        /* 日志头下方 1px 分隔线（设计稿 .log-head border-bottom） */
        if (bgmode == PANEL_BG_LOGHEAD) {
            HPEN pen = CreatePen(PS_SOLID, 1, C_BORDER);
            HGDIOBJ op = SelectObject(dc, pen);
            MoveToEx(dc, rc.left, rc.bottom - 1, NULL);
            LineTo(dc, rc.right, rc.bottom - 1);
            SelectObject(dc, op); DeleteObject(pen);
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_DRAWITEM:
        on_draw_item((DRAWITEMSTRUCT*)lParam);
        return TRUE;
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORSCROLLBAR:
    case WM_COMMAND:
    case WM_NOTIFY:
    case WM_HSCROLL:
    case WM_VSCROLL:
        return forward_to_main(hwnd, msg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

/* 通用面板容器：style 见 PANEL_BG_* / PANEL_BORDER */
static HWND new_panel(HWND parent, int x, int y, int w, int ht, LONG_PTR style) {
    HWND hh = CreateWindowW(L"backimgCardPanel", L"", WS_CHILD | WS_VISIBLE,
                         x, y, w, ht, parent, NULL, NULL, NULL);
    SetWindowLongPtrW(hh, GWLP_USERDATA, style);
    return hh;
}

static HWND new_card_panel(HWND parent, int x, int y, int w, int ht) {
    return new_panel(parent, x, y, w, ht, PANEL_BORDER | PANEL_BG_WHITE);
}

/* v1.3.3：自绘滑块（设计稿 input[type=range] accent-color；原生 trackbar 外观差太远）
   6px 圆角轨道（未填 #E5EAF3 / 已填 #2F6BFF）+ 14px 白圆钮（1px 边框 + 浅阴影）。
   对外仍支持 TBM_SETPOS / TBM_GETPOS / TBM_SETRANGE，上层（load_config / on_slider_change
   / rebuild_ui）逻辑一行不改。值域与当前位置打包存在 GWLP_USERDATA：
   pos(8bit) | min<<8(8bit) | max<<16(8bit) */
static LRESULT CALLBACK SliderProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    LONG_PTR st = GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    int pos = (int)(st & 0xFF);
    int minv = (int)((st >> 8) & 0xFF);
    int maxv = (int)((st >> 16) & 0xFF);
    if (maxv < minv) { maxv = 255; minv = 0; }

    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);
        /* v1.3.13：先铺满客户区底色再画（轨道只有 6px 高，圆钮/阴影凸出轨道
           上下，旧实现凸出区从不重画——快速拖动时旧圆钮残环逐帧累积成鬼影）；
           双缓冲绘制，顺带消除"backfill + repaint"两步之间的闪烁 */
        HDC mem = CreateCompatibleDC(dc);
        HBITMAP bmp = CreateCompatibleBitmap(dc, rc.right, rc.bottom);
        HGDIOBJ ob = SelectObject(mem, bmp);
        HBRUSH bgb = CreateSolidBrush(C_PANEL);
        FillRect(mem, &rc, bgb);
        DeleteObject(bgb);
        int rad = pt_scaled(7);
        int travel = rc.right - rc.left - rad * 2;
        if (travel < 1) travel = 1;
        int cy = (rc.top + rc.bottom) / 2;
        int th = pt_scaled(6);
        RECT tr;
        tr.top = cy - th / 2; tr.bottom = tr.top + th;
        tr.left = rc.left; tr.right = rc.right;
        fill_rounded_rect(mem, &tr, th / 2, C_TRACK_BG);
        int cxleft = rc.left + rad;
        int cx = cxleft + (int)round((double)(pos - minv) * travel / (maxv > minv ? maxv - minv : 1));
        RECT fr = tr;
        fr.right = cx > fr.left ? cx : fr.left + 1;
        fill_rounded_rect(mem, &fr, th / 2, C_ACCENT);
        /* 圆钮：先画 1px 下偏移的浅阴影，再画白底 + 蓝边 */
        RECT sh = { cx - rad, cy - rad + pt_scaled(1), cx + rad, cy + rad + pt_scaled(1) };
        fill_ellipse(mem, &sh, C_SHADOW);
        RECT kb = { cx - rad, cy - rad, cx + rad, cy + rad };
        fill_rounded_rect(mem, &kb, rad, C_PANEL);
        stroke_ellipse(mem, &kb, C_ACCENT);
        BitBlt(dc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
        SelectObject(mem, ob);
        DeleteObject(bmp);
        DeleteDC(mem);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case TBM_GETPOS:
        return (LRESULT)pos;
    case TBM_SETPOS: {
        int v = (int)lParam;
        if (v < minv) v = minv;
        if (v > maxv) v = maxv;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)((v & 0xFF) | ((minv & 0xFF) << 8) | ((maxv & 0xFF) << 16)));
        if (wParam) InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    }
    case TBM_SETRANGE: {
        int lo = (int)LOWORD(lParam), hi = (int)HIWORD(lParam);
        if (lo < 0) lo = 0;
        if (lo > 255) lo = 255;
        if (hi < lo) hi = lo;
        if (hi > 255) hi = 255;
        if (pos < lo) pos = lo;
        if (pos > hi) pos = hi;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)((pos & 0xFF) | ((lo & 0xFF) << 8) | ((hi & 0xFF) << 16)));
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    }
    case WM_GETDLGCODE:
        return DLGC_WANTARROWS;
    case WM_KEYDOWN: {
        int step = 1;
        switch (wParam) {
        case VK_LEFT:  case VK_DOWN:  step = -1; break;
        case VK_RIGHT: case VK_UP:    step = 1; break;
        case VK_PRIOR: step = 10; break;
        case VK_NEXT:  step = -10; break;
        case VK_HOME:  SendMessageW(hwnd, TBM_SETPOS, TRUE, minv); goto kdone;
        case VK_END:   SendMessageW(hwnd, TBM_SETPOS, TRUE, maxv); goto kdone;
        default: return DefWindowProcW(hwnd, msg, wParam, lParam);
        }
        SendMessageW(hwnd, TBM_SETPOS, TRUE, pos + step);
    kdone:
        SendMessageW(g_app.hwndMain, WM_HSCROLL, 0, (LPARAM)hwnd);
        return 0;
    }
    case WM_LBUTTONDOWN: {
        SetFocus(hwnd);
        SetCapture(hwnd);
        /* fallthrough 到 MOUSEMOVE 走同一套换算 */
    }
    case WM_MOUSEMOVE: {
        if (GetCapture() != hwnd) break;
        RECT rc; GetClientRect(hwnd, &rc);
        int rad = pt_scaled(7);
        int travel = rc.right - rc.left - rad * 2;
        int x = GET_X_LPARAM(lParam);
        if (travel < 1) travel = 1;
        int v = minv + (int)round((double)(x - rad) * (maxv - minv) / travel);
        if (v < minv) v = minv;
        if (v > maxv) v = maxv;
        if (v != pos) {
            SendMessageW(hwnd, TBM_SETPOS, TRUE, v);
            SendMessageW(g_app.hwndMain, WM_HSCROLL, 0, (LPARAM)hwnd);
        }
        return 0;
    }
    case WM_LBUTTONUP: {
        if (GetCapture() == hwnd) ReleaseCapture();
        return 0;
    }
    case WM_CAPTURECHANGED:
    case WM_KILLFOCUS:
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static HWND new_trackbar(HWND parent, int id, int x, int y, int w, int ht, int minv, int maxv) {
    HWND hh = CreateWindowW(L"backimgSlider", L"",
                           WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                           x, y, w, ht, parent, (HMENU)(INT_PTR)id, NULL, NULL);
    if (minv < 0) minv = 0;
    if (minv > 255) minv = 255;
    if (maxv < minv) maxv = minv;
    if (maxv > 255) maxv = 255;
    SetWindowLongPtrW(hh, GWLP_USERDATA,
                      (LONG_PTR)(((minv & 0xFF) << 8) | ((maxv & 0xFF) << 16)));
    return hh;
}

static HWND new_listview(HWND parent, int id, int x, int y, int w, int ht) {
    HWND hh = CreateWindowW(WC_LISTVIEWW, L"",
                           WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL |
                           LVS_SHOWSELALWAYS | LVS_NOCOLUMNHEADER,
                           x, y, w, ht, parent, (HMENU)(INT_PTR)id, NULL, NULL);
    ListView_SetExtendedListViewStyle(hh, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    if (g_app.font_ui) SendMessageW(hh, WM_SETFONT, (WPARAM)g_app.font_ui, 0);
    return hh;
}

/* ------------------------------------------------------------------ */
/* UI 构建（按 HTML 设计稿布局）                                       */
/* ------------------------------------------------------------------ */
static HWND hTabBasic, hTabImages, hTabFolders;
static HWND hSwFolderExt, hSwNoerror, hSwRandom, hSwCustom;
static HWND hSegPos[7];
static HWND hSliderAlpha;

#define S(val)  pt_scaled(val)

/* v1.3.11：预览元信息行（.pmeta）显示/隐藏与 flex 排布，定义在图片预览段 */
static void preview_meta_update(const WCHAR *name, const WCHAR *dim, bool show);

/* 面板可用高度（由 build_ui 按客户区计算，供各页卡片公用） */
static int g_panel_h = 470;

/* ------------------------------------------------------------------
   v1.3.20(C)：定时轮换档位。v1.3.25(D1) 起内部单位统一为【秒】，
   ROT_SECONDS 与 ROT_CHOICES 一一对应；-1 = 自定义（输入框填数字，
   单位由「分钟/秒钟」下拉决定。v1.3.25：新增单位下拉；
   v1.3.26 用户红框：输入框+单位框挪到 random 行下方专用子行，宽度放开）。
   下拉按钮复用「界面缩放」的画法（new_btn_dropdown + TrackPopupMenu），
   当前档打勾，视觉与界面其他部分完全一致。
   ------------------------------------------------------------------ */
static const WCHAR *ROT_CHOICES[] = { L"No rotation", L"Every 1 min", L"Every 2 min",
                                      L"Every 5 min", L"Every 10 min", L"Every 30 min",
                                      L"Custom…" };
static const int ROT_SECONDS[]   = { 0, 60, 120, 300, 600, 1800, -1 };
/* v1.3.25(D1)：单位下拉两个选项（自定义态显示在输入框右侧） */
static const WCHAR *ROT_UNIT_CHOICES[] = { L"Minutes", L"Seconds" };
#define ROT_UNIT_MIN  0
#define ROT_UNIT_SEC  1
/* v1.3.28(P2，Deepseek 复审)：hRotLabel 死变量清除 —— v1.3.26 删掉
 * "caption slide"逻辑后它创建即无读者（赋值后无任何读取），句柄不必
 * 保存，new_label_ex 返回值直接丢弃。 */
/* v1.3.26(E1) 用户红框改动：自定义输入框+单位框挪到 random 行下方专用子行，
 * 宽度放开 —— 数字与「分钟/秒钟」都不再挤在开关行里。
 * v1.3.27(F2，Deepseek 审查)：v1.3.25 遗留的五个位置全局（g_rot_cx /
 * g_rot_label_y / g_rot_sub_y / g_rot_edit_x / g_rot_unit_x）在"caption\n * slide"逻辑删除后已无读者（或只在 rot_ui_init 内部使用），全部降为
 * rot_ui_init 局部变量；全局只留两个子行控件句柄 + 自定义态标志。 */
static HWND hRotSubLabel;   /* 子行标题"custom interval"（v1.3.27(F1)：随随机开关显隐） */
static HWND hRotSubHint;    /* 非自定义态的子行提示文字 */
/* v1.3.27(F3，Deepseek 审查)：自定义态改用独立状态位判定，不再纯靠数值反推 ——
 * 用户在自定义态手敲 60/120/300 等恰好命中预设秒数时，输入框不再当场消失、
 * 档位不再跳回预设档。启动时 false（配置里是预设值就如实显示预设档）。 */
static bool g_rot_custom_picked = false;
static void rot_ui_init(HWND parent, int cx, int cy, int w, int h, int suby);
static void rot_ui_sync(void);
static void on_rot_menu(HWND hwnd);
static void on_rotunit_menu(HWND hwnd);    /* v1.3.25(D1)：单位切换 */
static void update_summary(void);   /* v1.3.21(G2)：on_rot_menu 选档后要调 */
static void on_rotval_edit_change(void);   /* v1.3.25(D1)：EN_CHANGE（原 on_rotmin_edit_change） */
/* v1.3.25(D1)：按当前显示单位把 v_rotsec 换算成输入框里的整数 */
static int rotval_to_display(int sec) {
    return (g_app.v_rotunit == ROT_UNIT_MIN) ? sec / 60 : sec;
}

/* 开关行：标题 + key 芯片 同一行，说明独立一行灰色小字；
   行间 1px 虚线分隔（UI-7，第一行不画）
   v1.3.5：行高 40→44（设计稿 .sw-row padding 8px + 13px 标题 + 11px 说明 + 呼吸空间），
   开关 40→38 宽（设计稿 .sw-track 38×22）
   v1.3.8：行距弹性化（用户绿框：行距要填满开关区、对齐下方 posType 的呼吸感）——
   pitch 由 build_ui 按剩余空间算出，行内容在 pitch 内垂直居中、虚线钉行顶 */
static int add_switch_row(HWND parent, int cardw, int y, int id, HWND *sw,
                          const WCHAR *name, const WCHAR *key, const WCHAR *desc,
                          bool divider, int pitch) {
    if (divider) new_divider(parent, S(12), y, cardw - S(24));
    int off = (pitch - S(44)) / 2;
    if (off < 0) off = 0;
    *sw = new_switch(parent, id, cardw - S(54), y + S(11) + off, S(38), S(22));
    new_label_ex(parent, name, S(12), y + S(3) + off, cardw - S(72), S(16), true, C_TEXT, false, false);
    int nx = S(12) + text_width(name, true, false) + S(8);
    if (nx + S(60) < cardw - S(64))
        new_chip(parent, key, nx, y + S(4) + off, false, 0);
    new_label_ex(parent, desc, S(12), y + S(22) + off, cardw - S(72), S(14), false, C_TEXT3, false, false);
    return pitch;
}

static void build_basic_tab(HWND parent) {
    int pad = S(16);
    /* v1.3.2：卡片宽度按实际客户区像素算（旧版用未缩放的 base_w，缩放≠100% 时卡片偏窄） */
    int cardw = (g_app.cw - pad*3) / 2;
    /* 卡片高度随面板可用高度自适应：预留下方「自定义图片目录」卡与间距 */
    int folderCardH = S(104);
    int cardh = g_panel_h - pad*2 - S(14) - folderCardH;
    if (cardh < S(320)) cardh = S(320);
    int y = pad;
    int x1 = pad;
    int x2 = pad*2 + cardw;

    /* 左卡：显示选项 */
    HWND left = new_card_panel(parent, x1, y, cardw, cardh);
    /* 右卡：当前生效配置 */
    HWND right = new_card_panel(parent, x2, y, cardw, cardh);

    int ly = S(12);
    new_label_ex(left, L"Display options", S(12), ly, S(150), S(18), true, C_TEXT, false, false);
    /* UI-4：hint 右对齐到卡片右缘 */
    {
        const WCHAR *hint = L"After changes, click Save config at the bottom to apply";
        int hw = text_width_f(hint, g_app.font_ui);
        if (hw > cardw - S(140)) hw = cardw - S(140);
        new_label_ex(left, hint, cardw - S(16) - hw, ly + S(2), hw, S(16),
                     false, C_TEXT3, false, false);
    }
    ly += S(30);

    /* v1.3.5：左卡弹性填满（框二）——内容基准高度之外的剩余空间按
       35/35/30 分摊到「开关区后 / 分段容器后 / 卡底」三处间距，
       卡片被拉伸后不再出现大片空白；空间不足时反向压缩间距 */
    int gapA = S(8), gapB = S(12), gapC = S(12);
    int row_pitch = S(44);
    {
        int seg_h = S(30);
    int used = S(12) + S(30) + S(44)*4 + S(28) + gapA + S(22) +
               (S(3) + seg_h + S(2) + seg_h + S(3)) + S(2) + S(30) + gapB + S(22) + S(30) + gapC;
        int extra = cardh - used;
        if (extra > 0) {
            /* v1.3.8（用户绿框）：剩余空间 70% 先注入开关行距，让 4 行开关
               撑满开关区、行距呼吸感对齐下方 posType 区块；其余 30% 再按
               35/35/30 分摊三处间距 */
            int addR = extra * 70 / 100;
            row_pitch += addR / 4;
            extra -= addR;
            int addA = extra * 35 / 100;
            int addB = extra * 35 / 100;
            int addC = extra - addA - addB;
            gapA += addA; gapB += addB; gapC += addC;
        } else if (extra < 0) {
            int need = -extra, take;
            take = row_pitch - S(40); if (take > need) take = need; if (take > 0) { row_pitch -= take; need -= take; }
            take = gapA - S(4); if (take > need) take = need; if (take > 0) { gapA -= take; need -= take; }
            take = gapB - S(4); if (take > need) take = need; if (take > 0) { gapB -= take; need -= take; }
            take = gapC - S(2); if (take > need) take = need; if (take > 0) { gapC -= take; need -= take; }
        }
    }

    ly += add_switch_row(left, cardw, ly, ID_SW_FOLDEREXT, &hSwFolderExt,
                         L"Also enable background in file dialogs", L"folderExt",
                         L"Folder windows only; tick adds Open/Save dialogs too", false, row_pitch);
    ly += add_switch_row(left, cardw, ly, ID_SW_NOERROR, &hSwNoerror,
                         L"Ignore errors", L"noerror",
                         L"No popup on missing image or error; keep ticked", true, row_pitch);
    int rot_row_y = ly;     /* v1.3.20(C)：random 行行顶，供轮换控件定位 */
    ly += add_switch_row(left, cardw, ly, ID_SW_RANDOM, &hSwRandom,
                         L"Randomize images", L"random",
                         L"Rotate randomly with multiple images (needs at least 2)", true, row_pitch);
    /* v1.3.26(E1) 用户红框：random 行下方常驻一条"custom interval"子行，
     * 高度计入版面（下方 used 同步 +S(28)），非自定义态显示提示文字，
     * 避免"sudden expand/collapse"的版面跳动。 */
    int rot_sub_y = ly;
    ly += S(28);
    {   /* v1.3.20(C)：random 行右侧加"timed rotation"下拉，紧贴开关左侧；
           v1.3.26(E1) 用户红框：自定义输入框+单位框挪到下方专用子行，
           这里只留档位下拉，坐标随子行行顶一并传入 */
        int off = (row_pitch - S(44)) / 2;
        if (off < 0) off = 0;
        rot_ui_init(left, cardw - S(54) - S(10) - S(112), rot_row_y + off,
                    S(112), S(24), rot_sub_y);
    }
    ly += add_switch_row(left, cardw, ly, ID_SW_CUSTOM, &hSwCustom,
                         L"Use a custom image folder", L"custom",
                         L"Tick to pick any folder, not just the plugin's Image folder", true, row_pitch);
    ly += gapA;

    new_label_ex(left, L"Image position", S(12), ly, S(140), S(16), true, C_TEXT, false, false);
    new_chip(left, L"posType", S(12) + text_width(L"Image position", true, false) + S(8),
             ly + S(1), false, 0);
    ly += S(22);
    /* UI-5：7 档外加统一容器，上行 4 个、下行 3 个
       v1.3.4：第二行 3 段在容器内水平居中
       v1.3.5：分段按钮 28→30 高（设计稿 .seg button padding 6px + 12px 字）
       v1.3.8（用户红框）：容器 #EDF1F7 灰底去掉改白底，未选中段同为白底，
       整块只剩 1px 边框勾勒的按钮轮廓 */
    {
        int boxw = cardw - S(24);
        int segw = (boxw - S(6) - S(2)*3) / 4;
        int seg_h = S(30);
        int segbox_h = S(3) + seg_h + S(2) + seg_h + S(3);
        HWND segbox = new_panel(left, S(12), ly, boxw, segbox_h, PANEL_BG_WHITE);
        for (int i = 0; i < 4; i++)
            hSegPos[i] = new_segment(segbox, ID_SEG_POSTYPE_0 + i, POS_NAMES[i],
                                     S(3) + i*(segw + S(2)), S(3), segw, seg_h);
        int row2_w = 3*segw + S(2)*2;
        int x2 = S(3) + (boxw - S(6) - row2_w) / 2;
        if (x2 < S(3)) x2 = S(3);
        for (int i = 4; i < 7; i++)
            hSegPos[i] = new_segment(segbox, ID_SEG_POSTYPE_0 + i, POS_NAMES[i],
                                     x2 + (i-4)*(segw + S(2)), S(3) + seg_h + S(2), segw, seg_h);
        ly += segbox_h;
    }
    /* v1.3.16：补一行行为说明 —— 背景图现在一律按原图宽高比等比缩放，
     * 窗口任意尺寸（拖动/最大化/全屏）下都不变形、不出框，并实时跟随。
     * "Fit"= 整图完整适配窗口；"Fit & fill"= 等比铺满并裁掉溢出。 */
    {
        /* v1.3.17：把"Fit"档的语义变化说明白 ——
         *   v1.3.15 的"Fit"是【拉伸填满窗口】（会变形）；
         *   v1.3.16 起改为【等比完整适配窗口】（不变形，四周可能留白）。
         *   想要"fill without distortion"请用"Fit & fill"（等比铺满+居中裁切）。 */
        const WCHAR *note =
            L"Always scaled to original aspect ratio, no distortion, follows the window;\n"
            L"\"Fit\" = whole image visible; \"Fill\" = fill and crop the overflow";
        {
            /* 英文版（人类开发者第四轮）：SS_LEFTNOWORDWRAP 不换行，说明改两行
             * 需要换成 SS_LEFT 才能按 \n 分行显示 */
            HWND hn = new_label_ex(left, note, S(12), ly + S(2), cardw - S(24), S(30),
                                   false, C_TEXT3, false, false);
            SetWindowLongPtrW(hn, GWL_STYLE,
                              (GetWindowLongPtrW(hn, GWL_STYLE) & ~SS_TYPEMASK) | SS_LEFT);
        }
        ly += S(34);
    }
    ly += gapB;

    /* UI-3：不透明度 readout 改芯片
       v1.3.5：滑块 26→30 高（设计稿 input[type=range] height:26px + 触控余量） */
    new_label_ex(left, L"Opacity", S(12), ly, S(70), S(16), true, C_TEXT, false, false);
    new_chip(left, L"imgAlpha", S(12) + text_width(L"Opacity", true, false) + S(8),
             ly + S(1), false, 0);
    {
        int rw = text_width_f(L"255 / 255(100%)", g_app.font_mono) + pt_scaled(18) + 2;
        hSliderAlpha = new_trackbar(left, ID_SLIDER_ALPHA, S(12), ly + S(22),
                                    cardw - S(24) - rw - S(12), S(30), 0, 255);
        g_app.hwndAlphaReadout = new_chip(left, L"70 / 255(27%)",
                                          cardw - S(12) - rw, ly + S(26), true, rw);
    }

    /* 右卡：当前生效配置 */
    int ry = S(12);
    new_label_ex(right, L"Live configuration", S(12), ry, S(170), S(18), true, C_TEXT, false, false);
    new_chip(right, L"Live", S(12) + text_width(L"Live configuration", true, false) + S(10),
             ry + S(1), false, 0);
    {
        const WCHAR *ht = L"Last refresh: --:--:--";
        int hw = text_width_f(ht, g_app.font_ui) + S(24);   /* v1.3.9 加大余量：时间尾部曾被裁 */
        g_app.hwndSumTime = new_label_ex(right, ht, cardw - S(20) - hw, ry + S(2), hw, S(16),
                                         false, C_TEXT3, false, false);
    }
    ry += S(28);

    /* v1.3.9：右卡整体填充规划（用户红框：所有行距填满整卡）——基准内容之外
       的剩余空间 40% 注入摘要网格行距（每行封顶 S(6)），其余给流程块三处
       间距（各自封顶），再有余量留在网格与按钮排之间。v1.3.8 的流程块把
       extra 全计入块高却只消耗封顶部分，超出量沉淀在卡底形成大空白 */
    int grid_pitch = S(24);
    int flow_eg = 0, flow_ep = 0, flow_et = 0;
    {
        const WCHAR *bw_txt[5] = { L"Refresh summary", L"View config.ini", L"Copy summary", L"Emergency off", L"Restore original" };
        int tot = 0;
        for (int i = 0; i < 5; i++) tot += text_width_f(bw_txt[i], g_app.font_ui) + S(26);
        tot += S(8) * 4;
        int btn_block = (tot <= cardw - S(24)) ? S(40) : S(84);
        int flow_base = S(16) + S(22) + S(22)*2 + S(6) + S(16) + S(26);
        int extra = cardh - S(10) - S(40) - S(24)*5 - btn_block - S(8) - flow_base;
        if (extra > 0) {
            int addR = extra * 40 / 100;
            grid_pitch += addR / 5;
            if (grid_pitch > S(24) + S(6)) grid_pitch = S(24) + S(6);
            extra -= (grid_pitch - S(24)) * 5;
            if (extra > 0) {
                flow_eg = extra * 20 / 100;  if (flow_eg > S(8))  flow_eg = S(8);
                flow_ep = extra * 25 / 100;  if (flow_ep > S(10)) flow_ep = S(10);
                flow_et = extra * 25 / 100;  if (flow_et > S(18)) flow_et = S(18);
            }
        }
    }

    /* 摘要网格：宽度充足时前 6 项两两并排，否则全部单列
       UI-2：键名改自绘（超宽出省略号），键列 128；UI-8：值行比键行低 1px */
    const WCHAR *sum_keys[9] = {
        L"File dialogs", L"Ignore errors", L"Random images",
        L"Custom folder", L"Position", L"Opacity",
        L"Custom path", L"Image count", L"DLL status"
    };
    /* v1.3.3：键列 128→108（设计稿 .skey width 106px），省下的宽度给值列 */
    int colw = (cardw - S(24) - S(16)) / 2;
    int keyw2 = S(96);
    if (colw >= S(200)) {
        /* 设计稿：9 项两列网格；最后一项 dllstatus 值长，独占整行 */
        for (int i = 0; i < 9; i++) {
            int col = i % 2, row = i / 2;
            int cx = S(12) + col * (colw + S(16));
            int cy = ry + row * grid_pitch;   /* v1.3.9 弹性行距 */
            new_sum_key(right, sum_keys[i], cx, cy, keyw2, S(16), C_SUM_KEY);
            int vx = cx + keyw2 + S(8);
            int vw = (i == 8) ? (cardw - S(12) - vx) : (colw - keyw2 - S(12));
            g_app.hwndSummaryLabels[i] = new_sum_val(right, L"—", vx, cy + S(1),
                                                     vw, S(16), C_ACCENT);
        }
        ry += grid_pitch * 5;
    } else {
        /* 窄卡：9 项全部单列 */
        for (int i = 0; i < 9; i++) {
            new_sum_key(right, sum_keys[i], S(12), ry, keyw2, S(16), C_SUM_KEY);
            g_app.hwndSummaryLabels[i] = new_sum_val(right, L"—", S(12) + keyw2 + S(8), ry + S(1),
                                                     cardw - S(24) - keyw2 - S(8), S(16), C_ACCENT);
            ry += grid_pitch;
        }
    }

    /* 摘要操作按钮 + 生效流程（v1.3.8 重排，用户黄框箭头）：
       流程块保持贴卡底，摘要按钮排从"right under the summary grid"下移到流程块正上方
       S(8) 处——v1.3.7 两者之间 add_s 的大空档，现在改到按钮排之前
       （按钮排与摘要网格之间留白）。矮窗流程块跳过时按钮回退紧跟网格。 */
    {
        const struct { int id; const WCHAR *txt; bool danger; } items[5] = {
            { ID_BTN_SUM_REFRESH,  L"Refresh summary",       false },
            { ID_BTN_OPEN_INI,     L"View config.ini", false },
            { ID_BTN_COPY_SUM,     L"Copy summary",       false },
            { ID_BTN_EMERGENCY,    L"Emergency off",       true  },
            { ID_BTN_RESTORE_PREV, L"Restore original",     false },
        };
        int bw[5];
        int total = 0;
        for (int i = 0; i < 5; i++) {
            bw[i] = text_width_f(items[i].txt, g_app.font_ui) + S(26);
            total += bw[i];
        }
        total += S(8) * 4;
        bool one_row = total <= cardw - S(24);
        int btn_block = one_row ? S(40) : S(84);   /* 按钮排总高（含排后余量） */

        /* 流程块几何（贴卡底）：v1.3.9 增量改用规划阶段的封顶值，块高只计
           实际消耗——提示条正好落在卡底 S(10) 上，不再有沉底空白 */
        int tip_h = S(26);
        int flow_base = S(16) + S(22) + S(22)*2 + S(6) + S(16) + tip_h;
        int flow_top = -1, g1 = 0, pitch = 0, gtip = 0;
        int flow_used = flow_base + flow_eg + flow_ep*2 + flow_et;
        if (cardh - S(10) - ry - btn_block - S(8) >= flow_used) {
            g1    = S(22) + flow_eg;
            pitch = S(22) + flow_ep;
            gtip  = S(6)  + flow_et;
            flow_top = cardh - S(10) - flow_used;
        }

        /* 摘要按钮排：流程块上方 S(8)；流程块被跳过时紧跟摘要网格 */
        int cy = (flow_top >= 0) ? flow_top - S(8) - (one_row ? S(32) : S(72))
                                 : ry + S(4);
        if (one_row) {
            int cx = S(12);
            for (int i = 0; i < 5; i++) {
                HWND hb = new_btn(right, items[i].id, items[i].txt, cx, cy, bw[i], S(32),
                                  false, items[i].danger);
                if (items[i].id == ID_BTN_EMERGENCY)    g_app.hwndBtnEmergency = hb;
                if (items[i].id == ID_BTN_RESTORE_PREV) g_app.hwndBtnRestorePrev = hb;
                cx += bw[i] + S(8);
            }
        } else {
            int cx = S(12);
            for (int i = 0; i < 3; i++) {
                HWND hb = new_btn(right, items[i].id, items[i].txt, cx, cy, bw[i], S(32),
                                  false, items[i].danger);
                if (items[i].id == ID_BTN_EMERGENCY)    g_app.hwndBtnEmergency = hb;
                cx += bw[i] + S(8);
            }
            cy += S(40);
            cx = S(12);
            for (int i = 3; i < 5; i++) {
                HWND hb = new_btn(right, items[i].id, items[i].txt, cx, cy, bw[i], S(32),
                                  false, items[i].danger);
                if (items[i].id == ID_BTN_RESTORE_PREV) g_app.hwndBtnRestorePrev = hb;
                cx += bw[i] + S(8);
            }
        }
        EnableWindow(g_app.hwndBtnRestorePrev, FALSE);

        /* 生效流程（几何已在上面算好，贴卡底） */
        if (flow_top >= 0) {
            int fy = flow_top;
            new_label_ex(right, L"How to apply · follow this order after changes", S(12), fy, cardw - S(24), S(16), false, C_TEXT3, false, false);
            fy += g1;
            const WCHAR *flow_name[3] = { L"Save config", L"Register (admin)", L"Restart Explorer" };
            const WCHAR *flow_desc[3] = { L"Write config.ini", L"Only needed once, or after swapping the DLL", L"Makes the background take effect immediately" };
            for (int i = 0; i < 3; i++) {
                new_step_no(right, i + 1, S(12), fy + S(1), S(18));
                new_label_ex(right, flow_name[i], S(38), fy, S(120), S(16), true, C_TEXT, false, false);
                new_label_ex(right, flow_desc[i], S(38) + text_width(flow_name[i], true, false) + S(10), fy + S(1),
                             cardw - S(190), S(16), false, C_TEXT3, false, false);
                fy += pitch;
            }
            fy += S(16) + gtip - pitch;   /* fy 在最后一步 +pitch，提示条顶 = 最后一步内容底 + gtip */
            new_flow_tip(right, L"Afterwards only: Save config → Restart Explorer",
                         S(12), fy, cardw - S(24), tip_h);
        }
    }

    /* 全宽卡：自定义图片目录 */
    int cy2 = y + cardh + S(14);
    int cw = g_app.cw - pad*2;
    HWND folderCard = new_card_panel(parent, x1, cy2, cw, folderCardH);
    new_label_ex(folderCard, L"Custom image folder", S(12), S(12), S(150), S(18), true, C_TEXT, false, false);
    new_chip(folderCard, L"folder", S(12) + text_width(L"Custom image folder", true, false) + S(10), S(13),
             false, 0);
    {
        const WCHAR *hint = L"Only works when custom above is ticked";
        int hw = text_width_f(hint, g_app.font_ui);
        new_label_ex(folderCard, hint, cw - S(16) - hw, S(14), hw, S(16),
                     false, C_TEXT3, false, false);
    }
    /* v1.3.3：输入框与按钮统一 32px 高（设计稿 .path-row input / .btn height:32px） */
    g_app.hwndEditFolder = new_edit(folderCard, ID_EDIT_FOLDER, L"", S(12), S(36), cw - S(100), S(32), false);
    new_btn(folderCard, ID_BTN_PICK_FOLDER, L"Browse...", cw - S(80), S(36), S(68), S(32), false, false);
    new_label_ex(folderCard, L"Tip: with no custom folder set, the plugin's Image folder is used by default.",
                 S(12), S(74), cw - S(24), S(16), false, C_TEXT3, false, false);
}

static void build_images_tab(HWND parent) {
    int pad = S(16);
    int y = pad;
    int h = g_panel_h - pad*2;          /* 用面板可用高度，避免与底部操作栏重叠 */
    if (h < S(240)) h = S(240);
    int lw = (g_app.cw - pad*3) * 55 / 100;
    int rw = g_app.cw - pad*3 - lw;

    HWND left = new_card_panel(parent, pad, y, lw, h);
    HWND right = new_card_panel(parent, pad*2+lw, y, rw, h);

    new_label(left, L"Image list", S(12), S(10), S(100), S(18), true, C_TEXT);
    /* 计数靠右（设计稿 .list-head .cnt margin-left:auto） */
    {
        const WCHAR *cnt = L"0 images · png/jpg/bmp recommended";
        int tw = text_width_f(cnt, g_app.font_ui) + S(24);
        if (tw > lw - S(160)) tw = lw - S(160);
        g_app.hwndImgCount = new_label_ex(left, cnt, lw - S(12) - tw, S(12), tw, S(16),
                                          false, C_TEXT3, true, false);
    }
    new_btn(left, ID_BTN_IMG_REFRESH, L"Refresh list", S(12), S(34), S(88), S(32), false, false);
    new_btn(left, ID_BTN_OPEN_IMG_DIR, L"Open image folder", S(108), S(34), S(122), S(32), false, false);
    int lvH = h - S(84);
    if (lvH < S(60)) lvH = S(60);
    g_app.hwndImgList = new_listview(left, ID_LV_IMAGES, S(12), S(72), lw-S(24), lvH);
    /* 两列：文件名靠左、体积靠右（设计稿 file-item 的 fi-name / fi-meta 布局） */
    {
        int lwv = lw - S(24);
        int sizew = S(84);
        LVCOLUMNW col = {0};
        col.mask = LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
        col.fmt = LVCFMT_LEFT;  col.cx = lwv - sizew - S(8); ListView_InsertColumn(g_app.hwndImgList, 0, &col);
        col.fmt = LVCFMT_RIGHT; col.cx = sizew;              ListView_InsertColumn(g_app.hwndImgList, 1, &col);
    }

    new_label(right, L"Preview", S(12), S(10), S(80), S(18), true, C_TEXT);
    /* 预览台按设计稿 16:9，空间不够时按可用高度反推宽度并居中 */
    {
        int avail_w = rw - S(24);
        int avail_h = h - S(34) - S(48);
        int pv_w = avail_w, pv_h = avail_w * 9 / 16;
        if (pv_h > avail_h) { pv_h = avail_h; pv_w = pv_h * 16 / 9; }
        if (pv_h < S(60)) pv_h = S(60);
        int pv_x = S(12) + (avail_w - pv_w) / 2;
        g_app.hwndPreview = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_CENTERIMAGE,
                                          pv_x, S(34), pv_w, pv_h, right,
                                          (HMENU)(INT_PTR)ID_ST_PREVIEW, NULL, NULL);
        /* 设计稿 .pmeta：文件名（mono 粗）+ 尺寸 + GDI+ 解码 chip。
           v1.3.11（用户红框）：三件套是"after picking an image"的元信息行，空状态下
           文件名/尺寸为空串、只剩芯片孤零零挂着像乱入文字——初始隐藏，
           选中图片后由 preview_meta_update() 按 flex gap 排布显示 */
        int my = S(34) + pv_h + S(8);
        g_app.hwndPreviewName = new_label_ex(right, L"", S(12), my, S(190), S(16),
                                             true, C_TEXT2, false, true);
        g_app.hwndPreviewDim = new_label_ex(right, L"", S(12) + S(196), my, S(100), S(16),
                                            false, C_TEXT3, false, false);
        g_app.hwndPreviewCodec = new_chip(right, L"GDI+ decode", S(12) + S(300), my + S(1), false, 0);
        ShowWindow(g_app.hwndPreviewName, SW_HIDE);
        ShowWindow(g_app.hwndPreviewDim, SW_HIDE);
        ShowWindow(g_app.hwndPreviewCodec, SW_HIDE);
    }
}

/* ------------------------------------------------------------------ */
/* v1.3.9：自绘下拉选择器状态（替代原生 COMBOBOX——原生下拉的立体边框、  */
/* 箭头与整体风格不符；交互与「界面缩放」按钮一致：按钮 + 弹出菜单）     */
/* ------------------------------------------------------------------ */
typedef struct {
    HWND   btn;
    WCHAR (*items)[MAX_PATH_W];
    int    count, cap;
    int    cur;                       /* 当前选中 index，-1 = 未选 */
} ComboState;

static ComboState g_cb_clsid, g_cb_img, g_cb_rot;   /* g_cb_rot：v1.3.20(C) 定时轮换 */
static ComboState g_cb_rotunit;   /* v1.3.25(D1)：单位下拉（分钟/秒钟） */

static void combo_state_clear(ComboState *cb) {
    free(cb->items);
    cb->items = NULL; cb->count = 0; cb->cap = 0; cb->cur = -1;
}
static int combo_state_add(ComboState *cb, const WCHAR *txt) {
    if (cb->count == cb->cap) {
        int nc = cb->cap ? cb->cap * 2 : 16;
        WCHAR (*ni)[MAX_PATH_W] = (WCHAR(*)[MAX_PATH_W])realloc(cb->items, (size_t)nc * sizeof(*ni));
        if (!ni) return -1;
        cb->items = ni; cb->cap = nc;
    }
    int i = cb->count++;
    lstrcpynW(cb->items[i], txt, MAX_PATH_W);
    return i;
}
static void combo_state_set(ComboState *cb, int idx) {
    if (idx < -1 || idx >= cb->count) idx = -1;
    cb->cur = idx;
    if (cb->btn) SetWindowTextW(cb->btn, idx >= 0 ? cb->items[idx] : L"");
}
static int combo_state_find(ComboState *cb, const WCHAR *txt) {
    for (int i = 0; i < cb->count; i++)
        if (_wcsicmp(cb->items[i], txt) == 0) return i;
    return -1;
}

/* ------------------------------------------------------------------ */
/* v1.3.20(C)：定时轮换控件（random 行内右侧）                          */
/* 下拉按钮与「界面缩放」同源（new_btn_dropdown + TrackPopupMenu，      */
/* 当前档打勾）；选「自定义…」时左侧出现整数分钟输入框 —— 内层 EDIT     */
/* 挂 ES_NUMBER，小数点/字母根本输不进去（从输入源头兑现"integers only"）。  */
/* 随机显示关闭时整组置灰。                                            */
/* ------------------------------------------------------------------ */

static void rot_ui_init(HWND parent, int cx, int cy, int w, int h, int suby) {
    /* v1.3.27(F2)：原五个文件级全局全部降为本函数局部变量（见全局块注释） */
    int label_y = cy + (h - S(16)) / 2;
    int sx = S(12);
    int edit_x, unit_x;
    /* v1.3.26(E1) 用户红框：顶部行只留 [说明文字][档位下拉]，
     * 输入框/单位框搬进 random 行下方的专用子行，宽度放开 ——
     * 输入框 S(80)（容 6 位数字，远超 86400 上限），单位框 S(88)
     * 让"Seconds"二字完整显示，不再与开关文字挤在一行。 */
    /* v1.3.28(P2，Deepseek 复审)：句柄无读者，不再保存（见全局块注释） */
    new_label_ex(parent, L"Timed rotation", cx - S(148), label_y,
                 S(140), S(16), false, C_TEXT3, true, false);
    g_app.hwndRotBtn = new_btn_dropdown(parent, ID_CB_ROT, L"", cx, cy, w, h);
    /* v1.3.21(G1，Deepseek 审计)：漏了这行回填导致按钮面永远空白 ——
     * combo_state_set 里 if (cb->btn) 整句被跳过，按钮文字停留在创建时的 L""。
     * 对照 g_cb_clsid / g_cb_img 两处均有回填，唯独此处遗漏。 */
    g_cb_rot.btn = g_app.hwndRotBtn;
    combo_state_clear(&g_cb_rot);
    for (int i = 0; i < 7; i++) combo_state_add(&g_cb_rot, ROT_CHOICES[i]);
    /* 子行：标题 + 输入框 + 单位框（默认隐藏，选「自定义…」才出现） */
    hRotSubLabel = new_label_ex(parent, L"Custom interval", sx, suby + S(5),
                                S(150), S(16), false, C_TEXT3, false, false);
    edit_x = sx + S(150) + S(10);
    g_app.hwndRotMinEdit = new_edit(parent, ID_EDIT_ROTMIN, L"",
                                    edit_x, suby, S(80), S(24), false);
    SetWindowLongPtrW(g_app.hwndRotMinEdit, GWL_STYLE,
                      GetWindowLongPtrW(g_app.hwndRotMinEdit, GWL_STYLE) | ES_NUMBER);
    unit_x = edit_x + S(80) + S(6);
    g_app.hwndRotUnitBtn = new_btn_dropdown(parent, ID_CB_ROTUNIT, L"",
                                            unit_x, suby, S(96), S(24));
    g_cb_rotunit.btn = g_app.hwndRotUnitBtn;
    combo_state_clear(&g_cb_rotunit);
    for (int i = 0; i < 2; i++) combo_state_add(&g_cb_rotunit, ROT_UNIT_CHOICES[i]);
    /* 非自定义态的子行提示（与控件互斥显隐，见 rot_ui_sync） */
    hRotSubHint = new_label_ex(parent,
        L"Pick \"Custom…\" above, then enter the interval here",
        sx, suby + S(5), S(424), S(16), false, C_TEXT3, false, false);
    rot_ui_sync();
}

/* 按 g_app.v_rotsec / v_rotunit / v_random 同步：下拉文本、子行控件显隐、
 * 整组置灰（v1.3.26(E1)：子行方案 —— 控件常驻 random 行下方专用子行，
 * 显隐只发生在子行内部，顶部说明文字位置不再横移。
 * v1.3.27(F3)：自定义态判定加入独立状态位，见 g_rot_custom_picked 注释） */
static void rot_ui_sync(void) {
    if (!g_app.hwndRotBtn || !g_app.hwndRotMinEdit || !g_app.hwndRotUnitBtn) return;
    int cur = -1;
    for (int i = 0; i < 6; i++)
        if (ROT_SECONDS[i] == g_app.v_rotsec) { cur = i; break; }
    /* v1.3.27(F3)：自定义态 = 用户主动选了「自定义…」 || 数值落在预设之外
     * （启动加载时按数值反推）。此前纯靠数值反推，用户在自定义态敲出
     * 60/120/300 等恰好命中预设秒数时，输入框当场消失、档位跳回预设，
     * 观感像"was reset"；现在只要用户主动进了自定义态就保持自定义显示，
     * 输入框里的数字不动，保存值也不变（数值语义与预设完全等价）。 */
    bool custom = g_rot_custom_picked || (cur < 0 && g_app.v_rotsec > 0);
    if (cur < 0 || g_rot_custom_picked)
        cur = custom ? 6 : 0;
    combo_state_set(&g_cb_rot, cur);
    HWND frame = GetParent(g_app.hwndRotMinEdit);
    bool show = (custom && g_app.v_random);
    if (frame)
        ShowWindow(frame, show ? SW_SHOW : SW_HIDE);
    ShowWindow(g_app.hwndRotUnitBtn, show ? SW_SHOW : SW_HIDE);
    /* v1.3.27(F1，Deepseek 审查)：子行标题随随机开关显隐 ——
     * 随机关时输入框/单位框/提示全部隐藏，只留标题就是一行孤立文字 */
    if (hRotSubLabel)
        ShowWindow(hRotSubLabel, g_app.v_random ? SW_SHOW : SW_HIDE);
    /* 子行提示只在"random on + not custom"时出现；随机关时整行无内容可填，
     * 提示反而误导，一并隐藏 */
    if (hRotSubHint)
        ShowWindow(hRotSubHint, (!custom && g_app.v_random) ? SW_SHOW : SW_HIDE);
    EnableWindow(g_app.hwndRotBtn, g_app.v_random);
    EnableWindow(g_app.hwndRotMinEdit, g_app.v_random);
    EnableWindow(g_app.hwndRotUnitBtn, g_app.v_random);
    if (custom) {
        /* 显示单位跟随用户选择；但"Minutes"只在整分钟时成立，否则自动落回
         * 秒钟显示（例如 90 秒没法用整数分钟表达，不静默改值）。
         * v1.3.25(D1) 一致性：落回时必须回写 v_rotunit —— 输入框显示的是
         * 秒数，EN_CHANGE 与保存路径都按 v_rotunit 换算，不同步就会把
         * "90 sec"错算成"90 minutes"。 */
        if (g_app.v_rotunit == ROT_UNIT_MIN && (g_app.v_rotsec % 60) != 0)
            g_app.v_rotunit = ROT_UNIT_SEC;
        combo_state_set(&g_cb_rotunit, g_app.v_rotunit);
        WCHAR b[32];
        sfmt(b, 32, L"%d", rotval_to_display(g_app.v_rotsec));
        SetWindowTextW(g_app.hwndRotMinEdit, b);
    }
    /* v1.3.26：顶部说明文字固定在档位下拉左侧，不再横移让位 */
}

/* 点「定时轮换」下拉按钮：弹 7 档菜单，当前档打勾（与「界面缩放」同模式） */
static void on_rot_menu(HWND hwnd) {
    if (!g_app.hwndRotBtn || !g_app.v_random) return;
    HMENU menu = CreatePopupMenu();
    if (!menu) return;
    int cur = -1;
    for (int i = 0; i < 6; i++)
        if (ROT_SECONDS[i] == g_app.v_rotsec) { cur = i; break; }
    /* v1.3.27(F3)：菜单打勾与 rot_ui_sync 的自定义态判定保持同一套逻辑 */
    if (cur < 0 || g_rot_custom_picked)
        cur = (g_rot_custom_picked || g_app.v_rotsec > 0) ? 6 : 0;
    for (int i = 0; i < 7; i++)
        AppendMenuW(menu, MF_STRING | MF_ENABLED | (i == cur ? MF_CHECKED : 0),
                    IDM_ROT_FIRST + i, ROT_CHOICES[i]);
    RECT rc; GetWindowRect(g_app.hwndRotBtn, &rc);
    int sel = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
                             rc.left, rc.bottom, 0, hwnd, NULL);
    DestroyMenu(menu);
    if (sel < IDM_ROT_FIRST || sel > IDM_ROT_FIRST + 6) return;
    int i = sel - IDM_ROT_FIRST;
    if (ROT_SECONDS[i] < 0) {
        /* 自定义：当前值是预设/无效时给一个非预设起点（3 分钟），直进编辑。
         * v1.3.25(D1)：单位随起点重置为分钟，用户可再切秒钟。
         * v1.3.27(F3)：标记"user explicitly entered custom mode"——之后手敲的值即使
         * 恰好等于预设秒数（如 120），也保持自定义显示不跳档。
         * v1.3.28(P3，Deepseek 复审)：重置只发生在"first entry into custom from a preset/invalid\n         * value"时；已在自定义态（用户手敲过值）再点一次「自定义…」
         * 不得重置 —— 120 是用户填的数，重新选个下拉不该把它顶回 180。
         * 注意先取旧值再置位，否则 already_custom 永远为 true。 */
        bool already_custom = g_rot_custom_picked;
        g_rot_custom_picked = true;
        if (!already_custom) {
            bool preset = false;
            for (int k = 0; k < 6; k++)
                if (ROT_SECONDS[k] == g_app.v_rotsec) preset = true;
            if (preset || g_app.v_rotsec <= 0) {
                g_app.v_rotsec = 180;
                g_app.v_rotunit = ROT_UNIT_MIN;
            }
        }
        rot_ui_sync();
        SetFocus(g_app.hwndRotMinEdit);
    } else {
        /* v1.3.27(F3)：回到预设档，清除自定义态标志 */
        g_rot_custom_picked = false;
        g_app.v_rotsec = ROT_SECONDS[i];
        rot_ui_sync();
    }
    /* v1.3.21(G2，Deepseek 审计)：选档后立即刷新"Live configuration"摘要，
     * 不再等用户手动点「刷新摘要」 */
    update_summary();
}

/* v1.3.25(D1)：点单位下拉（分钟/秒钟）。
 * 切到秒钟：直接换显示，实际间隔秒数不变；
 * 切到分钟：整分钟（%60==0）才切，否则向上取整到分钟并记日志 ——
 *           输入框是 ES_NUMBER 整数，"90 sec"没法用分钟如实显示，
 *           与其静默跳值，不如明说"rounded up to N minutes"。 */
static void on_rotunit_menu(HWND hwnd) {
    if (!g_app.hwndRotUnitBtn || !g_app.v_random) return;
    HMENU menu = CreatePopupMenu();
    if (!menu) return;
    int unit = g_app.v_rotunit;
    if (unit == ROT_UNIT_MIN && (g_app.v_rotsec % 60) != 0)
        unit = ROT_UNIT_SEC;    /* 与 rot_ui_sync 的显示逻辑一致 */
    for (int i = 0; i < 2; i++)
        AppendMenuW(menu, MF_STRING | MF_ENABLED | (i == unit ? MF_CHECKED : 0),
                    IDM_ROTUNIT_FIRST + i, ROT_UNIT_CHOICES[i]);
    RECT rc; GetWindowRect(g_app.hwndRotUnitBtn, &rc);
    int sel = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
                             rc.left, rc.bottom, 0, hwnd, NULL);
    DestroyMenu(menu);
    if (sel < IDM_ROTUNIT_FIRST || sel > IDM_ROTUNIT_FIRST + 1) return;
    int pick = sel - IDM_ROTUNIT_FIRST;
    if (pick == ROT_UNIT_MIN && (g_app.v_rotsec % 60) != 0) {
        int snapped = ((g_app.v_rotsec + 59) / 60) * 60;
        if (snapped > 86400) snapped = 86400;
        g_app.v_rotsec = snapped;
        WCHAR b[160];
        sfmt(b, 160, L"Sub-minute seconds can't be shown in whole minutes; interval rounded to %d min", snapped / 60);
        log_info(b);
    }
    g_app.v_rotunit = pick;
    rot_ui_sync();
    update_summary();
}

/* v1.3.21(G2，Deepseek 审计)：自定义时间输入框实时同步。
 * 手改数字时 g_app.v_rotsec 跟着走（按当前单位换算成秒，钳位 0~86400，
 * 与保存路径同一套解析），摘要的「本次刷新」时间戳立即跟上；
 * 不必等点「保存配置」才读一次。空串按 0 处理（不轮换）。
 * 注意：本函数不回写输入框（无 SetWindowTextW），EN_CHANGE 不会成环；
 * rot_ui_sync 里 SetWindowTextW 触发的一次 EN_CHANGE 解析回同值，无害。 */
static void on_rotval_edit_change(void) {
    if (!g_app.hwndRotMinEdit || !g_app.hwndSumTime) return;
    WCHAR b[32] = L"";
    GetWindowTextW(g_app.hwndRotMinEdit, b, 32);
    int v = _wtoi(b);
    if (v < 0) v = 0;
    int sec = (g_app.v_rotunit == ROT_UNIT_MIN) ? v * 60 : v;
    if (sec > 86400) sec = 86400;
    g_app.v_rotsec = sec;
    update_summary();
}

static void build_folders_tab(HWND parent) {
    int pad = S(16);
    int y = pad;
    int h = g_panel_h - pad*2;          /* 同上 */
    if (h < S(240)) h = S(240);
    int lw = (g_app.cw - pad*3) * 55 / 100;
    int rw = g_app.cw - pad*3 - lw;

    HWND left = new_card_panel(parent, pad, y, lw, h);
    HWND right = new_card_panel(parent, pad*2+lw, y, rw, h);

    new_label(left, L"Per-folder background rules", S(12), S(10), S(220), S(18), true, C_TEXT);
    {
        const WCHAR *cnt = L"0 rules";
        int tw = text_width_f(cnt, g_app.font_ui) + S(24);
        if (tw > lw - S(160)) tw = lw - S(160);
        g_app.hwndRuleCount = new_label_ex(left, cnt, lw - S(12) - tw, S(12), tw, S(16),
                                           false, C_TEXT3, true, false);
    }
    new_label_ex(left, L"Writes ini sections like [path] img=filename", S(12), S(30), lw-S(24), S(16),
                 false, C_TEXT3, false, false);
    g_app.hwndRuleList = new_listview(left, ID_LV_RULES, S(12), S(52), lw-S(24), h-S(64));
    /* 两列：文件夹路径靠左、图片名靠右（设计稿 rule-item 的 rule-folder / rule-img） */
    {
        int lwv = lw - S(24);
        int imgw = S(150);
        LVCOLUMNW col = {0};
        col.mask = LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
        col.fmt = LVCFMT_LEFT;  col.cx = lwv - imgw - S(8); ListView_InsertColumn(g_app.hwndRuleList, 0, &col);
        col.fmt = LVCFMT_RIGHT; col.cx = imgw;              ListView_InsertColumn(g_app.hwndRuleList, 1, &col);
    }

    int ry = S(10);
    new_label(right, L"Add / edit rule", S(12), ry, rw-S(24), S(18), true, C_TEXT);
    ry += S(26);
    new_label(right, L"Folder path", S(12), ry, S(100), S(16), false, C_TEXT);
    ry += S(20);
    g_app.hwndRulePath = new_edit(right, ID_EDIT_RULE_PATH, L"", S(12), ry, rw-S(120), S(32), false);
    new_btn(right, ID_BTN_RULE_ADD, L"Add/Update", rw-S(100), ry, S(88), S(32), false, false);
    ry += S(44);
    new_label(right, L"Insert CLSID", S(12), ry, S(100), S(16), false, C_TEXT);
    ry += S(20);
    /* v1.3.9：原生 COMBOBOX → 自绘下拉按钮（外观与「界面缩放」、输入框圆角统一） */
    combo_state_clear(&g_cb_clsid);
    g_app.hwndClsid = new_btn_dropdown(right, ID_CB_CLSID, L"", S(12), ry, rw-S(24), S(32));
    g_cb_clsid.btn = g_app.hwndClsid;
    combo_state_add(&g_cb_clsid, L"— pick a special system path —");
    for (int i = 0; i < 3; i++) combo_state_add(&g_cb_clsid, BUILTIN_CLIDS[i]);
    combo_state_set(&g_cb_clsid, 0);
    ry += S(44);
    new_label(right, L"Image file name", S(12), ry, S(110), S(16), false, C_TEXT);
    new_chip(right, L"In Image folder", S(12) + text_width(L"Image file name", false, false) + S(8), ry + S(1), false, 0);
    ry += S(20);
    combo_state_clear(&g_cb_img);
    g_app.hwndRuleImg = new_btn_dropdown(right, ID_CB_RULE_IMG, L"", S(12), ry, rw-S(24), S(32));
    g_cb_img.btn = g_app.hwndRuleImg;
    ry += S(44);
    new_btn(right, ID_BTN_RULE_DEL, L"Delete selected", S(12), ry, S(100), S(32), false, true);
    new_btn(right, ID_BTN_RULE_REFRESH, L"Refresh list", S(122), ry, S(86), S(32), false, false);
    ry += S(44);
    new_label(right, L"Select a rule on the left to autofill; Add/Update overwrites it.",
              S(12), ry, rw-S(24), S(32), false, C_TEXT3);
}

/* 当前活动页（0=基础配置 1=图片管理 2=文件夹专属背景）。
   v1.3.6 起选项卡为 3 个自绘按钮，hover 高亮走 BUTTON 自带的 ODS_HOTLIGHT，
   不再需要 TCM_HITTEST 子类追踪。 */
static int g_cur_tab = 0;

static void build_ui(HWND hwnd) {
    RECT rc; GetClientRect(hwnd, &rc);
    int w = rc.right, h = rc.bottom;
    /* 布局一律用实际客户区像素（v1.3.2） */
    g_app.cw = w; g_app.ch = h;
    g_app.base_w = w; g_app.base_h = h;

    /* 顶部工具栏（设计稿 .toolbar 背景 #fff）
       v1.3.3：工具栏、状态条、选项卡统一用累计 y 定位（不再写死 S(58)/S(100)/S(136)），
       方便按 32px 控件高度调整后不错位 */
    const int tool_h   = S(64);
    const int status_h = S(42);
    const int tab_h    = S(36);
    const int y_status = tool_h;
    const int y_tab    = y_status + status_h;
    const int y_panel  = y_tab + tab_h;

    HWND tool = new_panel(hwnd, 0, 0, w, tool_h, PANEL_BG_WHITE);
    new_label(tool, L"Plugin folder", S(16), S(8), S(120), S(16), false, C_TEXT2);
    /* 输入框 + 按钮统一 32px 高（v1.3.3），垂直方向在 64px 工具栏里居中偏下 */
    /* v1.3.5：输入框宽度修正——旧值 w-S(236) 的右缘(w-220)越过浏览按钮起点
       (w-228) 8px，两者真重叠（用户标注"blended together, no clear boundary"的根因）。
       现在：输入框右缘 w-236，与浏览按钮间隔 8px（设计稿 .fld-row gap:8px） */
    g_app.hwndPluginDir = new_edit(tool, ID_EDIT_PLUGIN_DIR, g_app.plugin_dir, S(16), S(26), w-S(264), S(32), false);
    new_btn(tool, ID_BTN_BROWSE_DIR, L"Browse...", w-S(240), S(26), S(68), S(32), false, false);
    new_btn(tool, ID_BTN_LOCATE, L"Reveal", w-S(160), S(26), S(48), S(32), false, false);
    /* 界面缩放：v1.3.4 改自绘下拉按钮 + TrackPopupMenu（原生 COMBOBOX 箭头不居中、
       外观突兀，用户点名要求替换）。右缘距窗口 16px；标签右对齐到按钮右缘 */
    new_label_ex(tool, L"UI scaling", w - S(16) - S(88), S(8), S(88), S(16), false, C_TEXT2, true, false);
    g_app.hwndScaleBtn = new_btn_dropdown(tool, ID_CB_SCALE, SCALE_CHOICES[g_app.scale_idx + 1],
                                          w - S(16) - S(88), S(26), S(88), S(32));

    /* 状态条（自绘窗口类，画状态胶囊） */
    g_app.hwndStatus = CreateWindowW(L"backimgStatusBar", L"",
                                      WS_CHILD | WS_VISIBLE,
                                      0, y_status, w, status_h, hwnd, NULL, NULL, NULL);

    /* 选项卡：v1.3.6 改 3 个纯自绘按钮——SysTabControl32 的 TCS_BUTTONS
       系统 3D 边框由系统绘制、owner-draw 只接管内容区，边框去不掉（绿框标注）。
       设计稿 .tabs padding 0 16px gap 2px；.tab padding 11px 16px 按文字自适应宽 */
    {
        const WCHAR *tab_names[3] = { L"Basic config", L"Image manager", L"Per-folder background" };
        int tx = S(16);
        for (int i = 0; i < 3; i++) {
            int tw = text_width_f(tab_names[i], g_app.font_bold) + S(32);
            g_app.hwndTabs[i] = new_tab_btn(hwnd, ID_TAB_BASIC + i, tab_names[i],
                                            tx, y_tab, tw, tab_h, i);
            tx += tw + S(2);
        }
    }    /* v1.3.5：选项卡行底 1px 全宽分隔线（设计稿 .tabs border-bottom #E3E7EF），
       活动页蓝色下划线正好压在线上，与设计稿一致 */
    new_panel(hwnd, 0, y_tab + tab_h - 1, w, 1, PANEL_BG_LINE);

    /* 底部布局：日志面板（头 + 正文）在最下，其上为底栏，剩余给面板区 */
    int log_body_h = S(130);
    int log_head_h = S(28);
    int log_panel_h = log_head_h + log_body_h + 2;   /* 2 = 上下边框 */
    int log_panel_y = h - S(16) - log_panel_h;
    int bot_y = log_panel_y - S(48);   /* 底栏高度同步 bot_h（v1.3.3 按钮 32px） */

    /* 面板区 */
    int py = y_panel;
    int ph = bot_y - py;
    if (ph < S(200)) ph = S(200);
    g_panel_h = ph;
    g_app.py_panel = py;
    g_app.ph_panel = ph;
    hTabBasic = CreateWindowExW(WS_EX_CONTROLPARENT, L"STATIC", L"",
                                 WS_CHILD | WS_VISIBLE, 0, py, w, ph, hwnd, NULL, NULL, NULL);
    hTabImages = CreateWindowExW(WS_EX_CONTROLPARENT, L"STATIC", L"",
                                  WS_CHILD, 0, py, w, ph, hwnd, NULL, NULL, NULL);
    hTabFolders = CreateWindowExW(WS_EX_CONTROLPARENT, L"STATIC", L"",
                                   WS_CHILD, 0, py, w, ph, hwnd, NULL, NULL, NULL);

    build_basic_tab(hTabBasic);
    build_images_tab(hTabImages);
    build_folders_tab(hTabFolders);

    /* 底部操作栏（设计稿 .actions 背景 #fff）
       v1.3.6：设计稿 .actions 里「重启资源管理器」和「刷新状态」之间有
       <span class="spacer">，即前 4 个左起、后 3 个（刷新状态/备份配置/
       恢复配置）右对齐到窗口右缘——用户红框箭头明确指向右缘。
       v1.3.4 起一直 7 个连排左起，是漏看 spacer。 */
    const int bot_h = S(48);
    HWND bot = new_panel(hwnd, 0, bot_y, w, bot_h, PANEL_BG_WHITE);
    {
        const struct { int id; const WCHAR *txt; int sty; } bb[] = {
            { ID_BTN_SAVE,     L"Save config",      3 },   /* primary + ✓ */
            { ID_BTN_REG,      L"Register (admin)",    0 },
            { ID_BTN_UNREG,    L"Unregister (admin)",    0 },
            { ID_BTN_RESTART,  L"Restart Explorer", 0 },
            { ID_BTN_REFRESH,  L"Refresh status",      0 },
            { ID_BTN_BACKUP,   L"Back up config",      0 },
            { ID_BTN_RESTORE,  L"Restore config",      0 },
        };
        const int n = 7;
        int byy = (bot_h - S(32)) / 2;
        int bw[7];
        for (int i = 0; i < n; i++) {
            bw[i] = text_width_f(bb[i].txt, g_app.font_ui) + S(26);
            if (bb[i].sty & 1) bw[i] += S(14);   /* ✓ 图标占位 */
        }
        int bx = S(16);
        for (int i = 0; i < 4; i++) {                       /* 左组：保存…重启 */
            HWND hb = (bb[i].sty & 1)
                        ? new_btn_ok(bot, bb[i].id, bb[i].txt, bx, byy, bw[i], S(32))
                        : new_btn(bot, bb[i].id, bb[i].txt, bx, byy, bw[i], S(32), false, false);
            if (bb[i].id == ID_BTN_REG)     g_app.hwndBtnReg = hb;
            if (bb[i].id == ID_BTN_UNREG)   g_app.hwndBtnUnreg = hb;
            if (bb[i].id == ID_BTN_RESTART) g_app.hwndBtnRestart = hb;
            bx += bw[i] + S(12);
        }
        int rx = w - S(16);                                 /* 右组：从右往左放 */
        for (int i = n - 1; i >= 4; i--) {
            rx -= bw[i];
            new_btn(bot, bb[i].id, bb[i].txt, rx, byy, bw[i], S(32), false, false);
            rx -= S(12);
        }
    }

    /* 日志面板：自绘圆角白卡（UI-6）+ 浅灰头 + 正文列表 */
    int lpw = w - S(32);
    HWND logPanel = new_card_panel(hwnd, S(16), log_panel_y, lpw, log_panel_h);
    HWND logHead = new_panel(logPanel, 1, 1, lpw - 2, log_head_h, PANEL_BG_LOGHEAD);
    {
        HWND logTitle = CreateWindowW(L"STATIC", L"Run log",
                                      WS_CHILD | WS_VISIBLE | SS_LEFT,
                                      S(12), 0, S(120), log_head_h, logHead, NULL, NULL, NULL);
        if (g_app.font_bold) SendMessageW(logTitle, WM_SETFONT, (WPARAM)g_app.font_bold, 0);
        SetWindowLongPtrW(logTitle, GWLP_USERDATA, (LONG_PTR)C_LOG_HEAD_TXT);
        new_mini_btn(logHead, ID_BTN_LOG_CLEAR, L"Clear",
                     lpw - 2 - S(12) - S(44), (log_head_h - S(20)) / 2, S(44), S(20));
    }
    g_app.hwndLog = CreateWindowW(L"LISTBOX", L"",
                                   WS_CHILD | WS_VISIBLE | WS_VSCROLL |
                                   LBS_OWNERDRAWFIXED | LBS_NOINTEGRALHEIGHT |
                                   LBS_HASSTRINGS | LBS_NOSEL,
                                   1, log_head_h + 1, lpw - 2, log_body_h,
                                   logPanel, NULL, NULL, NULL);
    SendMessageW(g_app.hwndLog, WM_SETFONT, (WPARAM)g_app.font_mono, 0);
    SendMessageW(g_app.hwndLog, LB_SETITEMHEIGHT, 0, MAKELPARAM(pt_scaled(17), 0));
}

#undef S

/* ------------------------------------------------------------------ */
/* 绘制处理                                                            */
/* ------------------------------------------------------------------ */
static void draw_owner_button(HWND hwnd, HDC dc, bool hover, bool pushed) {
    RECT rc; GetClientRect(hwnd, &rc);
    LONG style = GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    bool primary = (style & 1);
    bool danger = (style & 2);
    bool is_switch = (style & 4);
    bool is_seg = (style & 8);
    bool is_mini = (style & BTN_STYLE_MINI);
    bool is_okbtn = (style & BTN_STYLE_ICON_OK) != 0;

    if (is_mini) {
        /* 背景取父容器底色，hover 时浅蓝胶囊（设计稿 .log-head .mini:hover） */
        LONG_PTR pst = 0;
        HWND ph = GetParent(hwnd);
        if (ph) pst = GetWindowLongPtrW(ph, GWLP_USERDATA);
        COLORREF pbg = panel_bg_color(pst);
        fill_rounded_rect(dc, &rc, pt_scaled(4), hover ? C_ACCENT_SOFT : pbg);
        WCHAR txt[128] = {0};
        GetWindowTextW(hwnd, txt, 128);
        draw_text_dc(dc, txt, &rc, hover ? C_ACCENT : C_TEXT3, true, g_app.font_ui);
        return;
    }

    if (style & BTN_STYLE_TAB) {
        /* 设计稿 .tab：默认无边框透明底（tabs 行白底）、hover #F4F6FA、
           选中 color accent + ::after 底部 2.5px 蓝条（left/right 12px） */
        int idx = (int)((style >> 4) & 0xF);
        bool active = (g_cur_tab == idx);
        HBRUSH br = CreateSolidBrush(!active && hover ? C_HOVER_BG : C_PANEL);
        FillRect(dc, &rc, br); DeleteObject(br);
        WCHAR txt[64] = {0};
        GetWindowTextW(hwnd, txt, 64);
        draw_text_dc(dc, txt, &rc, active ? C_ACCENT : (hover ? C_TEXT : RGB(0x5B,0x64,0x72)),
                     true, g_app.font_bold);
        if (active) {
            RECT bar = { rc.left + pt_scaled(12), rc.bottom - pt_scaled(3),
                         rc.right - pt_scaled(12), rc.bottom };
            HBRUSH bb = CreateSolidBrush(C_ACCENT);
            FillRect(dc, &bar, bb); DeleteObject(bb);
        }
        return;
    }

    if (is_switch) {
        int idx = (int)((style >> 4) & 0xF);
        bool on = sw_value(idx);
        fill_rounded_rect(dc, &rc, pt_scaled(11), on ? C_ACCENT : C_TRACK_OFF);
        RECT knob;
        int r = (rc.bottom - rc.top) / 2 - pt_scaled(2);
        if (r < 2) r = 2;                       /* 控件被压扁时半径为负会画出怪图形 */
        if (r > (rc.bottom - rc.top) / 2) r = (rc.bottom - rc.top) / 2;
        int cx = on ? rc.right - r - pt_scaled(3) : rc.left + r + pt_scaled(3);
        /* v1.3.5 精修：圆钮先垫 1px 下移的浅灰阴影（设计稿 .sw-track::after
           box-shadow:0 1px 3px 的 GDI 近似），再画白底圆钮 + 1px 浅描边，
           立体感和边界感明显强于之前的纯白平面圆 */
        RECT sh = { cx - r, rc.top + pt_scaled(3), cx + r, rc.bottom - pt_scaled(1) };
        fill_rounded_rect(dc, &sh, r, C_SHADOW);
        knob.left = cx - r; knob.right = cx + r;
        knob.top = rc.top + pt_scaled(2); knob.bottom = rc.bottom - pt_scaled(2);
        fill_rounded_rect(dc, &knob, r, C_PANEL);
        stroke_ellipse(dc, &knob, RGB(0xE2, 0xE8, 0xF2));
        return;
    }

    /* v1.3.3：hover / danger 三态都按设计稿补上 hover 变化
       （.btn:hover 底 #F4F6FA 边框 #CDD5E1；.primary:hover #2456E0；.danger:hover #FDECEC/#E5A0A0） */
    COLORREF bg, fg, border;
    if (primary) { bg = hover ? C_ACCENT_HOVER : C_ACCENT; fg = C_PANEL; border = bg; }
    else if (danger) { bg = hover ? RGB(0xFD,0xEC,0xEC) : C_DANGER_SOFT; fg = C_DANGER; border = hover ? RGB(0xE5,0xA0,0xA0) : C_DANGER_LINE; }
    else { bg = hover ? C_HOVER_BG : C_PANEL; fg = C_TEXT; border = hover ? C_HOVER_LINE : C_BORDER; }

    WCHAR seg_txt[32] = L"";
    bool seg_on = false;
    if (is_seg) {
        int idx = (int)((style >> 4) & 0xF);
        seg_on = (idx == g_app.v_posType);
        /* v1.3.10（用户蓝框）：选中段整钮着色——之前只有文字变蓝，用户要求
           像"emergency off"按钮一样按下时整个按钮有颜色：选中段改蓝底白字
           （实机反馈优先于设计稿的白底蓝字）。未选中段保持白底灰边 */
        bg = seg_on ? C_ACCENT : C_PANEL;
        fg = seg_on ? C_PANEL : C_TEXT2;
        border = seg_on ? C_ACCENT : C_BORDER;
        if (idx >= 0 && idx < 7) wcscpy(seg_txt, POS_NAMES[idx]);
    }

    if (pushed) bg = RGB(0xE8,0xEA,0xEF);
    /* 选中段带浅阴影（设计稿 box-shadow 的 GDI 近似） */
    if (seg_on) {
        RECT sr = rc;
        sr.top += pt_scaled(1); sr.bottom += pt_scaled(1);
        fill_rounded_rect(dc, &sr, pt_scaled(7), C_SHADOW);
    }
    fill_rounded_rect(dc, &rc, pt_scaled(7), bg);
    HPEN pen = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ op = SelectObject(dc, pen);
    HBRUSH ob = (HBRUSH)SelectObject(dc, GetStockObject(NULL_BRUSH));
    RoundRect(dc, rc.left, rc.top, rc.right-1, rc.bottom-1, pt_scaled(14), pt_scaled(14));
    SelectObject(dc, op); SelectObject(dc, ob); DeleteObject(pen);

    WCHAR txt[220];
    txt[0] = 0;
    if (is_seg) {
        wcscpy(txt, seg_txt);
    } else if (style & BTN_STYLE_DROPDOWN) {
        /* 下拉选择器：文字居左（padding 12），右侧 22px 区域画垂直居中的小三角
           （原生 COMBOBOX 的箭头大且不居中，v1.3.4 起整体替代）
           v1.3.9：draw_text_dc 改传 center=false——旧值 true 把宽按钮文字
           （如 CLSID 选择器）居中了，与 HTML select 的左对齐不符 */
        GetWindowTextW(hwnd, txt, 210);
        RECT tr = rc;
        tr.left += pt_scaled(12);
        tr.right -= pt_scaled(30);
        draw_text_dc(dc, txt, &tr, fg, false, g_app.font_ui);
        int ax = rc.right - pt_scaled(17);
        int cy = (rc.top + rc.bottom) / 2;
        int d = pt_scaled(4);
        POINT tri[3] = { { ax - d, cy - d/2 }, { ax + d, cy - d/2 }, { ax, cy - d/2 + d + 1 } };
        HBRUSH btri = CreateSolidBrush(fg);
        HPEN   ptri = CreatePen(PS_SOLID, 1, fg);
        HGDIOBJ ob2 = SelectObject(dc, btri);
        HGDIOBJ op2 = SelectObject(dc, ptri);
        Polygon(dc, tri, 3);
        SelectObject(dc, ob2); SelectObject(dc, op2);
        DeleteObject(btri); DeleteObject(ptri);
        return;
    } else if (is_okbtn) {
        WCHAR raw[200] = {0};
        GetWindowTextW(hwnd, raw, 200);
        /* ✓ 近似设计稿 .btn svg 图标；整体左移半格让文字视觉居中 */
        sfmt(txt, 220, L"✓ %s", raw);
        rc.left -= pt_scaled(5); rc.right -= pt_scaled(5);
    } else {
        GetWindowTextW(hwnd, txt, 210);
    }
    draw_text_dc(dc, txt, &rc, fg, true, g_app.font_ui);
}

/* 状态胶囊：实心圆点 + 光晕环 + 文本（设计稿 .pill .dot）。返回胶囊宽度 */
static int draw_pill(HDC dc, int x, int y, int h,
                     COLORREF bg, COLORREF border, COLORREF dot, COLORREF glow,
                     const WCHAR *text, COLORREF fg) {
    int tw = text_width_f(text, g_app.font_ui);
    int w = pt_scaled(10) + pt_scaled(7) + pt_scaled(6) + tw + pt_scaled(10);
    RECT pr = { x, y, x + w, y + h };
    fill_rounded_rect(dc, &pr, h/2, bg);
    draw_rounded_rect(dc, &pr, h / 2, border, false);

    int cy = (pr.top + pr.bottom) / 2;
    int cx = pr.left + pt_scaled(10) + pt_scaled(3);
    int r  = pt_scaled(3);
    int rg = r + pt_scaled(2);
    {
        RECT grc = { cx - rg, cy - rg, cx + rg + 1, cy + rg + 1 };
        fill_ellipse(dc, &grc, glow);
    }
    {
        RECT drc = { cx - r, cy - r, cx + r + 1, cy + r + 1 };
        fill_ellipse(dc, &drc, dot);
    }

    RECT tr = { pr.left + pt_scaled(24), pr.top, pr.right - pt_scaled(8), pr.bottom };
    draw_text_dc(dc, text, &tr, fg, false, g_app.font_ui);
    return w;
}

static void paint_status(HDC dc) {
    RECT rc; GetClientRect(g_app.hwndStatus, &rc);
    HBRUSH br = CreateSolidBrush(C_STATUS_BG);
    FillRect(dc, &rc, br); DeleteObject(br);
    HPEN pen = CreatePen(PS_SOLID, 1, C_BORDER);
    HGDIOBJ op = SelectObject(dc, pen);
    MoveToEx(dc, 0, rc.bottom-1, NULL); LineTo(dc, rc.right, rc.bottom-1);
    SelectObject(dc, op); DeleteObject(pen);

    int x = pt_scaled(16);
    int ph = pt_scaled(22);
    int y = rc.top + (rc.bottom - rc.top - ph) / 2;
    WCHAR buf[512];

    /* 1) 注册状态 */
    bool ok = g_app.registered;
    sfmt(buf, 512, L"DLL %s", ok ? L"registered" : L"not registered");
    x += draw_pill(dc, x, y, ph,
                   ok ? C_OK_SOFT : C_WARN_SOFT, ok ? C_OK_LINE : C_WARN_LINE,
                   ok ? C_OK : C_WARN, ok ? C_OK_LINE : C_WARN_LINE,
                   buf, ok ? C_OK : C_WARN) + pt_scaled(8);

    /* 2) DLL 文件 */
    sfmt(buf, 512, L"DLL %s v%S", g_app.dll_exists ? L"exists" : L"missing", DLL_VERSION);
    x += draw_pill(dc, x, y, ph, C_PANEL, C_BORDER,
                   g_app.dll_exists ? C_DOT_IDLE : C_WARN,
                   g_app.dll_exists ? C_BORDER : C_WARN_LINE,
                   buf, g_app.dll_exists ? C_TEXT2 : C_WARN) + pt_scaled(8);

    /* 3) 图片张数（走缓存，避免每次重绘都扫盘） */
    sfmt(buf, 512, L"%d images", count_images_current());
    x += draw_pill(dc, x, y, ph, C_PANEL, C_BORDER, C_DOT_IDLE, C_BORDER, buf, C_TEXT2)
         + pt_scaled(10);

    /* v1.3.18：移除原来的第 4 个胶囊「已提权/未提权」。
     * 它显示的是外壳进程自身的管理员状态，静态不变、与注册状态无关，
     * 放在"DLL not registered"旁边极易让人误读成注册/提权出了问题，已删除。 */

    /* 右侧 ini 路径（设计稿 .status-path） */
    sfmt(buf, 512, L"config.ini: %s", g_app.ini_path);
    RECT tr = { x, y, rc.right - pt_scaled(16), y + ph };
    if (tr.right > tr.left)
        draw_text_dc(dc, buf, &tr, C_TEXT3, false, g_app.font_ui);
}

static LRESULT CALLBACK StatusBarProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc = BeginPaint(hwnd, &ps);
        paint_status(dc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

/* ------------------------------------------------------------------ */
/* 业务逻辑                                                            */
/* ------------------------------------------------------------------ */
static const COLORREF C_SUM_OFF = RGB(0x9C, 0xA3, 0xAF);   /* 关闭态灰 */

/* 摘要项：文本 + 状态色（启用绿 / 关闭灰 / 数值蓝 / 警告橙） */
/* 当前生效的图片目录 */
static const WCHAR *current_image_folder(void) {
    return (g_app.v_custom && g_app.v_folder[0]) ? g_app.v_folder : g_app.image_dir;
}

static void set_sum(int idx, const WCHAR *text, COLORREF color) {
    if (idx < 0 || idx > 8) return;
    SetWindowTextW(g_app.hwndSummaryLabels[idx], text);
    set_label_color(g_app.hwndSummaryLabels[idx], color);
}

static void invalidate_img_count(void) { g_app.img_count_valid = false; }

/* 张数带缓存：拖动透明度滑块 / 切开关时 update_summary 会被高频调用，
   旧实现每次都做一次目录扫描，图片多时会明显卡顿（v1.3.1 修复） */
static int count_images_current(void) {
    if (g_app.img_count_valid) return g_app.img_count_cached;
    int n = 0;
    ImgEntry *imgs = scan_images(current_image_folder(), &n);
    if (imgs) free(imgs);
    g_app.img_count_cached = n;
    g_app.img_count_valid = true;
    return n;
}

static void update_summary(void) {
    SYSTEMTIME st; GetLocalTime(&st);
    sfmt(g_app.last_refresh_time, 32, L"%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);
    if (g_app.hwndSumTime) {
        WCHAR tbuf[64];
        sfmt(tbuf, 64, L"Last refresh: %s", g_app.last_refresh_time);
        SetWindowTextW(g_app.hwndSumTime, tbuf);
    }

    WCHAR buf[1024];
    /* folderExt */
    sfmt(buf, 1024, L"%s", g_app.v_folderExt ? L"✓ Enabled" : L"✗ Off");
    set_sum(0, buf, g_app.v_folderExt ? C_OK : C_SUM_OFF);
    /* noerror */
    sfmt(buf, 1024, L"%s", g_app.v_noerror ? L"✓ Enabled" : L"✗ Off");
    set_sum(1, buf, g_app.v_noerror ? C_OK : C_SUM_OFF);
    /* random */
    sfmt(buf, 1024, L"%s", g_app.v_random ? L"✓ Enabled" : L"✗ Off");
    set_sum(2, buf, g_app.v_random ? C_OK : C_SUM_OFF);
    /* custom */
    sfmt(buf, 1024, L"%s", g_app.v_custom ? L"✓ Enabled" : L"✗ Off");
    set_sum(3, buf, g_app.v_custom ? C_OK : C_SUM_OFF);
    /* posType */
    sfmt(buf, 1024, L"%d · %s", g_app.v_posType, POS_NAMES[g_app.v_posType]);
    set_sum(4, buf, C_ACCENT);
    /* alpha */
    sfmt(buf, 1024, L"%d / 255(%d%%)", g_app.v_alpha, (int)round(g_app.v_alpha * 100.0 / 255.0));
    set_sum(5, buf, C_ACCENT);
    /* folder */
    if (g_app.v_folder[0]) set_sum(6, g_app.v_folder, C_TEXT);
    else set_sum(6, L"not set → using plugin folder/Image", C_SUM_OFF);
    /* imgcount */
    int n = count_images_current();
    bool warn_img = (n == 0) || (n < 2 && g_app.v_random);
    sfmt(buf, 1024, L"%d images%s", n,
         (n == 0) ? L" ⚠ no images" :
         (n < 2 && g_app.v_random) ? L" ⚠ need ≥2" : L"");
    set_sum(7, buf, warn_img ? C_WARN : C_ACCENT);
    /* dllstatus */
    if (!g_app.dll_exists) {
        set_sum(8, L"✗ DLL file missing", C_DANGER);
    } else {
        int64_t sz = file_size(g_app.dll_path);
        sfmt(buf, 1024, L"%s v%S  (%lld KB)",
             g_app.registered ? L"✓ registered" : L"⚠ not registered",
             DLL_VERSION, sz > 0 ? sz/1024 : 0);
        set_sum(8, buf, g_app.registered ? C_OK : C_WARN);
    }
}

static void refresh_image_list(void) {
    if (!g_app.hwndImgList) return;
    list_clear_with_param(g_app.hwndImgList);
    preview_meta_update(L"", L"", false);   /* v1.3.11：列表已重建，元信息行一并隐藏 */
    int n = 0;
    const WCHAR *folder = current_image_folder();
    ImgEntry *imgs = scan_images(folder, &n);
    /* 顺带刷新张数缓存 */
    g_app.img_count_cached = (imgs ? n : 0);
    g_app.img_count_valid = true;
    /* 计数标签（张数） */
    if (g_app.hwndImgCount) {
        WCHAR cb[80];
        sfmt(cb, 80, L"%d images · png/jpg/bmp recommended", n);
        SetWindowTextW(g_app.hwndImgCount, cb);
    }
    if (!imgs) return;
    for (int i = 0; i < n; i++) {
        LVITEMW it = {0};
        it.mask = LVIF_TEXT;
        it.iItem = i;
        it.pszText = imgs[i].name;
        WCHAR *dup = _wcsdup(imgs[i].full);
        if (dup) { it.mask |= LVIF_PARAM; it.lParam = (LPARAM)dup; }
        int pos = (int)ListView_InsertItem(g_app.hwndImgList, &it);
        WCHAR szbuf[32]; format_size(imgs[i].size, szbuf, 32);
        ListView_SetItemText(g_app.hwndImgList, pos, 1, szbuf);
    }
    free(imgs);

    /* Smart column widths (human developer, 2026-11-04): the name column
     * auto-fits the longest file name -- short names keep the size column
     * fully visible with no horizontal scrollbar; overlong names cap the
     * name column and the horizontal scrollbar takes over. The old fixed
     * widths ignored the client width eaten by the vertical scrollbar, so
     * with ~146 images the size column was clipped at the right edge
     * (11.1 MB rendered as 11.1 ME). */
    {
        RECT lrc; GetClientRect(g_app.hwndImgList, &lrc);
        int clientw = lrc.right - lrc.left;
        /* Size column: fit contents ("11.1 MB" etc.), floor 60px */
        SendMessageW(g_app.hwndImgList, LVM_SETCOLUMNWIDTH, 1, LVSCW_AUTOSIZE);
        int sizew = (int)SendMessageW(g_app.hwndImgList, LVM_GETCOLUMNWIDTH, 1, 0);
        if (sizew < pt_scaled(60)) sizew = pt_scaled(60);
        SendMessageW(g_app.hwndImgList, LVM_SETCOLUMNWIDTH, 1, sizew);
        /* Name column: fit longest item, cap at remaining width */
        SendMessageW(g_app.hwndImgList, LVM_SETCOLUMNWIDTH, 0, LVSCW_AUTOSIZE);
        int namew = (int)SendMessageW(g_app.hwndImgList, LVM_GETCOLUMNWIDTH, 0, 0);
        int avail = clientw - sizew - pt_scaled(8);
        if (namew > avail) namew = avail;
        if (namew < pt_scaled(60)) namew = pt_scaled(60);
        SendMessageW(g_app.hwndImgList, LVM_SETCOLUMNWIDTH, 0, namew);
    }
}

/* 清空列表并释放每行挂在 lParam 上的 section 副本 */
static void list_clear_with_param(HWND lv) {
    if (!lv) return;
    int cnt = (int)SendMessageW(lv, LVM_GETITEMCOUNT, 0, 0);
    for (int i = 0; i < cnt; i++) {
        LVITEMW it = {0};
        it.mask = LVIF_PARAM; it.iItem = i;
        if (SendMessageW(lv, LVM_GETITEMW, 0, (LPARAM)&it) && it.lParam) {
            free((void*)it.lParam);
        }
    }
    ListView_DeleteAllItems(lv);
}

static void refresh_rule_list(void) {
    if (!g_app.hwndRuleList) return;
    /* v1.3.1：section 名存进 lParam，删除/编辑时不再靠反解析显示文本
       （旧做法在路径很长、显示文本被截断时会删错规则） */
    list_clear_with_param(g_app.hwndRuleList);
    int n = 0;
    WCHAR **secs = list_folder_sections(&n);
    /* 计数标签（规则条数） */
    if (g_app.hwndRuleCount) {
        WCHAR cb[64];
        sfmt(cb, 64, L"%d rules", secs ? n : 0);
        SetWindowTextW(g_app.hwndRuleCount, cb);
    }
    if (!secs) return;
    for (int i = 0; i < n; i++) {
        char sec[4096]; wtou8(secs[i], sec, sizeof(sec));
        /* v1.3.24(N1，混元审计)：读侧口径与写侧（imgc[12288]）对齐 */
        char img[12288]; ini_get_buf(sec, "img", "", img, sizeof(img));
        WCHAR imgw[4096]; u8tow(img, imgw, 4096);
        LVITEMW it = {0};
        it.mask = LVIF_TEXT;
        it.iItem = i;
        it.pszText = secs[i];
        WCHAR *dup = _wcsdup(secs[i]);
        if (dup) { it.mask |= LVIF_PARAM; it.lParam = (LPARAM)dup; }
        int pos = (int)ListView_InsertItem(g_app.hwndRuleList, &it);
        ListView_SetItemText(g_app.hwndRuleList, pos, 1, imgw);
        free(secs[i]);
    }
    free(secs);
}

static void refresh_rule_img_options(void) {
    /* v1.3.9：自绘下拉状态表整表重建（行为对齐旧实现：重建后清空选中） */
    combo_state_clear(&g_cb_img);
    if (!g_cb_img.btn) return;
    int n; WCHAR *folder = g_app.v_custom && g_app.v_folder[0] ? g_app.v_folder : g_app.image_dir;
    ImgEntry *imgs = scan_images(folder, &n);
    if (!imgs) return;
    for (int i = 0; i < n; i++)
        combo_state_add(&g_cb_img, imgs[i].name);
    free(imgs);
}

static void browse_plugin(const WCHAR *dir) {
    if (dir) wcsncpy(g_app.plugin_dir, dir, MAX_PATH_W-1);
    path_join(g_app.dll_path, MAX_PATH_W, g_app.plugin_dir, DLL_NAME);
    path_join(g_app.ini_path, MAX_PATH_W, g_app.plugin_dir, INI_NAME);
    path_join(g_app.image_dir, MAX_PATH_W, g_app.plugin_dir, IMAGE_DIR_NAME);
    g_app.dll_exists = file_exists(g_app.dll_path);
    g_app.ini_exists = file_exists(g_app.ini_path);

    detect_registered();

    WCHAR lines[2048];
    sfmt(lines, 2048, L"DLL: %s  (%s)\r\nconfig.ini: %s\r\nRegistered: %s",
         g_app.dll_exists ? L"exists" : L"missing!", g_app.dll_path,
         g_app.ini_exists ? L"exists" : L"missing!",
         g_app.registered ? L"Yes" : L"No");
    log_info(lines);
    for (int i = 0; i < 4; i++) log_info(g_app.reg_detail[i]);

    if (g_app.registered && g_app.dll_exists && g_app.reg_dll_path_count > 0) {
        WCHAR cur[MAX_PATH_W]; GetFullPathNameW(g_app.dll_path, MAX_PATH_W, cur, NULL);
        for (int i = 0; i < g_app.reg_dll_path_count; i++) {
            WCHAR reg[MAX_PATH_W]; GetFullPathNameW(g_app.reg_dll_paths[i], MAX_PATH_W, reg, NULL);
            if (_wcsicmp(cur, reg) != 0) {
                log_warn(L"The DLL registered in the registry differs from the one in the current plugin folder; an older BHO may conflict.");
            }
        }
    }

    SetWindowTextW(g_app.hwndPluginDir, g_app.plugin_dir);
    invalidate_img_count();
    update_summary();
    refresh_image_list();
    refresh_rule_list();
    refresh_rule_img_options();
    InvalidateRect(g_app.hwndStatus, NULL, TRUE);
}

static void sync_switch_ui(void) {
    InvalidateRect(hSwFolderExt, NULL, FALSE);
    InvalidateRect(hSwNoerror, NULL, FALSE);
    InvalidateRect(hSwRandom, NULL, FALSE);
    InvalidateRect(hSwCustom, NULL, FALSE);
    /* v1.3.20(C)：随机显示关闭 → 轮换下拉与输入框整组置灰、输入框隐藏 */
    if (g_app.hwndRotBtn) rot_ui_sync();
}

static bool parse_bool_str(const char *s) {
    if (!s) return false;
    return (_stricmp(s, "true") == 0 || _stricmp(s, "ture") == 0 ||
            strcmp(s, "1") == 0 || _stricmp(s, "yes") == 0 || _stricmp(s, "on") == 0);
}

static void load_config(void) {
    char tmp[4096];
    static char folbuf[16384];   /* folder 是 UTF-8 路径，最长可达数万字节，单独给大缓冲 */
    ini_reload();
    ini_get_buf("load", "folderExt", "ture", tmp, sizeof(tmp));
    g_app.v_folderExt = parse_bool_str(tmp);
    ini_get_buf("load", "noerror", "false", tmp, sizeof(tmp));
    g_app.v_noerror = parse_bool_str(tmp);
    ini_get_buf("image", "random", "true", tmp, sizeof(tmp));
    g_app.v_random = parse_bool_str(tmp);
    /* v1.3.25(D1)：轮换间隔改读 randomVal（秒）+ randomUnit（min/sec）。
     * 兼容老配置：没有 randomVal 时回落读 randomMin（分钟）×60。 */
    {
        ini_get_buf("image", "randomVal", "", tmp, sizeof(tmp));
        if (tmp[0]) {
            g_app.v_rotsec = atoi(tmp);
            ini_get_buf("image", "randomUnit", "min", tmp, sizeof(tmp));
            g_app.v_rotunit = (_stricmp(tmp, "sec") == 0) ? ROT_UNIT_SEC : ROT_UNIT_MIN;
        } else {
            ini_get_buf("image", "randomMin", "0", tmp, sizeof(tmp));
            g_app.v_rotsec = atoi(tmp) * 60;
            g_app.v_rotunit = ROT_UNIT_MIN;
        }
        if (g_app.v_rotsec < 0) g_app.v_rotsec = 0;
        if (g_app.v_rotsec > 86400) g_app.v_rotsec = 86400;
        /* v1.3.28(P1，Deepseek 复审)：配置重新读入（含底部「恢复配置」
         * 从 .bak 恢复的路径）后，自定义态标志必须归零 —— 恢复出预设档
         * （如 randomVal=60）时 rot_ui_sync 若仍认为 custom，界面会错误
         * 停在"Custom…"档。该标志只表达"user explicitly picked custom this session"，
         * 配置一变即失效；桌面「恢复原配置」走 on_restore_prev 不经
         * load_config，不受本行影响（其本来就不动轮换配置）。 */
        g_rot_custom_picked = false;
    }
    ini_get_buf("image", "custom", "false", tmp, sizeof(tmp));
    g_app.v_custom = parse_bool_str(tmp);
    ini_get_buf("image", "posType", "3", tmp, sizeof(tmp));
    g_app.v_posType = atoi(tmp);
    if (g_app.v_posType < 0 || g_app.v_posType > 6) {
        log_warn(L"posType out of range, fell back to 3 (bottom-right)");
        g_app.v_posType = 3;
    }
    ini_get_buf("image", "imgAlpha", "70", tmp, sizeof(tmp));
    g_app.v_alpha = atoi(tmp);
    if (g_app.v_alpha < 0) g_app.v_alpha = 0;
    if (g_app.v_alpha > 255) g_app.v_alpha = 255;
    ini_get_buf("image", "folder", "", folbuf, sizeof(folbuf));
    u8tow(folbuf, g_app.v_folder, MAX_PATH_W);

    sync_switch_ui();
    SendMessageW(hSliderAlpha, TBM_SETPOS, TRUE, g_app.v_alpha);
    SetWindowTextW(g_app.hwndEditFolder, g_app.v_folder);
    for (int i = 0; i < 7; i++) InvalidateRect(hSegPos[i], NULL, FALSE);
    on_slider_change();
    /* custom/folder 可能刚从 ini 读出来，图片列表要按新目录重扫
       （旧实现启动后列表停留在默认 Image 目录，v1.3.1 修复） */
    invalidate_img_count();
    refresh_image_list();
    refresh_rule_img_options();
    /* P4-1（Deepseek V4.1 Flash 1017 复核）：首次进专属背景页显示 0 rules 的根因 ——
     * 启动顺序 browse_plugin(内含 refresh_rule_list) 在 load_config 之前，
     * 首刷时 ini 还没载入。此处补一次刷新，与 refresh_image_list 同理。 */
    refresh_rule_list();
    update_summary();
}

static void save_config(void) {
    if (!g_app.ini_exists) {
        log_err(L"config.ini not found. Please verify the plugin folder first.");
        MessageBoxW(g_app.hwndMain, L"config.ini not found. Please verify the plugin folder first.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
    if (g_app.v_folder[0] && !dir_exists(g_app.v_folder)) {
        ensure_dir(g_app.v_folder);
        log_info(L"Custom image folder did not exist; created automatically");
    }
    if (g_app.v_posType < 0 || g_app.v_posType > 6) {
        log_warn(L"posType out of range, fell back to 3 (bottom-right)");
        g_app.v_posType = 3;
    }
    ini_set("load", "folderExt", g_app.v_folderExt ? "ture" : "false");
    ini_set("load", "noerror", g_app.v_noerror ? "true" : "false");
    ini_set("image", "random", g_app.v_random ? "true" : "false");
    /* v1.3.25(D1)：轮换间隔落盘 randomVal（秒）+ randomUnit（显示单位）。
     * 自定义态从输入框取值（ES_NUMBER 只收数字），按当前单位换算成秒，
     * 再统一兜底：<0 → 0（不轮换），>86400 → 86400（一天）。
     * 同时兼容写一份 randomMin（分钟，向上保底 1 分钟）给旧版 DLL 用 ——
     * 秒级间隔在旧 DLL 上按 1 分钟轮换，功能不丢。 */
    {
        WCHAR wb[32] = L"";
        HWND frame = g_app.hwndRotMinEdit ? GetParent(g_app.hwndRotMinEdit) : NULL;
        if (frame && IsWindowVisible(frame))
            GetWindowTextW(g_app.hwndRotMinEdit, wb, 32);
        else
            sfmt(wb, 32, L"%d", rotval_to_display(g_app.v_rotsec));
        int v = _wtoi(wb);
        if (v < 0) v = 0;
        int sec = (g_app.v_rotunit == ROT_UNIT_MIN) ? v * 60 : v;
        if (sec > 86400) sec = 86400;
        g_app.v_rotsec = sec;
        /* v1.3.21(G3，Deepseek 审计)：钳位结果回写输入框 ——
         * 输入 99999 落盘的是 86400，框里不该还显示 99999 */
        if (frame && IsWindowVisible(frame)) {
            sfmt(wb, 32, L"%d", rotval_to_display(sec));
            SetWindowTextW(g_app.hwndRotMinEdit, wb);
        }
    }
    {
        char rb[16];
        sprintf(rb, "%d", g_app.v_rotsec); ini_set("image", "randomVal", rb);
        sprintf(rb, "%s", g_app.v_rotunit == ROT_UNIT_SEC ? "sec" : "min");
        ini_set("image", "randomUnit", rb);
        int rmin = (g_app.v_rotsec > 0) ? (g_app.v_rotsec + 59) / 60 : 0;
        if (rmin > 1440) rmin = 1440;
        sprintf(rb, "%d", rmin); ini_set("image", "randomMin", rb);
    }
    ini_set("image", "custom", g_app.v_custom ? "true" : "false");
    char buf[16]; sprintf(buf, "%d", g_app.v_posType); ini_set("image", "posType", buf);
    sprintf(buf, "%d", g_app.v_alpha); ini_set("image", "imgAlpha", buf);
    static char folbuf[16384];
    wtou8(g_app.v_folder, folbuf, sizeof(folbuf));
    ini_set("image", "folder", folbuf);
    if (!ini_commit()) {
        /* v1.3.12：只读/被占用/磁盘满时不再假报"saved" */
        WCHAR b[512];
        sfmt(b, 512, L"Save failed: cannot write config.ini (error %lu).\nThe file may be read-only or locked by another program.", GetLastError());
        log_err(b);
        MessageBoxW(g_app.hwndMain, b, L"Save failed", MB_OK | MB_ICONERROR);
        return;
    }
    log_info(L"Configuration saved to config.ini");
    MessageBoxW(g_app.hwndMain, L"Configuration saved to config.ini.\nClick Restart Explorer to apply.", L"Done", MB_OK | MB_ICONINFORMATION);
}

/* ------------------------------------------------------------------ */
/* 系统交互：注册 / 注销 / 重启资源管理器 / 备份恢复                   */
/* ------------------------------------------------------------------ */
static bool shell_exec_runas(const WCHAR *exe, const WCHAR *params, bool wait, DWORD *code) {
    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = L"runas";
    sei.lpFile = exe;
    sei.lpParameters = params;
    sei.nShow = SW_HIDE;
    if (code) *code = 0xFFFFFFFF;
    if (!ShellExecuteExW(&sei)) return false;
    /* 需要等退出码却拿不到进程句柄时不能报成功：
       否则上层会把 code=0 当成"registered OK"（v1.3.1 修复） */
    if (!sei.hProcess) return !wait ? true : false;
    bool ok = true;
    if (wait) {
        WaitForSingleObject(sei.hProcess, INFINITE);
        DWORD c = 0;
        if (GetExitCodeProcess(sei.hProcess, &c) && code) *code = c;
        else if (code) *code = 0xFFFFFFFF;
    }
    CloseHandle(sei.hProcess);
    return ok;
}

/* 启动后台线程并立即关闭句柄（旧实现每次注册/注销泄漏一个线程句柄） */
static bool start_thread(unsigned (__stdcall *fn)(void *)) {
    uintptr_t th = _beginthreadex(NULL, 0, fn, NULL, 0, NULL);
    if (!th) return false;
    CloseHandle((HANDLE)th);
    return true;
}

/* 看门狗：忙态 60 秒未结束则强制解锁，避免 UAC 不点或 regsvr32 卡死导致按钮永久禁用 */
#define TIMER_BUSY_WATCHDOG  3
#define TIMER_STARTUP_VERIFY 2
#define BUSY_TIMEOUT_MS      60000

/* 启动后的 DLL 完整性校验（定时器 2 只跑一次）
   v1.3.12：对齐 Python 版——内置资源 DLL 与 plugin 目录 DLL 各算一次 SHA256
   并比对，不一致弹警告（不阻断），缓存内置哈希供注册前二次确认 */
static void startup_verify(void) {
    /* 1) 内置资源 DLL 哈希（算一次缓存） */
    g_app.embedded_hash_ok = false; g_app.embedded_hash[0] = 0;
    {
        HRSRC hr = FindResourceW(NULL, MAKEINTRESOURCEW(IDR_DLL), (LPCWSTR)RT_RCDATA);
        HGLOBAL hg = hr ? LoadResource(NULL, hr) : NULL;
        DWORD  sz = hr ? SizeofResource(NULL, hr) : 0;
        const BYTE *data = hg ? (const BYTE*)LockResource(hg) : NULL;
        if (data && sz) {
            g_app.embedded_hash_ok = sha256_mem(data, sz, g_app.embedded_hash);
            if (!g_app.embedded_hash_ok)
                log_warn(L"Startup check: cannot hash the embedded DLL (crypto API unavailable); pre-register comparison skipped");
        } else {
            log_warn(L"Startup check: embedded DLL resource not found in the EXE; pre-register comparison skipped");
        }
    }
    /* 2) plugin 目录 DLL 与内置比对 */
    if (!g_app.dll_exists) {
        log_warn(L"Startup check: no ExplorerBgTool.dll in the plugin folder; register/unregister unavailable");
    } else {
        char hex[65];
        if (sha256_file(g_app.dll_path, hex)) {
            WCHAR hw[80], b[256];
            u8tow(hex, hw, 80);
            sfmt(b, 256, L"Startup check: DLL SHA256 %s", hw);
            log_info(b);
            if (g_app.embedded_hash_ok && strcmp(hex, g_app.embedded_hash) != 0) {
                log_warn(L"Startup check: the plugin folder DLL differs from the embedded DLL (maybe hand-swapped); confirm before registering");
                MessageBoxW(g_app.hwndMain,
                    L"The ExplorerBgTool.dll in the plugin folder differs from the embedded DLL (maybe hand-swapped).\n\n"
                    L"Confirm this is the DLL you expect before registering. To restore: delete the DLL in the plugin folder and restart this program to re-unpack.",
                    L"DLL mismatch", MB_OK | MB_ICONWARNING);
            }
        } else {
            log_warn(L"Startup check: cannot hash the DLL (file locked or crypto API unavailable)");
        }
    }
    if (!g_app.ini_exists)
        log_warn(L"Startup check: config.ini missing; verify the plugin folder before saving");
}

static void clear_busy(void) {
    g_app.busy = false;
    if (g_app.hwndMain) KillTimer(g_app.hwndMain, TIMER_BUSY_WATCHDOG);
    SetWindowTextW(g_app.hwndBtnReg, L"Register (admin)");
    SetWindowTextW(g_app.hwndBtnUnreg, L"Unregister (admin)");
    SetWindowTextW(g_app.hwndBtnRestart, L"Restart Explorer");
    EnableWindow(g_app.hwndBtnReg, TRUE);
    EnableWindow(g_app.hwndBtnUnreg, TRUE);
    EnableWindow(g_app.hwndBtnRestart, TRUE);
}

static void start_busy(const WCHAR *which) {
    g_app.busy = true;
    EnableWindow(g_app.hwndBtnReg, FALSE);
    EnableWindow(g_app.hwndBtnUnreg, FALSE);
    EnableWindow(g_app.hwndBtnRestart, FALSE);
    if (wcscmp(which, L"reg") == 0) SetWindowTextW(g_app.hwndBtnReg, L"Registering…");
    else if (wcscmp(which, L"unreg") == 0) SetWindowTextW(g_app.hwndBtnUnreg, L"Unregistering…");
    else SetWindowTextW(g_app.hwndBtnRestart, L"Restarting…");
    if (g_app.hwndMain) SetTimer(g_app.hwndMain, TIMER_BUSY_WATCHDOG, BUSY_TIMEOUT_MS, NULL);
}

static unsigned __stdcall thread_register(void *param) {
    (void)param;
    WCHAR sysdir[MAX_PATH_W]; GetSystemDirectoryW(sysdir, MAX_PATH_W);
    WCHAR regsvr32[MAX_PATH_W]; path_join(regsvr32, MAX_PATH_W, sysdir, L"regsvr32.exe");
    WCHAR params[MAX_PATH_W]; sfmt(params, MAX_PATH_W, L"/s \"%s\"", g_app.dll_path);
    DWORD code = 0;
    bool ok = shell_exec_runas(regsvr32, params, true, &code);
    PostMessageW(g_app.hwndMain, WM_USER + 10, ok ? 1 : 0, code);
    return 0;
}

static unsigned __stdcall thread_unregister(void *param) {
    (void)param;
    WCHAR sysdir[MAX_PATH_W]; GetSystemDirectoryW(sysdir, MAX_PATH_W);
    WCHAR regsvr32[MAX_PATH_W]; path_join(regsvr32, MAX_PATH_W, sysdir, L"regsvr32.exe");
    WCHAR params[MAX_PATH_W]; sfmt(params, MAX_PATH_W, L"/s /u \"%s\"", g_app.dll_path);
    DWORD code = 0;
    bool ok = shell_exec_runas(regsvr32, params, true, &code);
    PostMessageW(g_app.hwndMain, WM_USER + 11, ok ? 1 : 0, code);
    return 0;
}

static unsigned __stdcall thread_restart(void *param) {
    (void)param;
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = {0};
    WCHAR cmd1[] = L"taskkill /F /IM explorer.exe";
    DWORD tk_code = 0xFFFFFFFF;   /* -1 = 未能取得退出码 */
    BOOL ok1 = CreateProcessW(NULL, cmd1, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    if (ok1) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        GetExitCodeProcess(pi.hProcess, &tk_code);
        CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
    }
    Sleep(300);
    WCHAR cmd2[] = L"explorer.exe";
    BOOL ok2 = CreateProcessW(NULL, cmd2, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    if (ok2) { CloseHandle(pi.hProcess); CloseHandle(pi.hThread); }
    /* wParam: 0=失败 1=taskkill 成功 2=taskkill 未杀到(explorer 可能本来就没运行但仍已启动新实例)
       lParam: taskkill 退出码 */
    int flag = ok1 ? (tk_code == 0 ? 1 : 2) : 0;
    PostMessageW(g_app.hwndMain, WM_USER + 12, (WPARAM)flag, (LPARAM)tk_code);
    return 0;
}

/* v1.3.12：注册前 DLL 完整性确认（对齐 Python 版 _check_dll_integrity）。
   返回 false = 用户选择中止。内置哈希不可用（老机器加密 API 异常等）时不拦。 */
static bool check_dll_integrity_before_register(void) {
    if (!g_app.embedded_hash_ok) return true;
    char hex[65];
    if (!sha256_file(g_app.dll_path, hex)) return true;   /* 算不出摘要不拦（注册本身会给出结果） */
    if (strcmp(hex, g_app.embedded_hash) == 0) return true;
    WCHAR b[1024];
    sfmt(b, 1024,
         L"The ExplorerBgTool.dll in the plugin folder differs from the DLL embedded in this program (maybe hand-swapped).\n\n"
         L"DLL path:\n%s\n\n"
         L"A swapped DLL may misbehave or pose security risks. Continue registering anyway?",
         g_app.dll_path);
    if (MessageBoxW(g_app.hwndMain, b, L"DLL integrity check", MB_YESNO | MB_ICONEXCLAMATION) != IDYES) {
        log_warn(L"Registration cancelled: DLL differs from the embedded version; user aborted");
        return false;
    }
    log_warn(L"Registration continued: DLL differs from the embedded version; user confirmed");
    return true;
}

static void on_register(void) {
    if (g_app.busy) return;
    if (!g_app.dll_exists) { MessageBoxW(g_app.hwndMain, L"DLL not found", L"Error", MB_OK|MB_ICONERROR); return; }
    if (!check_dll_integrity_before_register()) return;
    start_busy(L"reg");
    log_info(L"Registering with elevation (calling regsvr32.exe directly)…");
    if (!start_thread(thread_register)) { clear_busy(); log_err(L"Failed to create the registration thread"); }
}

static void on_unregister(void) {
    if (g_app.busy) return;
    if (!g_app.dll_exists) { MessageBoxW(g_app.hwndMain, L"DLL not found", L"Error", MB_OK|MB_ICONERROR); return; }
    start_busy(L"unreg");
    log_info(L"Unregistering with elevation (calling regsvr32.exe directly)…");
    if (!start_thread(thread_unregister)) { clear_busy(); log_err(L"Failed to create the unregistration thread"); }
}

static void on_restart(void) {
    if (g_app.busy) return;
    if (MessageBoxW(g_app.hwndMain,
        L"Restarting Explorer closes all open folder windows\nand may lose unsaved window layout.\n\nContinue?",
        L"Confirm restart", MB_YESNO | MB_ICONEXCLAMATION) != IDYES) return;
    start_busy(L"restart");
    log_info(L"Ending explorer.exe and starting it again…");
    if (!start_thread(thread_restart)) { clear_busy(); log_err(L"Failed to create the restart thread"); }
}

/* 定位：读取编辑框当前文本，而不是用旧的全局目录 */
static void on_locate(void) {
    WCHAR path[MAX_PATH_W];
    GetWindowTextW(g_app.hwndPluginDir, path, MAX_PATH_W);
    if (!path[0]) {
        MessageBoxW(g_app.hwndMain, L"Enter the plugin folder path first", L"Notice", MB_OK);
        return;
    }
    if (!dir_exists(path)) {
        MessageBoxW(g_app.hwndMain, L"That folder does not exist; check the path", L"Notice", MB_OK | MB_ICONWARNING);
        return;
    }
    WCHAR buf[1024];
    sfmt(buf, 1024, L"Revealed: %s", path);
    log_info(buf);
    browse_plugin(path);
}

static void on_backup(void) {
    if (!g_app.ini_exists) { MessageBoxW(g_app.hwndMain, L"config.ini not found", L"Error", MB_OK|MB_ICONERROR); return; }
    SYSTEMTIME st; GetLocalTime(&st);
    WCHAR dst[MAX_PATH_W];
    sfmt(dst, MAX_PATH_W, L"%s.bak%04d%02d%02d-%02d%02d%02d",
         g_app.ini_path, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    if (CopyFileW(g_app.ini_path, dst, FALSE)) {
        WCHAR b[1024]; sfmt(b, 1024, L"Config backed up: %s", dst);
        log_info(b);
        MessageBoxW(g_app.hwndMain, L"Config backed up", L"Done", MB_OK|MB_ICONINFORMATION);
    } else {
        WCHAR b[256]; sfmt(b, 256, L"Backup failed (error %lu)", GetLastError());
        log_err(b);
        MessageBoxW(g_app.hwndMain, b, L"Failed", MB_OK|MB_ICONERROR);
    }
}

static void on_restore(void) {
    WCHAR pat[MAX_PATH_W]; sfmt(pat, MAX_PATH_W, L"%s.bak*", g_app.ini_path);
    WIN32_FIND_DATAW fd; HANDLE h = FindFirstFileW(pat, &fd);
    if (h == INVALID_HANDLE_VALUE) { MessageBoxW(g_app.hwndMain, L"No backup found", L"Notice", MB_OK); return; }
    WCHAR latest[MAX_PATH_W]; path_join(latest, MAX_PATH_W, g_app.plugin_dir, fd.cFileName);
    FILETIME ft = fd.ftLastWriteTime;
    while (FindNextFileW(h, &fd)) {
        if (CompareFileTime(&fd.ftLastWriteTime, &ft) > 0) {
            ft = fd.ftLastWriteTime;
            path_join(latest, MAX_PATH_W, g_app.plugin_dir, fd.cFileName);
        }
    }
    FindClose(h);
    if (CopyFileW(latest, g_app.ini_path, FALSE)) {
        load_config();
        WCHAR b[1024]; sfmt(b, 1024, L"Config restored: %s", latest);
        log_info(b);
    } else {
        WCHAR b[256]; sfmt(b, 256, L"Restore failed (error %lu)", GetLastError());
        log_err(b);
        MessageBoxW(g_app.hwndMain, b, L"Failed", MB_OK|MB_ICONERROR);
    }
}

static void on_emergency(void) {
    if (g_app.emergency_backup_valid) {
        MessageBoxW(g_app.hwndMain, L"Already in emergency-off state", L"Notice", MB_OK);
        return;
    }
    if (MessageBoxW(g_app.hwndMain,
        L"Emergency off temporarily switches folder/random/custom/noerror off,\n"
        L"Continue?",
        L"Confirm emergency off", MB_YESNO | MB_ICONEXCLAMATION) != IDYES) return;
    g_app.emergency_backup_valid = true;
    wcsncpy(g_app.emergency_folder, g_app.v_folder, MAX_PATH_W-1);
    g_app.emergency_random = g_app.v_random;
    g_app.emergency_custom = g_app.v_custom;
    g_app.emergency_noerror = g_app.v_noerror;
    g_app.v_folder[0] = 0; g_app.v_random = false; g_app.v_custom = false; g_app.v_noerror = true;
    SetWindowTextW(g_app.hwndEditFolder, L"");
    sync_switch_ui();
    save_config();
    EnableWindow(g_app.hwndBtnEmergency, FALSE);
    EnableWindow(g_app.hwndBtnRestorePrev, TRUE);
    /* v1.3.2：右卡摘要即时刷新（folder/random/custom/noerror 四行 + 张数） */
    invalidate_img_count();
    update_summary();
    refresh_image_list();
    log_info(L"Disabled config saved; click Restart Explorer to reload the DLL");
}

static void on_restore_prev(void) {
    if (!g_app.emergency_backup_valid) {
        MessageBoxW(g_app.hwndMain, L"Not in emergency-off state", L"Notice", MB_OK);
        return;
    }
    wcsncpy(g_app.v_folder, g_app.emergency_folder, MAX_PATH_W-1);
    g_app.v_random = g_app.emergency_random;
    g_app.v_custom = g_app.emergency_custom;
    g_app.v_noerror = g_app.emergency_noerror;
    g_app.emergency_backup_valid = false;
    SetWindowTextW(g_app.hwndEditFolder, g_app.v_folder);
    sync_switch_ui();
    save_config();
    EnableWindow(g_app.hwndBtnEmergency, TRUE);
    EnableWindow(g_app.hwndBtnRestorePrev, FALSE);
    /* 同上：恢复后也要即时刷新摘要 */
    invalidate_img_count();
    update_summary();
    refresh_image_list();
    log_info(L"Original config written back to config.ini; click Restart Explorer to reload the DLL");
}

/* ------------------------------------------------------------------ */
/* 命令处理                                                            */
/* ------------------------------------------------------------------ */
static void on_folder_ext_toggle(bool on) {
    g_app.v_folderExt = on;
    if (on) {
        if (MessageBoxW(g_app.hwndMain,
            L"With folderExt on, the DLL injects into dialogs of 13 Windows system processes:\n"
            L"SettingSyncHost / SkyDrive / FileManager / vmtoolsd / svchost / SearchIndexer /\n"
            L"WSHost / wmpnetwk / dllhost / spoolsv / PhoneExperienceHost / RadeonSoftware / usysdiag.\n\n"
            L"Open/Save dialogs in those processes will show the background.\n\nEnable?",
            L"Enable file dialog background", MB_YESNO | MB_ICONEXCLAMATION) != IDYES) {
            g_app.v_folderExt = false;
        }
    }
    sync_switch_ui();
    update_summary();
}

static void on_custom_toggle(bool on) {
    g_app.v_custom = on;
    invalidate_img_count();
    update_summary();
    refresh_image_list();
    refresh_rule_img_options();
}

static void on_pos_select(int idx) {
    g_app.v_posType = idx;
    for (int i = 0; i < 7; i++) InvalidateRect(hSegPos[i], NULL, FALSE);
    update_summary();
}

static void on_slider_change(void) {
    g_app.v_alpha = (int)SendMessageW(hSliderAlpha, TBM_GETPOS, 0, 0);
    WCHAR buf[64]; sfmt(buf, 64, L"%d / 255(%d%%)", g_app.v_alpha, (int)round(g_app.v_alpha*100.0/255.0));
    SetWindowTextW(g_app.hwndAlphaReadout, buf);
    update_summary();
}

static void on_browse_dir(void) {
    BROWSEINFOW bi = {0};
    bi.hwndOwner = g_app.hwndMain;
    bi.lpszTitle = L"Select plugin folder";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (pidl) {
        WCHAR path[MAX_PATH_W];
        SHGetPathFromIDListW(pidl, path);
        CoTaskMemFree(pidl);
        browse_plugin(path);
    }
}

static void on_browse_folder(void) {
    BROWSEINFOW bi = {0};
    bi.hwndOwner = g_app.hwndMain;
    bi.lpszTitle = L"Select custom image folder";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (pidl) {
        WCHAR path[MAX_PATH_W];
        SHGetPathFromIDListW(pidl, path);
        CoTaskMemFree(pidl);
        wcsncpy(g_app.v_folder, path, MAX_PATH_W-1);
        SetWindowTextW(g_app.hwndEditFolder, g_app.v_folder);
        /* v1.3.13（豆包核验观察点 #2）：custom 未勾选时列表/下拉用的目录没变，
           不做无谓的全目录重扫；勾选时才刷 */
        if (g_app.v_custom) {
            invalidate_img_count();
            refresh_image_list();
            refresh_rule_img_options();
        }
        update_summary();
        /* v1.3.12：custom 未勾选时该目录不会生效（列表/预览仍用插件 Image 目录），
           用户会以为"picked but no reaction"——对齐 Python 版"switches right after picking"的观感，弹问是否勾选 */
        if (!g_app.v_custom) {
            WCHAR msg[1024];
            sfmt(msg, 1024,
                 L"Folder selected:\n%s\n\n"
                 L"This folder is only used after ticking \"Use a custom image folder\";"
                 L"otherwise the list still shows the plugin's Image folder.\n\nTick it now?",
                 path);
            if (MessageBoxW(g_app.hwndMain, msg, L"Notice", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                on_custom_toggle(true);
                sync_switch_ui();
            }
        }
    }
}

static void on_open_img_dir(void) {
    const WCHAR *folder = current_image_folder();
    if (!dir_exists(folder)) {
        WCHAR msg[1024];
        sfmt(msg, 1024, L"Image folder does not exist:\n%s\n\nCreate it now?", folder);
        if (MessageBoxW(g_app.hwndMain, msg, L"Folder does not exist", MB_YESNO | MB_ICONEXCLAMATION) != IDYES) return;
        if (!CreateDirectoryW(folder, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
            WCHAR b[256]; sfmt(b, 256, L"Creation failed (error %lu)", GetLastError());
            log_err(b);
            MessageBoxW(g_app.hwndMain, b, L"Failed", MB_OK | MB_ICONERROR);
            return;
        }
        invalidate_img_count();
        refresh_image_list();
    }
    if ((INT_PTR)ShellExecuteW(g_app.hwndMain, L"open", folder, NULL, NULL, SW_SHOWNORMAL) <= 32)
        log_warn(L"Failed to open the image folder (no associated program?)");
}

static void on_open_ini(void) {
    if (!g_app.ini_exists) { MessageBoxW(g_app.hwndMain, L"config.ini does not exist", L"Notice", MB_OK); return; }
    ShellExecuteW(g_app.hwndMain, L"open", g_app.ini_path, NULL, NULL, SW_SHOWNORMAL);
}

static void on_copy_summary(void) {
    WCHAR buf[4096];
    WCHAR buf2[128];   /* v1.3.25(D1)：randomVal 行的临时拼装 */
    /* v1.3.25(D1)：整分钟按分钟显示，秒级不足一分钟按秒显示 */
    if (g_app.v_rotsec > 0) {
        if (g_app.v_rotsec % 60 == 0)
            sfmt(buf2, 128, L"%d min (%d sec)", g_app.v_rotsec / 60, g_app.v_rotsec);
        else
            sfmt(buf2, 128, L"%d sec", g_app.v_rotsec);
    } else {
        lstrcpynW(buf2, L"No rotation", 128);
    }
    WCHAR ver_w[32];
    u8tow(GUI_VERSION, ver_w, 32);   /* 统一走安全转换助手（v1.3.3：补上本处漏网） */
    /* v1.3.12：补诊断三件套（对齐 Python 版 v1.2.4）——DLL SHA256 / 大小 / CLSID，
       用户拿摘要反馈问题时信息更全 */
    WCHAR dll_sha[80] = L"—";
    WCHAR dll_size[32] = L"—";
    {
        char hex[65];
        if (g_app.dll_exists && sha256_file(g_app.dll_path, hex)) u8tow(hex, dll_sha, 80);
        if (g_app.dll_exists) {
            WIN32_FILE_ATTRIBUTE_DATA fa;
            if (GetFileAttributesExW(g_app.dll_path, GetFileExInfoStandard, &fa)) {
                int64_t sz = ((int64_t)fa.nFileSizeHigh << 32) | fa.nFileSizeLow;
                format_size(sz, dll_size, 32);
            }
        }
    }
    sfmt(buf, 4096,
        L"Last refresh: %s\r\n"
        L"folderExt    : %s\r\n"
        L"noerror      : %s\r\n"
        L"random       : %s\r\n"
        L"randomVal   : %s (0 = no rotation; seconds supported since v1.3.25; legacy key randomMin kept in sync)\r\n"
        L"custom       : %s\r\n"
        L"posType      : %d · %s\r\n"
        L"imgAlpha     : %d / 255(%d%%)\r\n"
        L"folder       : %s\r\n"
        L"imgcount     : %d images\r\n"
        L"dllstatus    : %s\r\n"
        L"DLL path     : %s\r\n"
        L"DLL size     : %s\r\n"
        L"DLL SHA256   : %s\r\n"
        L"Plugin CLSID : %s\r\n"
        L"Program ver  : backimg Panel v%s\r\n"
        L"Paired DLL ver: v%S",
        g_app.last_refresh_time,
        g_app.v_folderExt ? L"Enabled" : L"Off",
        g_app.v_noerror ? L"Enabled" : L"Off",
        g_app.v_random ? L"Enabled" : L"Off",
        buf2,
        g_app.v_custom ? L"Enabled" : L"Off",
        g_app.v_posType, POS_NAMES[g_app.v_posType],
        g_app.v_alpha, (int)round(g_app.v_alpha*100.0/255.0),
        g_app.v_folder[0] ? g_app.v_folder : L"not set → using plugin folder/Image",
        count_images_current(),
        g_app.dll_exists ? (g_app.registered ? L"registered" : L"not registered") : L"DLL missing",
        g_app.dll_path,
        dll_size,
        dll_sha,
        CLSID_STR_W,
        ver_w, DLL_VERSION);
    if (OpenClipboard(g_app.hwndMain)) {
        EmptyClipboard();
        size_t n = (wcslen(buf) + 1) * sizeof(WCHAR);
        HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, n);
        if (h) {
            void *p = GlobalLock(h);
            if (p) {
                memcpy(p, buf, n);
                GlobalUnlock(h);
                SetClipboardData(CF_UNICODETEXT, h);
                log_info(L"Summary copied to the clipboard");
            } else {
                GlobalFree(h);
                log_err(L"Copy failed: cannot lock clipboard memory");
            }
        } else {
            log_err(L"Copy failed: clipboard memory allocation failed");
        }
        CloseClipboard();
    } else {
        log_err(L"Copy failed: cannot open the clipboard");
    }
}

static void on_clsid_select(int idx) {
    if (idx > 0 && idx <= 3) {
        WCHAR path[MAX_PATH_W]; sfmt(path, MAX_PATH_W, L"::{%s}", BUILTIN_GUIDS[idx-1]);
        SetWindowTextW(g_app.hwndRulePath, path);
        combo_state_set(&g_cb_clsid, 0);   /* v1.3.9：回到占位项 */
    }
}

/* v1.3.9：自绘下拉按钮的弹出菜单（当前项打勾，模式与「界面缩放」一致） */
static void combo_state_menu(HWND hwnd, ComboState *cb) {
    if (!cb->btn || cb->count <= 0) return;
    HMENU menu = CreatePopupMenu();
    if (!menu) return;
    for (int i = 0; i < cb->count; i++)
        AppendMenuW(menu, MF_STRING | MF_ENABLED | (i == cb->cur ? MF_CHECKED : 0),
                    IDM_CB_FIRST + i, cb->items[i]);
    RECT rc; GetWindowRect(cb->btn, &rc);
    int sel = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
                             rc.left, rc.bottom, 0, hwnd, NULL);
    DestroyMenu(menu);
    if (sel >= IDM_CB_FIRST && sel < IDM_CB_FIRST + cb->count)
        combo_state_set(cb, sel - IDM_CB_FIRST);
}

static void on_clsid_menu(HWND hwnd) {
    combo_state_menu(hwnd, &g_cb_clsid);
    on_clsid_select(g_cb_clsid.cur);      /* 与旧 CBN_SELCHANGE 行为一致 */
}

static void on_ruleimg_menu(HWND hwnd) {
    combo_state_menu(hwnd, &g_cb_img);
}

static void on_rule_add(void) {
    WCHAR path[MAX_PATH_W]; GetWindowTextW(g_app.hwndRulePath, path, MAX_PATH_W);
    WCHAR img[MAX_PATH_W] = L"";
    if (g_cb_img.cur >= 0) lstrcpynW(img, g_cb_img.items[g_cb_img.cur], MAX_PATH_W);   /* v1.3.9 */
    if (!path[0]) { MessageBoxW(g_app.hwndMain, L"Enter a folder path first", L"Notice", MB_OK); return; }
    if (wcschr(path, L'#') || wcschr(path, L'[') || wcschr(path, L']')) {
        MessageBoxW(g_app.hwndMain, L"Path cannot contain # [ ] characters; they break INI syntax", L"Invalid path", MB_OK|MB_ICONWARNING);
        return;
    }
    /* v1.3.13（豆包核验观察点 #3）：允许 img 留空——"no image on this path"的规则用于
       清背景（对齐 Python 版 _adv_add：只拦空路径，不拦空图片） */
    /* 图片非空时校验是否存在（不存在二次确认，与原版行为一致） */
    if (img[0]) {
        WCHAR full[MAX_PATH_W];
        WCHAR *folder = g_app.v_custom && g_app.v_folder[0] ? g_app.v_folder : g_app.image_dir;
        path_join(full, MAX_PATH_W, folder, img);
        if (!file_exists(full)) {
            WCHAR msg[1024];
            sfmt(msg, 1024,
                 L"File not found in the image folder:\n%s\n\n"
                 L"(Folder: %s)\n\nWrite this rule anyway?", img, folder);
            if (MessageBoxW(g_app.hwndMain, msg, L"Image not found", MB_YESNO | MB_ICONEXCLAMATION) != IDYES) return;
            log_warn(L"Image file missing; rule written after user confirmation");
        }
    }
    char sec[4096]; wtou8(path, sec, sizeof(sec));
    /* v1.3.24(N1，混元审计)：imgc 1024→12288 字节——img 源最长 4096 WCHAR，
     * UTF-8 上限 12288 字节，原 1024 字节贴着 NTFS 255 字符名长的理论边界
     * （255×4+1=1021≤1024，合法文件名实际装得下），但无余量、无注释，
     * 属脆弱写法；读侧同步放大对齐（见 on_rule_sel），消除口径不对称。
     * 放大本身不产生超长行：合法文件名 ≤255 字符，实际写入值 ≤1020 字节。 */
    char imgc[12288]; wtou8(img, imgc, sizeof(imgc));
    ini_set(sec, "img", imgc);
    if (!ini_commit()) { log_err(L"Failed to write per-folder rule: config.ini not writable (read-only or locked)"); return; }
    log_info(L"Per-folder rule written");
    refresh_rule_list();
}

static void on_rule_del(void) {
    int idx = (int)SendMessageW(g_app.hwndRuleList, LVM_GETNEXTITEM, -1, LVNI_SELECTED);
    if (idx < 0) {
        MessageBoxW(g_app.hwndMain, L"Select a rule in the list on the left first.", L"Notice", MB_OK);
        return;
    }
    LVITEMW it = {0};
    it.mask = LVIF_PARAM; it.iItem = idx;
    if (!SendMessageW(g_app.hwndRuleList, LVM_GETITEMW, 0, (LPARAM)&it) || !it.lParam) {
        MessageBoxW(g_app.hwndMain, L"Failed to read rules; click Refresh list and retry.", L"Notice", MB_OK | MB_ICONWARNING);
        return;
    }
    const WCHAR *secw = (const WCHAR*)it.lParam;
    char sec[4096]; wtou8(secw, sec, sizeof(sec));
    if (MessageBoxW(g_app.hwndMain, L"Delete this per-folder background rule?", L"Confirm delete",
                    MB_YESNO | MB_ICONEXCLAMATION) != IDYES) return;
    remove_folder_section(sec);
    if (!ini_commit()) { log_err(L"Failed to delete per-folder rule: config.ini not writable (read-only or locked)"); return; }
    log_info(L"Per-folder rule deleted");
    refresh_rule_list();
}

/* 设置预览位图。
   关键：STATIC(SS_BITMAP) 会持有上一次 SETIMAGE 的 HBITMAP，
   直接 DeleteObject 旧图等于销毁仍在使用的对象（黑块/崩溃）。
   正确顺序：先 SETIMAGE 换上新图，返回值即旧图句柄，再删旧图（v1.3.1 修复）。 */
static void set_preview_bitmap(HBITMAP hbm) {
    if (!g_app.hwndPreview) {
        if (hbm && hbm != g_app.hbm_preview) DeleteObject(hbm);
        return;
    }
    HBITMAP old = (HBITMAP)SendMessageW(g_app.hwndPreview, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbm);
    if (old && old != hbm) DeleteObject(old);
    g_app.hbm_preview = hbm;
    InvalidateRect(g_app.hwndPreview, NULL, TRUE);
}

/* v1.3.11：预览元信息行（.pmeta 三件套）按实际文字宽度 flex gap 排布显示，
   无图时整体隐藏——空状态下只剩「GDI+ 解码」芯片挂着，观感像乱入文字 */
static void preview_meta_update(const WCHAR *name, const WCHAR *dim, bool show) {
    HWND name_h = g_app.hwndPreviewName, dim_h = g_app.hwndPreviewDim, cod_h = g_app.hwndPreviewCodec;
    if (!name_h || !dim_h || !cod_h) return;
    if (!show) {
        ShowWindow(name_h, SW_HIDE);
        ShowWindow(dim_h, SW_HIDE);
        ShowWindow(cod_h, SW_HIDE);
        return;
    }
    SetWindowTextW(name_h, name);
    SetWindowTextW(dim_h, dim);
    RECT rc; GetWindowRect(name_h, &rc);
    MapWindowPoints(NULL, GetParent(name_h), (POINT*)&rc, 1);
    int row_y = rc.top;                           /* 行 y 由创建时的布局决定 */
    int x = pt_scaled(12);
    RECT rc2; GetWindowRect(dim_h, &rc2);
    int dh = rc2.bottom - rc2.top;
    int dw = text_width_f(dim, g_app.font_bold) + pt_scaled(4);
    RECT rc3; GetWindowRect(cod_h, &rc3);
    int cw3 = rc3.right - rc3.left;
    /* R4（Deepseek V4.1 Flash 1017 复核）：长文件名压缩 —— name 超宽时截断加省略号，
     * 保证 name+尺寸+芯片三件套不越出预览卡右缘（45 字符文件名时芯片曾越界 33px） */
    RECT prc; GetClientRect(GetParent(name_h), &prc);
    int nw_max = prc.right - x - pt_scaled(8) - dw - pt_scaled(8) - cw3 - pt_scaled(12);
    if (nw_max < pt_scaled(40)) nw_max = pt_scaled(40);
    WCHAR shown[MAX_PATH_W];
    lstrcpynW(shown, name, MAX_PATH_W);
    int nw = text_width_f(shown, g_app.font_bold) + pt_scaled(4);
    if (nw > nw_max) {
        int keep = lstrlenW(shown);
        while (keep > 4 && nw > nw_max) {
            shown[keep - 1] = L'…';
            shown[keep] = 0;
            nw = text_width_f(shown, g_app.font_bold) + pt_scaled(4);
            keep--;
        }
        SetWindowTextW(name_h, shown);
    }
    MoveWindow(name_h, x, row_y, nw, rc.bottom - rc.top, TRUE);
    x += nw + pt_scaled(8);
    MoveWindow(dim_h, x, row_y, dw, dh, TRUE);
    x += dw + pt_scaled(8);
    MoveWindow(cod_h, x, row_y + pt_scaled(1), cw3, rc3.bottom - rc3.top, TRUE);
    ShowWindow(name_h, SW_SHOW);
    ShowWindow(dim_h, SW_SHOW);
    ShowWindow(cod_h, SW_SHOW);
}

static void on_img_select(int idx) {
    if (!g_app.hwndImgList || !g_app.hwndPreview) return;
    /* 优先用行上挂的完整路径（lParam），没有再退回按名字拼 */
    LVITEMW it2 = {0};
    it2.mask = LVIF_PARAM; it2.iItem = idx;
    WCHAR full[MAX_PATH_W];
    if (SendMessageW(g_app.hwndImgList, LVM_GETITEMW, 0, (LPARAM)&it2) && it2.lParam) {
        sfmt(full, MAX_PATH_W, L"%s", (const WCHAR*)it2.lParam);
    } else {
        WCHAR name[MAX_PATH_W] = {0};
        LVITEMW ti = {0};
        ti.iSubItem = 0; ti.cchTextMax = MAX_PATH_W; ti.pszText = name;
        SendMessageW(g_app.hwndImgList, LVM_GETITEMTEXTW, (WPARAM)idx, (LPARAM)&ti);
        WCHAR *folder = g_app.v_custom && g_app.v_folder[0] ? g_app.v_folder : g_app.image_dir;
        path_join(full, MAX_PATH_W, folder, name);
    }
    if (!file_exists(full)) {
        set_preview_bitmap(NULL);
        preview_meta_update(L"", L"", false);
        log_warn(L"Preview failed: the image file no longer exists");
        return;
    }
    sfmt(g_app.preview_path, MAX_PATH_W, L"%s", full);
    RECT rc; GetClientRect(g_app.hwndPreview, &rc);
    int pw = rc.right  - pt_scaled(24);
    int ph = rc.bottom - pt_scaled(24);
    if (pw < 16) pw = 16;      /* 控件过小时不能传负值给 GDI+ */
    if (ph < 16) ph = 16;
    /* v1.3.12：大图解码前先提示（对齐 Python 版 v1.2.3"prompt first"行为）——
       解码在主线程同步进行，>30MB 会卡几秒，先清旧图、日志提示、换等待光标，
       避免"clicked but no reaction"的观感 */
    bool big = false;
    HCURSOR hcur_old = NULL;
    {
        WIN32_FILE_ATTRIBUTE_DATA fa;
        if (GetFileAttributesExW(full, GetFileExInfoStandard, &fa)) {
            int64_t fsz = ((int64_t)fa.nFileSizeHigh << 32) | fa.nFileSizeLow;
            if (fsz > 30LL * 1024 * 1024) {
                big = true;
                set_preview_bitmap(NULL);
                WCHAR b[160];
                sfmt(b, 160, L"Large image (%.1f MB); generating preview…", fsz / (1024.0 * 1024.0));
                log_info(b);
                UpdateWindow(g_app.hwndLog);
                UpdateWindow(g_app.hwndMain);
                hcur_old = SetCursor(LoadCursorW(NULL, (LPCWSTR)IDC_WAIT));
            }
        }
    }
    UINT ow = 0, oh = 0;
    set_preview_bitmap(load_thumb(full, pw, ph, &ow, &oh));
    if (big && hcur_old) SetCursor(hcur_old);
    /* 预览信息条（v1.3.11）：文件名 + 原图尺寸 + 解码芯片，整行 flex 排布显示 */
    {
        const WCHAR *nm = wcsrchr(full, L'\\');
        nm = (nm && nm[1]) ? nm + 1 : full;
        WCHAR db[64];
        if (ow && oh) sfmt(db, 64, L"%lu×%lu", (unsigned long)ow, (unsigned long)oh);
        else sfmt(db, 64, L"—");
        preview_meta_update(nm, db, true);
    }
}

static void on_rule_select(int idx) {
    LVITEMW it = {0};
    it.mask = LVIF_PARAM; it.iItem = idx;
    if (!SendMessageW(g_app.hwndRuleList, LVM_GETITEMW, 0, (LPARAM)&it) || !it.lParam) return;
    const WCHAR *secw = (const WCHAR*)it.lParam;
    SetWindowTextW(g_app.hwndRulePath, secw);
    /* 从 ini 直接读该规则的 img，再在下拉框里选中，不依赖显示文本解析 */
    char sec[4096]; wtou8(secw, sec, sizeof(sec));
    /* v1.3.24(N1，混元审计)：读侧口径与写侧（imgc[12288]）对齐 */
    char img[12288]; ini_get_buf(sec, "img", "", img, sizeof(img));
    WCHAR imgw[4096]; u8tow(img, imgw, 4096);
    /* v1.3.9：自绘下拉状态表按文本匹配选中（找不到保持原样，同旧行为） */
    {
        int hit = combo_state_find(&g_cb_img, imgw);
        if (hit >= 0) combo_state_set(&g_cb_img, hit);
    }
}

/* ------------------------------------------------------------------ */
/* 窗口过程                                                            */
/* ------------------------------------------------------------------ */
static void on_draw_item(DRAWITEMSTRUCT *di) {
    if (di->CtlType == ODT_LISTBOX && di->hwndItem == g_app.hwndLog) {
        /* 日志行：时间灰 + 正文按级别着色 */
        int level = (int)di->itemData;
        COLORREF fg = C_LOG_TEXT;
        if (level == 1) fg = C_DANGER;
        else if (level == 2) fg = C_WARN;
        else if (level == 3) fg = C_ACCENT;
        /* 缓冲必须 ≥ 日志行最大长度（2048），否则 LB_GETTEXT 失败直接显示空行 */
        WCHAR txt[2048] = {0};
        if (SendMessageW(di->hwndItem, LB_GETTEXT, di->itemID, (LPARAM)txt) < 0) txt[0] = 0;
        txt[2047] = 0;
        RECT rc = di->rcItem;
        HBRUSH br = CreateSolidBrush(C_LOG_BG);
        FillRect(di->hDC, &rc, br); DeleteObject(br);
        HGDIOBJ of = SelectObject(di->hDC, g_app.font_mono ? g_app.font_mono : g_app.font_ui);
        SetBkMode(di->hDC, TRANSPARENT);
        int tlen = 0; while (tlen < 10 && txt[tlen]) tlen++;   /* "[hh:mm:ss]" */
        SIZE sz = {0};
        SetTextColor(di->hDC, C_LOG_TIME);
        TextOutW(di->hDC, rc.left + 2, rc.top, txt, tlen);
        GetTextExtentPoint32W(di->hDC, txt, tlen, &sz);
        SetTextColor(di->hDC, fg);
        TextOutW(di->hDC, rc.left + 2 + sz.cx + 6, rc.top, txt + tlen, (int)wcslen(txt) - tlen);
        SelectObject(di->hDC, of);
        return;
    }
    if (di->CtlType == ODT_BUTTON) {
        bool hover = (di->itemState & ODS_HOTLIGHT) || (di->itemState & ODS_SELECTED);
        bool pushed = (di->itemState & ODS_SELECTED);
        draw_owner_button(di->hwndItem, di->hDC, hover, pushed);
    }
}

static void switch_tab(int idx) {
    ShowWindow(hTabBasic, idx == 0 ? SW_SHOW : SW_HIDE);
    ShowWindow(hTabImages, idx == 1 ? SW_SHOW : SW_HIDE);
    ShowWindow(hTabFolders, idx == 2 ? SW_SHOW : SW_HIDE);
    for (int i = 0; i < 3; i++)
        if (g_app.hwndTabs[i]) InvalidateRect(g_app.hwndTabs[i], NULL, TRUE);
}

/* ------------------------------------------------------------------ */
/* 布局重建（窗口拉伸 / 缩放切换后按新客户区重排）                      */
/* ------------------------------------------------------------------ */
#define TIMER_RELAYOUT 4
static bool g_first_size = true;   /* 首次 WM_SIZE 不做重建（布局本来就是刚建好的） */

/* 记忆的窗口位置是否仍在某个显示器可见范围内（避免关机时窗口在第二块屏导致下次开在屏幕外） */
static bool window_visible_on_screen(int x, int y, int w, int h) {
    RECT rc; rc.left = x; rc.top = y; rc.right = x + w; rc.bottom = y + h;
    return MonitorFromRect(&rc, MONITOR_DEFAULTTONULL) != NULL;
}

/* 主窗口分区底色：工具栏/状态/选项卡与底部操作栏为 #fff，中间面板区为 #EEF1F6
   （设计稿 .toolbar/.tabs/.actions 白底、.panels 画布灰；v1.3.2 修正旧版全灰） */
static void paint_main_bg(HDC dc, HWND hwnd) {
    RECT rc; GetClientRect(hwnd, &rc);
    int py = g_app.py_panel;
    int ph = g_app.ph_panel;
    HBRUSH bw = CreateSolidBrush(C_PANEL);
    HBRUSH bc = CreateSolidBrush(C_CANVAS);
    if (py > 0) { RECT t = { 0, 0, rc.right, py }; FillRect(dc, &t, bw); }
    if (ph > 0) { RECT m = { 0, py, rc.right, py + ph }; FillRect(dc, &m, bc); }
    if (py + ph < rc.bottom) { RECT b = { 0, py + ph, rc.right, rc.bottom }; FillRect(dc, &b, bw); }
    if (py <= 0 && ph <= 0) FillRect(dc, &rc, bw);
    DeleteObject(bw);
    DeleteObject(bc);
}

/* 按「名义基准 × 当前缩放」设置窗口外框，使客户区正好等于 nominal×factor */
static void resize_window_to_nominal(HWND hwnd) {
    int cw = (int)round(g_app.nominal_w * g_app.scale_factor);
    int ch = (int)round(g_app.nominal_h * g_app.scale_factor);
    RECT wr = { 0, 0, cw, ch };
    AdjustWindowRectEx(&wr, (DWORD)GetWindowLongPtrW(hwnd, GWL_STYLE), FALSE,
                       (DWORD)GetWindowLongPtrW(hwnd, GWL_EXSTYLE));
    RECT cur;
    GetWindowRect(hwnd, &cur);
    SetWindowPos(hwnd, NULL, cur.left, cur.top,
                 wr.right - wr.left, wr.bottom - wr.top,
                 SWP_NOZORDER | SWP_NOACTIVATE);
}

static void rebuild_ui(void) {
    HWND hwnd = g_app.hwndMain;
    if (!hwnd) return;
    /* v1.3.2：重建前先把用户在编辑框里手改过的内容记下来，重建后回填，
       不再被 g_app 里的旧值覆盖（报告 B3） */
    WCHAR keep_plugin[MAX_PATH_W] = {0};
    WCHAR keep_folder[MAX_PATH_W] = {0};
    WCHAR keep_rule[MAX_PATH_W] = {0};
    if (g_app.hwndPluginDir) GetWindowTextW(g_app.hwndPluginDir, keep_plugin, MAX_PATH_W);
    if (g_app.hwndEditFolder) GetWindowTextW(g_app.hwndEditFolder, keep_folder, MAX_PATH_W);
    if (g_app.hwndRulePath)   GetWindowTextW(g_app.hwndRulePath, keep_rule, MAX_PATH_W);
    {
        RECT crc; GetClientRect(hwnd, &crc);
        WCHAR b[160];
        sfmt(b, 160, L"[rebuild layout] client %ldx%ld, base=%dx%d",
             (long)crc.right, (long)crc.bottom, g_app.base_w, g_app.base_h);
        log_info(b);
    }

    /* 先把预览位图从 STATIC 里取回来释放，避免重建后原图泄漏 */
    if (g_app.hwndPreview) {
        HBITMAP old = (HBITMAP)SendMessageW(g_app.hwndPreview, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)NULL);
        if (old) DeleteObject(old);
    }
    /* 销毁所有子控件后重建 */
    HWND c = GetWindow(hwnd, GW_CHILD);
    while (c) { HWND nx = GetWindow(c, GW_HWNDNEXT); DestroyWindow(c); c = nx; }
    g_app.hwndLog = NULL; g_app.hwndStatus = NULL;
    for (int i = 0; i < 3; i++) g_app.hwndTabs[i] = NULL;
    g_app.hwndImgList = NULL; g_app.hwndRuleList = NULL; g_app.hwndPreview = NULL;
    /* 预览控件已销毁，位图句柄不再被任何窗口持有，交给退出时统一释放 */
    g_app.hbm_preview = NULL;

    build_ui(hwnd);
    switch_tab(g_cur_tab);

    /* 回填数据：编辑框优先保留重建前用户手改的内容 */
    SetWindowTextW(g_app.hwndPluginDir, keep_plugin[0] ? keep_plugin : g_app.plugin_dir);
    SetWindowTextW(g_app.hwndEditFolder, keep_folder[0] ? keep_folder : g_app.v_folder);
    if (g_app.hwndRulePath && keep_rule[0]) SetWindowTextW(g_app.hwndRulePath, keep_rule);
    SendMessageW(hSliderAlpha, TBM_SETPOS, TRUE, g_app.v_alpha);
    on_slider_change();
    sync_switch_ui();
    refresh_image_list();
    refresh_rule_list();
    refresh_rule_img_options();
    update_summary();
    EnableWindow(g_app.hwndBtnEmergency, !g_app.emergency_backup_valid);
    EnableWindow(g_app.hwndBtnRestorePrev, g_app.emergency_backup_valid);
    log_view_restore();
    InvalidateRect(hwnd, NULL, TRUE);
}

/* ------------------------------------------------------------------ */
/* 界面缩放：自绘下拉按钮 + TrackPopupMenu（v1.3.4，替代原生 COMBOBOX） */
/* ------------------------------------------------------------------ */

/* 应用第 sel 档（0..7，对应 SCALE_CHOICES/SCALE_FACTORS）——逻辑与旧
   CBN_SELCHANGE 分支完全一致：写 scaling.ini → 换字体 → 调窗口 → 重建 */
static void apply_scale_choice(HWND hwnd, int sel) {
    if (sel < 0 || sel > 7) return;
    g_app.scale_idx = sel - 1;
    g_app.scale_factor = SCALE_FACTORS[sel];
    write_pref_file(L"scaling.ini", SCALE_CHOICES[sel]);
    {
        WCHAR b[128];
        sfmt(b, 128, L"UI scaling switched to %s; rebuilding the UI", SCALE_CHOICES[sel]);
        log_info(b);
    }
    SetWindowTextW(g_app.hwndScaleBtn, SCALE_CHOICES[sel]);
    /* 先换字体（字号随缩放），再改窗口尺寸，最后重建界面并释放旧字体 */
    HFONT oldf[4] = { g_app.font_ui, g_app.font_mono, g_app.font_bold, g_app.font_chip };
    init_fonts();
    resize_window_to_nominal(hwnd);
    rebuild_ui();
    for (int i = 0; i < 4; i++) if (oldf[i]) DeleteObject(oldf[i]);
}

/* 点下拉按钮：弹出 8 档菜单，当前档打勾 */
static void on_scale_menu(HWND hwnd) {
    if (!g_app.hwndScaleBtn) return;
    HMENU menu = CreatePopupMenu();
    if (!menu) return;
    int cur = g_app.scale_idx + 1;
    for (int i = 0; i < 8; i++)
        AppendMenuW(menu, MF_STRING | MF_ENABLED | (i == cur ? MF_CHECKED : 0),
                    IDM_SCALE_FIRST + i, SCALE_CHOICES[i]);
    RECT rc; GetWindowRect(g_app.hwndScaleBtn, &rc);
    int sel = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
                             rc.left, rc.bottom, 0, hwnd, NULL);
    DestroyMenu(menu);
    if (sel >= IDM_SCALE_FIRST && sel <= IDM_SCALE_FIRST + 7)
        apply_scale_choice(hwnd, sel - IDM_SCALE_FIRST);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        g_app.hwndMain = hwnd;
        init_fonts();
        build_ui(hwnd);
        switch_tab(0);
        { /* 诊断辅助：BACKIMG_TAB=n 启动时直接切到第 n 页 */
            char tb[8];
            if (GetEnvironmentVariableA("BACKIMG_TAB", tb, 8)) {
                int t = atoi(tb);
                if (t >= 0 && t <= 2) { g_cur_tab = t; switch_tab(t); }
            }
        }
        break;
    }
    case WM_GETMINMAXINFO: {
        /* 基础配置页内容约需 400px 卡片高 + 目录卡 + 底栏/日志，
           窗口再小内容会被裁切（v1.3.3 窄窗实测发现），故设最小可拉尺寸 */
        MINMAXINFO *mmi = (MINMAXINFO *)lParam;
        mmi->ptMinTrackSize.x = (LONG)(g_app.scale_factor * 1020);
        mmi->ptMinTrackSize.y = (LONG)(g_app.scale_factor * 970);
        return 0;
    }
    case WM_SIZE: {
        InvalidateRect(hwnd, NULL, TRUE);
        if (wParam == SIZE_MINIMIZED) break;
        if (g_first_size) { g_first_size = false; break; }
        RECT crc; GetClientRect(hwnd, &crc);
        if (crc.right <= 50 || crc.bottom <= 50) break;
        /* v1.3.2：只有"user manual stretch"才更新名义基准。
           缩放切换时窗口尺寸本来就是 nominal×factor，与期望值一致，
           若在这里再改写 nominal，每切一次缩放就会小掉一圈边框（上轮 P2）。 */
        int exp_w = (int)round(g_app.nominal_w * g_app.scale_factor);
        int exp_h = (int)round(g_app.nominal_h * g_app.scale_factor);
        if (abs(crc.right - exp_w) > 2 || abs(crc.bottom - exp_h) > 2) {
            g_app.nominal_w = (int)(crc.right / g_app.scale_factor);
            g_app.nominal_h = (int)(crc.bottom / g_app.scale_factor);
            /* 防抖：120ms 后按新客户区重建布局 */
            KillTimer(hwnd, TIMER_RELAYOUT);
            SetTimer(hwnd, TIMER_RELAYOUT, 120, NULL);
        }
        break;
    }
    case WM_DRAWITEM: {
        on_draw_item((DRAWITEMSTRUCT*)lParam);
        return TRUE;
    }
    case WM_CTLCOLORBTN: {
        /* v1.3.7：owner-draw 按钮先被本刷子擦整个矩形再画圆角主体——之前
           缺这个分支，DefWindowProc 返回系统灰 COLOR_BTNFACE，按钮圆角四角
           露出直角灰块（用户标注"shadow outside rounded corners is square"）。按按钮父容器底色
           返回画刷，规则与 WM_CTLCOLORSTATIC 一致（只定底色、不动文字色） */
        HWND parent = GetParent((HWND)lParam);
        static HBRUSH br_btn_white = NULL, br_btn_loghead = NULL, br_btn_seg = NULL;
        if (!br_btn_white)   br_btn_white   = CreateSolidBrush(C_PANEL);
        if (!br_btn_loghead) br_btn_loghead = CreateSolidBrush(C_LOG_HEAD_BG);
        if (!br_btn_seg)     br_btn_seg     = CreateSolidBrush(C_SEG_BG);
        if (parent) {
            WCHAR cls[32] = {0};
            GetClassNameW(parent, cls, 32);
            if (_wcsicmp(cls, L"backimgCardPanel") == 0 ||
                parent == g_app.hwndStatus) {
                switch (GetWindowLongPtrW(parent, GWLP_USERDATA) & PANEL_BG_MASK) {
                case PANEL_BG_LOGHEAD: return (LRESULT)br_btn_loghead;
                case PANEL_BG_SEG:     return (LRESULT)br_btn_seg;
                case PANEL_BG_WHITE:   return (LRESULT)br_btn_white;
                default:               return (LRESULT)br_btn_white;
                }
            }
        }
        return (LRESULT)br_btn_white;   /* 主窗口/STATIC 容器上的按钮都在白底区 */
    }
    case WM_CTLCOLORSTATIC: {
        HDC dc = (HDC)wParam;
        HWND ctl = (HWND)lParam;
        SetBkMode(dc, TRANSPARENT);
        HWND parent = GetParent(ctl);
        COLORREF tc = (COLORREF)GetWindowLongPtrW(ctl, GWLP_USERDATA);
        if (!tc) tc = C_TEXT;
        SetTextColor(dc, tc);

        static HBRUSH br_canvas = NULL, br_white = NULL, br_status = NULL;
        static HBRUSH br_loghead = NULL, br_seg = NULL;
        if (!br_canvas) br_canvas = CreateSolidBrush(C_CANVAS);
        if (!br_white) br_white = CreateSolidBrush(C_PANEL);
        if (!br_status) br_status = CreateSolidBrush(C_STATUS_BG);
        if (!br_loghead) br_loghead = CreateSolidBrush(C_LOG_HEAD_BG);
        if (!br_seg) br_seg = CreateSolidBrush(C_SEG_BG);

        WCHAR cls[32] = {0};
        if (parent) GetClassNameW(parent, cls, 32);
        if (parent == g_app.hwndStatus) return (LRESULT)br_status;
        if (_wcsicmp(cls, L"backimgCardPanel") == 0) {
            /* v1.3.2：按容器自身背景模式返回刷子（日志头/分段容器不再是白或灰） */
            LONG_PTR pst = GetWindowLongPtrW(parent, GWLP_USERDATA);
            switch (pst & PANEL_BG_MASK) {
            case PANEL_BG_WHITE:   return (LRESULT)br_white;
            case PANEL_BG_LOGHEAD: return (LRESULT)br_loghead;
            case PANEL_BG_SEG:     return (LRESULT)br_seg;
            default:               return (LRESULT)br_canvas;
            }
        }
        return (LRESULT)br_canvas;
    }
    case WM_CTLCOLOREDIT: {
        HDC dc = (HDC)wParam;
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, C_TEXT);
        return (LRESULT)GetStockObject(WHITE_BRUSH);
    }
    case WM_CTLCOLORLISTBOX: {
        static HBRUSH br_log = NULL;
        if (!br_log) br_log = CreateSolidBrush(C_LOG_BG);
        return (LRESULT)br_log;
    }
    case WM_NOTIFY: {
        NMHDR *nm = (NMHDR*)lParam;
        if (nm->code == NM_CLICK && nm->hwndFrom == g_app.hwndImgList) {
            int idx = (int)SendMessageW(g_app.hwndImgList, LVM_GETNEXTITEM, -1, LVNI_SELECTED);
            if (idx >= 0) on_img_select(idx);
        } else if (nm->code == NM_CLICK && nm->hwndFrom == g_app.hwndRuleList) {
            int idx = (int)SendMessageW(g_app.hwndRuleList, LVM_GETNEXTITEM, -1, LVNI_SELECTED);
            if (idx >= 0) on_rule_select(idx);
        }
        break;
    }
    case WM_HSCROLL: {
        if ((HWND)lParam == hSliderAlpha) on_slider_change();
        break;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam); int code = HIWORD(wParam);
        if (id >= ID_TAB_BASIC && id <= ID_TAB_FOLDERS) {
            g_cur_tab = id - ID_TAB_BASIC;
            switch_tab(g_cur_tab);
        }
        else if (id == ID_BTN_BROWSE_DIR) on_browse_dir();
        else if (id == ID_BTN_LOCATE) on_locate();
        else if (id == ID_BTN_SAVE) save_config();
        else if (id == ID_BTN_REG) on_register();
        else if (id == ID_BTN_UNREG) on_unregister();
        else if (id == ID_BTN_RESTART) on_restart();
        else if (id == ID_BTN_REFRESH) browse_plugin(g_app.plugin_dir);
        else if (id == ID_BTN_BACKUP) on_backup();
        else if (id == ID_BTN_RESTORE) on_restore();
        else if (id == ID_BTN_LOG_CLEAR) {
            SendMessageW(g_app.hwndLog, LB_RESETCONTENT, 0, 0);
            g_log_count = 0;               /* 同时清空缓冲，避免重建时回填 */
            InvalidateRect(g_app.hwndLog, NULL, TRUE);
        }
        else if (id == ID_BTN_PICK_FOLDER) on_browse_folder();
        else if (id == ID_BTN_SUM_REFRESH) update_summary();
        else if (id == ID_BTN_OPEN_INI) on_open_ini();
        else if (id == ID_BTN_COPY_SUM) on_copy_summary();
        else if (id == ID_BTN_EMERGENCY) on_emergency();
        else if (id == ID_BTN_RESTORE_PREV) on_restore_prev();
        else if (id == ID_BTN_IMG_REFRESH) { invalidate_img_count(); refresh_image_list(); update_summary(); }
        else if (id == ID_BTN_OPEN_IMG_DIR) on_open_img_dir();
        else if (id == ID_BTN_RULE_ADD) on_rule_add();
        else if (id == ID_BTN_RULE_DEL) on_rule_del();
        else if (id == ID_BTN_RULE_REFRESH) { refresh_rule_list(); refresh_rule_img_options(); }
        else if (id == ID_SW_FOLDEREXT && code == BN_CLICKED) { on_folder_ext_toggle(!g_app.v_folderExt); sync_switch_ui(); }
        else if (id == ID_SW_NOERROR && code == BN_CLICKED) { g_app.v_noerror = !g_app.v_noerror; sync_switch_ui(); update_summary(); }
        else if (id == ID_SW_RANDOM && code == BN_CLICKED) { g_app.v_random = !g_app.v_random; sync_switch_ui(); update_summary(); }
        else if (id == ID_SW_CUSTOM && code == BN_CLICKED) { on_custom_toggle(!g_app.v_custom); sync_switch_ui(); }
        else if (id >= ID_SEG_POSTYPE_0 && id <= ID_SEG_POSTYPE_6 && code == BN_CLICKED) on_pos_select(id - ID_SEG_POSTYPE_0);
        else if (id == ID_CB_SCALE && code == BN_CLICKED) {
            /* v1.3.4：自绘下拉按钮点击 → 弹出菜单（原 CBN_SELCHANGE 分支删除） */
            on_scale_menu(hwnd);
        }
        else if (id == ID_CB_CLSID && code == BN_CLICKED) {
            /* v1.3.9：自绘下拉按钮点击 → 弹出菜单（原 CBN_SELCHANGE 分支删除） */
            on_clsid_menu(hwnd);
        }
        else if (id == ID_CB_RULE_IMG && code == BN_CLICKED) {
            on_ruleimg_menu(hwnd);
        }
        else if (id == ID_CB_ROT && code == BN_CLICKED) {
            /* v1.3.20(C)：定时轮换下拉按钮点击 → 弹出菜单（与「界面缩放」同模式） */
            on_rot_menu(hwnd);
        }
        else if (id == ID_CB_ROTUNIT && code == BN_CLICKED) {
            /* v1.3.25(D1)：单位下拉按钮点击 → 弹出分钟/秒钟菜单 */
            on_rotunit_menu(hwnd);
        }
        else if (id == ID_EDIT_ROTMIN && code == EN_CHANGE) {
            /* v1.3.21(G2)：自定义时间输入框实时同步（v1.3.25 起按单位换算成秒） */
            on_rotval_edit_change();
        }
        else if (id == ID_EDIT_FOLDER && code == EN_CHANGE) {
            GetWindowTextW(g_app.hwndEditFolder, g_app.v_folder, MAX_PATH_W);
            /* v1.3.13（豆包核验观察点 #1）：custom 未勾选时 v_folder 不影响
               生效目录（仍用插件 Image），不失效张数缓存、不重扫；
               update_summary 保留——摘要的 folder 行要跟随显示 */
            if (g_app.v_custom && g_app.v_folder[0]) {
                invalidate_img_count();
                refresh_image_list();
                refresh_rule_img_options();
            }
            update_summary();
        }
        break;
    }
    case WM_USER + 10: { /* register done：按 regsvr32 退出码判定（lParam） */
        clear_busy();
        DWORD code = (DWORD)lParam;
        if (!wParam) {
            WCHAR b[256]; sfmt(b, 256, L"Register failed: could not start regsvr32 (UAC may have been cancelled)");
            log_err(b);
            MessageBoxW(hwnd, L"Register failed: could not start regsvr32 (UAC may have been cancelled).", L"Failed", MB_OK | MB_ICONERROR);
        } else if (code != 0) {
            WCHAR b[256]; sfmt(b, 256, L"Register failed: regsvr32 exit code %lu", code);
            log_err(b);
            MessageBoxW(hwnd, b, L"Failed", MB_OK | MB_ICONERROR);
        } else {
            log_info(L"Registration complete");
        }
        browse_plugin(g_app.plugin_dir);
        break;
    }
    case WM_USER + 11: { /* unregister done：同上 */
        clear_busy();
        DWORD code = (DWORD)lParam;
        if (!wParam) {
            log_err(L"Unregister failed: could not start regsvr32 (UAC may have been cancelled)");
            MessageBoxW(hwnd, L"Unregister failed: could not start regsvr32 (UAC may have been cancelled).", L"Failed", MB_OK | MB_ICONERROR);
        } else if (code != 0) {
            WCHAR b[256]; sfmt(b, 256, L"Unregister failed: regsvr32 exit code %lu", code);
            log_err(b);
            MessageBoxW(hwnd, b, L"Failed", MB_OK | MB_ICONERROR);
        } else {
            log_info(L"Unregistration complete");
        }
        browse_plugin(g_app.plugin_dir);
        break;
    }
    case WM_USER + 12: { /* restart done：区分 taskkill 是否真的杀到了 */
        clear_busy();
        int flag = (int)wParam;
        DWORD tk = (DWORD)lParam;
        if (flag == 0) {
            log_err(L"Restart Explorer failed: could not run taskkill");
        } else if (flag == 2) {
            WCHAR b[256]; sfmt(b, 256, L"taskkill exit code %lu (explorer may not be running); tried starting a new instance", tk);
            log_warn(b);
        } else {
            log_info(L"Explorer restarted");
        }
        SetTimer(hwnd, 1, 2500, NULL);
        break;
    }
    case WM_TIMER: {
        if (wParam == 1) { KillTimer(hwnd, 1); browse_plugin(g_app.plugin_dir); }
        else if (wParam == TIMER_STARTUP_VERIFY) {
            /* 旧实现只 SetTimer 从不 Kill，于是每 400ms 空转一次（v1.3.1 修复） */
            KillTimer(hwnd, TIMER_STARTUP_VERIFY);
            startup_verify();
        }
        else if (wParam == TIMER_BUSY_WATCHDOG) {
            KillTimer(hwnd, TIMER_BUSY_WATCHDOG);
            if (g_app.busy) {
                log_warn(L"Operation timed out (60 s, no response); buttons force-unlocked");
                clear_busy();
            }
        }
        else if (wParam == TIMER_RELAYOUT) {
            KillTimer(hwnd, TIMER_RELAYOUT);
            rebuild_ui();
        }
        break;
    }
    case WM_ERASEBKGND: {
        paint_main_bg((HDC)wParam, hwnd);
        return 1;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc = BeginPaint(hwnd, &ps);
        paint_main_bg(dc, hwnd);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_CLOSE: {
        /* 保存窗口尺寸：记客户区换算出的名义基准，下次打开时客户区一致（v1.3.2） */
        RECT crc; GetClientRect(hwnd, &crc);
        RECT wr; GetWindowRect(hwnd, &wr);
        int bw = (int)(crc.right / g_app.scale_factor);
        int bh = (int)(crc.bottom / g_app.scale_factor);
        g_app.nominal_w = bw;
        g_app.nominal_h = bh;
        WCHAR buf[128]; sfmt(buf, 128, L"%dx%d+%d+%d", bw, bh, wr.left, wr.top);
        write_pref_file(L"winsize.ini", buf);
        DestroyWindow(hwnd);
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* 入口                                                                */
/* ------------------------------------------------------------------ */
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmd, int nShow) {
    (void)hPrev; (void)lpCmd;

    if (GetVersion() >= 0x80000000) {
        MessageBoxW(NULL, L"This program supports Windows 10/11 only.", L"Notice", MB_OK);
        return 1;
    }

    /* v1.3.23(K4，Kimi 审计)：memset 挪到 CreateMutexW 之前——
     * 原顺序先把句柄存进 g_app.hMutex、又立刻 memset(&g_app,0,...) 清零，
     * 导致退出时 CloseHandle 因 hMutex==NULL 被跳过（句柄泄漏，
     * 系统兜底回收；单实例功能本身不受影响）。先清再建，顺序归位。 */
    memset(&g_app, 0, sizeof(g_app));

    /* 单实例锁 */
    g_app.hMutex = CreateMutexW(NULL, FALSE, L"{ED15A97D-FE3E-4CDE-98FF-BC46B02896B0}_backimg_gui");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBoxW(NULL, L"backimg Panel is already running.\nDo not open it twice.", L"Notice", MB_ICONWARNING);
        return 1;
    }

    /* 初始化 */
    g_app.scale_idx = -1;
    g_app.scale_factor = 1.0;
    g_app.base_w = 1060;
    g_app.base_h = 980;   /* 默认高度需容纳基础配置页全部内容 */
    g_app.nominal_w = g_app.base_w;
    g_app.nominal_h = g_app.base_h;
    g_app.win_x = CW_USEDEFAULT;
    g_app.win_y = CW_USEDEFAULT;

    get_exe_dir(g_app.exe_dir, MAX_PATH_W);
    ensure_self_contained();
    path_join(g_app.dll_path, MAX_PATH_W, g_app.plugin_dir, DLL_NAME);
    path_join(g_app.ini_path, MAX_PATH_W, g_app.plugin_dir, INI_NAME);
    path_join(g_app.image_dir, MAX_PATH_W, g_app.plugin_dir, IMAGE_DIR_NAME);

    /* 读缩放偏好 */
    WCHAR scal[64]; read_pref_file(L"scaling.ini", scal, 64);
    for (int i = 0; i < 8; i++) {
        if (_wcsicmp(scal, SCALE_CHOICES[i]) == 0) { g_app.scale_idx = i - 1; g_app.scale_factor = SCALE_FACTORS[i]; break; }
    }

    /* 读窗口尺寸 */
    WCHAR ws[128]; read_pref_file(L"winsize.ini", ws, 128);
    if (ws[0]) {
        int bw = 0, bh = 0, x = 0, y = 0;
        if (swscanf(ws, L"%dx%d+%d+%d", &bw, &bh, &x, &y) == 4) {
            /* v1.3.2：阈值放宽（旧版 bh>=940 太严，窗口调矮后不记忆） */
            /* v1.3.23(K5，Kimi 审计)：bw 补上限 7680（8K 屏宽）——winsize.ini
             * 属不可信输入，此前手工改成超大数会造出异常巨窗 */
            if (bw >= 760 && bw <= 7680 && bh >= 640 && bh <= 2400) {
                g_app.base_w = bw; g_app.base_h = bh;
                g_app.nominal_w = bw; g_app.nominal_h = bh;
                /* 位置仍在屏幕内才沿用，否则交给系统居中（v1.3.1） */
                if (window_visible_on_screen(x, y, (int)(bw * g_app.scale_factor),
                                                    (int)(bh * g_app.scale_factor))) {
                    g_app.win_x = x; g_app.win_y = y;
                }
            }
        }
    }

    g_app.admin = IsUserAnAdmin();

    /* GDI+（失败时预览功能降级，但主程序照常可用） */
    GdiplusStartupInput gsi;
    memset(&gsi, 0, sizeof(gsi));
    gsi.GdiplusVersion = 1;
    if (GdiplusStartup(&g_app.gdiplusToken, &gsi, NULL) == Ok) g_gdiplus_ok = true;

    /* Common Controls */
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES };
    InitCommonControlsEx(&icc);

    /* 注册窗口类 */
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hIcon = LoadIconW(hInst, MAKEINTRESOURCEW(IDI_APPICON));
    wc.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"backimgMainWnd";
    RegisterClassExW(&wc);

    WNDCLASSEXW wcc = { sizeof(wcc) };
    wcc.style = CS_HREDRAW | CS_VREDRAW;
    wcc.lpfnWndProc = CardPanelProc;
    wcc.hInstance = hInst;
    wcc.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wcc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcc.lpszClassName = L"backimgCardPanel";
    RegisterClassExW(&wcc);

    WNDCLASSEXW wcs = { sizeof(wcs) };
    wcs.style = CS_HREDRAW | CS_VREDRAW;
    wcs.lpfnWndProc = StatusBarProc;
    wcs.hInstance = hInst;
    wcs.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wcs.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcs.lpszClassName = L"backimgStatusBar";
    RegisterClassExW(&wcs);

    WNDCLASSEXW wcn = { sizeof(wcn) };
    wcn.style = CS_HREDRAW | CS_VREDRAW;
    wcn.lpfnWndProc = StepNoProc;
    wcn.hInstance = hInst;
    wcn.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wcn.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcn.lpszClassName = L"backimgStepNo";
    RegisterClassExW(&wcn);

    WNDCLASSEXW wct = { sizeof(wct) };
    wct.style = CS_HREDRAW | CS_VREDRAW;
    wct.lpfnWndProc = FlowTipProc;
    wct.hInstance = hInst;
    wct.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wct.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wct.lpszClassName = L"backimgFlowTip";
    RegisterClassExW(&wct);

    /* v1.3.2：key/readout 芯片、摘要键（省略号）、虚线分隔 */
    WNDCLASSEXW wcx = { sizeof(wcx) };
    wcx.style = CS_HREDRAW | CS_VREDRAW;
    wcx.hInstance = hInst;
    wcx.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wcx.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcx.lpfnWndProc = ChipProc;    wcx.lpszClassName = L"backimgChip";    RegisterClassExW(&wcx);
    wcx.lpfnWndProc = SumKeyProc;  wcx.lpszClassName = L"backimgSumKey";  RegisterClassExW(&wcx);
    wcx.lpfnWndProc = DividerProc; wcx.lpszClassName = L"backimgDivider"; RegisterClassExW(&wcx);

    /* v1.3.3：自绘输入框外框、自绘滑块、摘要值（省略号） */
    wcx.lpfnWndProc = EditFrameProc; wcx.lpszClassName = L"backimgEditFrame"; RegisterClassExW(&wcx);
    wcx.lpfnWndProc = SliderProc;    wcx.lpszClassName = L"backimgSlider";    RegisterClassExW(&wcx);
    wcx.lpfnWndProc = SumValProc;    wcx.lpszClassName = L"backimgSumVal";    RegisterClassExW(&wcx);

    /* 创建主窗口（标题带版本号；可拉伸，尺寸会记忆）
       v1.3.2：base 是客户区基准，需要按窗口样式换算外框，否则客户区比记忆值小一圈 */
    int ww = (int)(g_app.base_w * g_app.scale_factor);
    int wh = (int)(g_app.base_h * g_app.scale_factor);
    {
        RECT wr = { 0, 0, ww, wh };
        AdjustWindowRectEx(&wr, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);
        ww = wr.right - wr.left;
        wh = wr.bottom - wr.top;
    }
    WCHAR titlebuf[160], ver_w[32];
    u8tow(GUI_VERSION, ver_w, 32);   /* v1.3.2：统一走安全转换助手 */
    sfmt(titlebuf, 160, L"%s v%s (paired DLL v%S)", APP_NAME_W, ver_w, DLL_VERSION);
    HWND hwnd = CreateWindowExW(WS_EX_APPWINDOW | WS_EX_WINDOWEDGE,
                                L"backimgMainWnd",
                                titlebuf,
                                WS_OVERLAPPEDWINDOW,
                                g_app.win_x, g_app.win_y, ww, wh,
                                NULL, NULL, hInst, NULL);
    if (!hwnd) return 1;

    g_app.hwndMain = hwnd;

    /* 设置图标 */
    SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIconW(hInst, MAKEINTRESOURCEW(IDI_APPICON)));
    SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)LoadImageW(hInst, MAKEINTRESOURCEW(IDI_APPICON),
                  IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));

    ShowWindow(hwnd, nShow);
    UpdateWindow(hwnd);

    /* 初始状态检测与加载 */
    browse_plugin(g_app.plugin_dir);
    load_config();

    log_info(L"backimg Panel started");

    /* 启动后 DLL 校验 */
    SetTimer(hwnd, 2, 400, NULL);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (g_gdiplus_ok) GdiplusShutdown(g_app.gdiplusToken);
    if (g_app.hMutex) CloseHandle(g_app.hMutex);
    if (g_app.hbm_preview) DeleteObject(g_app.hbm_preview);
    if (g_ini_text) free(g_ini_text);
    return (int)msg.wParam;
}




