// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SP_OBJECT_H_SEEN
#define SP_OBJECT_H_SEEN

/*
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *   Adrian Boguszewski
 *
 * Copyright (C) 1999-2016 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <set>
#include <glibmm/ustring.h>
#include "util/const_char_ptr.h"
#include "xml/node-observer.h"
/* SPObject flags */

class SPObject;

/* Async modification flags */
#define SP_OBJECT_MODIFIED_FLAG (1 << 0)
#define SP_OBJECT_CHILD_MODIFIED_FLAG (1 << 1)
#define SP_OBJECT_PARENT_MODIFIED_FLAG (1 << 2)
#define SP_OBJECT_STYLE_MODIFIED_FLAG (1 << 3)
#define SP_OBJECT_VIEWPORT_MODIFIED_FLAG (1 << 4)
#define SP_OBJECT_USER_MODIFIED_FLAG_A (1 << 5)
#define SP_OBJECT_USER_MODIFIED_FLAG_B (1 << 6)
#define SP_OBJECT_STYLESHEET_MODIFIED_FLAG (1 << 7)

/* Convenience */
#define SP_OBJECT_FLAGS_ALL 0xff

// Tags that can be passed along with other "modified" flags.
// Client code can use them to track senders of modification requests.
// Tags themselves do not signify any modification to the object(s).
#define SP_OBJECT_USER_MODIFIED_TAG_1 (1 << 8)
#define SP_OBJECT_USER_MODIFIED_TAG_2 (1 << 9)
#define SP_OBJECT_USER_MODIFIED_TAG_3 (1 << 10)
#define SP_OBJECT_USER_MODIFIED_TAG_4 (1 << 11)
#define SP_OBJECT_USER_MODIFIED_TAG_5 (1 << 12)
#define SP_OBJECT_USER_MODIFIED_TAG_6 (1 << 13)
#define SP_OBJECT_USER_MODIFIED_TAG_7 (1 << 14)
#define SP_OBJECT_USER_MODIFIED_TAG_8 (1 << 15)

#define SP_OBJECT_USER_TAGS_ALL 0xff00

/* Flags that mark object as modified */
/* Object, Child, Style, Viewport, User */
#define SP_OBJECT_MODIFIED_STATE (SP_OBJECT_FLAGS_ALL & ~(SP_OBJECT_PARENT_MODIFIED_FLAG))

/* Flags that will propagate downstreams */
/* Parent, Style, Viewport, User */
#define SP_OBJECT_MODIFIED_CASCADE ((SP_OBJECT_FLAGS_ALL | SP_OBJECT_USER_TAGS_ALL) & ~(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))
inline unsigned cascade_flags(unsigned flags)
{
    // Unset object-modified and child-modified, set parent-modified if object-modified.
    static_assert(SP_OBJECT_PARENT_MODIFIED_FLAG == SP_OBJECT_MODIFIED_FLAG << 2);
    return (flags & SP_OBJECT_MODIFIED_CASCADE) | (flags & SP_OBJECT_MODIFIED_FLAG) << 2;
}

/* Write flags */
#define SP_OBJECT_WRITE_BUILD (1 << 0)
#define SP_OBJECT_WRITE_EXT (1 << 1)
#define SP_OBJECT_WRITE_ALL (1 << 2)
#define SP_OBJECT_WRITE_NO_CHILDREN (1 << 3)

#include <vector>
#include <cassert>
#include <cstddef>
#include <boost/intrusive/list.hpp>
#include <2geom/point.h> // Used for dpi only
#include <sigc++/connection.h>
#include <sigc++/functors/slot.h>
#include <sigc++/signal.h>
#include "util/forward-pointer-iterator.h"
#include "tags.h"
#include "version.h"

enum class SPAttr;

class SPCSSAttr;
class SPStyle;

namespace Inkscape::XML { class Node; struct Document; }

/// Unused
struct SPCtx
{
    unsigned int flags;
};

