# 变更清单 CHANGELOG v1.3.29（后台预热 + 轻量扫描 + 按需解码版 · R2）

- **日期**：2026-11-05/06
- **性质**：逻辑级更新（首次打开卡顿根治 + 内存与 I/O 釜底抽薪），DLL 与 GUI 同步升版
- **需求来源**：人类开发者实机反馈 —— 148 张大图（单张 10~16MB）、每 10 秒随机轮换配置下，系统启动后**第一次**打开任意资源管理器窗口卡顿 1~2 秒
- **方案演进**：首稿（已作废，存档于 `backimg_GUI_v1.3.29-作废方案\`）采用"后台线程 + 并行探测"只把工作搬走未减少；本版按 **Deepseek V4.1 Flash 三层优化建议**（2026-11-06 报告）+ **WorkBuddy（GLM）复核修订**（补 PNG IEND 放宽、判死跳过机制）重构
- **实现**：WorkBuddy（GLM）；决策与验收：人类开发者

---

## 一、根因定位

1. **扫描在界面线程且为全量读入**：BHO 构造链 `CObjectWithSite ctor → OnWindowLoad → LoadSettings` 在 explorer 界面线程同步完成"枚举 148 文件（8 种后缀）→ **逐文件全量读入**（`BitmapGDI` 构造函数 `m_raw` 常驻设计）→ **全量 FNV-1a 哈希** → 头部探测"，约 **1.85 GB 磁盘读 + 1.85 GB 哈希 + 1.85 GB 常驻内存**。
2. **开机竞态双扫描**：explorer 启动即实例化 BHO#1 开始扫描；用户很快打开窗口时 BHO#2 的"文件集合未变早退"检查因首个列表尚未换表而失效 → 界面线程**再扫一遍**并与 BHO#1 并发争抢 I/O。
3. **首帧解码缩放**：首个窗口绘制时每线程缩放缓存未命中，同步执行单张解码 + 双三次缩放（0.3~0.8s，必要成本，任何方案都省不掉）。

**图片数量不是主因**：148 只影响枚举；单张体量决定读入+哈希成本。

## 二、改动明细（三层）

### L1 扫描期只读文件头（WinAPI.cpp 构造函数重写 + ImageHeader.h）

- `BitmapGDI` 构造函数不再全量读入：只读**前 256KB**（`HEAD_MAX`，取 `min(文件大小, 256KB)`），用现有 `ProbeImageHeader` 取宽高。扫描 I/O 从 ~1.85GB 降到 **~38MB**（约 50 倍）。
- **headOnly 模式**（本版关键，漏掉即优化完全失效）：
  - **JPEG**（ImageHeader.h ProbeJpeg）：跳过缓冲区末尾 EOI 检查（头部模式下末尾必为中段数据，检查必然失手 → 全部 JPEG 路由 GDI+ 裁决）；marker 段越过缓冲区时由"确证截断 SUSPECT"降级为 UNKNOWN（交 GDI+）。
  - **PNG**（ProbePng，WorkBuddy 复核补充——原报告只提了 JPEG）：**IHDR 校验（含 CRC）通过即判 IMG_OK 立即返回**，不再向下走块走查——部分缓冲上"块超出缓冲区"会被误判为截断 SUSPECT，**大于 256KB 的 PNG 会被当场错杀**，比 JPEG 的路由退化更严重。
  - 小于 256KB 的文件整文件在缓冲区内，**保持 v1.3.28 严格语义**（EOI/IEND 检查照常）。
  - headOnly 判定：`headLen < total`（实读不足全文件）。
- **IMG_UNKNOWN 兜底改惰性**：原 `SHCreateMemStream(m_raw)` 整文件喂 GDI+，改 `Bitmap::FromFile(path)` —— 惰性解析，取宽高只读文件头，随即 delete（句柄关闭），不整文件进内存。

### L2 缓存键轻量化（WinAPI.h/.cpp）

- 缓存键从"全量内容 FNV-1a"改为 **"头部字节哈希 ^ (文件大小 × FNV 素数)"**。
- 同文件重建列表键不变（源位图/缩放缓存照常命中，v1.3.17 P0-2 修泄漏的机制保留）；内容被替换时头部或大小几乎必然变化（键自然失效）。
- 哈希成本从 GB 级降到 256KB 级。

### L3 解码按需（WinAPI.h/.cpp）

- **删除常驻 `m_raw` 成员**（148 张 × 10~16MB ≈ 1.85GB 常驻内存清零）；`BitmapGDI` 只保留 `m_path`、`Size`、`m_hash`、`m_valid`、`m_dead`。
- `ScaleCache::GetSourceLocked` 签名 `(hash, w, h, raw)` → `(hash, w, h, path, bool* decodeFailed)`；解码从 `SHCreateMemStream(m_raw)+FromStream` 改为 **`Bitmap::FromFile(path)`**（惰性，`delete` 后句柄关闭，不长期锁文件）。
- `GetScaled` 调用点同步更新；新增 **decodeFailed 出参**区分失败类型。

### 判死机制（本版新增，R2 复核补充）

- 文件级解码失败（`FromFile` 失败 / `GetLastStatus != Ok`）→ `m_dead` 置位：
  - **轮换抽签避死**：`RotateTimerProc` 抽签最多重试 4 次跳过死位（全死则按原值退化）；
  - **绘制跳过**：绘制路径取到死图本帧不画背景（pBgBmp reset）；
  - **GetScaled 判死早退**：死图直接返回空，不再反复读盘解码。
- 动机：L1 后截断/损坏文件（头部 OK 但内容损坏）会进入列表，无此机制会以 1/N 概率周期性让窗口闪空白。

### D1 后台预热（保留自首稿，dllmain.cpp）

- `InjectionEntryPoint`（DLL 加载即达，先于任何 BHO）创建常驻预热线程：`BELOW_NORMAL` 优先级 + 开头 `Sleep(200)` 让路 DllMain 返回（规避 loader lock 期 GDI+ LoadLibrary 边缘）。
- 复用 `call_once(s_initOnce, InitBackground)` 初始化协议；随后 `LoadSettings(true)` 完成扫描建表。
- `LoadSettings` 图像段改 **try_lock**：预热扫描中窗口线程零等待返回（标量段全原子无锁，v1.3.19 P2-2 已原子化）；重建互斥保留（同一时刻至多一线程重建）。
- R2 后预热的工作量已降至 38MB/约 30~60ms——保留理由：慢盘（200~500ms）与开机争抢场景仍值得离线做。

### D3 重建完成即整体重绘（保留自首稿）

- 列表换表后 `InvalidateRect` 全体已登记窗口（先例：`LoadRetryTimerProc` 自愈路径）。覆盖：a) 开机早期窗口背景浮现；b) 增删图片其它窗口立即生效。

### 移除

- 首稿 D2 并行探测整块作废（38MB 串行 30ms，无需线程；`<thread>` 等随之清场）。

### 版本联动

- 初始化日志头 `ExplorerBgTool v1.3.29 (backimg 后台预热+轻量扫描版)`；`GetTickCount64` 手工 extern 声明（MinGW 头按 `_WIN32_WINNT` 门控）。
- GUI：GUI_VERSION/DLL_VERSION → 1.3.29、resource.rc 版本资源同步、内嵌 DLL 更换为新版（SHA256 成对校验机制不变）。
- 探测耗时入日志：`探测 : N 个文件头，耗时 X ms（串行·头部≤256KB）`。

## 三、行为对照

| 指标 | v1.3.28 | v1.3.29（本版） |
|---|---|---|
| 启动扫描磁盘读 | ~1.85 GB | **~38 MB** |
| 启动扫描哈希 | 1.85 GB 字节流 | **38 MB** |
| 扫描耗时（SSD/机械盘） | 1~2s+（界面线程，开机竞态双倍） | 30~60ms / 200~500ms（后台线程） |
| explorer 常驻内存 | **+1.85 GB** | **≈ 0**（仅剩 32/64MB 解码缓存预算） |
| 首帧解码缩放 | 0.3~0.8s | 0.3~0.8s + 首次 12MB 读盘（约 +30~80ms，通常命中系统缓存） |
| 界面线程阻塞 | 首窗 1~2s | **无**（扫描在后台；首帧绘制本身除外） |
| 坏文件处理 | 扫描期剔除 | 头部确证损坏仍剔除；截断类由首绘失败判死跳过 |

## 四、语义变化（均为改善，审计请知悉）

1. **同名文件内容替换即时生效**：旧版需重启 explorer（解码源自扫描期常驻字节）；现解码按需读盘 → 下次轮换即显示新图。同名同大小同 mtime 的替换仍会沿用旧缓存（现实中几乎不存在）。
2. **同内容不同文件名不再共享缓存条目**：键从内容哈希改为指纹（路径不再参与键）。实际影响可忽略（每线程预算 32/64MB 本就装不下第二份大图）。
3. **截断类损坏文件的暴露时点后移**：从"扫描期剔除"变为"首次绘制失败判死"。单文件最多浪费一次绘制尝试，其后被跳过。

## 五、红线核验（逐项）

- [x] 配置键/协议/定时器行为与 v1.3.28 完全一致（无新增键）
- [x] 随机列表顺序确定性不变（串行探测按枚举顺序）
- [x] 非随机模式行为不变（串行早停路径原样）
- [x] 绘制质量不变（PARGB + 高质量双三次插值路径零改动）
- [x] COM 导出表不变（objdump 比对 4 导出）
- [x] use-after-free 防线不降级：换表 shared_ptr 凭证机制原样；L3 的 `Bitmap::FromFile` 在 GdiplusMutex 内、decoded 位图先于缓存条目销毁（与旧 FromStream 生命周期语义一致，且无"流必须活到位图销毁"的负担）
- [x] 锁序无环：s_loadMx → {g_listMx | g_wndDataMx | GdiplusMutex} 全为叶获取
- [x] 异常边界：预热线程整体 try/catch；ctor/LoadSettings 异常路径与 v1.3.28 一致
- [x] `-Wall` 编译零警告（仅历史 `#pragma comment` MinGW 提示）
- [x] 256MB 单文件上限、512MB 单源位图上限、256MB 目标位图上限等防御全部保留

