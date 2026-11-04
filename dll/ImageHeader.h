/*
* ImageHeader.h —— 轻量图片头解析（纯计算，不依赖 GDI+）
*
* v1.3.17 新增。解决两件事：
*
* 【A】BitmapGDI::Size 必须有确定的值。
*   v1.3.16 的 Size 没有初值，构造函数只在"文件打得开且 GDI+ 探测解码成功"
*   时才赋值；文件打不开时读到的是堆上的垃圾值（实测 200/200 次非零），
*   而上层过滤条件又退化成只比 Size.cx > 0 —— 坏文件会被当成有效图片收进列表。
*
* 【B】结构损坏的图片不该被送进 GDI+。
*   v1.3.16 把完整原始字节交给 GDI+ 解码；v1.3.15 恰好在
*   SHCreateMemStream 之前就 delete[] 了缓冲区，GDI+ 读到的是被 free-list
*   元数据污染的首字节，PNG 签名被破坏后 GDI+ 会提前安全退出 ——
*   那是"歪打正着"的保护。v1.3.16 保留了完整字节，损坏的 PNG 会被真正解析。
*
* ------------------------------------------------------------------
* 【本模块的核心准则：绝不单独否决一张图】
* ------------------------------------------------------------------
* 第一版实现里，本模块写死了"最多 64 个 PNG 块"的上限，
* 结果把用户目录里一张带 212 个 IDAT 块的合法 PNG 误判成损坏。
* 错杀比漏杀危险得多 —— 漏杀只是画不出背景，错杀会让用户的图凭空消失。
*
* 所以现在的规则是【只有能证明"这张图确实坏了"时才下否决结论】：
*   - 首个块不是 IHDR / 长度不是 13 / CRC 对不上 / 尺寸离谱
*                                  -> PNG 规范强制要求，绝不会误判
*   - 某个块的数据长度超出文件末尾  -> 必然是截断，绝不会误判
*   - 只是"没找到 IEND"            -> 【不否决】，交给 GDI+ 判
*   - 签名不认识 / 其它格式        -> 【不否决】，交给 GDI+ 判
*
* 即：本模块只做两件事 —— 快速取尺寸、识别"确证损坏"。
* 最终"这张图能不能用"的裁决权永远交给 GDI+（GetLastStatus）。
*
* 本模块只做"读字节 + 算偏移"，不分配内存、不调用 Win32 API，
* 因此可以像 DrawScale.h 一样独立单元测试。
*/

#ifndef IMAGEHEADER_H
#define IMAGEHEADER_H

#include <stddef.h>
#include <string.h>

/* 探测结果 */
enum ImageProbeResult
{
    IMG_UNKNOWN = 0,   /* 看不懂 / 不确定 -> 交给 GDI+ 裁决 */
    IMG_OK      = 1,   /* 确证是结构完整的图片，w/h 有效 */
    IMG_SUSPECT = 2    /* 确证损坏 -> 直接拒绝，绝不送进 GDI+ */
};

/* PNG 签名：89 50 4E 47 0D 0A 1A 0A */
static const unsigned char kPngSig[8] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };

/* 大端 32 位读取（PNG/JPEG 都是大端） */
inline unsigned long ReadBE32(const unsigned char* p)
{
    return ((unsigned long)p[0] << 24) | ((unsigned long)p[1] << 16)
         | ((unsigned long)p[2] << 8)  | ((unsigned long)p[3]);
}

inline unsigned int ReadBE16(const unsigned char* p)
{
    return ((unsigned int)p[0] << 8) | (unsigned int)p[1];
}

/*
* CRC-32（PNG 用 polynom 0xEDB88320 的标准实现）
* 只用于校验 IHDR 块 —— 宽高就写在那里，位翻转会得到离谱的尺寸。
*/
inline unsigned int Crc32(const unsigned char* data, size_t len, unsigned int crc = 0)
{
    static unsigned int table[256];
    static bool tableReady = false;
    if (!tableReady)
    {
        for (unsigned int i = 0; i < 256; i++)
        {
            unsigned int c = i;
            for (int k = 0; k < 8; k++)
                c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            table[i] = c;
        }
        tableReady = true;
    }
    crc = crc ^ 0xFFFFFFFFu;
    for (size_t i = 0; i < len; i++)
        crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFFu;
}

