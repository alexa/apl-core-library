/*
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
 *
 * Command line parsing for viewport settings
 */

#ifndef _VIEWPORT_SETTINGS_H
#define _VIEWPORT_SETTINGS_H

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/error/en.h"

#include "rapidjson/stringbuffer.h"
#include "rapidjson/ostreamwrapper.h"

#include "apl/apl.h"
#include "apl/engine/context.h"
#include "apl/utils/streamer.h"
#include "apl/component/corecomponent.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <numeric>
#include <chrono>

#ifdef APL_CORE_UWP
#include <direct.h>   // _getcwd, _mkdir
#define getcwd _getcwd
#else
#include <sys/stat.h>
#endif

class Argument {
public:
    enum ArgCount {
        NONE = ' ',
        ONE = '1',
        TWO = '2',
    };

    using ArgFunc = std::function<void(const std::vector<std::string>&)>;


    explicit Argument(const char *name,
                      const char *altName,
                      ArgCount argCount,
                      const char *description,
                      const char *token,
                      ArgFunc&& func)
          : mArgCount(argCount),
            mNames({name, altName}),
            mFunction(std::move(func)),
            mDescription(description),
            mToken(token)
    {
    }

    bool consume(std::vector<std::string>& args) const {
        auto it = args.begin();
        if (std::find(mNames.begin(), mNames.end(), args.at(0)) == mNames.end())
            return false;

        it = args.erase(it);

        switch (mArgCount) {
            case NONE:
                mFunction({""});
                return true;

            case ONE:
                if (it == args.end())
                    throw std::runtime_error("Expected argument after " + mNames.at(0));
                mFunction({*it});
                args.erase(it);
                return true;

            case TWO:
                if (it == args.end() || (it + 1) == args.end())
                    throw std::runtime_error("Expected two arguments after " + mNames.at(0));
                mFunction({*it, *(it+1)});
                args.erase(it, it+2);
                return true;
        }

        throw std::runtime_error("Should not reach here");
    }

    std::pair<std::string, std::string> getDocString() const {
        auto s = std::accumulate(mNames.begin(), mNames.end(), std::string(),
                                 [](const std::string& a, const std::string& b) -> std::string {
                                     return a + (a.empty() ? "" : ", ") + b;
                                 }) + " " + mToken;
        return {s, mDescription};
    }

private:
    ArgCount mArgCount;
    std::vector<std::string> mNames;
    ArgFunc mFunction;
    std::string mDescription;
    std::string mToken;
};

class ArgumentSet {
public:
    ArgumentSet(const char *usageText) : mUsage(usageText) {
        mArguments.emplace_back(Argument(
            "-h",
            "--help",
            Argument::NONE,
            "Show this help",
            "",
            [&](const std::vector<std::string>&) -> void {
                usage();
                exit(0);
            }
        ));
    }

    void add(std::vector<Argument>&& arguments) {
        mArguments.insert(mArguments.end(), arguments.begin(), arguments.end());
    }

    void usage() {
        std::vector<std::pair<std::string, std::string>> displayList;
        for (const auto& m : mArguments)
            displayList.emplace_back(m.getDocString());

        // Find the widest
        int len = 0;
        for (const auto& m : displayList) {
            auto i = m.first.size();
            if (i > len)
                len = i;
        }

        sort(displayList.begin(), displayList.end());

        std::cout << mUsage << std::endl << "Options" << std::endl << std::endl;

        // Print them out
        for (const auto& m : displayList)
            std::cout << "  " << m.first << std::string(len + 2 - m.first.size(), ' ') << m.second << std::endl;
    }

    void parse(std::vector<std::string>& args) {
        try {
            while (!args.empty()) {
                // Exit with the first argument that doesn't start with '-'
                if (args.at(0).empty() || args.at(0).at(0) != '-')
                    return;

                // Also exit for a "--" marker (but remove it)
                if (args.at(0) == "--") {
                    args.erase(args.begin());
                    return;
                }

                bool foundArg = false;
                for (const auto& m : mArguments) {
                    if (m.consume(args)) {
                        foundArg = true;
                        break;
                    }
                }
                if (!foundArg)
                    throw std::runtime_error("Unexpected argument: "+args.at(0));
            }
        }
        catch (std::runtime_error& e) {
            std::cerr << e.what() << std::endl;
            exit(-1);
        }
    }

private:
    std::string mUsage;
    std::vector<Argument> mArguments;
};


