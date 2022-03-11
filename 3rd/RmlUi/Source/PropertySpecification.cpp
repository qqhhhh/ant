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

#include "../Include/RmlUi/PropertySpecification.h"
#include "../Include/RmlUi/Log.h"
#include "../Include/RmlUi/PropertyDefinition.h"
#include "../Include/RmlUi/StringUtilities.h"
#include "PropertyShorthandDefinition.h"
#include "IdNameMap.h"
#include <limits.h>
#include <stdint.h>

namespace Rml {

PropertySpecification::PropertySpecification() :
	properties(), shorthands(),
	property_map(std::make_unique<PropertyIdNameMap>()), shorthand_map(std::make_unique<ShorthandIdNameMap>())
{
}

PropertySpecification::~PropertySpecification()
{
}

PropertyDefinition& PropertySpecification::RegisterProperty(PropertyId id, const std::string& property_name, bool inherited, const std::string& default_value) {
	assert (id != PropertyId::Invalid);
	property_map->AddPair(id, property_name);
	size_t index = (size_t)id;
	if (index < properties.size()) 	{
		if (properties[index]) {
			Log::Message(Log::Level::Error, "While registering property '%s': The property is already registered.", property_name.c_str());
			return *properties[index];
		}
	}
	else {
		properties.resize((index*3)/2 + 1);
	}
	properties[index] = std::make_unique<PropertyDefinition>(id, default_value, inherited);
	property_ids.Insert(id);
	if (inherited)
		property_ids_inherited.Insert(id);
	return *properties[index];
}

PropertyDefinition& PropertySpecification::RegisterProperty(PropertyId id, const std::string& property_name, bool inherited) {
	assert (id != PropertyId::Invalid);
	property_map->AddPair(id, property_name);
	size_t index = (size_t)id;
	if (index < properties.size()) {
		if (properties[index]) {
			Log::Message(Log::Level::Error, "While registering property '%s': The property is already registered.", property_name.c_str());
			return *properties[index];
		}
	}
	else {
		properties.resize((index*3)/2 + 1);
	}
	properties[index] = std::make_unique<PropertyDefinition>(id, inherited);
	property_ids.Insert(id);
	if (inherited)
		property_ids_inherited.Insert(id);
	return *properties[index];
}

const PropertyDefinition* PropertySpecification::GetProperty(PropertyId id) const
{
	if (id == PropertyId::Invalid || (size_t)id >= properties.size())
		return nullptr;

	return properties[(size_t)id].get();
}

const PropertyDefinition* PropertySpecification::GetProperty(const std::string& property_name) const
{
	return GetProperty(property_map->GetId(property_name));
}

// Fetches a list of the names of all registered property definitions.
const PropertyIdSet& PropertySpecification::GetRegisteredProperties(void) const
{
	return property_ids;
}

// Fetches a list of the names of all registered property definitions.
const PropertyIdSet& PropertySpecification::GetRegisteredInheritedProperties(void) const
{
	return property_ids_inherited;
}

// Registers a shorthand property definition.
ShorthandId PropertySpecification::RegisterShorthand(const std::string& shorthand_name, const std::string& property_names, ShorthandType type, ShorthandId id)
{
	if (id == ShorthandId::Invalid)
		id = shorthand_map->GetOrCreateId(shorthand_name);
	else
		shorthand_map->AddPair(id, shorthand_name);

	std::vector<std::string> property_list;
	StringUtilities::ExpandString(property_list, StringUtilities::ToLower(property_names));

	// Construct the new shorthand definition and resolve its properties.
	std::unique_ptr<ShorthandDefinition> property_shorthand(new ShorthandDefinition());

	for (const std::string& raw_name : property_list)
	{
		ShorthandItem item;
		bool optional = false;
		std::string name = raw_name;

		if (!raw_name.empty() && raw_name.back() == '?')
		{
			optional = true;
			name.pop_back();
		}

		PropertyId property_id = property_map->GetId(name);
		if (property_id != PropertyId::Invalid)
		{
			// We have a valid property
			if (const PropertyDefinition* property = GetProperty(property_id))
				item = ShorthandItem(property_id, property, optional);
		}
		else
		{
			// Otherwise, we must be a shorthand
			ShorthandId shorthand_id = shorthand_map->GetId(name);

			// Test for valid shorthand id. The recursive types (and only those) can hold other shorthands.
			if (shorthand_id != ShorthandId::Invalid && (type == ShorthandType::RecursiveRepeat || type == ShorthandType::RecursiveCommaSeparated))
			{
				if (const ShorthandDefinition * shorthand = GetShorthand(shorthand_id))
					item = ShorthandItem(shorthand_id, shorthand, optional);
			}
		}

		if (item.type == ShorthandItemType::Invalid)
		{
			Log::Message(Log::Level::Error, "Shorthand property '%s' was registered with invalid property '%s'.", shorthand_name.c_str(), name.c_str());
			return ShorthandId::Invalid;
		}
		property_shorthand->items.push_back(item);
	}

	property_shorthand->id = id;
	property_shorthand->type = type;

	const size_t index = (size_t)id;

	if (index < shorthands.size())
	{
		// We don't want to owerwrite an existing entry.
		if (shorthands[index])
		{
			Log::Message(Log::Level::Error, "The shorthand '%s' already exists, ignoring.", shorthand_name.c_str());
			return ShorthandId::Invalid;
		}
	}
	else
	{
		// Resize vector to hold the new index
		shorthands.resize((index * 3) / 2 + 1);
	}

	shorthands[index] = std::move(property_shorthand);
	return id;
}

// Returns a shorthand definition.
const ShorthandDefinition* PropertySpecification::GetShorthand(ShorthandId id) const
{
	if (id == ShorthandId::Invalid || (size_t)id >= shorthands.size())
		return nullptr;

	return shorthands[(size_t)id].get();
}

const ShorthandDefinition* PropertySpecification::GetShorthand(const std::string& shorthand_name) const
{
	return GetShorthand(shorthand_map->GetId(shorthand_name));
}

void PropertySpecification::ParseShorthandDeclaration(PropertyIdSet& set, ShorthandId shorthand_id) const {
	const ShorthandDefinition* shorthand_definition = GetShorthand(shorthand_id);
	for (size_t i = 0; i < shorthand_definition->items.size(); ++i) {
		const ShorthandItem& item = shorthand_definition->items[i];
		if (item.type == ShorthandItemType::Property)
			set.Insert(item.property_id);
		else if (item.type == ShorthandItemType::Shorthand)
			ParseShorthandDeclaration(set, item.shorthand_id);
	}
}

bool PropertySpecification::ParsePropertyDeclaration(PropertyIdSet& set, const std::string& property_name) const
{
	// Try as a property first
	PropertyId property_id = property_map->GetId(property_name);
	if (property_id != PropertyId::Invalid) {
		set.Insert(property_id);
		return true;
	}

	// Then, as a shorthand
	ShorthandId shorthand_id = shorthand_map->GetId(property_name);
	if (shorthand_id != ShorthandId::Invalid) {
		ParseShorthandDeclaration(set, shorthand_id);
		return true;
	}

	return false;
}

bool PropertySpecification::ParsePropertyDeclaration(PropertyDictionary& dictionary, const std::string& property_name, const std::string& property_value) const
{
	// Try as a property first
	PropertyId property_id = property_map->GetId(property_name);
	if (property_id != PropertyId::Invalid) {
		if (ParsePropertyDeclaration(dictionary, property_id, property_value)) {
			return true;
		}
	}

	// Then, as a shorthand
	ShorthandId shorthand_id = shorthand_map->GetId(property_name);
	if (shorthand_id != ShorthandId::Invalid) {
		if (ParseShorthandDeclaration(dictionary, shorthand_id, property_value)){
			return true;
		}
	}

	return false;
}

bool PropertySpecification::ParsePropertyDeclaration(PropertyDictionary& dictionary, PropertyId property_id, const std::string& property_value) const
{
	// Parse as a single property.
	const PropertyDefinition* property_definition = GetProperty(property_id);
	if (!property_definition)
		return false;

	std::vector<std::string> property_values;
	if (!ParsePropertyValues(property_values, property_value, false) || property_values.size() == 0)
		return false;

	Property new_property;
	if (!property_definition->ParseValue(new_property, property_values[0]))
		return false;
	
	dictionary[property_id] = new_property;
	return true;
}

// Parses a property declaration, setting any parsed and validated properties on the given dictionary.
bool PropertySpecification::ParseShorthandDeclaration(PropertyDictionary& dictionary, ShorthandId shorthand_id, const std::string& property_value) const
{
	std::vector<std::string> property_values;
	if (!ParsePropertyValues(property_values, property_value, true) || property_values.size() == 0)
		return false;

	// Parse as a shorthand.
	const ShorthandDefinition* shorthand_definition = GetShorthand(shorthand_id);
	if (!shorthand_definition)
		return false;

	// If this definition is a 'box'-style shorthand (x-top, x-right, x-bottom, x-left, etc) and there are fewer
	// than four values
	if (shorthand_definition->type == ShorthandType::Box &&
		property_values.size() < 4)
	{
		// This array tells which property index each side is parsed from
		std::array<int, 4> box_side_to_value_index = { 0,0,0,0 };
		switch (property_values.size())
		{
		case 1:
			// Only one value is defined, so it is parsed onto all four sides.
			box_side_to_value_index = { 0,0,0,0 };
			break;
		case 2:
			// Two values are defined, so the first one is parsed onto the top and bottom value, the second onto
			// the left and right.
			box_side_to_value_index = { 0,1,0,1 };
			break;
		case 3:
			// Three values are defined, so the first is parsed into the top value, the second onto the left and
			// right, and the third onto the bottom.
			box_side_to_value_index = { 0,1,2,1 };
			break;
		default:
			assert(false);
			break;
		}

		for (int i = 0; i < 4; i++)
		{
			assert(shorthand_definition->items[i].type == ShorthandItemType::Property);
			Property new_property;
			int value_index = box_side_to_value_index[i];
			if (!shorthand_definition->items[i].property_definition->ParseValue(new_property, property_values[value_index]))
				return false;

			dictionary[shorthand_definition->items[i].property_definition->GetId()] = new_property;
		}
	}
	else if (shorthand_definition->type == ShorthandType::RecursiveRepeat)
	{
		bool result = true;

		for (size_t i = 0; i < shorthand_definition->items.size(); i++)
		{
			const ShorthandItem& item = shorthand_definition->items[i];
			if (item.type == ShorthandItemType::Property)
				result &= ParsePropertyDeclaration(dictionary, item.property_id, property_value);
			else if (item.type == ShorthandItemType::Shorthand)
				result &= ParseShorthandDeclaration(dictionary, item.shorthand_id, property_value);
			else
				result = false;
		}

		if (!result)
			return false;
	}
	else if (shorthand_definition->type == ShorthandType::RecursiveCommaSeparated)
	{
		std::vector<std::string> subvalues;
		StringUtilities::ExpandString(subvalues, property_value);

		size_t num_optional = 0;
		for (auto& item : shorthand_definition->items)
			if (item.optional)
				num_optional += 1;

		if (subvalues.size() + num_optional < shorthand_definition->items.size())
		{
			// Not enough subvalues declared.
			return false;
		}

		size_t subvalue_i = 0;
		for (size_t i = 0; i < shorthand_definition->items.size() && subvalue_i < subvalues.size(); i++)
		{
			bool result = false;

			const ShorthandItem& item = shorthand_definition->items[i];
			if (item.type == ShorthandItemType::Property)
				result = ParsePropertyDeclaration(dictionary, item.property_id, subvalues[subvalue_i]);
			else if (item.type == ShorthandItemType::Shorthand)
				result = ParseShorthandDeclaration(dictionary, item.shorthand_id, subvalues[subvalue_i]);

			if (result)
				subvalue_i += 1;
			else if (!item.optional)
				return false;
		}
	}
	else
	{
		size_t value_index = 0;
		size_t property_index = 0;

		for (; value_index < property_values.size() && property_index < shorthand_definition->items.size(); property_index++)
		{
			Property new_property;

			if (!shorthand_definition->items[property_index].property_definition->ParseValue(new_property, property_values[value_index]))
			{
				// This definition failed to parse; if we're falling through, try the next property. If there is no
				// next property, then abort!
				if (shorthand_definition->type == ShorthandType::FallThrough)
				{
					if (property_index + 1 < shorthand_definition->items.size())
						continue;
				}
				return false;
			}

			dictionary[shorthand_definition->items[property_index].property_id] = new_property;

			// Increment the value index, unless we're replicating the last value and we're up to the last value.
			if (shorthand_definition->type != ShorthandType::Replicate ||
				value_index < property_values.size() - 1)
				value_index++;
		}
	}

	return true;
}

bool PropertySpecification::ParsePropertyValues(std::vector<std::string>& values_list, const std::string& values, bool split_values) const
{
	std::string value;

	enum ParseState { VALUE, VALUE_PARENTHESIS, VALUE_QUOTE };
	ParseState state = VALUE;
	int open_parentheses = 0;

	size_t character_index = 0;
	char previous_character = 0;
	while (character_index < values.size())
	{
		char character = values[character_index];
		character_index++;

		switch (state)
		{
			case VALUE:
			{
				if (character == ';')
				{
					value = StringUtilities::StripWhitespace(value);
					if (value.size() > 0)
					{
						values_list.push_back(value);
						value.clear();
					}
				}
				else if (StringUtilities::IsWhitespace(character))
				{
					if (split_values)
					{
						value = StringUtilities::StripWhitespace(value);
						if (value.size() > 0)
						{
							values_list.push_back(value);
							value.clear();
						}
					}
					else
						value += character;
				}
				else if (character == '"')
				{
					if (split_values)
					{
						value = StringUtilities::StripWhitespace(value);
						if (value.size() > 0)
						{
							values_list.push_back(value);
							value.clear();
						}
						state = VALUE_QUOTE;
					}
					else
					{
						value += ' ';
						state = VALUE_QUOTE;
					}
				}
				else if (character == '(')
				{
					open_parentheses = 1;
					value += character;
					state = VALUE_PARENTHESIS;
				}
				else
				{
					value += character;
				}
			}
			break;

			case VALUE_PARENTHESIS:
			{
				if (previous_character == '/')
				{
					if (character == ')' || character == '(')
						value += character;
					else
					{
						value += '/';
						value += character;
					}
				}
				else
				{
					if (character == '(')
					{
						open_parentheses++;
						value += character;
					}
					else if (character == ')')
					{
						open_parentheses--;
						value += character;
						if (open_parentheses == 0)
							state = VALUE;
					}
					else if (character != '/')
					{
						value += character;
					}
				}
			}
			break;

			case VALUE_QUOTE:
			{
				if (previous_character == '/')
				{
					if (character == '"')
						value += character;
					else
					{
						value += '/';
						value += character;
					}
				}
				else
				{
					if (character == '"')
					{
						if (split_values)
						{
							value = StringUtilities::StripWhitespace(value);
							if (value.size() > 0)
							{
								values_list.push_back(value);
								value.clear();
							}
						}
						else
							value += ' ';
						state = VALUE;
					}
					else if (character != '/')
					{
						value += character;
					}
				}
			}
		}

		previous_character = character;
	}

	if (state == VALUE)
	{
		value = StringUtilities::StripWhitespace(value);
		if (value.size() > 0)
			values_list.push_back(value);
	}

	return true;
}

} // namespace Rml
