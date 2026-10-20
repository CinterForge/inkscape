// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef OK_COLOR_H
#define OK_COLOR_H

#pragma once

// Copyright(c) 2021 Björn Ottosson
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this softwareand associated documentation files(the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and /or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions :
// The above copyright noticeand this permission notice shall be included in all
// copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

namespace ok_color {

struct Lab { float L; float a; float b; };
struct RGB { float r; float g; float b; };
struct HSV { float h; float s; float v; };
struct HSL { float h; float s; float l; };
struct LC { float L; float C; };

RGB okhsv_to_srgb(HSV hsv);
HSV srgb_to_okhsv(RGB rgb);
RGB okhsl_to_srgb(HSL hsv);

} // namespace ok_color


#endif //OK_COLOR_H
