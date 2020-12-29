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

#ifndef RMLUI_CORE_EVENT_H
#define RMLUI_CORE_EVENT_H

#include "Header.h"
#include "Dictionary.h"
#include "ID.h"

namespace Rml {

class Factory;
class Element;
struct EventSpecification;

enum class EventPhase { None, Capture = 1, Target = 2, Bubble = 4 };
enum class DefaultActionPhase { None, Target = (int)EventPhase::Target, TargetAndBubble = ((int)Target | (int)EventPhase::Bubble) };

/**
	An event that propogates through the element hierarchy. Events follow the DOM3 event specification. See
	http://www.w3.org/TR/DOM-Level-3-Events/events.html.

	@author Lloyd Weehuizen
 */

class RMLUICORE_API Event
{
public:
	/// Constructor
	/// @param[in] target The target element of this event
	/// @param[in] parameters The event parameters
	/// @param[in] interruptible Can this event have is propagation stopped?
	Event(Element* target, EventId id, const Dictionary& parameters, bool interruptible);
	/// Destructor
	virtual ~Event();

	/// Get the current propagation phase.
	EventPhase GetPhase() const;
	/// Set the current propagation phase
	void SetPhase(EventPhase phase);

	/// Set the current element in the propagation.
	void SetCurrentElement(Element* element);
	/// Get the current element in the propagation.
	Element* GetCurrentElement() const;
	/// Get the target element of this event.
	Element* GetTargetElement() const;

	/// Get the event type.
	String GetType() const;
	/// Get the event id.
	EventId GetId() const;

	/// Stops propagation of the event if it is interruptible, but finish all listeners on the current element.
	void StopPropagation();
	/// Stops propagation of the event if it is interruptible, including to any other listeners on the current element.
	void StopImmediatePropagation();

	/// Returns true if the event can be interrupted, that is, stopped from propagating.
	bool IsInterruptible() const;
	/// Returns true if the event is still propagating.
	bool IsPropagating() const;
	/// Returns true if the event is still immediate propagating.
	bool IsImmediatePropagating() const;

	/// Returns the value of one of the event's parameters.
	/// @param key[in] The name of the desired parameter.
	/// @return The value of the requested parameter.
	template < typename T >
	T GetParameter(const String& key, const T& default_value) const
	{
		return Get(parameters, key, default_value);
	}
	/// Access the dictionary of parameters
	/// @return The dictionary of parameters
	const Dictionary& GetParameters() const;

protected:
	Dictionary parameters;
	Element* target_element = nullptr;
	Element* current_element = nullptr;

private:
	void InitMouseEvent();
	EventId id = EventId::Invalid;
	bool interruptible = false;
	bool interrupted = false;
	bool interrupted_immediate = false;
	EventPhase phase = EventPhase::None;
	friend class Rml::Factory;
};


} // namespace Rml
#endif
