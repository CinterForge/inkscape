// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Singleton class to access the preferences file in a convenient way.
 */
/* Authors:
 *   Krzysztof Kosi_ski <tweenk.pl@gmail.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2008,2009 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_PREFSTORE_H
#define INKSCAPE_PREFSTORE_H

#include <climits>
#include <cfloat>
#include <functional>
#include <glibmm/ustring.h>
#include <map>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>
#include <2geom/point.h>

#include "xml/repr.h"
#include "util/scope_exit.h"

class SPCSSAttr;

namespace Inkscape {

namespace Colors { class Color; }
class ErrorReporter;

/**
 * Preference storage class.
 *
 * This is a singleton that allows one to access the user preferences stored in
 * the preferences.xml file. The preferences are stored in a file system-like
 * hierarchy. They are generally typeless - it's up to the programmer to ensure
 * that a given preference is always accessed as the correct type. The backend
 * is not guaranteed to be tolerant to type mismatches.
 *
 * Preferences are identified by paths similar to file system paths. Components
 * of the path are separated by a slash (/). As an additional requirement,
 * the path must start with a slash, and not contain a trailing slash.
 * An example of a correct path would be "/options/some_group/some_option".
 *
 * All preferences are loaded when the first singleton pointer is requested.
 * To save the preferences, the method save() or the static function unload()
 * can be used.
 *
 * In future, this will be a virtual base from which specific backends
 * derive (e.g. GConf, flat XML file...)
 */
class Preferences {
    class _ObserverData;

public:
    // #############################
    // ## inner class definitions ##
    // #############################

    class Entry;
    class Observer;

    /**
     * Base class for preference observers.
     *
     * If you want to watch for changes in the preferences, you can
     * derive a class from this one and override the notify() method.
     *
     * Note that nowadays there are other ways to listen for preference changes,
     * which do not require creating a new subclass: see PreferencesObserver and
     * Pref<T> below, ready-to-go classes that support callbacks on pref change.
     */
    class Observer {
        friend class Preferences;

    public:

        /**
         * Constructor.
         *
         * Since each Observer is assigned to a single path, the base
         * constructor takes this path as an argument. This prevents one from
         * adding a single observer to multiple paths, but this is intentional
         * to simplify the implementation of observers and notifications.
         *
         * After you add the object with Preferences::addObserver(), you will
         * receive notifications for everything below the attachment point.
         * You can also specify a single preference as the watch point.
         * For example, watching the directory "/foo" will give you notifications
         * about "/foo/some_pref" as well as "/foo/some_dir/other_pref".
         * Watching the preference "/options/some_group/some_option" will only
         * generate notifications when this single preference changes.
         *
         * @param path Preference path the observer should watch.
         */
        Observer(Glib::ustring path);
        virtual ~Observer();

        /**
         * Notification about a preference change.
         *
         * @param new_val  Entry object containing information about
         *                 the modified preference.
         */
        virtual void notify(Preferences::Entry const &new_val) = 0;

        Glib::ustring const observed_path; ///< Path which the observer watches
    private:
        std::unique_ptr<_ObserverData> _data; ///< additional data used by the implementation while the observer is active
    };

    /**
     * Callback-based preferences observer
    */
    class PreferencesObserver : Observer {
    public:
        static std::unique_ptr<PreferencesObserver> create(Glib::ustring path, std::function<void (const Preferences::Entry& new_value)> callback);
        ~PreferencesObserver() override = default;

        /**
         * Manually call the observer with the original, unchanged value.
         * This is useful for initialisation functions.
         */
        void call();
    private:
        void call_recursive(Glib::ustring const &path);

        PreferencesObserver(Glib::ustring path, std::function<void (const Preferences::Entry& new_value)> callback);
        void notify(Preferences::Entry const& new_val) override;
        std::function<void (const Preferences::Entry&)> _callback;
    };