/*
* PNG 结构走查。
*
* 返回 IMG_OK     : IHDR 有效且走到了 IEND，宽高已写出
* 返回 IMG_SUSPECT: 确证损坏（IHDR 不合法，或有块超出文件末尾 = 截断）
* 返回 IMG_UNKNOWN: 没看懂（没找到 IEND，但也没发现越界）-> 交给 GDI+
*
* 注意：块的数量【不设上限】。合法 PNG 的块数可以很多
* （实测用户目录里有一张 212 个 IDAT 块的），
* 循环本身不会跑飞：每轮 pos 至少前进 12 字节，且块长已做越界校验。
*/
inline ImageProbeResult ProbePng(const unsigned char* d, size_t n, int* outW, int* outH)
{
    if (n < 8 + 8 + 13 + 4)
        return IMG_SUSPECT;                  /* 连 IHDR 都放不下 */

    size_t pos = 8;
    bool sawIHDR = false;
    bool sawIEND = false;

    while (!sawIEND)
    {
        if (pos + 8 > n)
            return IMG_UNKNOWN;              /* 块头不完整：不确定，交给 GDI+ */

        unsigned long chunkLen = ReadBE32(d + pos);
        const unsigned char* type = d + pos + 4;

        if (chunkLen > 0x7FFFFFFFu)
            return IMG_SUSPECT;              /* 块长离谱：确证损坏 */

        if (pos + 8 + (size_t)chunkLen + 4 > n)
            return IMG_SUSPECT;              /* 块数据或 CRC 超出文件末尾 = 截断 */

        if (type[0] == 'I' && type[1] == 'H' && type[2] == 'D' && type[3] == 'R')
        {
            if (!sawIHDR)
            {
                /* PNG 规范：第一个块必须是 IHDR，长度恰好 13 */
                if (chunkLen != 13)
                    return IMG_SUSPECT;
                unsigned long w = ReadBE32(d + pos + 8);
                unsigned long h = ReadBE32(d + pos + 12);
                if (w == 0 || h == 0 || w > 100000 || h > 100000)
                    return IMG_SUSPECT;
                unsigned int calc = Crc32(d + pos + 4, 4 + 13);
                unsigned int file = ReadBE32(d + pos + 8 + 13);
                if (calc != file)
                    return IMG_SUSPECT;      /* IHDR 位翻转 */
                *outW = (int)w;
                *outH = (int)h;
                sawIHDR = true;
            }
        }
        else if (type[0] == 'I' && type[1] == 'E' && type[2] == 'N' && type[3] == 'D')
        {
            sawIEND = true;
            break;
        }

        pos += 8 + (size_t)chunkLen + 4;
    }

    if (!sawIHDR)
        return IMG_SUSPECT;                  /* 没有 IHDR：不是合法 PNG */
    return IMG_OK;
}