## 六、验证记录（2026-11-06）

- MinGW-w64 g++ 编译 `Build OK`（-std=c++17 -O2 -Wall 零警告）
- 二进制：`v1.3.29` 戳嵌入、`v1.3.28` 戳零残留、`轻量扫描版`/`预热(D1)`/`串行·头部≤256KB` 日志串嵌入、`并行探测版` 零残留
- 导出表 objdump 比对：4 导出一致
- 成对校验：外壳内嵌 DLL = `plugin\ExplorerBgTool.dll`（同源构建，SHA256 一致）
- 交付物：`backimg图形板_v1.3.29.exe`（1,261,161 B）+ `plugin\ExplorerBgTool.dll`（941,986 B）
- 对照 diff：`DLL源码\diff_v1.3.28_to_v1.3.29.patch`（33 个改动块，覆盖 ImageHeader.h / WinAPI.h / WinAPI.cpp / dllmain.cpp）

## 七、复核清单（实机验收 / 审计用）

- [ ] 重启 explorer → 等 5~10 秒 → 开文件夹窗口：无可感知停顿；背景若短暂缺席 1~2 秒内自动浮现（D3）
- [ ] `plugin\ExplorerBgTool_init.log` 应含：`预热(D1) : 预热完成，总耗时 X ms`、`探测 : 148 个文件头，耗时 Y ms（串行·头部≤256KB）`——Y 应 < 100ms（SSD）
- [ ] 任务管理器：开 10 个文件夹窗口后 explorer.exe 工作集增量 < 150MB（旧版为 +1.8GB 量级）
- [ ] 10 秒轮换连续 5 分钟：切换正常、无卡顿、内存不持续增长
- [ ] 替换某张同名图片内容 → 下次轮换显示新图（新行为验证）
- [ ] 故意放一张截断的 JPG 进目录 → 该图被跳过（判死）、其余轮换正常、日志无崩溃
- [ ] 非随机模式（random=false）回归正常
- [ ] 代码审计重点：`ImageHeader.h` 的 headOnly 两个探测器、`WinAPI.cpp` 构造函数与 `GetSourceLocked`、`dllmain.cpp` 的 try_lock/D3/判死三处

