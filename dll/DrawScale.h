/*
* DrawScale.h —— 背景图绘制几何（纯计算，无 Win32 依赖，可独立单元测试）
*
* v1.3.16 新增。抽出来的目的：
*   1) 把"图片该画多大、画在哪"的所有分支决策收敛到一个纯函数里，
*      便于写单元测试（不需要 explorer、不需要钩子）；
*   2) 保证"任何缩放都等比"这条硬约束只有一个实现点，不会散落在
*      各个 posType 分支里各自算错。
*
* 硬约束（v1.3.16 需求）：
*   除 posType=6（缩放并填充）保持原行为外，其余所有模式的缩放
*   一律"按图片原有宽高比等比缩放"，绝不允许拉伸变形。
*
* 模式语义表：
*   0 左上角   等比缩小到可放入窗口（不放大）  贴左上
*   1 右上角   同上                            贴右上
*   2 左下角   同上                            贴左下
*   3 右下角   同上                            贴右下
*   4 居中     同上                            居中
*   5 缩放     等比缩放并完整可见，随窗口双向跟随  居中
*   6 缩放并填充 规则与旧版保持一致（等比铺满 + 居中裁切）
*/

#ifndef DRAWSCALE_H
#define DRAWSCALE_H

/* 绘制矩形（客户区坐标系，左上角为原点） */
struct DrawRect
{
    int x = 0;
    int y = 0;
    int w = 0;      /* 最终绘制宽度（已含缩放） */
    int h = 0;      /* 最终绘制高度（已含缩放） */
};

/* 四舍五入（避免 long 截断导致的 1px 抖动）
 * v1.3.19(审计修复 P1-3)：先钳位再转 int —— 极端宽高比下 imgW*scale
 * 可达数亿，double 直接 (int) 是未定义行为。 */
inline int ScaleRound(double v)
{
    if (v >= 2147483000.0) return 2147483000;
    if (v < 0.0) v = 0.0;
    int r = (int)(v + 0.5);
    return r;
}

/* v1.3.19(审计修复 P1-3)：单次绘制的目标位图像素总量上限。
 * 6400 万像素 = 32bpp 下 256MB，覆盖 8K 全屏（约 3300 万像素）仍有余量。
 * 超过它说明"这张图配这个窗口"不合常理（典型：10 万 x 1 的极端宽高比
 * 在"缩放并填充"模式下会算出数百 GiB 的目标位图），返回空矩形放弃绘制。 */
const unsigned long long DRAW_MAX_PIXELS = 64000000ull;

/*
* 计算背景图在窗口客户区里的目标矩形。
*
* imgW/imgH : 图片原始像素尺寸（未经任何缩放）
* winW/winH : 窗口客户区尺寸
* mode      : posType，0..6
*
* 返回值里的 w/h 就是最终要画出来的像素尺寸；
* 若 w/h 与 imgW/imgH 不同，说明需要走缩放渲染路径。
*/
inline DrawRect ComputeDrawRect(int mode, int imgW, int imgH, int winW, int winH)
{
    DrawRect r;
    r.x = r.y = r.w = r.h = 0;

    /* 任何除零 / 负值都要挡住：脏配置或窗口尚未布局完成时不能画 */
    if (imgW <= 0 || imgH <= 0 || winW <= 0 || winH <= 0)
        return r;

    const double sx = (double)winW / (double)imgW;
    const double sy = (double)winH / (double)imgH;

    double scale = 1.0;

    switch (mode)
    {
    case 5:
        /* 缩放：等比、完整可见，并随窗口双向跟随（窗口变大图片也变大） */
        scale = (sx < sy) ? sx : sy;
        break;

    case 6:
        /* 缩放并填充：等比铺满窗口，短边溢出部分由窗口裁掉（规则同旧版） */
        scale = (sx > sy) ? sx : sy;
        break;

    default:
        /* 0..4 角落/居中：只在图片放不下时才等比缩小，
         * 放得下就保持原始像素尺寸（避免小图被放大成糊图） */
        scale = 1.0;
        if (sx < scale) scale = sx;
        if (sy < scale) scale = sy;
        break;
    }

    r.w = ScaleRound(imgW * scale);
    r.h = ScaleRound(imgH * scale);
    if (r.w < 1) r.w = 1;
    if (r.h < 1) r.h = 1;

    /* v1.3.19(审计修复 P1-3)：目标像素总量超限时放弃绘制。
     * 必须在 r.x/r.y 计算之前返回，且乘法用 64 位避免溢出。 */
    if ((unsigned long long)r.w * (unsigned long long)r.h > DRAW_MAX_PIXELS)
    {
        r.x = r.y = r.w = r.h = 0;
        return r;
    }

    switch (mode)
    {
    case 0: /* 左上角 */
        r.x = 0;
        r.y = 0;
        break;
    case 1: /* 右上角 */
        r.x = winW - r.w;
        r.y = 0;
        break;
    case 2: /* 左下角 */
        r.x = 0;
        r.y = winH - r.h;
        break;
    case 3: /* 右下角 */
        r.x = winW - r.w;
        r.y = winH - r.h;
        break;
    case 4: /* 居中 */
        r.x = (winW - r.w) / 2;
        r.y = (winH - r.h) / 2;
        break;
    case 5: /* 缩放：居中 */
        r.x = (winW - r.w) / 2;
        r.y = (winH - r.h) / 2;
        break;
    case 6: /* 缩放并填充：居中（溢出部分被客户区裁掉） */
        r.x = (winW - r.w) / 2;
        r.y = (winH - r.h) / 2;
        break;
    default:
        r.x = 0;
        r.y = 0;
        break;
    }

    return r;
}

#endif /* DRAWSCALE_H */
