// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_MARKER_H
#define SEEN_SP_MARKER_H

/*
 * SVG <marker> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2003 Lauris Kaplinski
 * Copyright (C) 2008      Johan Engelen
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
/*
 * This is quite similar in logic to <svg>
 * Maybe we should merge them somehow (Lauris)
 */

class SPMarkerView;

#include <map>

#include <2geom/rect.h>
#include <2geom/affine.h>

#include "enums.h"
#include "svg/svg-length.h"
#include "svg/svg-angle.h"
#include "sp-item-group.h"
#include "uri-references.h"
#include "viewbox.h"

enum markerOrient {
  MARKER_ORIENT_ANGLE,
  MARKER_ORIENT_AUTO,
  MARKER_ORIENT_AUTO_START_REVERSE
};

class SPMarker final : public SPGroup, public SPViewBox {
public:
	SPMarker();
	~SPMarker() override;
	int tag() const override { return tag_of<decltype(*this)>; }

	/* units */
	unsigned int markerUnits_set : 1;
	unsigned int markerUnits : 1;

	/* reference point */
	SVGLength refX;
	SVGLength refY;

	/* dimensions */
	SVGLength markerWidth;
	SVGLength markerHeight;

	/* orient */
	unsigned int orient_set : 1;
	markerOrient orient_mode : 2;
	SVGAngle orient;

    Geom::Affine get_marker_transform(Geom::Affine const &base, double linewidth, bool for_display = false) const;

	/* Private views indexed by key that corresponds to a
	 * particular marker type (start, mid, end) on a particular
	 * path. SPMarkerView is a wrapper for a vector of pointers to
	 * Inkscape::DrawingItem instances, one pointer for each
	 * rendered marker.
	 */
	std::map<unsigned int, SPMarkerView> views_map;

	void build(SPDocument *document, Inkscape::XML::Node *repr) override;
	void release() override;
	void set(SPAttr key, gchar const* value) override;
	void update(SPCtx *ctx, guint flags) override;
	Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) override;

	Inkscape::DrawingItem* show(Inkscape::Drawing &drawing, unsigned int key, unsigned int flags) override;
	virtual Inkscape::DrawingItem* private_show(Inkscape::Drawing &drawing, unsigned int key, unsigned int flags);
	void hide(unsigned int key) override;

	void print(SPPrintContext *ctx) override;
};


class SPMarkerReference : public Inkscape::URIReference {
public:
	SPMarkerReference(SPObject *obj) : URIReference(obj) {}
	SPMarker *getObject() const {
		return static_cast<SPMarker *>(URIReference::getObject());
	}
protected:
	bool _acceptObject(SPObject *obj) const override {
		return is<SPMarker>(obj) && URIReference::_acceptObject(obj);
	}
};

void sp_validate_marker(SPMarker *sp_marker, SPDocument *doc);
void sp_marker_show_dimension (SPMarker *marker, unsigned int key, unsigned int size);
Inkscape::DrawingItem *sp_marker_show_instance (SPMarker *marker, Inkscape::DrawingItem *parent,
				      unsigned int key, unsigned int pos, unsigned int z_order,
				      Geom::Affine const &marker_transform, float linewidth);
void sp_marker_hide (SPMarker *marker, unsigned int key);
const char *generate_marker (std::vector<Inkscape::XML::Node*> &reprs, Geom::Rect bounds, SPDocument *document, Geom::Point center, Geom::Affine move);
SPObject *sp_marker_fork_if_necessary(SPObject *marker);

void sp_marker_set_orient(SPMarker* marker, const char* value);
void sp_marker_set_size(SPMarker* marker, double sx, double sy);
void sp_marker_scale_with_stroke(SPMarker* marker, bool scale_with_stroke);
void sp_marker_set_offset(SPMarker* marker, double dx, double dy);
void sp_marker_set_uniform_scale(SPMarker* marker, bool uniform);
void sp_marker_flip_horizontally(SPMarker* marker);
void sp_marker_set_opacity(SPMarker* marker, double alpha);

#endif