/*
* JPEG 结构走查。
* 保守检查：SOI 开头、EOI 结尾，并尽力从 SOF 段取宽高。
* JPEG 的 marker 结构很复杂，取不到宽高就交回 UNKNOWN，交给 GDI+。
*/
inline ImageProbeResult ProbeJpeg(const unsigned char* d, size_t n, int* outW, int* outH)
{
    if (n < 4)
        return IMG_SUSPECT;
    if (d[0] != 0xFF || d[1] != 0xD8)          /* SOI */
        return IMG_UNKNOWN;                    /* 不是 JPEG，交给 GDI+ 判 */
    /* v1.3.19(审计修复 P3-1)：结尾没有 EOI 不再直接判死。
     * 带尾随数据的 JPEG（结尾被追加过字节）同样缺 EOI，但 GDI+ 照样能解；
     * 按"解析器绝不单独否决一张图"的原则交给 GDI+ 裁决。 */
    if (d[n - 2] != 0xFF || d[n - 1] != 0xD9)  /* EOI */
        return IMG_UNKNOWN;

    /* 顺着 marker 链找 SOF（C0-CF，排除 C4/C8/CC） */
    size_t pos = 2;
    bool gotSof = false;                       /* v1.3.19：是否真的拿到了 SOF 尺寸 */
    for (int guard = 0; guard < 256 && pos + 4 <= n; guard++)
    {
        if (d[pos] != 0xFF)
            return IMG_UNKNOWN;                /* marker 链断了：不确定 */
        unsigned char m = d[pos + 1];
        if (m == 0xD8 || m == 0x01 || (m >= 0xD0 && m <= 0xD7))
        {
            pos += 2;                          /* 独立 marker，无长度域 */
            continue;
        }
        if (m == 0xD9)                         /* EOI */
            break;
        if (pos + 4 > n)
            return IMG_SUSPECT;
        unsigned int segLen = ReadBE16(d + pos + 2);
        if (segLen < 2 || pos + 2 + segLen > n)
            return IMG_SUSPECT;                /* 段长越界 = 截断 */

        if (m >= 0xC0 && m <= 0xCF && m != 0xC4 && m != 0xC8 && m != 0xCC)
        {
            if (segLen >= 7)
            {
                unsigned int h = ReadBE16(d + pos + 5);
                unsigned int w = ReadBE16(d + pos + 7);
                if (w > 0 && h > 0 && w <= 100000 && h <= 100000)
                {
                    *outW = (int)w;
                    *outH = (int)h;
                    gotSof = true;
                }
            }
            break;                              /* SOF 之后是图像数据 */
        }
        pos += 2 + segLen;
    }
    /* v1.3.19(审计修复 P3-1)：没拿到有效 SOF 尺寸就不能报 IMG_OK ——
     * 旧写法在 APP/COM 段较多（marker 扫描超 256 次）等情况下
     * 空手返回 IMG_OK，宽高为 0，构造函数直接把合法图片过滤掉了，
     * 连 GDI+ 回退的机会都没有。现在交回 UNKNOWN 让 GDI+ 裁决。 */
    return gotSof ? IMG_OK : IMG_UNKNOWN;
}

/*
* 总入口：探测图片头。
*
* IMG_OK      -> w/h 有效，可直接用
* IMG_SUSPECT -> 确证损坏，直接拒绝
* IMG_UNKNOWN -> 看不懂，调用方应交给 GDI+ 裁决
*/
inline ImageProbeResult ProbeImageHeader(const unsigned char* d, size_t n,
                                         int* outW, int* outH)
{
    if (outW) *outW = 0;
    if (outH) *outH = 0;
    if (!d || n < 4)
        return IMG_SUSPECT;                    /* 没有数据 / 少于 4 字节：肯定不是图 */

    /* --- PNG --- */
    if (n >= 8 && memcmp(d, kPngSig, 8) == 0)
        return ProbePng(d, n, outW, outH);

    /* --- JPEG --- */
    if (d[0] == 0xFF && d[1] == 0xD8 && d[2] == 0xFF)
        return ProbeJpeg(d, n, outW, outH);

    /* --- 其它常见格式：只认签名，结构交给 GDI+ --- */
    if (d[0] == 'B' && d[1] == 'M')                       /* BMP */
        return IMG_UNKNOWN;
    if (d[0] == 'G' && d[1] == 'I' && d[2] == 'F' && d[3] == '8')  /* GIF */
        return IMG_UNKNOWN;
    if (d[0] == 'R' && d[1] == 'I' && d[2] == 'F' && d[3] == 'F'
        && n >= 12 && d[8] == 'W' && d[9] == 'E' && d[10] == 'B' && d[11] == 'P')  /* WebP */
        return IMG_UNKNOWN;
    if ((d[0] == 'I' && d[1] == 'I' && d[2] == 42 && d[3] == 0)
        || (d[0] == 'M' && d[1] == 'M' && d[2] == 0 && d[3] == 42))  /* TIFF */
        return IMG_UNKNOWN;
    if (d[0] == 0 && d[1] == 0 && d[2] == 1 && d[3] == 0)           /* ICO */
        return IMG_UNKNOWN;

    /* 签名对不上任何已知图片格式：多半根本不是图片，但【不下否决结论】——
     * 交给 GDI+ 裁决，避免误杀。 */
    return IMG_UNKNOWN;
}

#endif /* IMAGEHEADER_H */