enum
{
    SP_XML_SPACE_DEFAULT,
    SP_XML_SPACE_PRESERVE
};

class SPDocument;

/// Internal class consisting of two bits.
class SPIXmlSpace {
public:    
    SPIXmlSpace(): set(0), value(SP_XML_SPACE_DEFAULT) {};
    unsigned int set : 1;
    unsigned int value : 1;
};

/*
 * Refcounting
 *
 * Owner is here for debug reasons, you can set it to NULL safely
 * Ref should return object, NULL is error, unref return always NULL
 */

/**
 * Increase reference count of object, with possible debugging.
 *
 * @param owner If non-NULL, make debug log entry.
 * @return object, NULL is error.
 * \pre object points to real object
 * @todo need to move this to be a member of SPObject.
 */
SPObject *sp_object_ref(SPObject *object, SPObject *owner=nullptr);

/**
 * Decrease reference count of object, with possible debugging and
 * finalization.
 *
 * @param owner If non-NULL, make debug log entry.
 * @return always NULL
 * \pre object points to real object
 * @todo need to move this to be a member of SPObject.
 */
SPObject *sp_object_unref(SPObject *object, SPObject *owner=nullptr);

/**
 * SPObject is an abstract base class of all of the document nodes at the
 * SVG document level. Each SPObject subclass implements a certain SVG
 * element node type, or is an abstract base class for different node
 * types.  The SPObject layer is bound to the SPRepr layer, closely
 * following the SPRepr mutations via callbacks.  During creation,
 * SPObject parses and interprets all textual attributes and CSS style
 * strings of the SPRepr, and later updates the internal state whenever
 * it receives a signal about a change. The opposite is not true - there
 * are methods manipulating SPObjects directly and such changes do not
 * propagate to the SPRepr layer. This is important for implementation of
 * the undo stack, animations and other features.
 *
 * SPObjects are bound to the higher-level container SPDocument, which
 * provides document level functionality such as the undo stack,
 * dictionary and so on. Source: doc/architecture.txt
 */
class SPObject : private Inkscape::XML::NodeObserver
{
public:
    enum CollectionPolicy
    {
        COLLECT_WITH_PARENT,
        ALWAYS_COLLECT
    };
    enum class LinkedObjectNature
    {
        DEPENDENT = -1,
        ANY = 0,
        DEPENDENCY = 1,
    };

    SPObject();
    SPObject(SPObject const &) = delete;
    SPObject &operator=(SPObject const &) = delete;
    ~SPObject() override;
    virtual int tag() const { return tag_of<decltype(*this)>; }

    unsigned int cloned : 1;
    SPObject *clone_original{nullptr};
    unsigned int uflags : 16;
    unsigned int mflags : 16;
    SPIXmlSpace xml_space;
    unsigned int hrefcount{0};        /* number of xlink:href references */
    unsigned int _total_hrefcount{0}; /* our hrefcount + total descendants */
    SPDocument *document{nullptr};    /* Document we are part of */
    SPObject *parent{nullptr};        /* Our parent (only one allowed) */

private:
    char *id{nullptr};                  /* Our very own unique id */
    Inkscape::XML::Node *repr{nullptr}; /* Our xml representation */

    /**
     * The value of the `xml:lang` or `lang` attribute, if set.
     */
    std::optional<Glib::ustring> _lang_attribute;

public:
    int refCount{1};
    std::list<SPObject *> hrefList;

    /**
     * Returns the objects current ID string.
     */
    char const* getId() const;

    void getIds(std::set<std::string> &ret) const;

    /**
     * Get the id in a URL format.
     */
    std::string getUrl() const;

    /**
     * Get the value of the language attribute explicitly set on this object, if available.
     *
     * To get the effective language of this object, use getLanguage() instead.
     */
    std::optional<Glib::ustring> const &getLangAttribute() const { return _lang_attribute; }

