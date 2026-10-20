
# Generic widgets for Gtk4

This directory contains code for creating custom Inkscape specific widgets using the Gtkmm / Gtk4 libraries.

A general rule of thumb is that if you can see this widget being dropped into some other Gtk application without
having to pull out notions or connections to SVG, and Inkscape then it's a generic widget.

# Rules for Widgets in this Directory

## Widget MUST NOT implement

Widgets are expected to be generic and not have specific implementation for data and structure from other parts of the project codebase:

  * The same functionality as another widget in this directory
  * String parsing, such as converting from string to numbers
  * File opening, such as opening a palette file
  * Format decoding, such as splitting strings into lists
  * Attributes, such as setting or getting SPObject attributes
  * Properties, such as setting or getting SVG CSS Styles

## Widget MUST implement

All widgets should be usable via Gtk Builder, and use well understood mechanisms to communicate and be in a Gtkmm style:

 * Implement GtkBuilder constructors
 * Use an initaliser so they can be used in ui xml files
 * Add properties where needed so they can be used
 * Report changes using signals with non-Gtk/Gdk values
 * Have setters and getters for their data using non-Gtk/Gdk values

## Mixing Widgets

 * All sub-widgets should come from this directory OR Gtkmm NOT from parent directories.
 * All sub-widgets should be deconstructed by regular Gtkmm referencing (destroyed when unparented for example)

## Template

Because the creation of widgets is quite delicate, this template should help you make a new widget, while understanding why
each parent of the copypasta is necessary to interact with gtkmm and glibmm.

### Header file

```
class MyNewWidget : public BuildableWidget<MyNewWidget, BaseWidget>
    // BaseWidget may be any other generic widget, or any widget from the Gtk:: namespace

public:
    // Bare constructor is used in registration to initalise the GType
    MyNewWidget();

    // The Gtk builder constructor allows this widget to be created by ui xml files
    MyNewWidget(GtkWidget *cobject, const Glib::RefPtr<Gtk::Builder>& builder = {});

private:
    // Optional constructor method for duplicated constructor logic
    void construct();
};
```

We use `BuildableWidget` to implement registration code that is specific to each custom widget.

Note 1: If your widget implements an interface, like `Gtk::Orientable`, you will need to add a `using` alias
to disambiguate `BaseObjectType` definition, as it appears in two places in a hierarchy.

Example: a custom widget that derives from `Gtk::Widget` will need to add `using BaseObjectType = GtkWidget;` line.

Note 2: Interfaces (like `Gtk::Orientable`) or helper classes (like `CssNameClassInit`) need to be specified *before* `BuildableWidget` to be initialized properly.

Note 3: To change the name of `MyNewWidget` as seen by CSS, we can use 
`CssNameClassInit` to provide a new one. For example `CssNameClassInit("my-new-widget")`.

### Code file

```
// First the bare constructor, it must be EXACTLY the same as the builder constructor.
MyNewWidget::MyNewWidget()
// This allows the new widget to be correctly registered and for gobject properties of the parent
//   to be accessible to this class too.
 , Glib::ObjectBase("MyNewWidget")
// The parent constructor must omit the cobject entirely because the chain of construction generates
//   a new cobject only if there is no argument, and not if that argument is nullptr.
// : ParentWidget() // This isn't required, just noted here for explicit symmetry
 // More construction here
{
   construct(); // optional
}

// This constructor means we already have a gobject, and the builder is passing it
// to us so we can bind it to our C++ code correctly.
MyNewWidget::MyNewWidget(GtkWidget *cobject, const Glib::RefPtr<Gtk::Builder>& builder)
 , Glib::ObjectBase("MyNewWidget")
// As above the cobject must be passed into the base widget. builder can be omitted
// if the base is a Gtk widget and not a custom widget.
 : BaseWidget(cobject, builder)
 // The same construction here, you may want to use a define to reduce duplication
{
   construct(); // optional
}

```

Note the call to the `Glib::ObjectBase` constructor. It is a virtual base, and as such it needs to be
initialized by the most derived class (here: `MyNewWidget`).
This call is what creates a unique `Glib` `GType` for `MyNewWidget`.

Reference: [Derived widgets](https://gnome.pages.gitlab.gnome.org/gtkmm-documentation/sec-builder-using-derived-widgets.html)

### Registration in gtk-registry.cpp

```

#include "generic/my-new-widget.h"

void register_all()
{
   // This makes sure the ctype is ready for the Gtk Builder and adds the required
   // names to the lookup. Our widget will be available as gtkmm__CustomObject_MyNewWidget
   MyNewWidget::register_type();
}

```


