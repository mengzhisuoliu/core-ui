#include "image_source.h"
#include "renderer.h"
#include <d2d1_3.h>
#include <shlwapi.h>
#include <windows.h>
#include <vector>
/* shlwapi 由 CMakeLists 的 UI_CORE_SYSTEM_LIBS 统一链接，不在这里 #pragma comment
 * 因为 MinGW 不识别该 pragma（无害，但保持风格一致）。*/

namespace ui {

// Renderer 需要暴露一个 ID2D1DeviceContext5*（支持检测）
// 见 renderer.h 的增量：ID2D1DeviceContext5* RT5() { return ctx5_.Get(); }

// ========= 原生路径：ID2D1SvgDocument =========

class SvgSourceNative : public IImageSource {
public:
    SvgSourceNative(ComPtr<ID2D1SvgDocument> doc, int w, int h)
        : doc_(std::move(doc)), w_(w), h_(h) {}

    int  Width()  const override { return w_; }
    int  Height() const override { return h_; }
    ImageCaps Caps() const override {
        ImageCaps c; c.vector = true; c.alpha = true; return c;
    }
    const char* TypeName() const override { return "SvgSourceNative"; }

    void Draw(Renderer& r, const ImageDrawContext& ctx) override {
        auto* ctx5 = r.RT5();
        if (!ctx5 || !doc_) return;

        float drawW = ctx.dest.right  - ctx.dest.left;
        float drawH = ctx.dest.bottom - ctx.dest.top;
        if (drawW <= 0 || drawH <= 0 || w_ <= 0 || h_ <= 0) return;

        /* 关键：SVG viewport 是 SVG 内部 user-unit 的参考坐标（用于解析 % 值），
         * **不是**最终渲染像素尺寸。真正控制绘制大小用 SetTransform scale。
         * 这样 D2D 按矢量光栅化到任意缩放级别都清晰。
         *
         * viewport 设为 SVG 原生宽高（w_, h_），transform 里做 scale 到 drawW/drawH。
         */
        doc_->SetViewportSize(D2D1::SizeF((float)w_, (float)h_));

        D2D1_MATRIX_3X2_F old;
        ctx5->GetTransform(&old);

        float sx = drawW / (float)w_;
        float sy = drawH / (float)h_;
        float cx = (ctx.dest.left + ctx.dest.right) / 2.0f;
        float cy = (ctx.dest.top  + ctx.dest.bottom) / 2.0f;

        /* 矩阵顺序（行向量 P * M）：
         *   Scale(sx, sy) 把 (0,0)-(w_,h_) 映射到 (0,0)-(drawW, drawH)
         *   Translation 平移到 dest 左上角
         *   Rotation 绕 dest 中心旋转
         *   * old 叠加外层 transform
         */
        auto xf =
            D2D1::Matrix3x2F::Scale(sx, sy) *
            D2D1::Matrix3x2F::Translation(ctx.dest.left, ctx.dest.top) *
            D2D1::Matrix3x2F::Rotation((float)ctx.rotation,
                                        D2D1::Point2F(cx, cy)) *
            old;
        ctx5->SetTransform(xf);

        ctx5->DrawSvgDocument(doc_.Get());
        ctx5->SetTransform(old);
    }

private:
    ComPtr<ID2D1SvgDocument> doc_;
    int w_, h_;
};

// ========= Fallback：用 Renderer::ParseSvgIcon，只支持 SVG 子集 =========

class SvgSourceFallback : public ISvgFallbackSource {
public:
    SvgSourceFallback(SvgIcon icon) : icon_(std::move(icon)) {
        w_ = (int)icon_.viewBoxW;
        h_ = (int)icon_.viewBoxH;
    }

    int  Width()  const override { return w_; }
    int  Height() const override { return h_; }
    ImageCaps Caps() const override {
        ImageCaps c; c.vector = true; c.alpha = true; return c;
    }
    const char* TypeName() const override { return "SvgSourceFallback"; }
    const SvgIcon& Icon() const override { return icon_; }

