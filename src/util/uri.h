// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * URI functions as per 4.3.4 of CSS 2.1
 *   http://www.w3.org/TR/CSS21/syndata.html#uri
 *
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2006-2024 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_UTIL_URI_H
#define SEEN_UTIL_URI_H

#include <optional>
#include <string>
#include <tuple>

enum class Base64Data
{
    NONE,
    RASTER,
    SVG,
};

/**
 * Parse functional URI notation, as per 4.3.4 of CSS 2.1
 *
 * http://www.w3.org/TR/CSS21/syndata.html#uri
 *
 * > The format of a URI value is 'url(' followed by optional white space
 * > followed by an optional single quote (') or double quote (") character
 * > followed by the URI itself, followed by an optional single quote (')
 * > or double quote (") character followed by optional white space
 * > followed by ')'. The two quote characters must be the same.
 *
 * Example:
 * \verbatim
   url = extract_uri("url('foo')bar", &out);
   -> url == "foo"
   -> out == "bar"
   \endverbatim
 *
 * @param s String which starts with "url("
 * @param[out] endptr points to \c s + N, where N is the number of characters parsed or to a nullptr when an invalid URL is found
 * @return URL string, or empty string on failure
 */
std::string extract_uri(char const *s, char const **endptr = nullptr);

/**
 * @brief Try extracting URI from "url(xyz)" string using extract_uri
 * 
 * @param url string input that may or may not be a link
 * @return Extracted non-empty link or no value if provided input is not an URI
 */
std::optional<std::string> try_extract_uri(const char* url);

/**
 * @brief Try extracting the object id from "url(#obj_id)" string using extract_uri
 * 
 * @param url string input that may or may not be a link
 * @return Extracted non-empty objectid or no value if provided input is not an URI or not an id
 */
std::optional<std::string> try_extract_uri_id(const char* url);

/**
 * @brief Attempt to extract the data in a data uri, but does not decode the base64.
 *
 * @param uri_data pointer to the uri data encoded in base64
 * @param base64_type the detected base64 type based on the mime-type.
 *
 * @return The same uri_data pointer advanced to just after the uri components.
 */
std::tuple<char const *, Base64Data> extract_uri_data(char const *uri_data);

#endif /* !SEEN_UTIL_URI_H */

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