    /**
     * Data type representing a typeless value of a preference.
     *
     * This is passed to the observer in the notify() method.
     *
     * Most methods of @c Entry return the preference value at the time the @c Entry was created.
     * In a few special cases, values are read "on-demand" from the @c Preferences, which is ugly but currently hard to
     * avoid. To avoid surprising behavior, users should drop *all* previously obtained @c Entry variables after any
     * write to @c Preferences . The only exception is @c Preferences itself, which uses @c Entry for caching but takes
     * care to remove all outdated entries.
     */
    class Entry
    {
    public:
        /**
         * Construct new Entry
         * @param path Preferences path, e.g., "/options/some_group/some_option"
         * @param value Raw value string from preferences XML file, or @c std::nullopt if no preference exists for the
         * given path.
         */
        Entry(Glib::ustring path, std::optional<Glib::ustring> value)
            : _pref_path(std::move(path))
            , _value(std::move(value))
        {}

        ~Entry() = default;
        Entry() {} // needed to enable use in maps
        Entry(Entry const &other) = default;

        /**
         * Check whether the received entry is set.
         * This means that the requested preference path exists.
         *
         * If not, then the get...() functions will return the default value.
         *
         * @return True if the entry exists.
         */
        bool isSet() const { return _value.has_value(); }

        /**
         * Check if the preference value can be interpreted as a Boolean.
         */
        bool isValidBool() const;

        /**
         * Check if the preference value can be interpreted as an integer without any overflow.
         * It also treats true and false as valid values.
         */
        bool isValidInt() const;

        /**
         * Check if the preference value can be interpreted as an unsigned integer.
         */
        bool isValidUInt() const;

        /**
         * Check if the preference value can be interpreted as a floating point value.
         * This will also return true if the value is a valid unit. (eg: "1.0pt")
         */
        bool isValidDouble() const;

        /**
         * Check if the preference value can be converted to a particular unit.
         */

        bool isConvertibleTo(Glib::ustring const &unit) const;

        /**
         * Check if the preference value can be interpreted as a color.
         */
        bool isValidColor() const;

        /**
         * Check if the preference value is a valid String
         * @return True if the preference is set, as all data is stored as strings.
         */
        bool isValidString() const { return isSet(); };

        /**
         * Interpret the preference as a Boolean value.
         *
         * @param def Default value if the preference is not set.
         */
        bool getBool(bool def = false) const;

        /**
         * Interpret the preference as an integer.
         *
         * @param def Default value if the preference is not set.
         */
        int getInt(int def = 0) const;

        /**
         * Interpret the preference as a limited integer.
         *
         * This method will return the default value if the interpreted value is
         * larger than @c max or smaller than @c min. Do not use to store
         * Boolean values as integers.
         *
         * @param def Default value if the preference is not set.
         * @param min Minimum value allowed to return.
         * @param max Maximum value allowed to return.
         */
        int getIntLimited(int def = 0, int min = INT_MIN, int max = INT_MAX) const;

        /**
         * Interpret the preference as an unsigned integer.
         *
         * @param def Default value if the preference is not set.
         */
        unsigned int getUInt(unsigned int def = 0) const;

        /**
         * Interpret the preference as a floating point value.
         *
         * @param def  Default value if the preference is not set.
         * @param unit Specifies the unit of the returned result. Will be ignored when equal to "". If the preference has no unit set, the default unit will be assumed.
         */
        double getDouble(double def = 0.0, Glib::ustring const &unit = "") const;

        /**
         * Interpret the preference as a limited floating point value.
         *
         * This method will return the default value if the interpreted value is
         * larger than @c max or smaller than @c min.
         *
         * @param def Default value if the preference is not set.
         * @param min Minimum value allowed to return.
         * @param max Maximum value allowed to return.
         * @param unit Specifies the unit of the returned result. Will be ignored when equal to "". If the preference has no unit set, the default unit will be assumed.
         */
        double getDoubleLimited(double def = 0.0, double min = DBL_MIN, double max = DBL_MAX,
                                Glib::ustring const &unit = "") const;