    /**
     * Get the effective language of this object, either set by the `xml:lang`/`lang` attributes or inherited from
     * the parent.
     */
    char const *getLanguage() const;

    /**
     * Set the language of this object. Use std::nullopt to unset and inherit from the parent.
     */
    void setLanguage(std::optional<Glib::ustring> value) { _lang_attribute = std::move(value); }

    /**
     * Returns the XML representation of tree
     */
//protected:
    Inkscape::XML::Node * getRepr();

    /**
     * Returns the XML representation of tree
     */
    Inkscape::XML::Node const* getRepr() const;

public:

    /**
     * Cleans up an SPObject, releasing its references and
     * requesting that references to it be released
     */
    void releaseReferences();

    /**
     * Connects to the release request signal
     *
     * @param slot the slot to connect
     *
     * @return the sigc::connection formed
     */
    sigc::connection connectRelease(sigc::slot<void (SPObject *)> slot) {
        return _release_signal.connect(slot);
    }

    /**
     * Represents the style properties, whether from presentation attributes, the <tt>style</tt>
     * attribute, or inherited.
     *
     * Note that some non-SPItem SPObject's, such as SPStop, do need styling information,
     * and need to inherit properties even through other non-SPItem parents like \<defs\>.
     */
    SPStyle *style;

    /**
     * Represents the style that should be used to resolve 'context-fill' and 'context-stroke'
     */
    SPStyle *context_style;

    /// Switch containing next() method.
    struct ParentIteratorStrategy {
        static SPObject const *next(SPObject const *object) {
            return object->parent;
        }
    };

    typedef Inkscape::Util::ForwardPointerIterator<SPObject, ParentIteratorStrategy> ParentIterator;
    typedef Inkscape::Util::ForwardPointerIterator<SPObject const, ParentIteratorStrategy> ConstParentIterator;

    bool isSiblingOf(SPObject const *object) const {
        if (object == nullptr) return false;
        return this->parent && this->parent == object->parent;
    }

    /**
     * Get objects which are linked to this object as either a source or a target.
     *
     * @arg[out] objects - A list which is added to of all found links in this direction.
     * @arg direction - Which objects to include in the output
     */
    virtual void getLinked(std::vector<SPObject *> &objects, LinkedObjectNature direction = LinkedObjectNature::ANY) const;

    /**
     * Get objects which are linked, like above. But returns a new vector of objects.
     *
     * @arg direction - Which objects to include in the output
     * @returns A list of SPObjects directly linked to this object in the direction specified.
     */
    std::vector<SPObject *> getLinked(LinkedObjectNature direction = LinkedObjectNature::ANY) const {
        std::vector<SPObject *> ret;
        getLinked(ret, direction);
        return ret;
    }

    /**
     * True if object is non-NULL and this is some in/direct parent of object.
     */
    bool isAncestorOf(SPObject const *object) const;

    /**
     * Returns youngest object being parent to this and object.
     */
    SPObject const *nearestCommonAncestor(SPObject const *object) const;

    /**
     * Returns ancestor non layer.
     */
    SPObject const * getTopAncestorNonLayer() const;

    /* Returns next object in sibling list or NULL. */
    SPObject *getNext();

    /**
     * Returns previous object in sibling list or NULL.
     */
    SPObject *getPrev();

    bool hasChildren() const { return ( children.size() > 0 ); }

    SPObject *firstChild() { return children.empty() ? nullptr : &children.front(); }
    SPObject const *firstChild() const { return children.empty() ? nullptr : &children.front(); }

    SPObject *lastChild() { return children.empty() ? nullptr : &children.back(); }
    SPObject const *lastChild() const { return children.empty() ? nullptr : &children.back(); }

    SPObject *nthChild(unsigned index);
    SPObject const *nthChild(unsigned index) const;

    enum Action { ActionGeneral, ActionBBox, ActionUpdate, ActionShow };

