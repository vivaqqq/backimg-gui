# backimg-gui（backimg 图形板）

给 Windows 文件资源管理器的文件夹窗口铺上自定义背景图——支持多图随机、秒/分钟级定时轮换、等比缩放实时跟随窗口、文件对话框铺图。

![backimg 图形板主界面](docs/screenshot.png)

> **📌 诚实声明**：这个仓库、所有源代码的修复与增强、以及把它开源上架的整个过程，**全部由 AI 完成**——主力是 WorkBuddy（GLM 大模型），修复与审计环节还用了千问、Kimi、DeepSeek、混元、豆包做多轮交叉审计。仓库主人**完全不懂代码**，是零代码用户，全程只负责提需求、验收和按下"发布"。源代码和程序都放在这里，MIT 协议随便用——**觉得不满意？欢迎自己拿回家改**，这正是开源的意义。
>
> **📌 Honest Disclosure**: This repository — every fix, every feature, and the entire process of open-sourcing it — was **built entirely with AI**. The main workhorse was WorkBuddy (the GLM large language model), with fixes and audits cross-checked over multiple rounds by Qwen, Kimi, DeepSeek, Hunyuan, and Doubao. The repo owner **cannot write code at all** — a zero-code user who only provided requirements, reviewed the results, and pressed "publish". Both the source code and the program are right here under the MIT license — **not satisfied? Feel free to take it home and modify it yourself**. That is exactly what open source is for.

> 本项目基于 [Maplespe/explorerTool](https://github.com/Maplespe/explorerTool)（MIT License，Copyright © 2021 Maplespe）深度增强而来，原版 DLL 的全部能力保留，其上叠加了大量修复与功能扩展。原作介绍页：[winmoes.com](https://winmoes.com/tools/12556.html)。

## 定位声明（请先读这段）

- 这是作者的**个人魔改增强版**，按 **AS-IS** 提供：**不承诺维护**，不承诺修复任何 Issue。
- 欢迎提交带完整日志的 Bug 报告与 PR；能否修复、何时修复不作保证。
- **兼容性**：作者在 Windows 11 实机开发测试；Windows 10 是原版支持的平台，本分支改动集中于 GDI+ 绘图层，理论上兼容但**未实测**，欢迎实测反馈。

## 相对原版的主要增强

| # | 增强 | 说明 |
|---|------|------|
| 1 | 图片格式扩至 7 种 | png / jpg / jpeg / bmp / gif（取首帧）/ tif / tiff，原版仅 png、jpg |
| 2 | 运行日志 | `plugin/ExplorerBgTool_init.log`，排查问题不再靠猜 |
| 3 | 坏文件拒收 | 伪装成图片的损坏文件不再进入随机池 |
| 4 | 等比缩放 + 实时跟随 | 七档显示位置均等比不变形；拖动/最大化实时跟随；GDI+ 双三次高质量插值；缩放缓存 |
| 5 | 定时轮换 | 真·随机源；支持秒级（如 30 秒）与分钟级间隔，每个文件夹窗口独立换图 |
| 6 | 自定义目录自愈 | 目录不存在时轻量重试，配置恢复后自动恢复显示 |
| 7 | 图形面板（图形板） | Win32 纯 C 外壳：注册/注销 DLL、选图、调参、恢复配置，内嵌 DLL 并做 SHA256 成对校验 |

完整对照见 [docs/功能对照说明.md](docs/功能对照说明.md)；逐版本修改细节见 [dll/changelog/](dll/changelog/)（v1.3.15 → v1.3.28 共 14 份变更清单与迭代总卷）。

## 构建

需要 [MinGW-w64](https://www.mingw-w64.org/)（g++ / gcc / windres）。

```bat
:: 1. 构建 DLL（输出到 ..\build\ExplorerBgTool.dll）
cd dll
build.cmd <MinGW-w64的bin目录>

:: 2. 构建图形面板（输出 backimg图形板_v1.3.28.exe）
cd ..\shell
build.cmd
```

## 使用方法

1. 管理员身份运行图形板（需要注册 DLL：`regsvr32`）
2. 选择图片目录 → 选择显示位置 → 保存配置 → 重启资源管理器
3. 想要定时换图：基础配置页「随机显示图片」选档位或"自定义…"，填数字选分钟/秒钟 → 保存 → 重启资源管理器
4. 升级安装时，`exe` 与 `plugin/ExplorerBgTool.dll` 必须**成对替换**（内嵌 SHA256 校验不一致会弹警告）
5. 出现崩溃时：按住 `ESC` 再打开资源管理器（不加载图片），然后注销本工具

## 目录结构

```
├── shell/              图形面板源码（纯 C + Win32）
├── dll/                ExplorerBgTool.dll 源码（C++，MinHook 钩子）
│   └── changelog/      v1.3.15 → v1.3.28 逐版本变更清单 + 迭代总卷
├── docs/               原版 vs 本版功能对照说明
└── plugin/
    └── config.ini.example  配置文件模板（含全部配置键的中英注释）
```

## 配置

配置键说明见 [`plugin/config.ini.example`](plugin/config.ini.example)，与上游 v1.3.25+ 完全兼容，旧配置可直接使用。修改后重开资源管理器窗口即生效（无需重启 explorer）。

## 已知设计取舍（非 Bug）

- 「左上/右上/左下/右下/居中」五档：图片放得下按原始像素摆放，放不下才等比缩小——小图不会被放大成糊图；需要随窗口放大请选「缩放」档
- 「缩放」= 整图完整适配（等比，可能留白）；「缩放并填充」= 等比铺满居中裁溢出
- 替换同名图片内容需重启 explorer 才生效（与上游一致）
- 缓存内存上界语义：单线程 = 预算（缩放 32MB / 源图 64MB）+ 最大单条；单一超大图允许超预算驻留，最坏情况有界且已知

## 许可证

[MIT](LICENSE)。

- 原版 ExplorerBgTool：Copyright © 2021 [Maplespe](https://github.com/Maplespe)
- 本仓库中的修改部分：同样以 MIT 协议发布