---

*本变更由 WorkBuddy（GLM）实现；三层方案出自 Deepseek V4.1 Flash（2026-11-06 报告，其 JPEG EOI 陷阱预警被证实为关键）；PNG IEND 同款陷阱与判死机制由 WorkBuddy 复核补充；根因定位、方案取舍与验收标准由人类开发者主导。历史版本见《版本迭代报告_总卷.md》。*


---

## 修订 R3（2026-11-06，Deepseek V4.1 Flash bug 审计修复）

审计报告：《backimg_v1.3.29_复核报告_2026-11-06》（含实测：截断图经 GDI+ FromFile/DrawImage 均返回 **Ok** 并渲染"上半正常 + 下半黑边"，证据图 backimg_v1.3.29_证据-截断图GDI渲染.png）。本轮按人类开发者指定的四条护栏执行。

### Bug 确认（审计结论，源码核实属实）

- **Q4 判死机制挂在永不带电的线上**：截断图 GDI+ 返回 Ok → decodeFailed 不置位 → m_dead 不触发 → 坏图长期留在轮换（实测复现，P1）
- **同名换不同尺寸图拉伸变形**：s_lastFiles 只比文件名，DrawImage 以旧 Size 为源矩形（P1）
- **m_dead 无复活**：瞬时故障（同步盘写入/杀软独占）= 本次会话永久消失（P2）
- **regsvr32 也起预热线程**：靠 Sleep(200) 时序侥幸避开 FreeLibrary 竞态（P2）
- 审计同时确认：L1/L2/L3 全部落地有效；PNG IEND 补充被点名肯定