    /**
     * Retrieves the children as a std vector object, optionally ref'ing the children
     * in the process, if add_ref is specified.
     */
    std::vector<SPObject*> childList(bool add_ref, Action action = ActionGeneral);


    /**
     * Retrieves a list of ancestors of the object, as an easy to use vector
     * @param root_to_tip - If set, orders the list from the svg root to the tip.
     */
    std::vector<SPObject*> ancestorList(bool root_to_tip);

    /**
     * Append repr as child of this object.
     * \pre this is not a cloned object
     */
    SPObject *appendChildRepr(Inkscape::XML::Node *repr);

    /**
     * Gets the author-visible label property for the object or a default if
     * no label is defined.
     */
    char const *label() const;

    /**
     * Returns a default label property for this object.
     */
    char const *defaultLabel() const;

    /**
     * Sets the author-visible label for this object.
     *
     * @param label the new label.
     */
    void setLabel(char const *label);

    /**
     * Returns the title of this object, or NULL if there is none.
     * The caller must free the returned string using g_free() - see comment
     * for getTitleOrDesc() below.
     */
    char *title() const;

    /**
     * Sets the title of this object.
     * A NULL first argument is interpreted as meaning that the existing title
     * (if any) should be deleted.
     * The second argument is optional - @see setTitleOrDesc() below for details.
     */
    bool setTitle(char const *title, bool verbatim = false);

    /**
     * Returns the description of this object, or NULL if there is none.
     * The caller must free the returned string using g_free() - see comment
     * for getTitleOrDesc() below.
     */
    char *desc() const;

    /**
     * Sets the description of this object.
     * A NULL first argument is interpreted as meaning that the existing
     * description (if any) should be deleted.
     * The second argument is optional - @see setTitleOrDesc() below for details.
     */
    bool setDesc(char const *desc, bool verbatim=false);

    /**
     * Get and set the exportable filename on this object. Usually sp-item or sp-page
     */
    Glib::ustring getExportFilename() const;
    void setExportFilename(Glib::ustring filename);

    /**
     * Get and set the exported DPI for this objet, if available.
     */
    Geom::Point getExportDpi() const;
    void setExportDpi(Geom::Point dpi);

    /**
     * Set the policy under which this object will be orphan-collected.
     *
     * Orphan-collection is the process of deleting all objects which no longer have
     * hyper-references pointing to them.  The policy determines when this happens.  Many objects
     * should not be deleted simply because they are no longer referred to; other objects (like
     * "intermediate" gradients) are more or less throw-away and should always be collected when no
     * longer in use.
     *
     * Along these lines, there are currently two orphan-collection policies:
     *
     *  COLLECT_WITH_PARENT - don't worry about the object's hrefcount;
     *                        if its parent is collected, this object
     *                        will be too
     *
     *  COLLECT_ALWAYS - always collect the object as soon as its
     *                   hrefcount reaches zero
     *
     * @return the current collection policy in effect for this object
     */
    CollectionPolicy collectionPolicy() const { return _collection_policy; }

    /**
     * Sets the orphan-collection policy in effect for this object.
     *
     * @param policy the new policy to adopt
     *
     * @see SPObject::collectionPolicy
     */
    void setCollectionPolicy(CollectionPolicy policy) {
        _collection_policy = policy;
    }

    /**
     * Requests a later automatic call to collectOrphan().
     *
     * This method requests that collectOrphan() be called during the document update cycle,
     * deleting the object if it is no longer used.
     *
     * If the current collection policy is COLLECT_WITH_PARENT, this function has no effect.
     *
     * @see SPObject::collectOrphan
     */
    void requestOrphanCollection();

    /**
     * Unconditionally delete the object if it is not referenced.
     *
     * Unconditionally delete the object if there are no outstanding hyper-references to it.
     * Observers are not notified of the object's deletion (at the SPObject level; XML tree
     * notifications still fire).
     *
     * @see SPObject::deleteObject
     */
    void collectOrphan() {
        if ( _total_hrefcount == 0 ) {
            deleteObject(false);
        }
    }

