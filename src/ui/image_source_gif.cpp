#include "image_source.h"
#include "renderer.h"
#include <memory>

namespace ui {

/*
 * GifSource —— 用 Renderer::AnimatedPlayer 按需解码 + 单纹理 CopyFromMemory。
 * 相比 ImageViewWidget 的做法把 timer 移到 widget，source 只提供 Tick。
 */
class GifSource : public IImageSource {
public:
    GifSource(std::unique_ptr<Renderer::AnimatedPlayer> player,
              ComPtr<ID2D1Bitmap> bmp)
        : player_(std::move(player)), bmp_(std::move(bmp)) {
        if (player_) {
            w_ = player_->CanvasWidth();
            h_ = player_->CanvasHeight();
        }
    }

    int Width()  const override { return w_; }
    int Height() const override { return h_; }
    ImageCaps Caps() const override {
        ImageCaps c; c.animated = (player_ && player_->FrameCount() > 1);
        c.alpha = true;  // GIF 几乎都有透明色
        return c;
    }
    const char* TypeName() const override { return "GifSource"; }

    int  FrameCount()   const override { return player_ ? player_->FrameCount() : 1; }
    int  CurrentFrame() const override { return currentFrame_; }
    void SeekFrame(int i) override {
        if (!player_ || !bmp_) return;
        int fc = player_->FrameCount();
        if (fc <= 0) return;
        currentFrame_ = ((i % fc) + fc) % fc;
        Upload();
        accumMs_ = 0;
    }

    bool Tick(double dtMs) override {
        if (!player_ || !bmp_ || player_->FrameCount() <= 1) return false;
        accumMs_ += dtMs;
        int delay = player_->DelayMs(currentFrame_);
        if (delay <= 0) delay = 100;
        if (accumMs_ < delay) return false;
        accumMs_ -= delay;
        currentFrame_ = (currentFrame_ + 1) % player_->FrameCount();
        Upload();
        return true;
    }

    void Draw(Renderer& r, const ImageDrawContext& ctx) override {
        if (!bmp_) return;
        auto interp = ctx.antialias
            ? D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
            : (ctx.zoom >= 1.0f
                ? D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR
                : D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
        if (ctx.rotation == 0) {
            r.DrawBitmap(bmp_.Get(), ctx.dest, 1.0f, interp);
            return;
        }
        auto* rt = r.RT();
        D2D1_MATRIX_3X2_F old; rt->GetTransform(&old);
        float dcx = (ctx.dest.left + ctx.dest.right ) / 2.0f;
        float dcy = (ctx.dest.top  + ctx.dest.bottom) / 2.0f;
        float drawW = ctx.dest.right  - ctx.dest.left;
        float drawH = ctx.dest.bottom - ctx.dest.top;
        float effW = (ctx.rotation == 90 || ctx.rotation == 270) ? (float)h_ : (float)w_;
        float effH = (ctx.rotation == 90 || ctx.rotation == 270) ? (float)w_ : (float)h_;
        auto xf =
            D2D1::Matrix3x2F::Translation(-(float)w_/2, -(float)h_/2) *
            D2D1::Matrix3x2F::Rotation((float)ctx.rotation) *
            D2D1::Matrix3x2F::Scale(drawW / effW, drawH / effH) *
            D2D1::Matrix3x2F::Translation(dcx, dcy) *
            old;
        rt->SetTransform(xf);
        D2D1_RECT_F src = {0, 0, (float)w_, (float)h_};
        rt->DrawBitmap(bmp_.Get(), &src, 1.0f, interp);
        rt->SetTransform(old);
    }

private:
    void Upload() {
        const uint8_t* px = player_->ComposeTo(currentFrame_);
        if (px && bmp_) {
            UINT stride = (UINT)w_ * 4;
            bmp_->CopyFromMemory(nullptr, px, stride);
        }
    }

    std::unique_ptr<Renderer::AnimatedPlayer> player_;
    ComPtr<ID2D1Bitmap> bmp_;
    int w_ = 0, h_ = 0;
    int currentFrame_ = 0;
    double accumMs_ = 0;
};

// ========= 工厂 =========

std::unique_ptr<IImageSource>
CreateGifSourceFromFile(const std::wstring& path, Renderer& r) {
    auto player = r.OpenAnimatedImage(path);
    if (!player) return nullptr;
    // 创建初始纹理
    const uint8_t* px = player->ComposeTo(0);
    if (!px) return nullptr;
    int w = player->CanvasWidth();
    int h = player->CanvasHeight();
    auto bmp = r.CreateBitmapFromPixels(px, w, h, w * 4);
    if (!bmp) return nullptr;
    return std::make_unique<GifSource>(std::move(player), bmp);
}

} // namespace ui
