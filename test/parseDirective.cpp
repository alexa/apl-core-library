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
 */
/*
 * Loading and displaying results of parsing of a directive.
 */

#include <clocale>
#ifdef APL_CORE_UWP
    #include <direct.h>
    #define getcwd _getcwd
    #define chdir _chdir
#else
    #include <unistd.h>
#endif

#include "apl/content/directive.h"

#include "utils.h"

using namespace apl;

const char *USAGE_STRING = R"(
parseDirective [OPTIONS] DIRECTIVE

  Parse a directive and inflate it into the virtual DOM hierarchy.
  If the directive depends upon external packages, the parser expects
  to find those packages in the local 'packages' directory.  Use the
  "-p" option to download and retrieve all required package dependencies.
  To automatically download packages your host operating system needs
  either curl or wget installed.
)";


struct Options {
    Options()
        : fixFields(false),
          downloadPackages(false),
          useWget(false),
          verbose(false),
          packageDirectory("packages") {}

    bool fixFields;
    bool downloadPackages;
    bool useWget;
    bool verbose;
    std::string packageDirectory;
};

Options gOptions;

inline std::string
doubleToString(double value)
{
    static char *separator = std::localeconv()->decimal_point;

    if (value > 100000000000) return "INF";
    if (value < -100000000000) return "-INF";

    auto s = std::to_string(value);
    auto it = s.find_last_not_of('0');
    if (it != s.find(separator))   // Remove a trailing decimal point
        it++;
    s.erase(it, std::string::npos);
    return s;
}

/**
 * Simplify the transform
 */
void fixArray(rapidjson::Value& tree, const char *name, rapidjson::Document::AllocatorType& alloc)
{
    auto it = tree.FindMember(name);
    if (it == tree.MemberEnd())
        return;

    auto& transform = it->value;
    std::vector<double> values;
    for (int i = 0; i < transform.Size(); i++)
        values.push_back(transform[i].GetDouble());

    auto s = "["+ std::accumulate(values.begin(), values.end(), std::string(),
                             [](const std::string& a, double b) -> std::string {
                                 return a + (a.empty() ? "" : ",") + doubleToString(b);
                             }) + "]";
    transform.SetString(s.c_str(), s.size(), alloc);
}

void downloadURL(const std::string& url)
{
    auto cmd = (gOptions.useWget ? "wget -q -o document.json " : "curl -s -o document.json ") + url;
    if (gOptions.verbose)
        std::cerr << "Downloading " << url << std::endl;
    int result = system(cmd.c_str());
    if (result) {
        std::cerr << "download of "+url+" failed" << std::endl;
        exit(1);
    }
}

void fixComponentTree(rapidjson::Value& tree, rapidjson::Document::AllocatorType& alloc)
{
    fixArray(tree, "_transform", alloc);
    fixArray(tree, "_innerBounds", alloc);
    fixArray(tree, "_bounds", alloc);
    fixArray(tree, "_borderRadii", alloc);

    // Children are recursively processed
    auto children = tree.FindMember("children");
    if (children != tree.MemberEnd())
        for (auto it = children->value.Begin(); it != children->value.End(); it++)
            fixComponentTree(*it, alloc);
}

#ifndef PATH_MAX
// if not already defined, define it.
// value is borrowed from the clang toolchain's definition
#define PATH_MAX 260
#endif

class WorkingDirectory {
public:
    WorkingDirectory(std::initializer_list<std::string> list) {
        char temp[PATH_MAX];
        if (getcwd(temp, sizeof(temp)) == nullptr) {
            std::cerr << "Unable to get current directory" << std::endl;
            exit(1);
        }

        mWorkingDirectory = temp;

        for (const auto& item : list) {
            int status = createDirectory(item);
            if (status != 0 && errno != EEXIST) {
                std::cerr << "Unable to create directory " << item << ": " << errno << std::endl;
                exit(1);
            }
            status = chdir(item.c_str());
            if (status != 0) {
                std::cerr << "Unable to chdir " << item << " : " << errno << std::endl;
                exit(1);
            }
        }
    }