    /**
     * Increase weak refcount.
     *
     * Hrefcount is used for weak references, for example, to
     * determine whether any graphical element references a certain gradient
     * node.
     * It keeps a list of "owners".
     * @param owner Used to track who uses this object.
     */
    void hrefObject(SPObject* owner = nullptr);

    /**
     * Decrease weak refcount.
     *
     * Hrefcount is used for weak references, for example, to determine whether
     * any graphical element references a certain gradient node.
     * @param owner Used to track who uses this object.
     * \pre hrefcount>0
     */
    void unhrefObject(SPObject* owner = nullptr);

    /**
     * Check if object is referenced by any other object.
     */
    bool isReferenced() { return ( _total_hrefcount > 0 ); }

    /**
     * Deletes an object, unparenting it from its parent.
     *
     * Detaches the object's repr, and optionally sends notification that the object has been
     * deleted.
     *
     * @param propagate If it is set to true, it emits a delete signal.
     *
     * @param propagate_descendants If it is true, it recursively sends the delete signal to children.
     */
    void deleteObject(bool propagate, bool propagate_descendants);

    /**
     * Deletes on object.
     *
     * @param propagate Notify observers of this object and its children that they have been
     *                  deleted?
     */
    void deleteObject(bool propagate = true)
    {
        deleteObject(propagate, propagate);
    }

    /**
     * Removes all children except for the given object, it's children and it's ancesstors.
     */
    void cropToObject(SPObject *except);
    void cropToObjects(std::vector<SPObject *> except_objects);

    /**
     * Get all child objects except for any in the list.
     */
    void getObjectsExcept(std::vector<SPObject *> &objects, const std::vector<SPObject *> &except);

    /**
     * Grows the input list with all linked items recursively in both child nodes and links of links.
     *
     * @arg[out] objects - The list of objects to append to
     * @arg direction - see SPObject::getLinked direction arg.
     */
    void getLinkedRecursive(std::vector<SPObject *> &objects, LinkedObjectNature direction = LinkedObjectNature::ANY) const;

    /**
     * Connects a slot to be called when an object is deleted.
     *
     * This connects a slot to an object's internal delete signal, which is invoked when the object
     * is deleted
     *
     * The signal is mainly useful for e.g. knowing when to break hrefs or dissociate clones.
     *
     * @param slot the slot to connect
     *
     * @see SPObject::deleteObject
     */
    sigc::connection connectDelete(sigc::slot<void (SPObject *)> slot) {
        return _delete_signal.connect(slot);
    }

    sigc::connection connectPositionChanged(sigc::slot<void (SPObject *)> slot) {
        return _position_changed_signal.connect(slot);
    }

    /**
     * Returns the object which supercedes this one (if any).
     *
     * This is mainly useful for ensuring we can correctly perform a series of moves or deletes,
     * even if the objects in question have been replaced in the middle of the sequence.
     */
    SPObject *successor() { return _successor; }

    /**
     * Indicates that another object supercedes this one.
     */
    void setSuccessor(SPObject *successor) {
        assert(successor != NULL);
        assert(_successor == NULL);
        assert(successor->_successor == NULL);
        sp_object_ref(successor, nullptr);
        _successor = successor;
    }

    /**
     * Indicates that another object supercedes temporaty this one.
     */
    void setTmpSuccessor(SPObject *tmpsuccessor);

    /**
     * Unset object supercedes.
     */
    void unsetTmpSuccessor();

    /**
     * Fix temporary successors in duple stamp.
     */
    void fixTmpSuccessors();

    /* modifications; all three sets of methods should probably ultimately be protected, as they
     * are not really part of its public interface.  However, other parts of the code to
     * occasionally use them at present. */

    /* the no-argument version of updateRepr() is intended to be a bit more public, however -- it
     * essentially just flushes any changes back to the backing store (the repr layer); maybe it
     * should be called something else and made public at that point. */