        /**
         * Interpret the preference as an UTF-8 string.
         *
         * If unset, return the default value given by @c def.
         *
         * To store a filename, convert it using Glib::filename_to_utf8().
         */
        Glib::ustring getString(Glib::ustring const &def = "") const;

        /**
         * Interpret the preference as a number followed by a unit (without space), and return this unit string.
         * If unset, return "".
         */
        Glib::ustring getUnit() const;

        /**
         * Interpret the preference as a css color value.
         */
        Colors::Color getColor(std::string const &def) const;

        /**
         * Interpret the preference as a CSS style.
         *
         * @return A CSS style that has to be unrefed when no longer necessary. Never NULL.
         */
        SPCSSAttr *getStyle() const;

        /**
         * Interpret the preference as a CSS style with directory-based
         * inheritance.
         *
         * This function will look up the preferences with the same entry name
         * in ancestor directories and return the inherited CSS style.
         *
         * @return Inherited CSS style that has to be unrefed after use. Never NULL.
         */
        SPCSSAttr *getInheritedStyle() const;

        /**
         * Get the full path of the preference described by this Entry.
         */
        Glib::ustring const &getPath() const { return _pref_path; }

        /**
         * Get the last component of the preference's path.
         *
         * E.g. For "/options/some_group/some_option" it will return "some_option".
         */
        Glib::ustring getEntryName() const;
    private:
        double _getDoubleAssumeExisting() const;

        Glib::ustring _pref_path;
        std::optional<Glib::ustring> _value;

        mutable bool value_bool = false;
        mutable int value_int = 0;
        mutable unsigned int value_uint = 0;
        mutable double value_double = 0.;
        mutable Glib::ustring value_unit;

        mutable bool cached_bool = false;
        mutable bool cached_point = false;
        mutable bool cached_int = false;
        mutable bool cached_uint = false;
        mutable bool cached_double = false;
        mutable bool cached_unit = false;
    };

    // disable copying
    Preferences(Preferences const &) = delete;
    Preferences operator=(Preferences const &) = delete;

    // utility methods

    /**
     * Save all preferences to the hard disk.
     *
     * For some backends, the preferences may be saved as they are modified.
     * Not calling this method doesn't guarantee the preferences are unmodified
     * the next time Inkscape runs.
     */
    void save();

    /**
     * Deletes the preferences.xml file.
     */
    void reset();
    /**
     * Check whether saving the preferences will have any effect.
     */
    bool isWritable() { return _writable; }
    /*@}*/

    /**
     * Return details of the last encountered error, if any.
     *
     * This method will return true if an error has been encountered, and fill
     * in the primary and secondary error strings of the last error. If an error
     * had been encountered, this will reset it.
     *
     * @param string to set to the primary error message.
     * @param string to set to the secondary error message.
     *
     * @return True if an error has occurred since last checking, false otherwise.
     */
    bool getLastError( Glib::ustring& primary, Glib::ustring& secondary );

    /**
     * @name Iterate over directories and entries.
     * @{
     */

    /**
     * Get all entries from the specified directory.
     *
     * This method will return a vector populated with preference entries
     * from the specified directory. Subdirectories will not be represented.
     */
    std::vector<Entry> getAllEntries(Glib::ustring const &path);

    /**
     * Get all subdirectories of the specified directory.
     *
     * This will return a vector populated with full paths to the subdirectories
     * present in the specified @c path.
     */
    std::vector<Glib::ustring> getAllDirs(Glib::ustring const &path);
    /*@}*/

    /**
     * Test the path if it is a directory
     */
    bool isDir(Glib::ustring const &path);

    /**
     * @name Retrieve data from the preference storage.
     * @{
     */

    /**
     * Return true if there is a value at this prefs-path
     *
     * @param pref_path Path to preference to check.
     */
    bool hasPref(Glib::ustring const &pref_path) {
        return getEntry(pref_path).isSet();
    }

    /**
     * Retrieve a Boolean value.
     *
     * @param pref_path Path to the retrieved preference.
     * @param def The default value to return if the preference is not set.
     */
    bool getBool(Glib::ustring const &pref_path, bool def=false) {
        return getEntry(pref_path).getBool(def);
    }