    void Draw(Renderer& r, const ImageDrawContext& ctx) override {
        // SvgIcon 原设计是 icon，染色统一。看图场景我们传白色让它保留原路径 fill。
        // 但 ParseSvgIcon 的 layers 里已有 path 自身的颜色信息时可按那个画。
        // 这里简化：用 DrawSvgIcon 的默认纯色绘制（这是 fallback，功能本来就打折扣）。
        auto color = D2D1::ColorF(D2D1::ColorF::Black);
        r.DrawSvgIcon(icon_, ctx.dest, color);
    }

private:
    SvgIcon icon_;
    int w_ = 0, h_ = 0;
};

// ========= 文件读取（跨 MSVC/MinGW：Win32 API，避开 std::ifstream 宽字符不兼容）=========

static std::string ReadFileUtf8(const std::wstring& path) {
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
                            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return "";
    LARGE_INTEGER sz;
    if (!GetFileSizeEx(h, &sz) || sz.QuadPart <= 0 || sz.QuadPart > (1LL << 30)) {
        CloseHandle(h); return "";
    }
    std::string buf((size_t)sz.QuadPart, '\0');
    DWORD read = 0;
    BOOL ok = ReadFile(h, buf.data(), (DWORD)sz.QuadPart, &read, nullptr);
    CloseHandle(h);
    if (!ok || read != (DWORD)sz.QuadPart) return "";
    return buf;
}

// ========= 工厂 =========

std::unique_ptr<IImageSource>
CreateSvgSourceFromFile(const std::wstring& path, Renderer& r) {
    // 先试原生路径（Win10 1607+）
    if (auto* ctx5 = r.RT5()) {
        ComPtr<IStream> stream;
        if (SUCCEEDED(SHCreateStreamOnFileEx(
                path.c_str(),
                STGM_READ | STGM_SHARE_DENY_WRITE,
                FILE_ATTRIBUTE_NORMAL, FALSE, nullptr, &stream)))
        {
            ComPtr<ID2D1SvgDocument> doc;
            // 初始 viewport 随意，Draw 时会改
            if (SUCCEEDED(ctx5->CreateSvgDocument(
                    stream.Get(), D2D1::SizeF(1024, 1024), &doc)))
            {
                // 从 root 的 viewBox 读真实尺寸
                ComPtr<ID2D1SvgElement> root;
                doc->GetRoot(&root);
                D2D1_SVG_VIEWBOX vb{};
                int w = 1024, h = 1024;
                if (root && SUCCEEDED(root->GetAttributeValue(
                        L"viewBox",
                        D2D1_SVG_ATTRIBUTE_POD_TYPE_VIEWBOX,
                        &vb, sizeof(vb))) && vb.width > 0 && vb.height > 0)
                {
                    w = (int)vb.width; h = (int)vb.height;
                } else if (root) {
                    D2D1_SVG_LENGTH lw{}, lh{};
                    if (SUCCEEDED(root->GetAttributeValue(
                            L"width",
                            D2D1_SVG_ATTRIBUTE_POD_TYPE_LENGTH, &lw, sizeof(lw)))
                        && lw.value > 0) w = (int)lw.value;
                    if (SUCCEEDED(root->GetAttributeValue(
                            L"height",
                            D2D1_SVG_ATTRIBUTE_POD_TYPE_LENGTH, &lh, sizeof(lh)))
                        && lh.value > 0) h = (int)lh.value;
                }
                return std::make_unique<SvgSourceNative>(doc, w, h);
            }
        }
    }

    // Fallback：读文件内容 → ParseSvgIcon
    std::string content = ReadFileUtf8(path);
    if (content.empty()) return nullptr;
    SvgIcon icon = r.ParseSvgIcon(content);
    if (!icon.valid) return nullptr;
    return std::make_unique<SvgSourceFallback>(std::move(icon));
}

} // namespace ui