### 修复清单

| # | 修复 | 落点 |
|---|---|---|
| S1（P1） | 扫描期尾部终止符检查：部分读入的大文件，JPEG 验尾部 2 字节 FF D9、PNG 验尾部 12 字节 IEND 块；未命中再搜尾部 64KB（容忍尾部追加）；仍未找到 → 确证截断不进列表。读尾失败按本轮拒绝，下轮自愈。范围仅 JPEG/PNG（BMP/GIF/TIFF 无可靠终止符，记录为已知边界） | WinAPI.cpp 构造函数 |
| S2（P1） | 列表重建条件升级为"路径 + 大小 + 修改时间"三元组（FileSig），同名换图（同步盘常见）触发重建，消除拉伸变形 | dllmain.cpp LoadSettings |
| S3（P2） | 判死带 5 分钟 TTL 自动复活（m_deadSince + IsDead 到期清零），瞬时故障只短暂缺席 | WinAPI.h/.cpp |
| S4（P2） | 预热线程仅 explorer 启动（InjectionEntryPoint(bool isExplorer)），regsvr32 注册路径不再起线程 | dllmain.cpp |

### 四问自查（按人类开发者护栏）

1. **是否掩盖前置 Bug**：S1 把"坏文件防线"修回文件结构层（审计 Q4 指定的正确位置），不依赖 GDI+ 状态码、不转移问题；S4 把"时序侥幸"变成"结构上不可能"。
2. **是否引入连锁隐性 Bug**：S1 尾部搜索安全性有据——JPEG 熵编码有字节填充（图像数据不含 FF D9）、PNG IEND 8 字节签名误配概率 2^-64；读尾失败按本轮拒绝，由既有 LoadSettings 自愈机制兜底；S2 属性读取失败按 {0,0} 记名触发重建（自愈方向正确）；S4 对 regsvr32 行为仅"少起一个线程"。
3. **是否陷入修复循环**：本轮全部为增量修复 + 一处签名带参，未触动 L1/L2/L3 主体与 D1/D3；无回滚无重写。
4. **是否在错误链路迭代**：无——S1 明确落在审计 Q4 指定的"文件结构完整性"链路，绘制路径零改动。

### 实证验证（复刻 Deepseek 方法论）

独立编译本版 ImageHeader.h + 复刻构造函数扫描逻辑（s1test.cpp），对 4000×3000 噪声图全量/截断（60%）样本验证：

    big.png         probe=1  --> [OK] in list
    big.jpg         probe=1  --> [OK] in list
    trunc_big.png   probe=1  --> [S1-REJECTED] not in list
    trunc_big.jpg   probe=1  --> [S1-REJECTED] not in list

截断样本在 headOnly 探测下确实放行（复现审计发现），S1 全部拦截。测试代码与样本存档于 backimg-gui-release\s1-test\。

### 明确不修（本轮）

- ProbeHead/ProbeFull 拆分（审计 P3，下一版重构时做）
- D3 重绘 erase=FALSE（P3；与 FillRect 钩子有交互风险，收益不抵）
- BMP/GIF/TIFF 截断检查（无可靠终止符；记录为已知边界，JPEG/PNG 已覆盖主流格式）


---

## 修订 R4（2026-11-06，Deepseek V4.1 Flash R3 复核遗留项修复）

审计报告：《backimg_v1.3.29_R3复核报告_2026-11-06》——**S1~S4 全部确认落地正确，六组样本实测通过，判定"收敛、可发布"**；遗留 3 条 P3 边界问题，本轮全部处理（R3-1/R3-2 修复 + R3-3 顺手完成）。

### 遗留项确认（源码逐条验证属实）

- **R3-1 小 JPEG 截断漏网（P3）**：S1 外层条件 `headLen < total` 把小文件整体排除在尾检之外；<256KB 的截断 JPEG 走 `probe=UNKNOWN → GDI+ 裁决 Ok → 跳过 S1` 进列表（黑边渲染）。PNG 不受影响（headOnly=false 严格走查直接 SUSPECT）。
- **R3-2 读尾失败误判（P3）**：`f2` 打开失败与慢路径 seek/fread 失败都让 `terminated` 保持 false，与"确认无终止符"共用判死分支；而 `s_lastSigs` 已含该文件 → 下轮 LoadSettings 早退 → **无声缺席持续到 explorer 重启或文件变动**，"下轮自愈"前提不成立。
- **R3-3 二次开档（P3，性能）**：头读一次 `_wfopen_s` + 尾读再一次，网络盘上 148 文件 = 148 次额外往返（0.3~1.5 s）。

