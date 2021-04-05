/**
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include "enumparser.h"

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/abnf.hpp>

#include <sstream>
#include <set>

namespace pegtl = tao::TAO_PEGTL_NAMESPACE;
using namespace pegtl;


namespace enums {

// ****************** Data Structures ****************

/**
 * A single entry in an enumeration.  The item may have a numeric value assigned to it,
 * or it may reference an entry from a different enumeration.
 */
struct EnumItemP {
    std::string name;       // The name of the enumerated value
    std::string comment;

    enum ValueType { EMPTY, ASSIGNED, LOCALREF, REMOTEREF };
    ValueType type = EMPTY;
    int assigned;               // ASSIGNED value
    std::string ref_name;       // Name of an enumerated value in a LOCALREF or REMOTEREF
    std::string ref_namespace;  // Name of a REMOTEREF enumeration
};

using EnumItemPtr = std::shared_ptr<EnumItemP>;

/**
 * A C++ enumeration.  This contains a list and map of EnumItemP.
 */
struct EnumerationP {
    explicit EnumerationP(std::string name) : name(std::move(name)) {}

    std::string name;
    std::vector<EnumItemPtr> values;
    std::map<std::string, EnumItemPtr> valuesMap;
    std::set<std::string> references;
    int tmp;      // Temporary value, used for sorting
};

using EnumerationPtr = std::shared_ptr<EnumerationP>;

/**
 * Private storage class for EnumParser
 */
struct EnumParserP {
    std::map<std::string, EnumerationPtr> enumerations;

    std::vector<EnumerationPtr> sort();
};

/**
 * Parser state
 */
struct State {
    explicit State(std::string msg) : msg(std::move(msg)) {}

    std::string comment;        // The last comment read in
    int commentLine = -1;       // The line the last comment occurred on.
    EnumerationPtr workingEnum; // The current enumeration we are working on
    EnumItemPtr workingItem;    // The current item in the enumeration we are processing
    int workingItemLine = -1;   // The line of the source file where the current enumeration item starts

    std::vector<EnumerationPtr> enums;
    std::map<std::string, EnumerationPtr> enumsMap;
    std::string msg;
};


// ******************* Grammar Rules *********************

/*
 * enum [class] <enum_name> {
 *   <item_name>,                    // Implicit assignment (Enumeration.index++)
 *   <item_name> = NUMBER,           // Number
 *   <item_name> = STRING,           // Match a previous enum_number in the same enum_number class
 *   <item_name> = STRING::STRING,   // Assign a value from another enumeration
 * };
 *
 * Note that we do not correctly parse all variations of the C++-11 enumeration declaration.
 * Please check your output and update this code as necessary.
 */

template<typename Key>
struct key : seq<Key, not_at< identifier_other >> {};

struct sym_enum : string<'e','n','u','m'>{};
struct sym_class : string<'c','l','a','s','s'>{};
struct key_enum : key<sym_enum>{};
struct key_class : key<sym_class>{};

struct single_line_comment : seq< string<'/','/'>, until<eolf>> {};
struct multi_line_comment : seq< string<'/','*'>, until<string<'*','/'>>> {};
struct comment : sor<single_line_comment, multi_line_comment> {};
struct sep : sor<space, comment> {};
struct ws : star<sep> {};

struct enum_number : seq<opt<one<'-'>>, plus<digit>> {};
struct ref1 : identifier {};
struct ref2 : identifier {};
struct enum_ref : seq<ref1, opt_must<string<':', ':'>, ref2>> {};

struct enum_assignment : if_must<one<'='>, ws, sor<enum_number, enum_ref>> {};
struct enum_item_name : identifier {};
struct enum_item : seq<enum_item_name, opt<ws, enum_assignment>> {};

struct enumerator_list : list_tail<enum_item, one<','>, sep>{};
struct enum_name : identifier {};    // The global name of the enumeration
struct enum_key : seq<key_enum, opt<ws, key_class>> {};   // "enum_number" or "enum_number class"
struct enumeration : seq<enum_key, ws, enum_name, ws, one<'{'>, ws, enumerator_list, ws, one<'}'>, ws, one<';'>> {};
struct cpp : star<sor<enumeration, any>> {};


