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

#ifndef RMLUI_CORE_FACTORY_H
#define RMLUI_CORE_FACTORY_H

#include "XMLParser.h"
#include "Header.h"

namespace Rml {

class Context;
class DataControllerInstancer;
class DataViewInstancer;
class Element;
class ElementDocument;
class Event;
class EventListener;
class EventListenerInstancer;
class FontEffect;
class FontEffectInstancer;
class StyleSheet;
class PropertyDictionary;
class PropertySpecification;
enum class EventId : uint16_t;

/**
	The Factory contains a registry of instancers for different types.

	All instantiation of these rmlui types should go through the factory
	so that scripting API's can bind in new types.

	@author Lloyd Weehuizen
 */

class RMLUICORE_API Factory
{
public:
	/// Initialise the element factory
	static bool Initialise();
	/// Cleanup and shutdown the factory
	static void Shutdown();

	/// Instances a text element containing a string.
	/// More than one element may be instanced if the string contains RML or RML is introduced during translation.
	/// @param[in] parent The element any instanced elements will be parented to.
	/// @param[in] text The text to instance the element (or elements) from.
	/// @return True if the string was parsed without error, false otherwise.
	static bool InstanceElementText(Element* parent, const String& text);

	/// Registers a non-owning pointer to an instancer that will be used to instance font effects.
	/// @param[in] name The name of the font effect the instancer will be called for.
	/// @param[in] instancer The instancer to call when the font effect name is encountered.
	/// @lifetime The instancer must be kept alive until after the call to Rml::Shutdown.
	/// @return The added instancer if the registration was successful, nullptr otherwise.
	static void RegisterFontEffectInstancer(const String& name, FontEffectInstancer* instancer);
	/// Retrieves a font-effect instancer registered with the factory.
	/// @param[in] name The name of the desired font-effect type.
	/// @return The font-effect instancer it it exists, nullptr otherwise.
	static FontEffectInstancer* GetFontEffectInstancer(const String& name);

	/// Creates a style sheet from a user-generated string.
	/// @param[in] string The contents of the style sheet.
	/// @return A pointer to the newly created style sheet.
	static SharedPtr<StyleSheet> InstanceStyleSheetString(const String& string);
	/// Creates a style sheet from a file.
	/// @param[in] file_name The location of the style sheet file.
	/// @return A pointer to the newly created style sheet.
	static SharedPtr<StyleSheet> InstanceStyleSheetFile(const String& file_name);
	/// Creates a style sheet from an Stream.
	/// @param[in] stream A pointer to the stream containing the style sheet's contents.
	/// @return A pointer to the newly created style sheet.
	static SharedPtr<StyleSheet> InstanceStyleSheetStream(Stream* stream);
	/// Clears the style sheet cache. This will force style sheets to be reloaded.
	static void ClearStyleSheetCache();

	/// Register the instancer to be used for all event listeners.
	/// @lifetime The instancer must be kept alive until after the call to Rml::Shutdown, or until a new instancer is set.
	static void RegisterEventListenerInstancer(EventListenerInstancer* instancer);
	/// Instance an event listener with the given string. This is used for instancing listeners for the on* events from RML.
	/// @param[in] value The parameters to the event listener.
	/// @return The instanced event listener.
	static EventListener* InstanceEventListener(const String& value, Element* element);

	/// Register an instancer for data views.
	/// Structural views start a special XML parsing procedure when encountering a declaration of the view. Instead of instancing
	/// children elements, the raw inner XML/RML contents are submitted to the initializing procedure of the view.
	/// @param[in] instancer  The instancer to be called.
	/// @param[in] type_name  The type name of the view, determines the element attribute that is used to initialize it.
	/// @param[in] is_structural_view  Set true if the view should be parsed as a structural view.
	/// @lifetime The instancer must be kept alive until after the call to Rml::Shutdown.
	static void RegisterDataViewInstancer(DataViewInstancer* instancer, const String& type_name, bool is_structural_view = false);

	/// Register an instancer for data controllers.
	/// @param[in] instancer  The instancer to be called.
	/// @param[in] type_name  The type name of the controller, determines the element attribute that is used to initialize it.
	/// @lifetime The instancer must be kept alive until after the call to Rml::Shutdown.
	static void RegisterDataControllerInstancer(DataControllerInstancer* instancer, const String& type_name);

	/// Instance the data view with the given type name.
	static DataViewPtr InstanceDataView(const String& type_name, Element* element, bool is_structural_view);

	/// Instance the data controller with the given type name.
	static DataControllerPtr InstanceDataController(const String& type_name, Element* element);

	/// Returns true if the given type name is a structural data view.
	static bool IsStructuralDataView(const String& type_name);

	/// Returns the list of element attribute names with an associated structural data view instancer.
	static const StringList& GetStructuralDataViewAttributeNames();

private:
	Factory();
	~Factory();
};

} // namespace Rml
#endif
