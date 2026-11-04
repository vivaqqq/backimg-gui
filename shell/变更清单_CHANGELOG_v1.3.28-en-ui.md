# backimg Panel 英文版 UI · 变更清单（CHANGELOG v1.3.28-en-ui）

| 项目 | 内容 |
| --- | --- |
| 基线 | 中文版 v1.3.28（backimg 图形板，含全部 14 轮中文版迭代） |
| 变更范围 | **仅外壳 UI 文本与布局常量**；代码逻辑零改动；DLL 一字节未动（仍为 v1.3.28，与中文版共用，保证 SHA256 配对校验一致） |
| 产物 | `backimg-gui_en_v1.3.28.exe` + `backimg-gui_v1.3.28_english-ui.zip` |
| 源码 | `shell/main.c`（english-ui 分支，发布时同步） |

---

## EN-0 翻译基线（WorkBuddy / GLM）

- 307 条唯一字符串（377 处出现）全部译英：UI 标签用短英文防溢出，日志用自然英文
- `resource.rc` 本地化为英文区域设置（040904B0 / 0x0409），版本信息全英文
- 首次编译即通过；exe 二进制扫描：中文字符串（UTF-16/UTF-8 双编码）零残留

## EN-1 ~ EN-3 人类开发者三轮排版修正（实机截图红/蓝/绿框标注）

> 以下三轮均为**人类开发者实机验收提出**，WorkBuddy 定位代码并落实：

**第一轮（基础配置页 + 图片管理页）：**

| # | 问题 | 修正 |
| --- | --- | --- |
| 1 | `Browse...` 按钮文字贴边 | 按钮 60→68px，输入框右缘相应缩 12px 让位 |
| 2 | folderExt 行灰色说明文字右缘截断 | 精简为 "Folder windows only; tick adds Open/Save dialogs too" |
| 3 | `Custom image f...` 卡片标题截断 | 标签宽 110→150px（[folder] 芯片按 text_width 动态跟随） |
| 4 | `Ignore errors` 行说明 borderline | 精简为 "No popup on missing image or error; keep ticked" |

**第二轮（图片管理页 + 文件夹专属背景页）：**

| # | 问题 | 修正 |
| --- | --- | --- |
| 5 | `Refresh list` 按钮截断 | 宽 70→88px |
| 6 | `Open image folder` 两头削 | 宽 100→122px，右移让出间距 |
| 7 | `Per-folder bac` 标题截断 | 标签宽 100→180px |
| 8 | `Add/Update` 按钮边缘削字 | 宽 68→76px，右对齐位置同步外移 |
| 9 | `Image file nam` 标签截断 | 宽 80→110px（[Inside Image] 芯片动态跟随） |
| 10 | `elete selecte` 两头削 | `Delete selected` 70→100px |
| 11 | `Refresh list`（规则页）挤 | 右移至 122px、宽 70→86px |
| 12 | 规则说明句右缘截断 | 精简为 "Select a rule on the left to autofill; Add/Update overwrites it." |

**第三轮（文件夹专属背景页细节）：**

| # | 问题 | 修正 |
| --- | --- | --- |
| 13 | `Add/Update` 文字贴圆角边框 | 按钮 76→88px，文字与边框两侧留白 |
| 14 | 按钮与 Folder path 输入框零间距贴死 | 输入框右缘缩 12px，恢复 8px 标准间距 |

## EN-4 Deepseek V4.1 Flash 1017 审计修复（2026-11-04，实机运行+三页截图+最小窗宽对照+源码定位）

> 审计报告：`backimg英文版UI审查_2026-11-04\审查报告.md`（P1 五处 / P2 四处+漏网一处 / P3 六条 / P4 三项）

**P1-1 五处卡片标题硬切（连省略号都没有）+ 提示行截断：**

