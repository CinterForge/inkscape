// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include <glibmm.h>

#include "attributes.h"
#include "display/cairo-utils.h"
#include "display/drawing-paintserver.h"

#include "sp-mesh-gradient.h"

/*
 * Mesh Gradient
 */
//#define MESH_DEBUG
//#define OBJECT_TRACE

SPMeshGradient::SPMeshGradient() : SPGradient(), type( SP_MESH_TYPE_COONS ), type_set(false) {
#ifdef OBJECT_TRACE
  objectTrace( "SPMeshGradient::SPMeshGradient" );
#endif

    // Start coordinate of mesh
    this->x.unset(SVGLength::NONE, 0.0, 0.0);
    this->y.unset(SVGLength::NONE, 0.0, 0.0);

#ifdef OBJECT_TRACE
  objectTrace( "SPMeshGradient::SPMeshGradient", false );
#endif
}

SPMeshGradient::~SPMeshGradient() {
#ifdef OBJECT_TRACE
  objectTrace( "SPMeshGradient::~SPMeshGradient (empty function)" );
  objectTrace( "SPMeshGradient::~SPMeshGradient", false );
#endif
}

void SPMeshGradient::build(SPDocument *document, Inkscape::XML::Node *repr) {
#ifdef OBJECT_TRACE
  objectTrace( "SPMeshGradient::build" );
#endif

    SPGradient::build(document, repr);

    // Start coordinate of meshgradient
    this->readAttr(SPAttr::X);
    this->readAttr(SPAttr::Y);

    this->readAttr(SPAttr::TYPE);

#ifdef OBJECT_TRACE
    objectTrace( "SPMeshGradient::build", false );
#endif
}


void SPMeshGradient::set(SPAttr key, gchar const *value) {
#ifdef OBJECT_TRACE
  objectTrace( "SPMeshGradient::set" );
#endif

    switch (key) {
        case SPAttr::X:
            if (!this->x.read(value)) {
                this->x.unset(SVGLength::NONE, 0.0, 0.0);
            }

            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;

        case SPAttr::Y:
            if (!this->y.read(value)) {
                this->y.unset(SVGLength::NONE, 0.0, 0.0);
            }

            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;

        case SPAttr::TYPE:
	    if (value) {
	      if (!strcmp(value, "coons")) {
		this->type = SP_MESH_TYPE_COONS;
	      } else if (!strcmp(value, "bicubic")) {
		this->type = SP_MESH_TYPE_BICUBIC;
	      } else {
		std::cerr << "SPMeshGradient::set(): invalid value " << value << std::endl;
	      }
	      this->type_set = TRUE;
	    } else {
	      // std::cout << "SPMeshGradient::set() No value " << std::endl;
	      this->type = SP_MESH_TYPE_COONS;
	      this->type_set = FALSE;
	    }

            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;

        default:
            SPGradient::set(key, value);
            break;
    }

#ifdef OBJECT_TRACE
    objectTrace( "SPMeshGradient::set", false );
#endif
}

/**
 * Write mesh gradient attributes to associated repr.
 */
Inkscape::XML::Node* SPMeshGradient::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
#ifdef OBJECT_TRACE
    objectTrace( "SPMeshGradient::write", false );
#endif

    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = xml_doc->createElement("svg:meshgradient");
    }

    if ((flags & SP_OBJECT_WRITE_ALL) || this->x._set) {
    	repr->setAttributeSvgDouble("x", this->x.computed);
    }

    if ((flags & SP_OBJECT_WRITE_ALL) || this->y._set) {
    	repr->setAttributeSvgDouble("y", this->y.computed);
    }

    if ((flags & SP_OBJECT_WRITE_ALL) || this->type_set) {
        switch (this->type) {
	    case SP_MESH_TYPE_COONS:
	      repr->setAttribute("type", "coons");
	      break;
	    case SP_MESH_TYPE_BICUBIC:
	      repr->setAttribute("type", "bicubic");
	      break;
	    default:
	      // Do nothing
	      break;
	}
    }

    SPGradient::write(xml_doc, repr, flags);

#ifdef OBJECT_TRACE
    objectTrace( "SPMeshGradient::write", false );
#endif
    return repr;
}

std::vector<std::vector<SPGradientPatch>> SPMeshGradient::getGradientPatches() const
{
    SPMeshNodeArray* my_array = &array;

    if (type_set) {
        switch (type) {
        case SP_MESH_TYPE_COONS:
            // std::cout << "SPMeshGradient::pattern_new: Coons" << std::endl;
            break;
        case SP_MESH_TYPE_BICUBIC:
            array.bicubic(&array_smoothed, type);
            my_array = &array_smoothed;
            break;
        }
    }

    int rows = my_array->patch_rows();
    int cols = my_array->patch_columns();

    std::vector<std::vector<SPGradientPatch>> patchdata;
    patchdata.resize(rows);
    for (auto &row : patchdata) {
        row.resize(cols);
    }

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            auto patch = SPMeshPatchI(&my_array->nodes, i, j);
            auto &data = patchdata[i][j];

            for (int x = 0; x < 4; x++) {
                for (int y = 0; y < 4; y++) {
                    data.points[x][y] = patch.getPoint(x, y);
                }
            }

            for (int k = 0; k < 4; k++) {
                #ifdef DEBUG_MESH
                    std::cout << i << " " << j << " " << patch.getPathType(k) << "  (";
                    for (int p = 0; p < 4; p++) {
                        std::cout << patch.getPoint(k, p);
                    }
                    std::cout << ") " << patch.getColor(k).toString() << std::endl;
                #endif

                data.pathtype[k] = patch.getPathType(k);

                if (patch.tensorIsSet(k)) {
                    data.tensorIsSet[k] = true;
                    data.tensorpoints[k] = patch.getTensorPoint(k);
                    //auto t = patch.getTensorPoint(k);
                    //std::cout << "  sp_mesh_create_pattern: tensor " << k
                    //          << " set to " << t << "." << std::endl;
                } else {
                    data.tensorIsSet[k] = false;
                    //auto t = patch.coonsTensorPoint(k);
                    //std::cout << "  sp_mesh_create_pattern: tensor " << k
                    //          << " calculated as " << t << "." << std::endl;
                }
                data.color[k] = patch.getColor(k);
            }
        }
    }
    return patchdata;
}
