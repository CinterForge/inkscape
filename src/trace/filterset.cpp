// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Some filters for Potrace in Inkscape
 *
 * Authors:
 *   Bob Jamison <rjamison@titan.com>
 *   Stéphane Gimenez <dev@gim.name>
 *
 * Copyright (C) 2004-2006 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include <cstdio>
#include "imagemap-gdk.h"
#include "filterset.h"
#include "quantize.h"

namespace Inkscape {
namespace Trace {

/*#########################################################################
### G A U S S I A N  (smoothing)
#########################################################################*/

static int gaussMatrix[] =
{
    2,  4,  5,  4, 2,
    4,  9, 12,  9, 4,
    5, 12, 15, 12, 5,
    4,  9, 12,  9, 4,
    2,  4,  5,  4, 2
};

GrayMap grayMapGaussian(GrayMap const &me) // Todo: Make member function, keep implementation here
{
    int width  = me.width;
    int height = me.height;
    int firstX = 2;
    int lastX  = width - 3;
    int firstY = 2;
    int lastY  = height - 3;

    auto newGm = GrayMap(width, height);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // image boundaries
            if (x < firstX || x > lastX || y < firstY || y > lastY) {
                newGm.setPixel(x, y, me.getPixel(x, y));
                continue;
            }

            // all other pixels
            int gaussIndex = 0;
            unsigned long sum = 0;
            for (int i = y - 2; i <= y + 2; i++) {
                for (int j = x - 2; j <= x + 2; j++) {
                    int weight = gaussMatrix[gaussIndex++];
                    sum += me.getPixel(j, i) * weight;
                }
            }
            sum /= 159;
            sum = std::min(sum, GrayMap::WHITE);
            newGm.setPixel(x, y, sum);
        }
    }

    return newGm;
}

RgbMap rgbMapGaussian(RgbMap const &me)
{
    int width  = me.width;
    int height = me.height;
    int firstX = 2;
    int lastX  = width-3;
    int firstY = 2;
    int lastY  = height-3;

    auto newGm = RgbMap(width, height);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // image boundaries
            if (x < firstX || x > lastX || y < firstY || y > lastY) {
                newGm.setPixel(x, y, me.getPixel(x, y));
                continue;
            }

            // all other pixels
            int gaussIndex = 0;
            int sumR       = 0;
            int sumG       = 0;
            int sumB       = 0;
            for (int i = y - 2; i <= y + 2; i++) {
                for (int j = x - 2; j <= x + 2; j++) {
                    int weight = gaussMatrix[gaussIndex++];
                    RGB rgb = me.getPixel(j, i);
                    sumR += weight * rgb.r;
                    sumG += weight * rgb.g;
                    sumB += weight * rgb.b;
                }
	        }
            RGB rout;
            rout.r = (sumR / 159) & 0xff;
            rout.g = (sumG / 159) & 0xff;
            rout.b = (sumB / 159) & 0xff;
            newGm.setPixel(x, y, rout);
	    }
	}

    return newGm;
}

/*#########################################################################
### C A N N Y    E D G E    D E T E C T I O N
#########################################################################*/

static int sobelX[] =
{
    -1,  0,  1 ,
    -2,  0,  2 ,
    -1,  0,  1 
};

static int sobelY[] =
{
     1,  2,  1 ,
     0,  0,  0 ,
    -1, -2, -1 
};

/**
 * Perform Sobel convolution on a GrayMap.
 */