    ~WorkingDirectory() {
        int status = chdir(mWorkingDirectory.c_str());
        if (status != 0) {
            std::cerr << "Unable to restore working directory " << mWorkingDirectory << " : " << errno << std::endl;
            exit(1);
        }
    }

private:
    std::string mWorkingDirectory;
};

int
main(int argc, char *argv[]) {
    ArgumentSet argumentSet(USAGE_STRING);
    argumentSet.add({
                        Argument(
                            "-f",
                            "--fix",
                            Argument::NONE,
                            "Fix various fields to be more clear",
                            "",
                            [&](const std::vector<std::string>&) { gOptions.fixFields = true; }),
                        Argument(
                            "-p",
                            "--packages",
                            Argument::NONE,
                            "Download all referenced packages in the content",
                            "",
                            [&](const std::vector<std::string>&) { gOptions.downloadPackages = true; }),
                        Argument(
                            "",
                            "--wget",
                            Argument::NONE,
                            "Use 'wget' instead of 'curl' to download packages",
                            "",
                            [&](const std::vector<std::string>&) { gOptions.useWget = true; }
                        ),
                        Argument(
                            "-v",
                            "--verbose",
                            Argument::NONE,
                            "Verbose mode",
                            "",
                            [&](const std::vector<std::string>&) { gOptions.verbose = true; }
                        ),
                        Argument(
                            "",
                            "--package-dir",
                            Argument::ONE,
                            "Set the directory packages will be stored in",
                            "DIR",
                            [&](const std::vector<std::string>& s) { gOptions.packageDirectory = s[0]; }
                        )
                    });

    ViewportSettings settings(argumentSet);

    std::vector<std::string> args(argv + 1, argv + argc);
    argumentSet.parse(args);

    if (args.size() != 1) {
        std::cerr << "Must supply a single directive" << std::endl;
        exit(1);
    }

    try {
        auto doc = Directive::create(loadFile(args[0]).c_str());
        auto content = doc->content();
        if (!content) {
            std::cerr << "Unable to load document" << std::endl;
            exit(1);
        } else {
            while (content->isWaiting()) {
                auto pkgList = content->getRequestedPackages();
                for (const auto& m : pkgList) {
                    std::cerr << "Loading package " << m.reference().name() << std::endl;
                    const auto& packageName = m.reference().name();
                    const auto& packageVersion = m.reference().version();

                    if (gOptions.downloadPackages) {
                        WorkingDirectory wd({gOptions.packageDirectory, packageName, packageVersion});
                        auto url = std::string("https://d2na8397m465mh.cloudfront.net/packages/") + packageName + "/" +
                                   packageVersion + "/document.json";
                        downloadURL(url);
                    }

                    auto fname =
                        gOptions.packageDirectory + "/" + packageName + "/" + packageVersion + "/document.json";
                    auto data = loadFile(fname);
                    if (data.empty()) {
                        std::cerr << "unable to find file " << fname << std::endl;
                        exit(1);
                    }
                    content->addPackage(m, data.c_str());
                    if (content->isError())
                        exit(1);
                }
            }

            RootConfig rootConfig;
            rootConfig.enforceAPLVersion(APLVersion::kAPLVersionIgnore);

            auto root = doc->build(settings.metrics(), rootConfig);
            if (!root) {
                std::cerr << "Failed to build" << std::endl;
                exit(1);
            }
            auto componentPtr = root->topComponent();
            rapidjson::Document docTree;
            auto tree = componentPtr->serializeAll(docTree.GetAllocator());

            if (gOptions.fixFields)
                fixComponentTree(tree, docTree.GetAllocator());

            char writeBuffer[65536];
            rapidjson::FileWriteStream os(stdout, writeBuffer, sizeof(writeBuffer));
            rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer(os);
            tree.Accept(writer);
        }
    }
    catch (std::exception e) {
        std::cerr << "Parse error!" << e.what() << std::endl;
        exit(1);
    }
}
