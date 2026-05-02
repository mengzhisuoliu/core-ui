#include "image_source.h"
#include "renderer.h"
#include <utility>

namespace ui {

class BitmapSource : public IImageSource {
public:
    explicit BitmapSource(ComPtr<ID2D1Bitmap> bmp) : bmp_(std::move(bmp)) {
        if (bmp_) {
            auto sz = bmp_->GetPixelSize();
            w_ = (int)sz.width;
            h_ = (int)sz.height;
            // PNG/WebP 通常带 alpha；JPG 不带。D2D 位图 alphaMode 可以查，
            // 这里简单标记 true（画棋盘的代价极小，避免漏检）
            // 想更精确可读 bmp->GetPixelFormat().alphaMode
            auto fmt = bmp_->GetPixelFormat();
            hasAlpha_ = (fmt.alphaMode == D2D1_ALPHA_MODE_PREMULTIPLIED ||
                         fmt.alphaMode == D2D1_ALPHA_MODE_STRAIGHT);
        }
    }

    int Width()  const override { return w_; }
    int Height() const override { return h_; }
    ImageCaps Caps() const override {
        ImageCaps c; c.alpha = hasAlpha_; return c;
    }
    const char* TypeName() const override { return "BitmapSource"; }

    void Draw(Renderer& r, const ImageDrawContext& ctx) override {
        if (!bmp_) return;
        // 与 ImageViewWidget 相同插值策略：放大 NEAREST（锐利）/ 缩小 HIGH_QUALITY_CUBIC
        bool useHQ = (ctx.zoom < 1.0f) || ctx.antialias;
        auto interp = (!ctx.antialias && ctx.zoom >= 1.0f)
            ? D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR
            : D2D1_BITMAP_INTERPOLATION_MODE_LINEAR;

        if (ctx.rotation == 0) {
            if (useHQ) r.DrawBitmapHQ(bmp_.Get(), ctx.dest, 1.0f,
                                       D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC);
            else       r.DrawBitmap  (bmp_.Get(), ctx.dest, 1.0f, interp);
            return;
        }

        // 旋转：矩阵变换
        auto* rt = r.RT();
        D2D1_MATRIX_3X2_F old;
        rt->GetTransform(&old);

        float drawW = ctx.dest.right - ctx.dest.left;
        float drawH = ctx.dest.bottom - ctx.dest.top;
        float effW = (ctx.rotation == 90 || ctx.rotation == 270) ? (float)h_ : (float)w_;
        float effH = (ctx.rotation == 90 || ctx.rotation == 270) ? (float)w_ : (float)h_;
        float sx = drawW / effW;
        float sy = drawH / effH;
        float dcx = (ctx.dest.left + ctx.dest.right ) / 2.0f;
        float dcy = (ctx.dest.top  + ctx.dest.bottom) / 2.0f;

        auto xf =
            D2D1::Matrix3x2F::Translation(-(float)w_/2, -(float)h_/2) *
            D2D1::Matrix3x2F::Rotation((float)ctx.rotation) *
            D2D1::Matrix3x2F::Scale(sx, sy) *
            D2D1::Matrix3x2F::Translation(dcx, dcy) *
            old;
        rt->SetTransform(xf);

        D2D1_RECT_F src = {0, 0, (float)w_, (float)h_};
        if (useHQ)
            rt->DrawBitmap(bmp_.Get(), src, 1.0f,
                           D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC, nullptr);
        else
            rt->DrawBitmap(bmp_.Get(), &src, 1.0f,
                           (D2D1_BITMAP_INTERPOLATION_MODE)interp);

        rt->SetTransform(old);
    }

private:
    ComPtr<ID2D1Bitmap> bmp_;
    int w_ = 0, h_ = 0;
    bool hasAlpha_ = false;
};

// ========= 工厂 =========

std::unique_ptr<IImageSource>
CreateBitmapSourceFromFile(const std::wstring& path, Renderer& r) {
    auto bmp = r.LoadImageFromFile(path);
    if (!bmp) return nullptr;
    return std::make_unique<BitmapSource>(bmp);
}

std::unique_ptr<IImageSource>
CreateBitmapSourceFromBitmap(ComPtr<ID2D1Bitmap> bmp) {
    if (!bmp) return nullptr;
    return std::make_unique<BitmapSource>(std::move(bmp));
}

std::unique_ptr<IImageSource>
CreateBitmapSourceFromPixels(const void* px, int w, int h, int stride, Renderer& r) {
    auto bmp = r.CreateBitmapFromPixels(px, w, h, stride);
    if (!bmp) return nullptr;
    return std::make_unique<BitmapSource>(bmp);
}

} // namespace ui