    /**
     * Retrieve a Boolean value, if it exists.
     *
     * @param pref_path Path to the retrieved preference.
     */
    std::optional<bool> getOptionalBool(Glib::ustring const &pref_path) {
        if (hasPref(pref_path)) {
            return getBool(pref_path);
        }
        return {};
    }

    /**
     * Retrieve a point.
     *
     * @param pref_path Path to the retrieved preference.
     * @param def The default value to return if the preference is not set.
     */
    Geom::Point getPoint(Glib::ustring const &pref_path, Geom::Point def=Geom::Point()) {
        return Geom::Point(
            getEntry(pref_path + "/x").getDouble(def[Geom::X]),
            getEntry(pref_path + "/y").getDouble(def[Geom::Y])
        );
    }

    /**
     * Retrieve an integer.
     *
     * @param pref_path Path to the retrieved preference.
     * @param def The default value to return if the preference is not set.
     */
    int getInt(Glib::ustring const &pref_path, int def=0) {
        return getEntry(pref_path).getInt(def);
    }

    /**
     * Retrieve a limited integer.
     *
     * The default value is returned if the actual value is larger than @c max
     * or smaller than @c min. Do not use to store Boolean values.
     *
     * @param pref_path Path to the retrieved preference.
     * @param def The default value to return if the preference is not set.
     * @param min Minimum value to return.
     * @param max Maximum value to return.
     */
    int getIntLimited(Glib::ustring const &pref_path, int def=0, int min=INT_MIN, int max=INT_MAX) {
        return getEntry(pref_path).getIntLimited(def, min, max);
    }

    /**
     * Retrieve an unsigned integer.
     *
     * @param pref_path Path to the retrieved preference.
     * @param def The default value to return if the preference is not set.
     */
    unsigned int getUInt(Glib::ustring const &pref_path, unsigned int def=0) {
        return getEntry(pref_path).getUInt(def);
    }

    /**
     * Retrieve a floating point value.
     *
     * @param pref_path Path to the retrieved preference.
     * @param def The default value to return if the preference is not set.
     * @param unit Specifies the unit of the returned result. Will be ignored when equal to "". If the preference has no
     * unit set, the default unit will be assumed.
     */
    double getDouble(Glib::ustring const &pref_path, double def=0.0, Glib::ustring const &unit = "") {
        return getEntry(pref_path).getDouble(def, unit);
    }

    /**
     * Retrieve a limited floating point value.
     *
     * The default value is returned if the actual value is larger than @c max
     * or smaller than @c min.
     *
     * @param pref_path Path to the retrieved preference.
     * @param def The default value to return if the preference is not set.
     * @param min Minimum value to return.
     * @param max Maximum value to return.
     * @param unit Specifies the unit of the returned result. Will be ignored when equal to "". If the preference has no unit set, the default unit will be assumed.
     */
    double getDoubleLimited(Glib::ustring const &pref_path, double def=0.0, double min=DBL_MIN, double max=DBL_MAX, Glib::ustring const &unit = "") {
        return getEntry(pref_path).getDoubleLimited(def, min, max, unit);
    }

    /**
     * Retrieve an UTF-8 string.
     *
     * @param pref_path Path to the retrieved preference.
     */
    Glib::ustring getString(Glib::ustring const &pref_path, Glib::ustring const &def = "") {
        return getEntry(pref_path).getString(def);
    }


    /**
     * Retrieve the unit string.
     *
     * @param pref_path Path to the retrieved preference.
     */
    Glib::ustring getUnit(Glib::ustring const &pref_path) {
        return getEntry(pref_path).getUnit();
    }

    Colors::Color getColor(Glib::ustring const &pref_path, std::string const &def = "black");

    /**
     * Retrieve a CSS style.
     *
     * @param pref_path Path to the retrieved preference.
     * @return A CSS style that has to be unrefed after use.
     */
    SPCSSAttr *getStyle(Glib::ustring const &pref_path) {
        return getEntry(pref_path).getStyle();
    }