class ViewportSettings {
public:
    ViewportSettings(ArgumentSet& argumentSet)
        : mWidth(1280),
          mHeight(800),
          mDpi(160),
          mIsRound(false),
          mTheme("dark")
    {
        argumentSet.add({
            Argument(
                "-s",
                "--size",
                Argument::ONE,
                "Set the size of the viewport.  Should be in the form WIDTHxHEIGHT in pixels.",
                "WIDTHxHEIGHT",
                [this](const std::vector<std::string>& value) -> void {
                    auto offset = value[0].find("x");
                    if (offset == std::string::npos || offset == 0 ||
                        offset == value.size() - 1)
                        throw std::runtime_error("size expects a width/height pair of the form WIDTHxHEIGHT pixels");

                    mWidth = std::stoi(value[0].substr(0, offset));
                    mHeight = std::stoi(value[0].substr(offset + 1));
                }
            ),
            Argument(
                "-d",
                "--dpi",
                Argument::ONE,
                "Set the DPI of the viewport.  Should be an integer",
                "DPI",
                [this](const std::vector<std::string>& value) -> void { mDpi = std::stoi(value[0]); }
            ),
            Argument(
                "-r",
                "--round",
                Argument::NONE,
                "Change the viewport type from Rectangle to Round",
                "",
                [this](const std::vector<std::string>& value) -> void { mIsRound = true; }
            ),
            Argument(
                "-t",
                "--theme",
                Argument::ONE,
                "Set the default theme of the device",
                "THEME",
                [this](const std::vector<std::string>& value) -> void { mTheme = value[0]; }
            ),
            Argument(
                "",
                "--def",
                Argument::TWO,
                "Add a mutable variable to the context. The value should be in JSON format",
                "NAME VALUE",
                [this](const std::vector<std::string>& value) -> void {
                  rapidjson::Document doc;
                  rapidjson::ParseResult result = doc.Parse(value[1].c_str());
                  if (!result) {
                      std::cerr << "Unable to parse '" << value[1] << "'" << std::endl;
                      exit(-1);
                  }
                  auto v = doc.IsString() ? apl::Object(doc.GetString()) :
                           doc.IsNumber() ? apl::Object(doc.GetDouble()) :
                           doc.IsNull() ? apl::Object::NULL_OBJECT() :
                           doc.IsBool() ? apl::Object(doc.GetBool()) :
                           apl::Object(std::move(doc));
                  mVariables.emplace(value[0], v);
                })
        });
    }

    const apl::Metrics metrics() const {
        return apl::Metrics()
            .size(mWidth, mHeight)
            .dpi(mDpi)
            .shape(mIsRound ? apl::ScreenShape::ROUND : apl::ScreenShape::RECTANGLE)
            .theme(mTheme.c_str());
    }

    apl::ContextPtr
    createContext() const
    {
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch());

        auto rootConfig = apl::RootConfig().agent("APL", "1.3").utcTime(now.count());
        auto context = apl::Context::createTestContext(metrics(), rootConfig);
        for (const auto& m : mVariables)
            context->putUserWriteable(m.first, m.second);
        return context;
    }

    friend apl::streamer& operator<<(apl::streamer&, const ViewportSettings&);

private:
    int mWidth;
    int mHeight;
    int mDpi;
    bool mIsRound;
    std::string mTheme;
    apl::ObjectMap mVariables;
};

class MyVisitor : public apl::Visitor<apl::CoreComponent> {
public:
    static void dump(const apl::CoreComponent& component) {
        MyVisitor dv;
        apl::streamer s;
        s << "top: " << component;
        std::cout << s.str() << std::endl;
        std::cout << "Hierarchy" << std::endl;
        component.accept(dv);
        std::cout << "---------" << std::endl;
    }

    MyVisitor() : mIndent(0) {}

    void visit(const apl::CoreComponent& component) {
        std::cout << std::string(mIndent, ' ') << component.name() << std::endl;
        for (const auto& m : component.getCalculated()) {
            apl::streamer s;
            s << m.second;
            std::cout << std::string(mIndent+4, ' ') << apl::sComponentPropertyBimap.at(m.first)
                      << ": " << s.str() << std::endl;
        }
    }

    void push() { mIndent += 2; }
    void pop() { mIndent -= 2; }

private:
    int mIndent;
};


apl::streamer&
operator<<(apl::streamer& os, const ViewportSettings& settings)
{
    return os << "Viewport<width=" << settings.mWidth
        << " height=" << settings.mHeight
        << " dpi=" << settings.mDpi
        << " round=" << (settings.mIsRound ? "yes" : "no")
        << " theme=" << settings.mTheme
        << ">";
}


static std::string
loadFile(const std::string& filename)
{
    std::ifstream t(filename);
    if (!t.is_open())
        return "";

    std::stringstream buffer;
    buffer << t.rdbuf();
    return buffer.str();
}

// Note: This is an inline function to avoid unused function warnings with -Wunused-function
inline apl::RootContextPtr
createContext(std::vector<std::string>& args, const ViewportSettings& settings)
{
    if (args.size() < 1) {
        std::cerr << "Must supply a layout and zero or more data files" << std::endl;
        exit(1);
    }

    // Parse resources
    auto content = apl::Content::create(loadFile(args[0]));
    if (!content) {
        std::cerr << "Content pointer is empty" << std::endl;
        exit(1);
    }
    if (args.size() - 1 != content->getParameterCount()) {
        std::cerr << "Number of data files doesn't match the arguments in the layout" << std::endl;
        exit(1);
    }

    for (int i = 0 ; i < content->getParameterCount() ; i++)
        content->addData(content->getParameterAt(0), loadFile(args[1 + i]));

    if (!content->isReady()) {
        std::cerr << "Illegal content";
        exit(1);
    }

    return apl::RootContext::create(settings.metrics(), content);
}

int
createDirectory(const std::string& path) {
#ifdef APL_CORE_UWP
	// create a directory with the access token of the calling process
    return _mkdir(path.c_str());
#else
    return mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
}

#endif // _VIEWPORT_SETTINGS_H