// ************** Parsing actions ****************

/*
 * A comment assigned to a single enumeration item is stored and copied to the output file.
 * This is a little tricky because the comment could occur on a line BEFORE the enumeration item
 * or it could appear on the same line AFTER the enumeration item.  For example:
 *
 * enum Sample {
 *   kFirst,  // This comment is attached to "kFirst"
 *   // This comment is attached to "kSecond"
 *   kSecond,
 *   // This comment will be ignored because another is attached to "kThird"
 *   kThird,  // This comment is attached to "kThird"
 *   kFourth,
 *   // "kFourth" has no comments attached to it because this comment is on the line AFTER "kFourth"
 * };
 *
 */

template <typename Rule>
struct enumaction : pegtl::nothing<Rule> {};

template <>
struct enumaction<comment> {
    template< typename Input >
    static void apply(const Input& in, State& state) {
        if (EnumParser::sVerbosity > 3)
            std::cout << "  comment @" << in.position().line << " '" << in.string() << "'" << std::endl;

        state.comment = in.string();
        // Strip any trailing carriage return
        state.comment.erase(std::remove(state.comment.begin(), state.comment.end(), '\n'), state.comment.end());
        state.commentLine = in.position().line;

        // If this comment is on the same line as the previous enumerated value, attach the comment to that value
        if (!state.comment.empty() && state.workingItem && state.workingItemLine == state.commentLine) {
            state.workingItem->comment = state.comment;
            state.comment = "";
            state.commentLine = 0;
        }
    }
};

template <>
struct enumaction<enum_item_name> {
    template< typename Input >
    static void apply(const Input& in, State& state) {
        if (EnumParser::sVerbosity > 3)
            std::cout << "  enum_item_name " << in.string() << " position=" << in.position().line << std::endl;

        // Start a new enumeration item.
        // If there was an old item, push it onto the list and map of enumerated items
        if (state.workingItem) {
            state.workingEnum->values.emplace_back(state.workingItem);
            state.workingEnum->valuesMap[state.workingItem->name] = state.workingItem;
        }

        // Create a new "working" enumeration item
        state.workingItem = std::make_shared<EnumItemP>();
        state.workingItem->name = in.string();
        state.workingItem->comment = state.comment;

        state.workingItemLine = in.position().line;

        // Clear out the last comment
        state.comment = "";
        state.commentLine = -1;
    }
};

template <>
struct enumaction<ref1> {
    template< typename Input >
    static void apply(const Input& in, State& state) {
        if (EnumParser::sVerbosity > 3)
            std::cout << "  ref1 " << in.string() << std::endl;

        state.workingItem->ref_name = in.string();
    }
};

template <>
struct enumaction<ref2> {
    template< typename Input >
    static void apply(const Input& in, State& state) {
        if (EnumParser::sVerbosity > 3)
            std::cout << "  ref2 " << in.string() << std::endl;

        // Shift the old name over to be the namespace
        state.workingItem->ref_namespace = state.workingItem->ref_name;
        state.workingItem->ref_name = in.string();
    }
};


template <>
struct enumaction<enum_ref> {
    template< typename Input >
    static void apply(const Input& in, State& state) {
        if (EnumParser::sVerbosity > 3)
            std::cout << "  enum_ref_remote " << in.string() << std::endl;

        if (state.workingItem->ref_namespace.empty()) {  // Local reference
            // Check if this "bare" value is a previously-defined enumeration value
            auto it = state.workingEnum->valuesMap.find( state.workingItem->ref_name );
            if (it != state.workingEnum->valuesMap.end())
                state.workingItem->type = EnumItemP::LOCALREF;
            else
                throw pegtl::parse_error("Unrecognized local enumerated value", in);
        }
        else {
            state.workingItem->type = EnumItemP::REMOTEREF;
            state.workingEnum->references.emplace(state.workingItem->ref_namespace);
        }
    }
};

