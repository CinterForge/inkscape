// SPDX-License-Identifier: GPL-2.0-or-later
#include "util/treeify.h"

#include <array>
#include <random>
#include <gtest/gtest.h>

namespace Inkscape::Util {
namespace {

constexpr int N = 30;

bool verify(std::array<int, N> const &parent, TreeifyResult const &tree, int &pos, int expected_parent)
{
    int const i = tree.preorder[pos];
    pos++;

    if (parent[i] != expected_parent) {
        return false;
    }

    for (int c = 0; c < tree.num_children[i]; c++) {
        verify(parent, tree, pos, i);
    }

    return true;
}

} // namespace

TEST(TreeifyTest, RandomForest)
{
    std::array<int, N> parent;

    std::mt19937 gen;

    for (int a = 0; a < 100; a++) {
        // Build a random forest with N nodes.
        int num_trees = 1;
        parent[0] = -1;

        for (int i = 1; i < N; i++) {
            // Pick random existing node.
            int const j = gen() % i;

            if (gen() % 6 == 0) {
                // Reparent j to i.
                num_trees += parent[j] != -1;
                parent[j] = i;
                parent[i] = -1;
            } else {
                // Add i to j.
                parent[i] = j;
            }
        }

        // Return whether i contains j.
        auto contains = [&] (int i, int j) {
            while (true) {
                if (j == i) {
                    return true;
                } else if (j == -1) {
                    return false;
                }
                j = parent[j];
            }
        };

        // Ask treeify to reconstruct the tree structure from the containment function.
        auto const tree = treeify(N, contains);

        // Bounds-check the result.
        for (auto i : tree.preorder) {
            ASSERT_LE(0, i);
            ASSERT_LE(i, N);
        }

        for (auto n : tree.num_children) {
            ASSERT_LE(0, n);
            ASSERT_LE(n, N - 1);
        }

        // Verify that each tree in the result is valid.
        int pos = 0;
        int num_loops = 0;
        while (pos < N) {
            ASSERT_TRUE(verify(parent, tree, pos, -1));
            num_loops++;
        }

        // Verify that the number of trees is valid - otherwise treeify() could cheat the
        // test by reporting a collection of disjoint trees each having a single node.
        ASSERT_EQ(num_loops, num_trees);
    }
}

} // namespace Inkscape::Util
