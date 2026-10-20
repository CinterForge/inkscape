// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef _SEEN_COPY_RESOURCE_H_
#define _SEEN_COPY_RESOURCE_H_

class SPDocument;
class SPObject;

/**
 * @brief 
 * Copy source resource form one document into another destination document.
 * Resources are elements inside "def" element (gradients, markers, etc).
 * Also copy any href-ed objects or styles referencing other objects (like gradients).
 * In this sense it is a deep copy.
 * 
 * @param source 
 * @param dest_document 
 * @return SPObject* 
 */
SPObject* sp_copy_resource(const SPObject* source, SPDocument* dest_document);

#endif // _SEEN_COPY_RESOURCE_H_