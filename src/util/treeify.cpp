// SPDX-License-Identifier: GPL-2.0-or-later
#include "treeify.h"

#include <cassert>

namespace Inkscape::Util {

TreeifyResult treeify(int N, std::function<bool(int, int)> const &contains)
{
    // Todo: (C++23) Refactor away to a recursive lambda.
    class Treeifier
    {
    public:
        Treeifier(int N, std::function<bool(int, int)> const &contains)
            : N{N}
            , contains{contains}
            , data(N)
        {
            for (int i = 0; i < N; i++) {
                for (int j = 0; j < N; j++) {
                    if (j != i && contains(i, j)) {
                        data[j].num_containers++;
                        data[i].contained.emplace_back(j);
                    }
                }
            }

            result.num_children.resize(N);

            for (int i = 0; i < N; i++) {
                if (data[i].num_containers == 0) {
                    visit(i);
                }
            }

            for (int i = 0; i < N; i++) {
                if (data[i].num_containers != -1) {
                    result.preorder.emplace_back(i);
                }
            }

            assert(result.preorder.size() == N);
        }

        TreeifyResult moveResult() { return std::move(result); }

    private:
        // Input
        int N{};
        std::function<bool(int, int)> const &contains;

        // State
        struct Data
        {
            int num_containers = 0;
            std::vector<int> contained;
        };
        std::vector<Data> data;

        // Output
        TreeifyResult result;

        void visit(int i)
        {
            result.preorder.emplace_back(i);

            for (auto j : data[i].contained) {
                data[j].num_containers--;
            }

            for (auto j : data[i].contained) {
                if (data[j].num_containers == 0) {
                    result.num_children[i]++;
                    visit(j);
                }
            }

            data[i].num_containers = -1;
        }
    };

    return Treeifier(N, contains).moveResult();
}

} // namespace Inkscape::Util

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