### 修复清单

| # | 修复 | 落点 |
|---|---|---|
| R3-1 | S1 覆盖小文件：条件放宽为 `m_valid && total > 12 && (isJpeg\|\|isPng)`；小文件整份已在 head 缓冲，**零 I/O 直接在缓冲上判**（快路径查末尾 fastLen 字节，慢路径全缓冲搜索——搜索必然完成，结论只有 FOUND/ABSENT） | WinAPI.cpp 构造函数 |
| R3-2 | `terminated` 二值 → **三态枚举**（TERM_FOUND / TERM_ABSENT / TERM_UNKNOWN）：只有"慢路径搜索完成"才能给出确定性结论；任何读失败 → UNKNOWN → **保持 m_valid 放行**，交给绘制期解码与 S3 TTL 兜底。放行的"可见症状（黑边）"比"无声缺席"可诊断 | 同上 |
| R3-3 | **单句柄复用**：头读后不再 fclose，大文件尾检 seek 同句柄读，函数尾统一 fclose；`rd != headLen` 分支补 fclose。消除二次开档 | 同上 |

### 四问护栏自查（按人类开发者指定原则）

1. **是否掩盖前置 Bug**——否。R3-1 是扩大证据覆盖面（小文件纳入同一规则）；R3-2 是修正错误分类（"没读到"≠"文件坏了"），且放行后的症状可见、可诊断，不是把问题藏起来；R3-3 是结构优化（少一次开档），语义等价。
2. **是否产生新的连锁隐性 Bug**——已排查：单句柄为共享读模式（"rb"）不排他，不影响 GDI+ `FromFile` 的独立打开；fseek 越过 EOF 时 fread 失败 → UNKNOWN → 安全回落绘制期（GDI+ 读到的是真实当前文件）；UNKNOWN 放行的极端场景（头读成功、尾读失败的微秒窗口）比 ABSENT 误杀"追加型合法文件"的代价更可接受；小文件缓冲路径无任何 I/O，无新增失败模式。
3. **是否陷入修复循环**——否。纯增量：一处条件放宽 + 一个枚举三态 + 句柄生命周期调整，L1/L2/L3 主体、绘制路径、缓存预算零改动，无回滚无重写。
4. **是否在错误链路上迭代**——否。仍在审计 Q4 指定的"文件结构完整性"链（扫描期）；`DrawImage` 等绘制路径一行未动。

### 实证验证（复刻审计方法论，十样本）

独立编译本版 `ImageHeader.h` + 复刻 R4 三态判定（`s1test_r4.cpp`），4000×3000 噪声图与 320×240 小图：

```
big.png            36,050,144 B  probe=OK                          [ACCEPT]
big.jpg            16,776,517 B  probe=OK                          [ACCEPT]
trail_big.png      36,051,168 B  probe=OK                          [ACCEPT]  ← 尾部追加容忍
trail_big.jpg      16,777,541 B  probe=OK                          [ACCEPT]  ← 尾部追加容忍
trunc_big.png      19,827,579 B  probe=OK                          [REJECT]  ← 大文件截断拦截（无回归）
trunc_big.jpg       9,227,084 B  probe=OK                          [REJECT]  ← 大文件截断拦截（无回归）
small.png             230,924 B  probe=OK                          [ACCEPT]  ← 小文件正常放行
small.jpg             108,112 B  probe=OK                          [ACCEPT]  ← 小文件正常放行
trunc_small.png       127,008 B  probe=SUSPECT                     [REJECT]  ← PNG 严格走查（对照）
trunc_small.jpg        59,461 B  probe=UNKNOWN→GDI+(Ok,per audit)  [REJECT]  ← R3-1 修复实证（R3 版此处 ACCEPT）
```

关键行：`trunc_small.jpg` 在 R3 版经 `UNKNOWN → GDI+ Ok → headLen==total 跳过 S1` 漏进列表；R4 版走小文件缓冲判定 → ABSENT → 拒绝。审计报告第五节"改完 R3-1、R3-2，功能设计即完整"的目标达成：**扫描期只读 38MB + 尾部 2~12 字节，坏文件挡在列表外，瞬时故障能自愈，永久损坏有 TTL 兜底**。

### 构建

