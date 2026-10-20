// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef VARIANT_VISITOR_H_SEEN
#define VARIANT_VISITOR_H_SEEN

// Utility template for type matching. Useful for std::visit():
//
// std::visit(VariantVisitor{
//   [](Type1& t){...},
//   [](Type2& t){...},
//   val);

namespace Inkscape {

template <typename... Fs>
struct VariantVisitor : Fs...
{
    using Fs::operator()...;
};

} // namespace

#endif // VARIANT_VISITOR_H_SEEN
