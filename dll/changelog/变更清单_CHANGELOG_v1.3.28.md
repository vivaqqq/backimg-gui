# 变更清单 · v1.3.28（2026-10-29）

**主题：Deepseek 复审 v1.3.27 三修复（P1 真实 bug + P2/P3 整洁性与体验）**

- 性质：GUI 侧修复版；DLL 功能零改动，仅版本戳 1.3.27→1.3.28 联动升版
- 审查来源：用户外送《backimg_GUI_v1.3.27_代码复审报告.txt》（Deepseek，含上一轮 F1/F2/F3 修复核对）
- 审查核对结论：F1、F2 修复正确；F3 修复方向正确、主场景已达成，但引入问题 1（见下）

---

## 〇、审查结论核验（先核后修，写明属实性）

| 审查问题 | 核验过程与结论 | 处置 |
|---|---|---|
| **问题 1**：`g_rot_custom_picked` 未在 `load_config` 重置 | **属实**。全文件仅 2 处赋值（菜单选"自定义…"置 true / 选预设置 false），`load_config` 的轮换读取块（randomVal/randomUnit/randomMin 兼容路径）末尾直接 `sync_switch_ui() → rot_ui_sync()`，无任何重置。触发路径：用户选过自定义 → 底部「恢复配置」（on_restore → load_config）恢复出预设档备份 → `custom` 仍为 true → 界面错误显示"自定义…"档+输入框，与实际配置不符 | **P1 修复** |
| **问题 2**：`hRotLabel` 成为"赋值未读"死变量 | **属实**。全文件仅 2337 行声明 + 2825 行赋值，无任何读取（v1.3.26 删"说明文字横移"后即失效）。审查同时指出 gcc -Wall 对文件级 static 只写不读默认不报——与实测一致，不影响"零警告" | **P2 修复** |
| **问题 3**：自定义态已填 120 后重选「自定义…」仍被重置成 180 | **属实**（既有逻辑，v1.3.25 起 `preset` 重置就有；非本轮新引入，审查也只标"仅提示"） | **P3 修复** |

---

## 一、GUI 侧修复（main.c，共 3 处）

### P1：load_config 读取轮换配置后重置自定义态标志

```c
/* load_config() 轮换块末尾（钳位之后）新增： */
g_rot_custom_picked = false;
```

- 语义：该标志只表达"本次会话内用户主动选过自定义"，**配置一变即失效**——
  恢复出预设档（如 randomVal=60）时界面必须如实回到"每 1 分钟"档
- 触发面：底部「恢复配置」（.bak 恢复）与任何 load_config 调用路径；
  右卡「恢复原配置」走 on_restore_prev 不经 load_config，本来也不动轮换配置，不受影响

### P2：hRotLabel 死变量清除

- 删除文件级 `static HWND hRotLabel;`，`rot_ui_init` 里 `new_label_ex` 返回值直接丢弃
- 至此轮换 UI 的文件级状态只剩：两个子行控件句柄（hRotSubLabel/hRotSubHint）+
  自定义态标志（g_rot_custom_picked）+ 档位/单位 ComboState，无冗余

### P3：已在自定义态时重选「自定义…」不重置已填值

```c
bool already_custom = g_rot_custom_picked;   /* 先取旧值再置位 */
g_rot_custom_picked = true;
if (!already_custom) {
    /* 仅"从预设档/无效值首次进入自定义"才给 180 非预设起点 */
    ...
    if (preset || g_app.v_rotsec <= 0) { g_app.v_rotsec = 180; g_app.v_rotunit = ROT_UNIT_MIN; }
}
```

- 用户手敲的 120（恰等于"每 2 分钟"）在重开下拉再选一次「自定义…」后**保持 120 不动**
- 注意实现顺序：必须先取 `already_custom` 再置位，否则标志先变 true、判断永远失效

## 二、DLL 侧（版本联动，功能零改动）

- 内部日志版本戳 `ExplorerBgTool v1.3.27` → `v1.3.28`
- 头部注释新增 v1.3.28 条目（三修复摘要 + 与 v1.3.27 可直接互换说明）
- 配置键（randomVal/randomUnit/randomMin 兼容）、协议、定时器、目录自愈行为**与 v1.3.27 完全一致**

## 三、验证

- DLL：g++ 编译零错误零新警告，**942,255 字节**（与 v1.3.25~27 同尺寸，仅字符串戳差异），
  SHA256 `7846DEBD35F5F259A75BC45A047B13DB8B8360C1777C7CB47572EAEE183C18E7`
- 字节特征校验：DLL 内 UTF-16 串含 `v1.3.28`、不含 `v1.3.27` ✓；plugin 与 build 产物 SHA256 一致 ✓
- EXE：gcc -Wall 零错误零警告，**1,261,161 字节**，版本资源 1.3.28.0，
  SHA256 `B8EFD1936B0EB430A6859BCAFACAC07925996135603CAFA96EC4052047242DC5`
- 复查确认：P1 修复后「恢复配置」路径 rot_ui_sync 判定与恢复的配置值一致；
  P3 修复后菜单打勾逻辑（on_rot_menu）与面板显示（rot_ui_sync）使用同一套
  `g_rot_custom_picked || 数值反推` 判定，两处不再打架

## 四、上一轮（v1.3.27）修复核对存档

- 【F1】子行标题随随机开关显隐 —— 核对正确，维持现状
- 【F2】五个位置全局降为局部 —— 核对正确，本轮 P2 顺带补掉漏网的 hRotLabel
- 【F3】自定义态独立状态位 —— 主场景正确，本轮 P1/P3 补齐其两个边角
