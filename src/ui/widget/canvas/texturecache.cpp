// SPDX-License-Identifier: GPL-2.0-or-later

#include <vector>
#include <boost/unordered_map.hpp> // For hash of pair

#include "helper/mathfns.h"
#include "texturecache.h"

namespace Inkscape {
namespace UI {
namespace Widget {

namespace {

class BasicTextureCache : public TextureCache
{
    static int constexpr min_dimension = 16;
    static int constexpr expiration_timeout = 10000;

    static int constexpr dim_to_ind(unsigned dim) { return Util::index_to_binary_bucket<unsigned, min_dimension>(dim); }
    static int constexpr ind_to_maxdim(int index) { return min_dimension * (1 << index); }

    static std::pair<int, int> dims_to_inds(Geom::IntPoint const &dims) { return { dim_to_ind(dims.x()), dim_to_ind(dims.y()) }; }
    static Geom::IntPoint inds_to_maxdims(std::pair<int, int> const &inds) { return { ind_to_maxdim(inds.first), ind_to_maxdim(inds.second) }; }

    // A cache of POT textures.
    struct Bucket
    {
        std::vector<Texture> unused;
        int used = 0;
        int high_use_count = 0;
    };
    boost::unordered_map<std::pair<int, int>, Bucket> buckets;

    // Used to periodicially discard excess cached textures.
    int expiration_timer = 0;

public:
    Texture request(Geom::IntPoint const &dimensions) override
    {
        // Find the bucket that the dimensions fall into.
        auto indexes = dims_to_inds(dimensions);
        auto &b = buckets[indexes];

        // Reuse or create a texture of the appropriate dimensions.
        Texture tex;
        if (!b.unused.empty()) {
            tex = std::move(b.unused.back());
            b.unused.pop_back();
            glBindTexture(GL_TEXTURE_2D, tex.id());
        } else {
            tex = Texture(inds_to_maxdims(indexes)); // binds
        }

        // Record the new use count of the bucket.
        b.used++;
        if (b.used > b.high_use_count) {
            // If the use count has gone above the high-water mark, record this, and reset the timer for when to clean up excess unused textures.
            b.high_use_count = b.used;
            expiration_timer = 0;
        }

        return tex;
    }

    void finish(Texture tex) override
    {
        auto indexes = dims_to_inds(tex.size());
        auto &b = buckets[indexes];

        // Orphan the texture, if possible.
        tex.invalidate();

        // Put the texture back in its corresponding bucket's cache of unused textures.
        b.unused.emplace_back(std::move(tex));
        b.used--;

        // If the expiration timeout has been reached, prune the cache of textures down to what was actually used in the last cycle.
        expiration_timer++;
        if (expiration_timer >= expiration_timeout) {
            expiration_timer = 0;

            for (auto &[k, b] : buckets) {
                int max_unused = b.high_use_count - b.used;
                assert(max_unused >= 0);
                if (b.unused.size() > max_unused) {
                    b.unused.resize(max_unused);
                }
                b.high_use_count = b.used;
            }
        }
    }
};

} // namespace

std::unique_ptr<TextureCache> TextureCache::create()
{
    return std::make_unique<BasicTextureCache>();
}

} // namespace Widget
} // namespace UI
} // namespace Inkscape

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