    /**
     * Updates the object's repr based on the object's state.
     *
     *  This method updates the repr attached to the object to reflect the object's current
     *  state; see the three-argument version for details.
     *
     * @param flags object write flags that apply to this update
     *
     * @return the updated repr
     */
    Inkscape::XML::Node *updateRepr(unsigned int flags = SP_OBJECT_WRITE_EXT);

    /**
     * Updates the given repr based on the object's state.
     *
     * Used both to create reprs in the original document, and to create reprs
     * in another document (e.g. a temporary document used when saving as "Plain SVG".
     *
     *  This method updates the given repr to reflect the object's current state.  There are
     *  several flags that affect this:
     *
     *   SP_OBJECT_WRITE_BUILD - create new reprs
     *
     *   SP_OBJECT_WRITE_EXT   - write elements and attributes
     *                           which are not part of pure SVG
     *                           (i.e. the Inkscape and Sodipodi
     *                           namespaces)
     *
     *   SP_OBJECT_WRITE_ALL   - create all nodes and attributes,
     *                           even those which might be redundant
     *
     * @param repr the repr to update
     * @param flags object write flags that apply to this update
     *
     * @return the updated repr
     */
    Inkscape::XML::Node *updateRepr(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, unsigned int flags);

    /**
     * Queues an deferred update of this object's display.
     *
     *  This method sets flags to indicate updates to be performed later, during the idle loop.
     *
     *  There are several flags permitted here:
     *
     *   SP_OBJECT_MODIFIED_FLAG - the object has been modified
     *
     *   SP_OBJECT_CHILD_MODIFIED_FLAG - a child of the object has been
     *                                   modified
     *
     *   SP_OBJECT_STYLE_MODIFIED_FLAG - the object's style has been
     *                                   modified
     *
     *  There are also some subclass-specific modified flags which are hardly ever used.
     *
     *  One of either MODIFIED or CHILD_MODIFIED is required.
     *
     * @param flags flags indicating what to update
     */
    void requestDisplayUpdate(unsigned int flags);

    /**
     * Updates the object's display immediately
     *
     *  This method is called during the idle loop by SPDocument in order to update the object's
     *  display.
     *
     *  One additional flag is legal here:
     *
     *   SP_OBJECT_PARENT_MODIFIED_FLAG - the parent has been
     *                                    modified
     *
     * @param ctx an SPCtx which accumulates various state
     *             during the recursive update -- beware! some
     *             subclasses try to cast this to an SPItemCtx *
     *
     * @param flags flags indicating what to update (in addition
     *               to any already set flags)
     */
    void updateDisplay(SPCtx *ctx, unsigned int flags);

    /**
     * Requests that a modification notification signal
     * be emitted later (e.g. during the idle loop)
     *
     * Request modified always bubbles *up* the tree, as opposed to
     * request display update, which trickles down and relies on the
     * flags set during this pass...
     *
     * @param flags flags indicating what has been modified
     */
    void requestModified(unsigned int flags);

    /**
     *  Emits the MODIFIED signal with the object's flags.
     *  The object's mflags are the original set aside during the update pass for
     *  later delivery here.  Once emitModified() is called, those flags don't
     *  need to be stored any longer.
     *
     * @param flags indicating what has been modified.
     */
    void emitModified(unsigned int flags);

    /**
     * Connects to the modification notification signal
     *
     * @param slot the slot to connect
     *
     * @return the connection formed thereby
     */
    sigc::connection connectModified(
      sigc::slot<void (SPObject *, unsigned int)> slot
    ) {
        return _modified_signal.connect(slot);
    }

    /** Sends the delete signal to all children of this object recursively */
    void _sendDeleteSignalRecursive();

    /**
     * Adds increment to _total_hrefcount of object and its parents.
     */
    void _updateTotalHRefCount(int increment);