    /**
     * Retrieve an inherited CSS style.
     *
     * This method will look up preferences with the same entry name in ancestor
     * directories and return a style obtained by inheriting properties from
     * ancestor styles.
     *
     * @param pref_path Path to the retrieved preference.
     * @return An inherited CSS style that has to be unrefed after use.
     */
    SPCSSAttr *getInheritedStyle(Glib::ustring const &pref_path) {
        return getEntry(pref_path).getInheritedStyle();
    }

    /**
     * Retrieve a preference entry without specifying its type.
     */
    Entry const getEntry(Glib::ustring const &pref_path);
    /*@}*/


    /**
     * Get the preferences file name in UTF-8.
     */
    Glib::ustring getPrefsFilename() const;

    /**
     * @name Update preference values.
     * @{
     */

    /**
     * Set a Boolean value.
     */
    void setBool(Glib::ustring const &pref_path, bool value);

    /**
     * Set a point value.
     */
    void setPoint(Glib::ustring const &pref_path, Geom::Point value);

    /**
     * Set an integer value.
     */
    void setInt(Glib::ustring const &pref_path, int value);

    /**
     * Set an unsigned integer value.
     */
    void setUInt(Glib::ustring const &pref_path, unsigned int value);

    /**
     * Set a floating point value.
     */
    void setDouble(Glib::ustring const &pref_path, double value);

    /**
     * Set a floating point value with unit.
     */
    void setDoubleUnit(Glib::ustring const &pref_path, double value, Glib::ustring const &unit_abbr);

    /**
     * Set an UTF-8 string value.
     */
    void setString(Glib::ustring const &pref_path, Glib::ustring const &value);

    /**
     * Set an RGBA color value.
     */
    void setColor(Glib::ustring const &pref_path, Colors::Color const &color);

    /**
     * Set an RGBA color value from a string (if it can be parsed as a color).
     */
    void setColor(Glib::ustring const &pref_path, Glib::ustring const &value);

    /**
     * Set a CSS style.
     */
    void setStyle(Glib::ustring const &pref_path, SPCSSAttr *style);

    /**
     * Merge a CSS style with the current preference value.
     *
     * This method is similar to setStyle(), except that it merges the style
     * rather than replacing it. This means that if @c style doesn't have
     * a property set, it is left unchanged in the style stored in
     * the preferences.
     */
    void mergeStyle(Glib::ustring const &pref_path, SPCSSAttr *style);
    /*@}*/

    /**
     * @name Receive notifications about preference changes.
     * @{
     */

    /**
     * Register a preference observer.
     *
     * Limitations: If the observed preferences path does not exist or is removed,
     * then the observer may stop working, even if the path is re-added later.
     */
    void addObserver(Observer &);

    /**
     * Remove an observer an prevent further notifications to it.
     */
    void removeObserver(Observer &);
    /*@}*/

    /**
     * Create an observer watching preference 'path' and calling provided function when preference changes.
     * Function will be notified of changes to all leaves in a path: /path/ *
     * Returned pointer should be stored for as long as notifications are expected and disposed of
     * to remove observer object.
     */

    // observer callback with new preference value
    std::unique_ptr<PreferencesObserver> createObserver(Glib::ustring path, std::function<void (const Preferences::Entry& new_value)> callback);

    // observer callback without new preference value (read it with preferences->getXxx)
    std::unique_ptr<PreferencesObserver> createObserver(Glib::ustring path, std::function<void ()> callback);

    /**
     * @name Access and manipulate the Preferences object.
     * @{
     */


    /**
     * Access the singleton Preferences object.
     */
    static Preferences *get() {
        if (!_instance) {
            _instance = new Preferences();
        }
        return _instance;
    }

    void setErrorHandler(ErrorReporter* handler);

    /**
     * Unload all preferences.
     *
     * This deletes the singleton object. Calling get() after this function
     * will reinstate it, so you shouldn't. Call save() beforehand or the
     * preferences will not be saved.
     */
    static void unload();
    /*@}*/