| 位置 | 原宽 | 新宽 |
| --- | --- | --- |
| Display options（main.c:2401） | S(80) | S(150) |
| Image position（main.c:2470） | S(90) | S(140) |
| Live configuration（main.c:2529） | S(110) | S(170) |
| Timed rotation（main.c:2827，右对齐） | S(56)/x=cx-S(64) | S(140)/x=cx-S(148) |
| Custom interval（main.c:2837） | S(64) | S(150) |
| Custom interval 提示行（main.c:2852） | S(230)，起于 edit_x | S(424)，改起于行首 sx |

**P1-2 单位下拉 `Seconds` 被箭头压字：** 下拉宽 S(88)→S(96)

**P1-3 摘要卡键名全线截断（英文版最影响可读性的一处）：**
- 9 个键名去掉 ini 键前缀：`folderExt · File dialogs` → `File dialogs` 等（键名里的 ini 键仍可在 config.ini 对照，可读性优先）
- 键列 keyw2 S(108)→S(96)，省出的宽度全部让给值列（`not set → using plugin folder/Image` 等长值受益）

**P1-4 最小窗宽 880 时底栏 7 按钮互相压字（中文版同宽无重叠，英文文案变长引入）：**
- `WM_GETMINMAXINFO` 最小追踪宽度 880→**1020**（底栏按钮为 text_width 动态宽，估算总需求 ~990px，取 1020 留余量）

**P1-5 窄窗 random 芯片被 `Timed` 标签覆盖：** 随最小窗宽提升一并解决（880→1020 后左卡宽 466→486，芯片与右侧标签不再进入同一区域）

**P2 全角标点清零（7 处，含审计后终验补刀的窄串孪生 1 处）：**
- `（%d%%）` → `(%d%%)` ×4（摘要行/滑块读数/测量串/摘要导出）
- `config.ini：%s` → `config.ini: %s`
- 弹窗 `usysdiag。` → `usysdiag.`
- 标题栏 `backimg Panel  v1.3.28` 双空格 → 单空格

**P3 措辞（采纳审计建议 5/6 条）：**

| 原译 | 改为 | 理由 |
| --- | --- | --- |
| `Panic off`（6 处联动） | `Emergency off` | panic 在英文语境是"程序崩溃" |
| `Fit & fill`（3 处联动） | `Fill` | 与上一档 `Fit` 语义撞车 |
| `Inside Image` | `In Image folder` | 直译缺名词 |
| `png/jpg/bmp are most stable` ×2 | `png/jpg/bmp recommended` | 措辞 + 两处文案不一致 |
| 注册表警告句 | `The DLL registered in the registry differs from the one in the current plugin folder...` | 原句缺成分 |
| `Reveal` | **保留** | 审计标注"可选"；按钮宽 48px 约束，且 "Show in Explorer" 需重排整行 |

**P4 历史遗留（中文版同样存在）：**
- P4-2 预览元信息行测量用常规字体、控件为粗体 → 测量改用粗体字体（长文件名不再与尺寸粘连）
- P4-3 exe 内嵌 config.ini 残留开发路径 `folder=D:\backimg_GUI_v1.3.17\Image` → 清空（用户删 config.ini 重新解包时不再拿到无效路径）
- P4-1 专属背景页首次打开计数 `0 rules`、需手动刷新 → **未修**（中文版同样存在的历史行为，改动涉及页面初始化时序，留待上游统一处理）

## 已知行为（非 Bug）

- DLL 侧 `ExplorerBgTool_init.log` 日志仍为中文（DLL 与中文版共用 v1.3.28，保证配对校验与钩子链路稳定）
- 审计修复建议第 5 条（`new_label_ex` 固定宽度 vs 实测宽度的开发期自检）未实施，列为可选改进
- 窗口最小宽度由 880 提升至 1020（缩放系数联动），极窄窗口下底栏不再重叠

## 验证记录（2026-11-04）

- 编译：MinGW-w64，`Build OK`，零 error
- exe 二进制扫描：中文字符串（UTF-16/UTF-8）零残留；全角标点（字面量级）零残留；开发路径 `backimg_GUI_v1.3.17` 零残留
- 英文串嵌入确认：`Restart Explorer`×6、`Emergency off`、`Live configuration` 等
- 发布包：zip 内 exe 与磁盘最新构建 MD5 一致；config.ini 无私人路径；无 winsize.ini/gui_log.txt 运行时垃圾