MinGW `-Wall` 零警告；DLL 944,724 B；外壳内嵌同步成对同源；exe 1,263,721 B；对照补丁 `diff_v1.3.28_to_v1.3.29.patch` 35 块。测试代码与样本存档 `backimg-gui-release\s1-test\`（gen_samples.py / s1test_r4.cpp / r4_result.txt）。


---

## 修订 R5（2026-11-06，Deepseek V4.1 Flash R4 复核遗留收口）

审计报告：《backimg_v1.3.29_R4复核报告_2026-11-06》——**R3-1/R3-2/R3-3 全部确认落地正确，判定"可以发布"**；发现残留边界 R4-1（小文件 + 内嵌缩略图）与一条结构建议（抽取 VerifyTail 函数）。本轮按审计原话"如果这一版还要再动一次构造函数，建议把 R4-1 和 VerifyTail() 抽取一起做掉"一次完成。

### R4-1 确认（先实证复现，不轻信）

按报告方法构造样本（SOI 后插入 APP1 段内含 3371B 完整 JPEG 缩略图，截断 55%）：`thumb_trunc_small.jpg`（61KB）在 R4 判定下 **[ACCEPT]**——前向搜索从文件头扫起，首个命中的 `FF D9` 是缩略图的 EOI（offset 3389），带 EXIF 缩略图的小 JPEG 截断与否一律放行，尾检对该类文件形同虚设。**审计论断实证属实**（GDI+ 对该截断图返回 Ok，解码期兜不住，与审计实测一致）。

### 修复

| # | 修复 | 说明 |
|---|---|---|
| R4-1 | **反向搜索 + 距离阈值**：从尾部反向找最后一处终止符，距尾 <= 4KB 才判 FOUND（保留 v1.3.19"容忍尾部少量追加"语义）；窗口内无终止符或距尾超阈 → ABSENT | 反向首个命中即主图真 EOI，天然免疫缩略图干扰；"距尾 4KB 内无终止符 ⇒ 最后一处距尾 > 4KB 或不存在"，两种情况同判 ABSENT，无需全文件搜索 |
| 结构建议 | **抽取 `VerifyTailOnBuf()` 纯函数**（无 I/O）：R3~R4 累计 ~110 行 / 6 层嵌套的构造函数内联尾检整体移出，构造函数只留两处调用（大文件句柄读窗口 / 小文件缓冲直取） | 打破"第 5 次回到同一段代码"的修改循环；纯函数可独立单测，实现与测试程序剥除注释后逐字符一致 |

### 四问护栏自查（按人类开发者指定原则）

1. **是否掩盖前置 Bug**——否。反向搜索修正的是"实现不完整"（审计 Q1 原话：R4 的尾检对带缩略图文件实际不生效），修正后判据真实生效；不是绕过或转移。
2. **是否产生新的连锁隐性 Bug**——已逐场景推演：EXIF 正常文件（反向先命中主图真 EOI → FOUND）✓；尾部追加 1KB（距尾 1KB <= 4KB → FOUND，容忍语义保留）✓；无缩略图截断（JPEG 字节填充保证图像数据无 FF D9 → ABSENT）✓；PNG IEND 数据流误配（概率 2⁻⁶⁴ 且须落在距尾 4KB 窗口内）✓；截断点恰在缩略图 EOI 后 <4KB 的极端 case → 文件本身是可被 GDI+ 完整解码的合法 JPEG 流（画出缩略图内容而非黑边），行为可接受。唯一收紧：尾部追加 > 4KB 的合法文件将由容忍转为拒绝（此前 64KB 窗口）——症状为图缺席、重新保存即恢复，可诊断可恢复，记为已知边界。
3. **是否陷入修复循环**——否，且主动收敛：本轮以"抽取纯函数"替换"第 5 次内联修改"，改动后构造函数内尾检逻辑从 ~110 行缩至 ~40 行调用与注释；净代码量 −696 字节。
4. **是否在错误链路上迭代**——否。仍在扫描期文件结构完整性链；绘制路径、缓存、列表逻辑零改动。

### 实证验证（R4 vs R5 双判定对照，十四样本）

`s1test_r5.cpp`：R4 旧判定（前向搜索）与 R5 新判定（`VerifyTailOnBuf`）并排实现，对全部样本双跑：

```
sample                     size       probe-route                       R4         R5
big.png / big.jpg          36MB/16MB  probe=OK                          [ACCEPT]   [ACCEPT]
trail_big.png / jpg        +1KB追加   probe=OK                          [ACCEPT]   [ACCEPT]
trunc_big.png / jpg        55%截断    probe=OK                          [REJECT]   [REJECT]
small.png / small.jpg      231KB/108KB probe=OK                         [ACCEPT]   [ACCEPT]
trunc_small.png            55%截断    probe=SUSPECT                     [REJECT]   [REJECT]
trunc_small.jpg            55%截断    UNKNOWN->GDI+(Ok,per audit)       [REJECT]   [REJECT]
thumb_small.jpg            带EXIF完整 probe=OK                          [ACCEPT]   [ACCEPT]
thumb_trunc_small.jpg      带EXIF截断 UNKNOWN->GDI+(Ok,per audit)       [ACCEPT]❌ [REJECT]✅ ← R4-1 复现+修复
thumb_big.jpg              带EXIF完整 probe=OK                          [ACCEPT]   [ACCEPT]
thumb_trunc_big.jpg        带EXIF截断 probe=OK                          [REJECT]   [REJECT]
```

唯一分歧行即 R4-1 本身：R4 误放行、R5 正确拒绝；其余 13 样本两判定一致（无回归）。测试程序与 DLL 中 `VerifyTailOnBuf` 剥除注释后逐字符一致——测的就是发布的代码。样本构造脚本、测试代码与结果存档 `backimg-gui-release\s1-test\`（gen_thumb_samples.py / s1test_r5.cpp / r5_result.txt）。

### 构建

MinGW `-Wall` 零警告；DLL 944,274 B（较 R4 −450B）；外壳内嵌同步成对同源；exe 1,263,209 B；对照补丁 `diff_v1.3.28_to_v1.3.29.patch` 35 块。

### 明确不修（本轮）

- 审计"下一版"清单：ProbeHead/ProbeFull 拆分、D3 改 `erase=FALSE`（与 FillRect 钩子有交互风险）——维持前轮判定
- BMP/GIF/TIFF 截断检查（无可靠终止符）——维持"已知边界"
- 尾部追加 > 4KB 的合法文件被拒——新引入的已知边界（见四问自查第 2 条），症状可诊断可恢复


---

## 修订 R6（2026-11-06，Deepseek V4.1 Flash R5 复核遗留：R5-1 策略副作用收口）

审计报告：《backimg_v1.3.29_R5复核报告_2026-11-06》——**R4-1 修复用原样本验证通过、VerifyTail 抽取被评"这几轮里质量最高的改动"、判定"可以收手"**；指出 R5-1 策略副作用（尾检阈值收紧到 4KB 后，"主体 JPEG + 尾部载荷 >4KB"的合法照片被静默剔除且无日志），实测 Motion Photo 与 8KB 追加样本均被 GDI+ 正常解码却判 REJECT。人类开发者决策：公开分发前修复。

### R5-1 确认

按报告方法构造样本实测：`motion.jpg`（主图 + 2MB MP4 载荷，Pixel/三星动态照片结构）与 `trail8k.jpg`（+8KB 未知追加）在 R5 判定下均 **REJECT**——而用户图片目录正是手机备份目录，动态照片命中概率现实存在。**审计论断属实。**

### 修复（方案 A + B'）

| # | 修复 | 说明 |
|---|---|---|
| A（审计方案 A） | **剔除必留痕**：`LogRejectedFile()` 写入 `ExplorerBgTool_init.log`（`[扫描剔除][hh:mm:ss] 文件名 (原因)`）；进程内同一文件只记一次防重建灌日志；目录不可写静默放弃 | 任何剔除从此可诊断 |
| B'（审计方案 B 的可实现子集） | **载荷救回**：`RescuePayloadJPEG()`——JPEG 判 ABSENT 后全文件反向找最后一处 `FF D9`，其后 16 字节窗口内含已知容器签名（MP4 `ftyp` / ZIP `PK` / RIFF / Samsung `SEFH` / 次级 JPEG `FF D8 FF` / PNG）→ 主图完整的载荷文件，放行；其后是熵数据或再无 FF D9 → 真截断维持剔除 | Motion Photo 类合法文件救回；截断图（最后终止符是缩略图 EOI，其后是主图熵数据，签名命中概率 ~2⁻³⁰）不会误放行 |

### 对审计方案 B 的一处必要修正（如实记录）

审计方案 B 的第 2 步"在 64KB 窗口内找到终止符 → FOUND"**会复活 R4-1**：`thumb_trunc` 类截断文件的缩略图 EOI 距尾 32KB，落在 64KB 窗口内即 FOUND → 误放行。故本版不实现该步，采用 **B'（签名救回）**：只救"EOI 后紧跟已知容器签名"的文件。后果差异：`trail8k.jpg` 类"EOI 后跟未知格式数据"的文件仍被拒绝（签名不可判）——但**从静默吞图变为日志可查**（方案 A 兜底），用户可自行诊断处置。此取舍的理由：方案 B 两步中，第 3 步（签名判定）是既救回合法文件、又不与 R4-1 冲突的唯一可安全实现部分。

### 修复过程中的自我纠错（如实记录）

首轮实现后实测 `motion.jpg` 仍 REJECT——**分段反向读的块边界截断了签名窗口**：主图 EOI 恰落在 1MB 扫描块的尾部时，块内只剩 6 字节后文，`ftyp`（EOI 后第 4~8 字节）被切掉。修复：候选命中但后文不足 16 字节时用句柄从文件补读 16 字节再判（距尾 >4KB 的 EOI 后文必然完整在文件内，补读零风险）。该 bug 由本轮实证测试直接暴露——"测的就是发布的代码"的流程再次生效。

### 四问护栏自查（按人类开发者指定原则）

1. **是否掩盖前置 Bug**——否。方案 A 把 R5-1 的"无日志静默剔除"（审计 Q1 认定的唯一带掩盖色彩处）变为全程留痕；方案 B' 把二值判定升级为"结构分类"（载荷 vs 截断），是判据精度的正面提升。
2. **是否产生新的连锁隐性 Bug**——已逐场景排查：签名窗口 16 字节、熵数据误配概率 ~2⁻³⁰；MP4 内部散落的假 FF D9 被反向扫描自然跳过（每个候选查签名，非签名继续向前）；Motion Photo 被截断于视频段时主图完好、救回后画主图（行为合理）；补读只发生在候选命中且距尾 >4KB 时（后文必然在文件内）；`LogRejectedFile` 有互斥锁、不阻塞、目录不可写静默放弃。唯一已知取舍：未知追加 >4KB 仍拒绝但留痕可诊断。
3. **是否陷入修复循环**——否。增量三处（纯函数 ×2 新增 + 剔除分支一次接入），`VerifyTailOnBuf` 与第 1 步判定零改动；首轮实现的块边界 bug 由实证测试当场暴露、当场修复（一次通过），非反复打补丁。
4. **是否在错误链路上迭代**——否。仍在扫描期文件结构完整性链；绘制路径、缓存、列表逻辑零改动。

### 实证验证（R5 vs R6 双判定对照，十二样本）

`s1test_r6.cpp`（与 DLL 三函数剥除注释同源）：

```
sample                size        probe-route                  R5         R6
big.jpg               16,776,517  probe=OK                     [ACCEPT]   [ACCEPT]
trail_big.jpg         16,777,541  probe=OK                     [ACCEPT]   [ACCEPT]
trunc_big.jpg          9,227,084  probe=OK                     [REJECT]   [REJECT]
small.jpg               108,112   probe=OK                     [ACCEPT]   [ACCEPT]
trunc_small.jpg          59,461   UNKNOWN->GDI+(Ok,per audit)  [REJECT]   [REJECT]
thumb_small.jpg         111,501   probe=OK                     [ACCEPT]   [ACCEPT]
thumb_trunc_small.jpg    61,325   UNKNOWN->GDI+(Ok,per audit)  [REJECT]   [REJECT]  ← R4-1 不复活
motion.jpg           18,873,669   probe=OK                     [REJECT]❌ [ACCEPT]✅ ← R5-1 救回
trail8k.jpg          16,784,709   probe=OK                     [REJECT]   [REJECT]  ← 未知追加：拒绝但日志可查
big.png              36,050,144   probe=OK                     [ACCEPT]   [ACCEPT]
trunc_big.png        19,827,579   probe=OK                     [REJECT]   [REJECT]
trail_big.png        36,051,168   probe=OK                     [ACCEPT]   [ACCEPT]
```

**唯一目标分歧行 `motion.jpg` 救回成功，其余 11 样本零回归**（含 R4-1 原样本仍正确拒绝）。样本构造脚本、测试代码与结果存档 `backimg-gui-release\s1-test\`（gen_r6_samples.py / s1test_r6.cpp / r6_result.txt）。

### 构建

MinGW `-Wall` 零警告；DLL 951,990 B；外壳内嵌同步成对同源；exe 1,270,889 B；`[扫描剔除]` 日志串确认嵌入；对照补丁 `diff_v1.3.28_to_v1.3.29.patch` 36 块。

### 待实机验证（日志路径）

日志写入属 DLL 运行时行为，测试程序无法覆盖——实机验收时：造一张截断 JPEG 放入图片目录 → 保存配置重启资源管理器 → 该图不出现且 `plugin\ExplorerBgTool_init.log` 出现 `[扫描剔除]` 行；再造一张 Motion Photo（手机动态照片直接拷入）→ 正常参与轮换。