GrayMap grayMapCanny(GrayMap const &gm, double dLowThreshold, double dHighThreshold)
{
    int width  = gm.width;
    int height = gm.height;
    int firstX = 1;
    int lastX  = width - 2;
    int firstY = 1;
    int lastY  = height - 2;

    auto map = GrayMap(width, height);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            bool edge;
            // image boundaries
            if (x < firstX || x > lastX || y < firstY || y > lastY) {
                edge = false;
            } else {
                // SOBEL FILTERING
                long sumX = 0;
                long sumY = 0;
                int sobelIndex = 0;
                for (int i = y-1; i <= y + 1; i++) {
                    for (int j = x - 1; j <= x + 1; j++) {
                        sumX += gm.getPixel(j, i) * sobelX[sobelIndex++];
                    }
	            }

                sobelIndex = 0;
                for (int i = y - 1; i <= y + 1; i++) {
                    for (int j = x - 1; j <= x + 1; j++) {
                        sumY += gm.getPixel(j, i) * sobelY[sobelIndex++];
                    }
	            }

                // GET VALUE
                unsigned long sum = std::abs(sumX) + std::abs(sumY);
                sum = std::min(sum, GrayMap::WHITE);

                // GET EDGE DIRECTION (fast way)
                int edgeDirection = 0; // x, y = 0
                if (sumX == 0) {
                    if (sumY != 0) {
                        edgeDirection = 90;
                    }
                } else {
                    long slope = sumY * 1024 / sumX;
                    if (slope > 2472 || slope< -2472) { // tan(67.5) * 1024
                        edgeDirection = 90;
                    } else if (slope > 414) { // tan(22.5) * 1024
                        edgeDirection = 45;
                    } else if (slope < -414) { // -tan(22.5) * 1024
                        edgeDirection = 135;
                    }
                }

                // printf("%ld %ld %f %d\n", sumX, sumY, orient, edgeDirection);

                // Get two adjacent pixels in edge direction
                unsigned long leftPixel;
                unsigned long rightPixel;
                if (edgeDirection == 0) {
                    leftPixel  = gm.getPixel(x - 1, y);
                    rightPixel = gm.getPixel(x + 1, y);
                } else if (edgeDirection == 45) {
                    leftPixel  = gm.getPixel(x - 1, y + 1);
                    rightPixel = gm.getPixel(x + 1, y - 1);
                } else if (edgeDirection == 90) {
                    leftPixel  = gm.getPixel(x, y - 1);
                    rightPixel = gm.getPixel(x, y + 1);
                } else { // 135
                    leftPixel  = gm.getPixel(x - 1, y - 1);
                    rightPixel = gm.getPixel(x + 1, y + 1);
                }

                // Compare current value to adjacent pixels. (If less than either, suppress it.)
                if (sum < leftPixel || sum < rightPixel) {
                    edge = false;
                } else {
                    unsigned long highThreshold = dHighThreshold * GrayMap::WHITE;
                    unsigned long lowThreshold  = dLowThreshold  * GrayMap::WHITE;
                    if (sum >= highThreshold) {
                        edge = true;
                    } else if (sum < lowThreshold) {
                        edge = false;
                    } else {
                        edge = gm.getPixel(x - 1, y - 1) > highThreshold ||
                               gm.getPixel(x    , y - 1) > highThreshold ||
                               gm.getPixel(x + 1, y - 1) > highThreshold ||
                               gm.getPixel(x - 1, y    ) > highThreshold ||
                               gm.getPixel(x + 1, y    ) > highThreshold ||
                               gm.getPixel(x - 1, y + 1) > highThreshold ||
                               gm.getPixel(x    , y + 1) > highThreshold ||
                               gm.getPixel(x + 1, y + 1) > highThreshold;
                    }
                }
            }

            // show edges as dark over light
            map.setPixel(x, y, edge ? GrayMap::BLACK : GrayMap::WHITE);
        }
    }

    // map.writePPM("canny.ppm");
    return map;
}

/*#########################################################################
### Q U A N T I Z A T I O N
#########################################################################*/

GrayMap quantizeBand(RgbMap const &rgbMap, int nrColors)
{
    auto gaussMap = rgbMapGaussian(rgbMap);
    // gaussMap->writePPM(gaussMap, "rgbgauss.ppm");

    auto qMap = rgbMapQuantize(gaussMap, nrColors);
    // qMap->writePPM(qMap, "rgbquant.ppm");

    auto gm = GrayMap(rgbMap.width, rgbMap.height);

    // RGB is quantized. There should now be a small set of (R+G+B)
    for (int y = 0; y < qMap.height; y++) {
        for (int x = 0; x < qMap.width; x++) {
            auto rgb = qMap.getPixelValue(x, y);
            int sum = rgb.r + rgb.g + rgb.b;
            auto result = (sum & 1) ? GrayMap::WHITE : GrayMap::BLACK;
            // printf("%d %d %d : %d\n", rgb.r, rgb.g, rgb.b, index);
            gm.setPixel(x, y, result);
        }
    }

    return gm;
}

} // namespace Trace
} // namespace Inkscape
