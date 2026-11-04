# 变更清单 · v1.3.27（Deepseek V4 Pro 审查修复版）

- **日期**：2026-10-29
- **性质**：用户外送 Deepseek V4 Pro 审查 v1.3.26 全量改动，报告三问题逐条核验后修复
- **上一版**：v1.3.26（轮换自定义控件挪子行版）
- **审查报告**：`（本地路径已略）backimg_GUI_v1.3.26_代码审查报告.txt`（Deepseek V4 Pro）

## 〇、审查结论核验（先说属实性，再谈修法）

| 审查问题 | 核验结论 | 处置 |
|---|---|---|
| 问题 1：子行标题"自定义间隔"不随随机开关显隐 | **属实**——hRotSubLabel 创建后无任何 ShowWindow 控制，随机关时控件/提示全隐、只剩一行孤立标题 | **F1 修复** |
| 问题 2：五个全局变量赋值后不再读取 | **部分属实**——g_rot_cx、g_rot_sub_y 确为死变量（赋值无读者）；但 g_rot_label_y / g_rot_edit_x / g_rot_unit_x 在 rot_ui_init 内有读取。另"会触发 gcc -Wall 的 -Wunused-but-set-variable"说法不准——该警告只管局部变量，文件级 static 赋值即算引用，实际无警告 | **F2 清理**（五个全部降为 rot_ui_init 局部变量，比审查建议更彻底） |
| 问题 3：自定义值恰等于预设档会跳回预设 | **属实**（v1.3.25 起既有）——纯靠数值反推自定义态，手敲 120 恰命中"每 2 分钟"时输入框当场消失。审查也指出输入框加宽后更易踩到 | **F3 修复**（按审查建议加独立状态位） |

## 一、GUI 侧（backimg 图形板 v1.3.26 → v1.3.27）

### F1 子行标题随随机开关显隐（审查问题 1）

- rot_ui_sync 补 `ShowWindow(hRotSubLabel, v_random ? SW_SHOW : SW_HIDE)`
- 随机关时子行整体无内容：标题/输入框/单位框/提示全部隐藏，不再留孤立文字

### F2 死变量清理（审查问题 2）

- v1.3.25 遗留的五个位置全局 g_rot_cx / g_rot_label_y / g_rot_sub_y /
  g_rot_edit_x / g_rot_unit_x，在 v1.3.26 删除"说明文字横移"逻辑后
  已无跨函数读者（或有读者也仅限 rot_ui_init 内部）——**全部降为
  rot_ui_init 局部变量**，全局只留 hRotSubLabel / hRotSubHint 两个控件
  句柄 + g_rot_custom_picked 标志
- 重编 gcc -Wall 实测零警告，与 CHANGELOG 宣称一致

### F3 自定义态改用独立状态位（审查问题 3）

- 新增 `static bool g_rot_custom_picked`：
  - 用户在档位菜单点「自定义…」→ 置 true；点任意预设档 → 置 false
  - rot_ui_sync 判定改为 `custom = g_rot_custom_picked || (cur < 0 && v_rotsec > 0)`；
    置位时档位下拉恒显示"自定义…"，输入框保持可见
  - 启动加载时标志为 false，纯按数值反推显示（配置是预设就如实显示预设档）
- **效果**：自定义态手敲 60/120/300 等恰好等于预设秒数时，输入框不再当场
  消失、档位不再跳回预设；保存值不变（数值语义与预设完全等价，只是显示不跳）
- on_rot_menu 菜单打勾逻辑同步改为同一套判定（此前菜单里也会把 120 勾在
  "每 2 分钟"上，与面板显示不一致）
- 保存路径核验无需改动：save 从可见输入框取值，F3 后自定义态输入框恒可见，自洽

## 二、DLL 侧（ExplorerBgTool.dll v1.3.26 → v1.3.27）

- **功能零改动**，仅初始化日志首行版本戳 1.3.26 → 1.3.27 随外壳联动
- 配置键 / 协议 / 定时器行为与 v1.3.26 完全一致，两版 DLL 可直接互换
- 文件大小不变：942,255 字节

## 三、产物校验

- ExplorerBgTool.dll：**942,255 字节**，SHA256
  `85E19DF9FD4F36FB9E28EAB7A83B4415B7B53AB4C375CB41E58D2CAEBDE2F86B`（前 32 位 `85E19DF9FD4F36FB9E28EAB7A83B4415`）
- backimg图形板_v1.3.27.exe：**1,261,161 字节**，gcc -Wall 零错误零警告
- plugin\ 与 build\ 的 DLL 字节一致；外壳内嵌资源与 plugin 同源

## 四、升级方法

1. `backimg图形板_v1.3.27.exe`、`plugin\ExplorerBgTool.dll` 两个文件成对覆盖旧版
2. 打开面板 → 保存配置 → 重启资源管理器（或点面板上的「重启资源管理器」）
3. 验证：运行日志首行 `ExplorerBgTool v1.3.27`；标题栏 `v1.3.27 (配套 DLL v1.3.27)`

---

*v1.3.27 变更清单 ｜ 2026-10-29 ｜ Deepseek V4 Pro 审查三问题：F1/F2/F3 全修*
