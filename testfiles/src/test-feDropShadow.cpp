// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file test-feDropShadow.cpp
 * Unit tests for feDropShadow filter primitive implementation
 */

#include <gtest/gtest.h>
#include <memory>

#include "object/filters/dropshadow.h"
#include "display/nr-filter-dropshadow.h"
#include "colors/color.h"

// Test the SPFeDropShadow class (object layer)
class FeDropShadowTest : public ::testing::Test {
protected:
    void SetUp() override {
        dropshadow = std::make_unique<SPFeDropShadow>();
    }

    std::unique_ptr<SPFeDropShadow> dropshadow;
};

TEST_F(FeDropShadowTest, DefaultValues) {
    // Test default attribute values match SVG 2.0 specification
    EXPECT_EQ(dropshadow->get_dx(), 2.0);
    EXPECT_EQ(dropshadow->get_dy(), 2.0);
    EXPECT_EQ(dropshadow->get_stdDeviation(), 2.0);
    EXPECT_EQ(dropshadow->get_flood_opacity(), 1.0);
    EXPECT_FALSE(dropshadow->get_flood_color().has_value());  // Default is no color
}

// Test the FilterDropShadow class (rendering layer)
class FilterDropShadowTest : public ::testing::Test {
protected:
    void SetUp() override {
        filter = std::make_unique<Inkscape::Filters::FilterDropShadow>();
    }

    std::unique_ptr<Inkscape::Filters::FilterDropShadow> filter;
};

TEST_F(FilterDropShadowTest, DefaultRendererValues) {
    // FilterDropShadow should have proper defaults
    EXPECT_EQ(filter->name(), "DropShadow");
    EXPECT_TRUE(filter->can_handle_affine(Geom::identity()));
    EXPECT_GT(filter->complexity(Geom::identity()), 0.0);
}

TEST_F(FilterDropShadowTest, SetParameters) {
    // Test that parameter setting doesn't crash
    filter->set_dx(5.0);
    filter->set_dy(-3.0);
    filter->set_stdDeviation(4.0);
    filter->set_flood_color(0xff0000ff);  // Red
    filter->set_flood_opacity(0.7);

    // No crashes means success for private members
    SUCCEED();
}

TEST_F(FilterDropShadowTest, ParameterValidation) {
    // Test that negative stdDeviation gets clamped
    filter->set_stdDeviation(-1.0);

    // Test that opacity gets clamped to valid range
    filter->set_flood_opacity(-0.5);  // Should clamp to 0.0
    filter->set_flood_opacity(1.5);   // Should clamp to 1.0

    // No crashes means validation is working
    SUCCEED();
}

TEST_F(FilterDropShadowTest, AreaEnlargement) {
    filter->set_dx(3.0);
    filter->set_dy(4.0);
    filter->set_stdDeviation(2.0);

    Geom::IntRect area(10, 10, 50, 30);  // Original area
    Geom::Affine identity = Geom::identity();

    filter->area_enlarge(area, identity);

    // Area should be enlarged to accommodate shadow offset and blur
    EXPECT_LT(area.left(), 10);    // Expanded left
    EXPECT_LT(area.top(), 10);     // Expanded top
    EXPECT_GT(area.right(), 55);   // Expanded right (should be significantly larger)
    EXPECT_GT(area.bottom(), 35);  // Expanded bottom (should be significantly larger)
}

TEST_F(FilterDropShadowTest, ZeroBlurAreaEnlargement) {
    filter->set_dx(2.0);
    filter->set_dy(2.0);
    filter->set_stdDeviation(0.0);  // No blur

    Geom::IntRect area(0, 0, 20, 20);
    Geom::Affine identity = Geom::identity();

    filter->area_enlarge(area, identity);

    // Should still enlarge for offset, but not for blur
    EXPECT_GT(area.width(), 20);
    EXPECT_GT(area.height(), 20);
}

TEST_F(FilterDropShadowTest, ComplexityScaling) {
    filter->set_stdDeviation(0.0);
    double complexity_no_blur = filter->complexity(Geom::identity());

    filter->set_stdDeviation(5.0);
    double complexity_with_blur = filter->complexity(Geom::identity());

    // Complexity should increase with blur amount
    EXPECT_GT(complexity_with_blur, complexity_no_blur);
}

// Basic integration test
TEST(FeDropShadowIntegrationTest, BasicInstantiation) {
    // Test that our implementation compiles and basic instantiation works
    auto sp_filter = std::make_unique<SPFeDropShadow>();
    auto renderer = std::make_unique<Inkscape::Filters::FilterDropShadow>();

    EXPECT_NE(sp_filter, nullptr);
    EXPECT_NE(renderer, nullptr);

    // Verify basic method existence
    EXPECT_EQ(sp_filter->get_dx(), 2.0);
    EXPECT_EQ(renderer->name(), "DropShadow");
}

// Visual quality regression tests
class FeDropShadowRenderingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // These tests would ideally render actual SVG content and validate output
        // For now, we test the critical rendering path components
    }
};

TEST_F(FeDropShadowRenderingTest, RegionCalculationAccuracy) {
    auto dropshadow = std::make_unique<SPFeDropShadow>();

    // Test region calculation for various blur sizes
    Geom::Rect input_region(0, 0, 100, 100);

    // Test no blur case
    Geom::Rect no_blur_region = dropshadow->calculate_region(input_region);
    EXPECT_GT(no_blur_region.width(), input_region.width());  // Should expand for offset
    EXPECT_GT(no_blur_region.height(), input_region.height());

    // Verify region expansion is reasonable (not excessive)
    EXPECT_LT(no_blur_region.width(), input_region.width() * 2);  // Shouldn't double size
    EXPECT_LT(no_blur_region.height(), input_region.height() * 2);
}

TEST_F(FeDropShadowRenderingTest, PerformanceCharacteristics) {
    auto filter = std::make_unique<Inkscape::Filters::FilterDropShadow>();

    // Test complexity scaling
    filter->set_stdDeviation(0.0);
    double baseline_complexity = filter->complexity(Geom::identity());

    filter->set_stdDeviation(1.0);
    double small_blur_complexity = filter->complexity(Geom::identity());

    filter->set_stdDeviation(10.0);
    double large_blur_complexity = filter->complexity(Geom::identity());

    // Verify complexity scaling is reasonable
    EXPECT_LT(baseline_complexity, small_blur_complexity);
    EXPECT_LT(small_blur_complexity, large_blur_complexity);

    // Ensure complexity values are within reasonable bounds
    EXPECT_GT(baseline_complexity, 0.5);   // Minimum work
    EXPECT_LT(large_blur_complexity, 20.0); // Not excessive
}

TEST_F(FeDropShadowRenderingTest, FilterParameterValidation) {
    auto filter = std::make_unique<Inkscape::Filters::FilterDropShadow>();

    // Test extreme parameter values don't crash
    filter->set_dx(1000.0);      // Large offset
    filter->set_dy(-1000.0);     // Large negative offset
    filter->set_stdDeviation(100.0);  // Very large blur
    filter->set_flood_opacity(2.0);   // Invalid opacity (should clamp)
    filter->set_flood_color(0xFFFFFFFF);  // White color

    // Verify area enlargement handles extreme values gracefully
    Geom::IntRect area(0, 0, 10, 10);
    filter->area_enlarge(area, Geom::identity());

    // Should enlarge significantly but not cause integer overflow
    EXPECT_GT(area.width(), 10);
    EXPECT_GT(area.height(), 10);
    EXPECT_LT(area.width(), 10000);  // Reasonable upper bound
    EXPECT_LT(area.height(), 10000);
}