    void _requireSVGVersion(unsigned major, unsigned minor) { _requireSVGVersion(Inkscape::Version(major, minor)); }

    /**
     * Lifts SVG version of all root objects to version.
     */
    void _requireSVGVersion(Inkscape::Version version);

    sigc::signal<void (SPObject *)> _release_signal;
    sigc::signal<void (SPObject *)> _delete_signal;
    sigc::signal<void (SPObject *)> _position_changed_signal;
    sigc::signal<void (SPObject *, unsigned int)> _modified_signal;
    SPObject *_successor{nullptr};
    SPObject *_tmpsuccessor{nullptr};
    CollectionPolicy _collection_policy{SPObject::COLLECT_WITH_PARENT};
    char *_label{nullptr};
    mutable char *_default_label{nullptr};

    // WARNING:
    // Methods below should not be used outside of the SP tree,
    // as they operate directly on the XML representation.
    // In future, they will be made protected.

    /**
     * Put object into object tree, under parent, and behind prev;
     * also update object's XML space.
     */
    void attach(SPObject *object, SPObject *prev);

    /**
     * In list of object's children, move object behind prev.
     */
    void reorder(SPObject* obj, SPObject *prev);

    /**
     * Remove object from parent's children, release and unref it.
     */
    void detach(SPObject *object);

    /**
     * Return object's child whose node pointer equals repr.
     */
    SPObject *get_child_by_repr(Inkscape::XML::Node *repr);

    void invoke_build(SPDocument *document, Inkscape::XML::Node *repr, unsigned int cloned);

    int getIntAttribute(char const *key, int def);

    unsigned getPosition();

    char const * getAttribute(char const *name) const;

    void appendChild(Inkscape::XML::Node *child);

    void addChild(Inkscape::XML::Node *child,Inkscape::XML::Node *prev=nullptr);

    /**
     * Call virtual set() function of object.
     */
    void setKeyValue(SPAttr key, char const *value);


    void setAttribute(Inkscape::Util::const_char_ptr key,
                      Inkscape::Util::const_char_ptr value);

    void setAttributeDouble(Inkscape::Util::const_char_ptr key, double value);
    double getAttributeDouble(Inkscape::Util::const_char_ptr key, double def);

    void setAttributeOrRemoveIfEmpty(Inkscape::Util::const_char_ptr key,
                                     Inkscape::Util::const_char_ptr value) {
        this->setAttribute(key.data(),
                          (value.data() == nullptr || value.data()[0]=='\0') ? nullptr : value.data());
    }

    /**
     * Read value of key attribute from XML node into object.
     */
    void readAttr(char const *key);
    void readAttr(SPAttr keyid);

    char const *getTagName() const;

    void removeAttribute(char const *key);

    void setCSS(SPCSSAttr *css, char const *attr);

    void changeCSS(SPCSSAttr *css, char const *attr);

    bool storeAsDouble( char const *key, double *val ) const;

private:
    // Private member functions used in the definitions of setTitle(),
    // setDesc(), title() and desc().

    /**
     * Sets or deletes the title or description of this object.
     * A NULL 'value' argument causes the title or description to be deleted.
     *
     * 'verbatim' parameter:
     * If verbatim==true, then the title or description is set to exactly the
     * specified value.  If verbatim==false then two exceptions are made:
     *   (1) If the specified value is just whitespace, then the title/description
     *       is deleted.
     *   (2) If the specified value is the same as the current value except for
     *       mark-up, then the current value is left unchanged.
     * This is usually the desired behaviour, so 'verbatim' defaults to false for
     * setTitle() and setDesc().
     *
     * The return value is true if a change was made to the title/description,
     * and usually false otherwise.
     */
    bool setTitleOrDesc(char const *value, char const *svg_tagname, bool verbatim);