## 复核清单（下一轮验收用）

- [ ] 三个页面、100% 缩放，所有卡片标题完整显示
- [ ] 切到 `Custom…` 档，单位下拉完整显示 `Seconds`
- [ ] 摘要卡 9 个键名完整可读，不再出现 `Fil…` / `Ignor…`
- [ ] 窗口拉到 1020 宽，底栏 7 个按钮无重叠
- [ ] 窄窗时 random 行芯片不被 `Timed` 覆盖
- [ ] 界面与弹窗中全角标点数量为 0
- [ ] 标题栏、`DLL:` 日志行无双空格
- [ ] `Emergency off` 确认弹窗、`In Image folder` 芯片、`recommended` 计数显示正常

---

## EN-5 Deepseek V4.1 Flash 1017 第二轮复核修复（2026-11-04）

> 复核报告：`backimg英文版UI审查_第二轮_2026-11-04\复核报告.md`（解压官方 zip 实测 + EnumChildWindows 读真实控件矩形 + 源码逐条定位）
> 复核结论：第一轮 P1 五项全修、P2 七处清零、P3 采纳 5/6、P4-2/P4-3 已修——**判定主界面已达可发布状态**；剩余 4 项视觉细节（R1-R4）本轮全修，另附 P4-1 根因修复。

| # | 问题 | 修正 |
| --- | --- | --- |
| R1 | `Fit & fill` 残留于 `POS_NAMES[6]`（main.c:311）与说明行（main.c:2507），与上一轮 CHANGELOG 不符——两处是首轮替换的漏网 occurrence | `POS_NAMES[6]` → `Fill`（位置按钮与摘要 `%d · %s` 同源联动）；说明行 → `"Fill" = fill the window and crop the overflow` |
| R1 附带 | 说明行单行高度只够显示第一句，`Fit`/`Fill` 两档的语义解释用户看不到 | 说明标签行高 14→**30**（两行），下方布局整体顺移 16px |
| R2 | `random` 芯片掉尾字母（`rando`）：芯片宽 = 文本实测 + 内边距×2 + 2，实测差 1-2px | 余量 `+2` → `+4`（main.c:1962 new_chip，全部自动宽芯片受益） |
| R3 | 最小宽度 1020 下生效流程第 2 步说明被切（`...swapping the I`） | 文案缩短：`Only needed once, or after swapping the DLL` |
| R4 | 45 字符长文件名把 `GDI+ decode` 芯片顶出卡片右缘 33px（P4-2 改粗体测量后文件名不再收缩所致） | `preview_meta_update` 增加**文件名宽度上限**：超出时逐字符截断并补 `…`（按粗体实测宽度循环），三件套恒在卡片内 |
| P4-1 改判 | 专属背景页首开 `0 rules`：本轮给出根因（启动顺序 browse_plugin→refresh_rule_list 在 load_config 之前，首刷时 ini 未载入）及一行修复 | 采纳：`load_config()` 末尾补 `refresh_rule_list();`（与 refresh_image_list 同理），上一轮"维持不修"的结论就此改判 |

## EN-5 复核清单（第三轮验收用）

- [ ] 位置分段最后一档显示 `Fill`，摘要行显示 `posType · 6 · Fill`
- [ ] 说明标签两行完整：第一句等比缩放说明 + `"Fit"` / `"Fill"` 两档语义
- [ ] `random` 芯片完整显示（含尾字母 m）
- [ ] 生效流程第 2 步说明完整（`...swapping the DLL`）
- [ ] 选中 45 字符长文件名：预览元信息三件套完整在卡片内，文件名带 `…` 截断
- [ ] 全新启动直接进入专属背景页：规则列表直接显示已有条目（不再 0 rules）
- [ ] 回归：Save config / Register / Restart Explorer / Emergency off 弹窗正常