template <>
struct enumaction<enum_number> {
    template< typename Input >
    static void apply(const Input& in, State& state) {
        if (EnumParser::sVerbosity > 3)
            std::cout << "  enum_number " << in.string() << std::endl;

        state.workingItem->type = EnumItemP::ASSIGNED;
        state.workingItem->assigned = std::stoi(in.string());
    }
};

template <>
struct enumaction<enum_name> {
    template< typename Input >
    static void apply(const Input& in, State& state) {
        if (EnumParser::sVerbosity > 2)
            std::cout << "  start enumeration " << in.string() << std::endl;

        // If there was an existing enumeration, it has failed
        if (state.workingEnum)
            std::cerr << "FAILED " << state.workingEnum->name << std::endl;

        // Start tracking a new enumeration.  Clear all of the state values back to the defaults.
        state.comment = "";
        state.commentLine = -1;
        state.workingEnum = std::make_shared<EnumerationP>(in.string());
        state.workingItem = nullptr;
        state.workingItemLine = -1;
    }
};

template <>
struct enumaction<enumeration> {
    template< typename Input >
    static void apply(const Input& in, State& state) {
        if (EnumParser::sVerbosity > 1)
            std::cout << "Processed " << in.string()
                      << "   [" << state.msg << ":" << in.position().line << "]" << std::endl;

        // This enumeration has been accepted.
        // If an item was in progress, push it onto the list/map of items
        if (state.workingItem) {
            state.workingEnum->values.emplace_back(state.workingItem);
            state.workingEnum->valuesMap[state.workingItem->name] = state.workingItem;
        }

        // Copy the working enumeration into the official enumeration list and map.
        assert(state.workingEnum);
        state.enums.emplace_back(state.workingEnum);
        state.enumsMap[state.workingEnum->name] = state.workingEnum;

        // Clear out any working items so that comments will not be stored on them
        state.workingEnum = nullptr;
        state.workingItem = nullptr;
    }
};


void
visit(const EnumerationPtr& ptr, std::vector<EnumerationPtr>& result, const std::map<std::string, EnumerationPtr>& map)
{
    if (ptr->tmp == 2)   // Has already been sorted
        return;

    if (ptr->tmp == 1)   // There is a loop in the dependencies
        throw std::runtime_error("Dependency loop in enumerations");

    ptr->tmp = 1;   // Mark it as a being in process
    for (const auto& ref : ptr->references)
        visit(map.at(ref), result, map);
    ptr->tmp = 2;   // It's been processed

    result.emplace_back(ptr);
}

std::vector<EnumerationPtr>
EnumParserP::sort()
{
    // Copy all enumerations into a list and reset their "tmp" values
    std::vector<EnumerationPtr> working;
    for (auto& m : enumerations) {
        m.second->tmp = 0;
        working.emplace_back(m.second);
    }

    std::vector<EnumerationPtr> result;
    for (auto& m : working)
        visit(m, result, enumerations);

    return result;
}



int EnumParser::sVerbosity = 0;

EnumParser::EnumParser()
    : mPrivate(new EnumParserP)
{
}

// Note: This must be defined here because we are using std::unique_ptr<EnumParserP> to hold private class data.
EnumParser::~EnumParser() = default;

void
EnumParser::add(std::istream&& input, std::string msg)
{
    std::string line;
    std::stringstream ss;

    // Parse individual lines to remove preprocessor directives
    // For now we simply strip them out
    while (std::getline(input, line)) {
        if (line.empty() || line.at(0) == '#')
            continue;
        ss << line << '\n';
    }

    State state(std::move(msg));
    pegtl::string_input<> in(ss.str(), "");
    try {
        pegtl::parse<cpp, enumaction>(in, state);
        mPrivate->enumerations.insert(state.enumsMap.begin(), state.enumsMap.end());
    }
    catch (pegtl::parse_error& e) {
        const auto p = e.positions.front();
        auto start = p.byte > 20 ? p.byte - 20 : 0;
        throw std::runtime_error("Unable to parse file " + msg + ":" + e.what() + " [" + ss.str().substr(start, 40) + "]");
    }
}