    /**
     * Returns the title or description of this object, or NULL if there is none.
     *
     * The SVG spec allows 'title' and 'desc' elements to contain text marked up
     * using elements from other namespaces.  Therefore, this function cannot
     * in general just return a pointer to an existing string - it must instead
     * construct a string containing the title or description without the mark-up.
     * Consequently, the return value is a newly allocated string (or NULL), and
     * must be freed (using g_free()) by the caller.
     */
    char * getTitleOrDesc(char const *svg_tagname) const;

    /**
     * Find the first child of this object with a given tag name,
     * and return it.  Returns NULL if there is no matching child.
     */
    SPObject * findFirstChild(char const *tagname) const;

    /**
     * Return the full textual content of an element (typically all the
     * content except the tags).
     * Must not be used on anything except elements.
     */
    Glib::ustring textualContent() const;

    /* Real handlers of repr signals */

private:
    // XML::NodeObserver functions
    void notifyAttributeChanged(Inkscape::XML::Node &node, GQuark key, Inkscape::Util::ptr_shared oldval,
                                Inkscape::Util::ptr_shared newval) final;

    void notifyContentChanged(Inkscape::XML::Node &node, Inkscape::Util::ptr_shared oldcontent,
                              Inkscape::Util::ptr_shared newcontent) final;

    void notifyChildAdded(Inkscape::XML::Node &node, Inkscape::XML::Node &child,
                          Inkscape::XML::Node *prev) final;

    void notifyChildRemoved(Inkscape::XML::Node &node, Inkscape::XML::Node &child,
                            Inkscape::XML::Node *prev) final;

    void notifyChildOrderChanged(Inkscape::XML::Node &node, Inkscape::XML::Node &child, Inkscape::XML::Node *old_prev,
                                 Inkscape::XML::Node *new_prev) final;

    void notifyElementNameChanged(Inkscape::XML::Node &node, GQuark old_name, GQuark new_name) final;

    friend class SPObjectImpl;

protected:
    virtual void build(SPDocument *doc, Inkscape::XML::Node *repr);
    virtual void release();

    virtual void child_added(Inkscape::XML::Node *child, Inkscape::XML::Node *ref);
    virtual void remove_child(Inkscape::XML::Node *child);

    virtual void order_changed(Inkscape::XML::Node *child, Inkscape::XML::Node *old_repr,
                               Inkscape::XML::Node *new_repr);
    virtual void position_changed(SPObject *prev);
    virtual void tag_name_changed(gchar const *oldname, gchar const *newname);

    virtual void set(SPAttr key, const char *value);

    virtual void update(SPCtx *ctx, unsigned int flags);
    virtual void modified(unsigned int flags);

    virtual Inkscape::XML::Node *write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, unsigned int flags);

    typedef boost::intrusive::list_member_hook<> ListHook;
    ListHook _child_hook;

public:
    using ChildrenList = boost::intrusive::list<
        SPObject,
        boost::intrusive::member_hook<
            SPObject,
            ListHook,
            &SPObject::_child_hook
        >>;
    ChildrenList children;
    virtual void read_content();

    void recursivePrintTree(unsigned level = 0); // For debugging
    void objectTrace(std::string const &, bool in = true, unsigned flags = 0);

    /**
     * @brief Generate a document-wide unique id for this object.
     *
     * Returns an id string not in use by any object within the object's document.
     * If default_id is specified, it will be returned if possible.
     * Otherwise, an id will be generated based on the object's name.
     */
    std::string generate_unique_id(char const *default_id = nullptr) const;
};

std::ostream &operator<<(std::ostream &out, const SPObject &o);

/**
 * Compares height of objects in tree.
 *
 * Works for different-parent objects, so long as they have a common ancestor.
 * \return \verbatim
 *    0    positions are equivalent
 *    1    first object's position is greater than the second
 *   -1    first object's position is less than the second   \endverbatim
 */
int sp_object_compare_position(SPObject const *first, SPObject const *second);
bool sp_object_compare_position_bool(SPObject const *first, SPObject const *second);

#endif // SP_OBJECT_H_SEEN

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