---

## EN-6 人类开发者第四轮实机反馈（2026-11-04，蓝/绿/黄框标注）

| # | 问题 | 根因 | 修正 |
| --- | --- | --- | --- |
| 🔵 | posType 说明行只显示第一句（`Always scaled...window live`），`Fit`/`Fill` 语义解释完全不可见 | `new_label_ex` 创建的静态控件用 `SS_LEFTNOWORDWRAP` 样式——**明确禁止换行**，文字加高也没用 | 该标签样式改为 `SS_LEFT`（允许换行），说明文字重组为带 `\n` 的两行：第一行等比缩放行为，第二行 `"Fit"` / `"Fill"` 两档语义（压缩到单行宽度以内） |
| 🟢 | Opacity 区读数 `101 / 255(75%)` 贴着卡片底被裁一半 | 布局是弹性间距系统（gapA/B/C 自动伸缩），但"已用高度"公式**没算上 EN-1 加高的两行说明**（+32px），间距没有如需收缩 | `used` 公式补入说明行高度（`+ S(2) + S(30)`），弹性间距自动收缩 16px——即"上方整体上移"，由布局引擎自动完成 |
| 🟡 | （开发者提示的备选方案）Image position 区整体上移 | — | 未单独使用：弹性间距收缩已腾出足够空间，无需硬挪 |

## EN-6 复核清单（第五轮验收用）

- [ ] posType 说明两行完整：第一行等比缩放行为；第二行 `"Fit" = whole image visible; "Fill" = fill and crop the overflow`
- [ ] Opacity 读数（如 `191 / 255(75%)`）完整显示在卡片内，不再被裁
- [ ] Opacity 与 Custom image folder 卡片之间间距正常，无重叠
- [ ] 回归：随机轮换子行、Emergency off、摘要卡、位置七档均正常

---

## 发布前最终代码复核（WorkBuddy / GLM，2026-11-04）

> 应仓库主人要求，发布前由 WorkBuddy 对英文版全部改动做代码级复核（不另派模型）。

**方法**：与中文母本（v1.3.28 基线）全量 diff 逐块核对 + 基线/当前双编译警告集对比 + 二进制终验。

**结果：**

| 检查项 | 结果 |
| --- | --- |
| 全量 diff | 5215 行 → 5240 行；232 个字符串改动块 + 12 个结构性改动块，逐块过目 |
| 格式占位符（%s/%d/%lu/%S/%.1f/%ld） | 新旧 232 块逐一比对，**零丢失零错位** |
| 结构性改动 | 10 处符合预期（宽度/位置常量、min 宽度、refresh_rule_list、R4 截断块、SS_LEFT 样式块、used 公式）；**2 处注释瑕疵**（见下） |
| 编译警告（-Wall 全开） | 基线（中文母本）0 条，英文版 **0 条**——零警告纪律保持 |
| 字符串字面量中文/全角标点（源码级） | **0**（权威判定） |
| 二进制 utf-16 2 字节模式计数 | 19 处 `（` 为**随机字节噪声**（1.27MB 二进制的期望值即 ~19），非真实内容；4 字符级中文模式 0 |
| zip 完整性 | exe 与最新构建 MD5 一致；config.ini 无私人路径；无运行时垃圾 |

**已知的两处注释瑕疵（零运行时影响，发布不阻塞）：**
1. main.c:2343 批量翻译误改一条历史注释（"说明文字横移"→"caption\n * slide"，两行注释合为一行）
2. main.c:2937 同类（"从预设档/无效值首次进入自定义"片段变英文）

> 成因：这两处是**注释里的引号片段**（跨行的引号对被字符串正则误认为字面量）。纯注释层面，编译与运行零影响；如介意可在下轮顺手改回中文。

**判定：无语法错误、无占位符丢失、无新警告、无中文 UI 残留——具备发布条件。**

## EN-7 智能列宽（人类开发者第五轮实机反馈，2026-11-04，绿框标注）