    /**
     * Remove a node from prefs
     * @param pref_path　Path to entry
     *
     */
    void remove(Glib::ustring const &pref_path);

    /**
     * Create a temporary transaction which will be rolled back when the returned scope guard is destroyed.
     *
     * This will allow you to change preferences and have them revert/rollback when your local process is complete.
     */
    [[nodiscard]] auto temporaryPreferences() {
        bool new_transaction = !_prefs_doc->inTransaction();
        if (new_transaction)
            _prefs_doc->beginTransaction();
        return scope_exit([this,new_transaction] {
            if (new_transaction) {
                cachedEntry.clear();
                _prefs_doc->rollback();
            }
        });
    }

protected:
    // helper methods used by Entry
    /**
     * Interpret the preference as a CSS style with directory-based
     * inheritance.
     *
     * This function will look up the preferences with the same entry name
     * in ancestor directories and return the inherited CSS style.
     *
     * @return Inherited CSS style that has to be unrefed after use. Never NULL.
     */
    SPCSSAttr *_getInheritedStyleForPath(Glib::ustring const &prefPath);

private:
    Preferences();
    ~Preferences();
    void _loadDefaults();
    void _load();
    std::optional<Glib::ustring> _getRawValue(Glib::ustring const &path);
    void _setRawValue(Glib::ustring const &path, Glib::ustring const &value);
    void _reportError(Glib::ustring const &, Glib::ustring const &);
    void _keySplit(Glib::ustring const &pref_path, Glib::ustring &node_key, Glib::ustring &attr_key);
    XML::Node *_getNode(Glib::ustring const &pref_path, bool create=false);
    XML::Node *_findObserverNode(Glib::ustring const &pref_path, Glib::ustring &node_key, Glib::ustring &attr_key, bool create);

    std::string _prefs_filename; ///< Full filename (with directory) of the prefs file
    Glib::ustring _lastErrPrimary; ///< Last primary error message, if any.
    Glib::ustring _lastErrSecondary; ///< Last secondary error message, if any.
    XML::Document *_prefs_doc = nullptr; ///< XML document storing all the preferences
    ErrorReporter *_errorHandler = nullptr; ///< Pointer to object reporting errors.
    bool _writable = false; ///< Will the preferences be saved at exit?
    bool _hasError = false; ///< Indication that some error has occurred;
    bool _initialized = false; ///< Is this instance fully initialized? Caching should be avoided before.
    /// Cache for getEntry()
    // cache key has type std::string because Glib::ustring is slower for equality checks
    std::unordered_map<std::string, Entry> cachedEntry;

    /// Wrapper class for XML node observers
    class PrefNodeObserver;

    typedef std::map<Observer *, std::unique_ptr<PrefNodeObserver>> _ObsMap;
    /// Map that keeps track of wrappers assigned to PrefObservers
    _ObsMap _observer_map;

    // privilege escalation methods for PrefNodeObserver
    static _ObserverData *_get_pref_observer_data(Observer &o) { return o._data.get(); }

    static Preferences *_instance;

friend class PrefNodeObserver;
friend class Entry;
};

typedef std::unique_ptr<Preferences::PreferencesObserver> PrefObserver;

/**
 * @brief Proxy object providing a "live value" interface.
 *
 * A Pref<T> tracks a preference value. For the most part it behaves just like a T. For example
 *
 *     auto mybool = Pref<bool>("/path/to/mybool");
 *
 * can be used as a bool. The only difference is that it updates whenever the preference updates,
 * and allows you to perform an action when it does. For example,
 *
 *     mybool.action = [&] { std::cout << mybool << std::endl; };
 *
 * will cause the new value to be printed after each subsequent change. Pref<T> can be temporarily
 * disabled with a call to
 * 
 *     mybool.set_enabled(false);
 * 
 * during which time it will revert to its default value and ignore further updates until
 * re-enabled again.
 *
 *  - Most common types T are implemented. Feel free to add more as needed.
 *
 *  - Pref<void> allows listening for updates to a whole group of preferences. Although this
 *    entirely duplicates existing preferences functionality, it is provided for consistency.
 *
 */

