#include "image_source.h"
#include "renderer.h"
#include <map>
#include <cmath>
#include <algorithm>

namespace ui {

class TiledSource : public IImageSource, public IImageSource::ITiledSource {
public:
    TiledSource(int fullW, int fullH, int tileSize, Renderer& r)
        : fullW_(fullW), fullH_(fullH),
          tileSize_(tileSize > 0 ? tileSize : 512),
          renderer_(&r) {}

    int Width()  const override { return fullW_; }
    int Height() const override { return fullH_; }
    ImageCaps Caps() const override {
        ImageCaps c; c.tiled = true; return c;
    }
    const char* TypeName() const override { return "TiledSource"; }

    void Draw(Renderer& r, const ImageDrawContext& ctx) override {
        // 有 preview 则先画全图预览兜底
        if (preview_) {
            auto interp = (!ctx.antialias && ctx.zoom >= 4.0f)
                ? D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR
                : D2D1_BITMAP_INTERPOLATION_MODE_LINEAR;
            r.DrawBitmap(preview_.Get(), ctx.dest, 1.0f, interp);
        }
        // 计算可见瓦片范围
        float drawW = ctx.dest.right  - ctx.dest.left;
        float drawH = ctx.dest.bottom - ctx.dest.top;
        if (drawW <= 0 || drawH <= 0) return;

        int ts = tileSize_;
        int txMax = (fullW_ + ts - 1) / ts;
        int tyMax = (fullH_ + ts - 1) / ts;

        // 外扩 1 像素 + NEAREST 防缝
        auto interp = D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR;

        for (int ty = 0; ty < tyMax; ++ty) {
            for (int tx = 0; tx < txMax; ++tx) {
                auto it = tiles_.find({tx, ty});
                if (it == tiles_.end()) continue;
                int tw = it->second.w;
                int th = it->second.h;
                float x0 = std::floor(ctx.dest.left + (tx * ts)      * ctx.zoom) - 1.0f;
                float y0 = std::floor(ctx.dest.top  + (ty * ts)      * ctx.zoom) - 1.0f;
                float x1 = std::ceil (ctx.dest.left + (tx * ts + tw) * ctx.zoom) + 1.0f;
                float y1 = std::ceil (ctx.dest.top  + (ty * ts + th) * ctx.zoom) + 1.0f;
                D2D1_RECT_F dest = {x0, y0, x1, y1};
                r.DrawBitmap(it->second.bmp.Get(), dest, 1.0f, interp);
            }
        }
    }

    // ---- ITiledSource ----
    void SetTile(int tx, int ty, const void* pixels,
                 int w, int h, int stride) override {
        if (!renderer_ || !pixels || w <= 0 || h <= 0) return;
        auto bmp = renderer_->CreateBitmapFromPixels(pixels, w, h, stride);
        if (bmp) tiles_[{tx, ty}] = {bmp, w, h};
    }
    void SetPreview(ComPtr<ID2D1Bitmap> bmp, int /*w*/, int /*h*/) override {
        preview_ = std::move(bmp);
    }
    void ClearTiles() override { tiles_.clear(); preview_.Reset(); }
    int  TileSize() const override { return tileSize_; }

private:
    struct Tile {
        ComPtr<ID2D1Bitmap> bmp;
        int w = 0, h = 0;
    };
    int fullW_, fullH_, tileSize_;
    Renderer* renderer_;
    std::map<std::pair<int,int>, Tile> tiles_;
    ComPtr<ID2D1Bitmap> preview_;
};

std::unique_ptr<IImageSource>
CreateTiledSource(int fullW, int fullH, int tileSize, Renderer& r) {
    return std::make_unique<TiledSource>(fullW, fullH, tileSize, r);
}

} // namespace ui