**背景**：中文版同轮增补（总卷第十七章）发现图片列表横向裁切问题后，人类开发者在英文版实机上确认同样存在——文件名短时大小列仍被右缘裁掉（`11.1 MB` 显示为 `11.1 ME`）。

- **根因**（与中文版一致）：`refresh_image_list()` 的固定列宽未计入垂直滚动条占用的客户区宽度，图片较多时列表出现横向滚动条，大小列被右缘裁切。
- **修正**：将中文版的智能列宽逻辑原样移植到英文源码 `refresh_image_list()` 尾部——大小列按内容自适应（保底 60px），名称列按最长文件名自适应并封顶为剩余宽度。文件名短 → 大小列完整显示、无横向滚动条；名称超长 → 名称列封顶、滚动条接管。
- **技术细节**：该函数位于 3227 行 `#undef S` 之后的卫生区，补丁使用 `pt_scaled()` 直调（与中文版增补二同款处理）。

**构建**：2026-11-04 17:19 `Build OK`（-Wall 零警告）；zip 与 `en-package` 目录 exe 同步更新（MD5 `01e9e6ce`）。

**EN-7 复核清单（第六轮验收用）**：
- [ ] Image manager 页：短文件名时大小列完整显示（`11.1 MB` 全字），无横向滚动条
- [ ] 长文件名（≥45 字符）时名称列封顶、横向滚动条出现、大小列仍完整
- [ ] 刷新列表 / 切换目录后列宽重新自适应正常
- [ ] 其余页面无回归（Basic config / Per-folder background）

## Final Dual-Version Pre-release Audit (WorkBuddy / GLM, 2026-11-04)

**Scope**: an independent final check on BOTH the Chinese and English builds after all fixes (EN-1~7 + CN addendum), immediately before publishing. The human developer set the acceptance bar: *"works correctly is enough -- no nitpicking"*, on the grounds that this GUI panel is not a resident system component. Accordingly this audit targets correctness and usability, not style.

### Check matrix (four layers)

| Layer | Check | Chinese build | English build |
|---|---|---|---|
| 1. Compile | fresh build with `-Wall` | `Build OK`, **0 warnings** | `Build OK`, **0 warnings** |
| 2. Source integrity | byte-level diff vs backup | identical to `cn-main-with-addendum.c` | archive contains the EN-7 smart-column patch (marker comment + `pt_scaled(60)` x4) |
| 3. Binary content | embedded-string spot checks | `图片张数` present, `应急禁用` present, legacy prefix `folderExt · 文件对话框` = 0 | `Emergency off` present, `Fill` present, Chinese UI string `保存配置` = 0 |
| 4. Isolation | cross-contamination | no English UI bleed | no Chinese UI residue |

### Technical notes

- **On MD5 mismatches**: two builds from identical source may differ in MD5 because the PE header embeds the build timestamp; sizes stay equal. Content equivalence was judged by string spot-checks and source diffs. The desktop-root `backimg图形板_v1.3.28.exe` was re-synced to the latest build.
- **Build-file self-consistency**: the audit caught that `resource.rc` (lang 0409) and `build.cmd` (English output name) in the working dir were still the English ones from the EN patch round; both were restored from the D: master (0804 + `backimg图形板` output name). The full English trio (main.c / resource.rc / build.cmd, with EN-1~7) is preserved in `backimg-gui-release\en-source-backup\` and will be used for the `english-ui` branch.
- **Embedded resources**: both exes embed the same `config.ini` (dev path cleared) and the untouched v1.3.28 `ExplorerBgTool.dll`, honoring the "DLL untouched" boundary.

### Verdict

**Both builds pass all four layers -- no code errors, zero warnings, no cross-contamination. Cleared for release.** Release plan: Chinese and English zips (`_english-ui` suffix) side by side on the GitHub Release, English sources pushed to the `english-ui` branch.

*Audit performed by WorkBuddy (GLM); acceptance criteria set by the human developer ("it just has to work").*
