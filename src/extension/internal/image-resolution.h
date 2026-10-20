// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Daniel Wagenaar <daw@caltech.edu>
 *
 * Copyright (C) 2012 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */


#ifndef IMAGE_RESOLUTION_H
#define IMAGE_RESOLUTION_H

namespace Inkscape::Extension::Internal {

class ImageResolution
{
public:
    explicit ImageResolution(char const *fn);

    bool ok() const { return ok_; }
    double x() const { return x_; }
    double y() const { return y_; }

private:
    bool ok_;
    double x_;
    double y_;

    void readpng(char const *fn);
    void readexif(char const *fn);
    void readexiv(char const *fn);
    void readjfif(char const *fn);
    void readmagick(char const *fn);
};

} // namespace Inkscape::Extension::Internal

#endif // IMAGE_RESOLUTION_H