EnumMap
EnumParser::enumerations()
{
    EnumMap result;

    // An enumeration can reference a value defined in a different enumeration.
    // Hence, we need to topologically sort the processing order of the enumerations.
    auto enumerations = mPrivate->sort();

    for (const auto& m : enumerations) {
        std::vector<EnumItem> items;
        int next_index = 0;

        for (const auto& val : m->values) {
            int value = next_index;
            switch (val->type) {
                case EnumItemP::EMPTY:
                    break;
                case EnumItemP::ASSIGNED:
                    value = val->assigned;
                    break;
                case EnumItemP::LOCALREF: {
                    auto it = std::find_if(items.begin(), items.end(), [&](const EnumItem& item) {
                        return item.name == val->ref_name;
                    });
                    if (it == items.end())
                        throw std::runtime_error("Enumeration " + m->name + " value " +
                        val->name + " depends on missing value " + val->ref_name);
                    value = it->value;
                }
                    break;
                case EnumItemP::REMOTEREF: {
                    auto it = result.find(val->ref_namespace);
                    if (it == result.end())
                        throw std::runtime_error(
                            "Enumeration " + m->name + " value " + val->name + " depends on missing other enumeration");
                    auto it2 = std::find_if(it->second.begin(), it->second.end(), [&](const EnumItem& item) {
                        return item.name == val->ref_name;
                    });
                    if (it2 == it->second.end())
                        throw std::runtime_error(
                            "Enumeration " + m->name + " value " + val->name + " depends on a missing value");
                    value = it2->value;
                }
                    break;
            }
            next_index = value + 1;
            items.emplace_back(EnumItem{val->name, value, val->comment});
        }

        result.emplace(m->name, std::move(items));
    }

    return result;
}

void
addAutoGenComments(std::ostream& out) {
    out << "/*\n"
        << " * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.\n"
        << " */\n\n"
        << "/*\n"
        <<  " * AUTOGENERATED FILE. DO NOT MODIFY!\n"
        << " * This file is autogenerated by enumgen.\n"
        << " */\n\n";
}

std::ostream&
writeJava(std::ostream& out, const std::string& package, const std::string& name, const std::vector<EnumItem>& values)
{
    addAutoGenComments(out);
    out << "package " << package << ";\n\n";

    out << "import android.util.SparseArray;\n\n";
    out << "public enum " << name << " implements APLEnum {\n\n";
    for (int i = 0; i < values.size(); i++) {
        const auto& val = values[i];
        if (!val.comment.empty())
            out << "    " << val.comment << "\n";

        out << "    " << val.name << "(" << val.value << ")"
            << (i != values.size() - 1 ? "," : ";") << "\n";
    }

    out << "\n";
    out << "    private static SparseArray<" << name << "> values = null;\n";
    out << "\n";
    out << "    public static " << name << " valueOf(int idx) {\n";
    out << "        if(" << name << ".values == null) {\n";
    out << "            " << name << ".values = new SparseArray<>();\n";
    out << "            " << name << "[] values = " << name << ".values();\n";
    out << "            for(" << name << " value : values) {\n";
    out << "                " << name << ".values.put(value.getIndex(), value);\n";
    out << "            }\n";
    out << "        }\n";
    out << "        return " << name << ".values.get(idx);\n";
    out << "    }\n";
    out << "\n";
    out << "    private final int index;\n";
    out << "\n";
    out << "    " << name << " (int index) {\n";
    out << "        this.index = index;\n";
    out << "    }\n";
    out << "\n";
    out << "    @Override\n";
    out << "    public int getIndex() { return this.index; }\n";
    out << "}\n";

    return out;
}

std::ostream&
writeTypeScript(std::ostream& out, const std::string& name, const std::vector<EnumItem>& values)
{
    addAutoGenComments(out);

    out << "export enum " << name << " {\n";
    for (const auto& val : values) {
        if (!val.comment.empty())
            out << "    " << val.comment << "\n";
        out << "    " << val.name << " = " << val.value << ",\n";
    }
    out << "}\n";

    return out;
}


} // namespace enums