template<typename T>
struct Pref {};

template<typename T>
class PrefBase : public Preferences::Observer
{
public:
    /// The default value.
    T def;

    /// The current value.
    operator const T&() const { return val; }

    /// The action to perform when the value changes, if any.
    std::function<void()> action;

    /// Disable switch. If disabled, the Pref will stick at its default value.
    void set_enabled(bool enabled) { enabled ? enable() : disable(); }

protected:
    T val;

    PrefBase(Glib::ustring path, T def) : Observer(std::move(path)), def(std::move(def)) {}
    PrefBase(PrefBase const &) = delete;
    PrefBase &operator=(PrefBase const &) = delete;

    void init() { val = static_cast<Pref<T>*>(this)->read(); Inkscape::Preferences::get()->addObserver(*this); }
    void act() { if (action) action(); }
    void assign(T const &val2) { if (val != val2) { val = val2; act(); } }
    void enable() { assign(static_cast<Pref<T>*>(this)->read()); Inkscape::Preferences::get()->addObserver(*this); }
    void disable() { assign(def); Inkscape::Preferences::get()->removeObserver(*this); }
    void notify(Preferences::Entry const &e) override { assign(static_cast<Pref<T>*>(this)->changed(e)); }
};

template<>
class Pref<bool> : public PrefBase<bool>
{
public:
    Pref(Glib::ustring path, bool def = false) : PrefBase(std::move(path), def) { init(); }
private:
    friend PrefBase;
    auto read() const { return Inkscape::Preferences::get()->getBool(observed_path, def); }
    auto changed(Preferences::Entry const &e) const { return e.getBool(def); }
};

template<>
class Pref<int> : public PrefBase<int>
{
public:
    int min, max;
    Pref(Glib::ustring path, int def = 0, int min = INT_MIN, int max = INT_MAX) : PrefBase(std::move(path), def), min(min), max(max) { init(); }
private:
    friend PrefBase;
    auto read() const { return Inkscape::Preferences::get()->getIntLimited(observed_path, def, min, max); }
    auto changed(Preferences::Entry const &e) const { return e.getIntLimited(def, min, max); }
};

template<>
class Pref<double> : public PrefBase<double>
{
public:
    double min, max;
    Pref(Glib::ustring path, double def = 0.0, double min = DBL_MIN, double max = DBL_MAX) : PrefBase(std::move(path), def), min(min), max(max) { init(); }
private:
    friend PrefBase;
    auto read() const { return Inkscape::Preferences::get()->getDoubleLimited(observed_path, def, min, max); }
    auto changed(Preferences::Entry const &e) const { return e.getDoubleLimited(def, min, max); }
};

template<>
class Pref<Glib::ustring> : public PrefBase<Glib::ustring>
{
public:
    Pref(Glib::ustring path, Glib::ustring def = "")
        : PrefBase(std::move(path), def)
    {
        init();
    }
private:
    friend PrefBase;
    auto read() const { return Preferences::get()->getString(observed_path, def); }
    auto changed(Preferences::Entry const &e) const { return e.getString(def); }
};

template<>
class Pref<void> : public Preferences::Observer
{
public:
    std::function<void()> action;

    Pref(Glib::ustring path) : Observer(std::move(path)) { enable(); }
    Pref(Pref const &) = delete;
    Pref &operator=(Pref const &) = delete;

    void set_enabled(bool enabled) { enabled ? enable() : disable(); }

private:
    void enable() { Inkscape::Preferences::get()->addObserver(*this); }
    void disable() { Inkscape::Preferences::get()->removeObserver(*this); }
    void notify(Preferences::Entry const &e) override { if (action) action(); }
};

} // namespace Inkscape

#endif // INKSCAPE_PREFSTORE_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0))
  indent-tabs-mode:nil
  fill-column:75
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
