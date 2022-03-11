/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "ElementBackgroundBorder.h"
#include "../Include/RmlUi/Layout.h"
#include "../Include/RmlUi/ComputedValues.h"
#include "../Include/RmlUi/Element.h"
#include "../Include/RmlUi/ElementStyle.h"

namespace Rml {

static const auto PI = acosf(-1);

void ElementBackgroundBorder::GenerateGeometry(Element* element, Geometry& geometry, Geometry::Path& paddingEdge) {
	const Style::ComputedValues& computed = element->GetComputedValues();
	EdgeInsets<Color> border_color = computed.border_color;
	Color background_color = computed.background_color;
	float opacity = element->GetOpacity();
	if (opacity < 1) {
		ColorApplyOpacity(background_color, opacity);
		for (int i = 0; i < 4; ++i) {
			ColorApplyOpacity(border_color[i], opacity);
		}
	}

	CornerInsets<float> border_radius {
		ComputePropertyW(computed.border_radius.topLeft, element),
		ComputePropertyH(computed.border_radius.topRight, element),
		ComputePropertyW(computed.border_radius.bottomRight, element),
		ComputePropertyH(computed.border_radius.bottomLeft, element),
	};
	
	const Layout::Metrics& metrics = element->GetMetrics();

	float outlineWidth = ComputeProperty(element->GetComputedProperty(PropertyId::OutlineWidth), element);
	if (outlineWidth > 0.f) {
		Color outlineColor = element->GetComputedProperty(PropertyId::OutlineColor)->GetColor();
		geometry.AddRect(Rect {Point{}, metrics.frame.size}, outlineWidth, outlineColor);
	}

	bool topLeftInnerRounded     = metrics.borderWidth.left  < border_radius.topLeft     && metrics.borderWidth.top    < border_radius.topLeft;
	bool topRightInnerRounded    = metrics.borderWidth.right < border_radius.topRight    && metrics.borderWidth.top    < border_radius.topRight;
	bool bottomRightInnerRounded = metrics.borderWidth.right < border_radius.bottomRight && metrics.borderWidth.bottom < border_radius.bottomRight;
	bool bottomLeftInnerRounded  = metrics.borderWidth.left  < border_radius.bottomLeft  && metrics.borderWidth.bottom < border_radius.bottomLeft;
	auto topLeftInnerRadius = [&](float v) {
		return topLeftInnerRounded ? std::max(v, border_radius.topLeft) : v;
	};
	auto topRightInnerRadius = [&](float v) {
		return topRightInnerRounded ? std::max(v, border_radius.topRight) : v;
	};
	auto bottomRightInnerRadius = [&](float v) {
		return bottomRightInnerRounded ? std::max(v, border_radius.bottomRight) : v;
	};
	auto bottomLeftInnerRadius = [&](float v) {
		return bottomLeftInnerRounded ? std::max(v, border_radius.bottomLeft) : v;
	};

	Quad quadLeft = {
		{ 0, border_radius.topLeft },
		{ metrics.borderWidth.left, topLeftInnerRadius(metrics.borderWidth.top) },
		{ metrics.borderWidth.left, metrics.frame.size.h - bottomLeftInnerRadius(metrics.borderWidth.bottom) },
		{ 0, metrics.frame.size.h - border_radius.bottomLeft }
	};
	Quad quadTop = {
		{ border_radius.topLeft, 0 },
		{ metrics.frame.size.w - border_radius.topRight, 0 },
		{ metrics.frame.size.w - topRightInnerRadius(metrics.borderWidth.right), metrics.borderWidth.top },
		{ topLeftInnerRadius(metrics.borderWidth.left), metrics.borderWidth.top },
	};
	Quad quadRight = {
		{ metrics.frame.size.w - metrics.borderWidth.right,  topRightInnerRadius(metrics.borderWidth.top) },
		{ metrics.frame.size.w, border_radius.topRight },
		{ metrics.frame.size.w, metrics.frame.size.h - border_radius.bottomRight },
		{ metrics.frame.size.w - metrics.borderWidth.right, metrics.frame.size.h - bottomRightInnerRadius(metrics.borderWidth.bottom) },
	};
	Quad quadBottom = {
		{ bottomLeftInnerRadius(metrics.borderWidth.left), metrics.frame.size.h - metrics.borderWidth.bottom },
		{ metrics.frame.size.w - bottomRightInnerRadius(metrics.borderWidth.right), metrics.frame.size.h - metrics.borderWidth.bottom },
		{ metrics.frame.size.w - border_radius.bottomRight, metrics.frame.size.h },
		{ border_radius.bottomLeft, metrics.frame.size.h },
	};

	geometry.AddQuad(quadLeft, computed.border_color.left);
	Geometry::Path topLeftOuter;
	topLeftOuter.DrawArc(
		{ border_radius.topLeft, border_radius.topLeft },
		border_radius.topLeft,
		border_radius.topLeft,
		PI, PI * 1.5f
	);
	Geometry::Path topLeftInner;
	if (topLeftInnerRounded) {
		topLeftInner.DrawArc(
			{ border_radius.topLeft, border_radius.topLeft },
			border_radius.topLeft - metrics.borderWidth.left,
			border_radius.topLeft - metrics.borderWidth.top,
			PI, PI * 1.5f
		);
	}
	else {
		topLeftInner.emplace({
			metrics.borderWidth.left,
			metrics.borderWidth.top
		});
	}
	geometry.AddArc(topLeftOuter, topLeftInner, computed.border_color.left);

	geometry.AddQuad(quadTop, computed.border_color.top);
	Geometry::Path topRightOuter;
	topRightOuter.DrawArc(
		{ metrics.frame.size.w - border_radius.topRight, border_radius.topRight },
		border_radius.topRight,
		border_radius.topRight,
		PI * 1.5f, PI * 2.f
	);
	Geometry::Path topRightInner;
	if (topRightInnerRounded) {
		topRightInner.DrawArc(
			{ metrics.frame.size.w - border_radius.topRight, border_radius.topRight },
			border_radius.topRight - metrics.borderWidth.right,
			border_radius.topRight - metrics.borderWidth.top,
			PI * 1.5f, PI * 2.f
		);
	}
	else {
		topRightInner.emplace({
			metrics.frame.size.w - metrics.borderWidth.right,
			metrics.borderWidth.top
		});
	}
	geometry.AddArc(topRightOuter, topRightInner, computed.border_color.top);

	geometry.AddQuad(quadRight, computed.border_color.right);
	Geometry::Path bottomRightOuter;
	bottomRightOuter.DrawArc(
		{ metrics.frame.size.w - border_radius.bottomRight, metrics.frame.size.h - border_radius.bottomRight },
		border_radius.bottomRight,
		border_radius.bottomRight,
		0, PI * 0.5f
	);
	Geometry::Path bottomRightInner;
	if (bottomRightInnerRounded) {
		bottomRightInner.DrawArc(
			{ metrics.frame.size.w - border_radius.bottomRight, metrics.frame.size.h - border_radius.bottomRight },
			border_radius.bottomRight - metrics.borderWidth.right,
			border_radius.bottomRight - metrics.borderWidth.bottom,
			0, PI * 0.5f
		);
	}
	else {
		bottomRightInner.emplace({
			metrics.frame.size.w - metrics.borderWidth.right,
			metrics.frame.size.h - metrics.borderWidth.bottom
		});
	}
	geometry.AddArc(bottomRightOuter, bottomRightInner, computed.border_color.right);

	geometry.AddQuad(quadBottom, computed.border_color.bottom);
	Geometry::Path bottomLeftOuter;
	bottomLeftOuter.DrawArc(
		{ border_radius.bottomLeft, metrics.frame.size.h - border_radius.bottomLeft },
		border_radius.bottomLeft,
		border_radius.bottomLeft,
		PI * 0.5f, PI
	);
	Geometry::Path bottomLeftInner;
	if (bottomLeftInnerRounded) {
		bottomLeftInner.DrawArc(
			{ border_radius.bottomLeft, metrics.frame.size.h - border_radius.bottomLeft },
			border_radius.bottomLeft - metrics.borderWidth.left,
			border_radius.bottomLeft - metrics.borderWidth.bottom,
			PI * 0.5f, PI
		);
	}
	else {
		bottomLeftInner.emplace({
			metrics.borderWidth.left,
			metrics.frame.size.h - metrics.borderWidth.bottom
		});
	}
	geometry.AddArc(bottomLeftOuter, bottomLeftInner, computed.border_color.bottom);

	paddingEdge.clear();
	paddingEdge.append(topLeftInner);
	paddingEdge.append(topRightInner);
	paddingEdge.append(bottomRightInner);
	paddingEdge.append(bottomLeftInner);
	geometry.AddPolygon(paddingEdge, background_color);
}

} // namespace Rml
