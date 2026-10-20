// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_GRADIENT_SELECTOR_INTERFACE_H
#define SEEN_GRADIENT_SELECTOR_INTERFACE_H

#include "object/sp-gradient.h"
#include "object/sp-paint-server-data.h"

class GradientSelectorInterface {
public:
	enum SelectorMode { MODE_LINEAR, MODE_RADIAL, MODE_SWATCH };

	// pass gradient object (SPLinearGradient or SPRadialGradient)
	virtual void setGradient(SPGradient* gradient) = 0;

	virtual SPGradient* getVector() = 0;
	virtual void setVector(SPDocument* doc, SPGradient* vector) = 0;
	virtual void setMode(SelectorMode mode) = 0;
	virtual void setUnits(SPGradientUnits units) = 0;
	virtual SPGradientUnits getUnits() = 0;
	virtual void setSpread(SPGradientSpread spread) = 0;
	virtual SPGradientSpread getSpread() = 0;
	virtual void selectStop(SPStop* selected) {};

	sigc::signal<void (SPStop*)>& signal_stop_selected() { return _signal_stop_selected; }
	void emit_stop_selected(SPStop* stop) { _signal_stop_selected.emit(stop); }

private:
	sigc::signal<void (SPStop*)> _signal_stop_selected;
};

#endif
