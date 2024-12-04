#include <iostream>

#include "src/tag_manager.h"
#include "src/file_handler.h"

#include "argparse/argparse.hpp"

int main(int argc, char* argv[]) {
    using namespace TM;

    argparse::ArgumentParser app ("epictagmanager");
    // formatting help output
    app.set_usage_max_line_width(100);
    app.set_usage_break_on_mutex();

    // input files/directories
    app.add_argument("-i","--input")
    .nargs(argparse::nargs_pattern::at_least_one)
    .help("All processable files and folders")
    .metavar("PATHS");

    // set tag read or write mode
    auto &rwModeGroup = app.add_mutually_exclusive_group();
    rwModeGroup.add_argument("-r", "--read").flag()
    .help("Use read mode: Read all provided tags from input files");
    rwModeGroup.add_argument("-w", "--write").flag()
    .help("Use write mode: Write all provided tags to the input files, replacing any previous values");

    app.add_argument("-v","--verbose")
    .flag()
    .help("Output extra information about what the app is doing");

    app.add_argument("--all")
    .flag()
    .help("Flag that modifies mode behaviour - makes read mode read all tags the input files have, makes write mode write to all provided files instead of the first one");

    // map out all common tags
    std::map<std::vector<std::string>, int> basicTags = {
        {{"-a","--artist"},ARTIST},
        {{"-b","--bpm"},BPM},
        {{"-A","--album"},ALBUM},
        {{"-t","--title"},TITLE},
        {{"-y","--year"},YEAR},
        {{"-c","--comment"},COMMENT},
        {{"-g","--genre"},GENRE},
    };

    // create arguments for each defined tag
    auto &basicTagGroup = app.add_group("Basic tags");
    basicTagGroup.add_description("Read the passed tags when in read mode, writes the passed values in write mode");

    for (auto& [aliases, tag] : basicTags) {
        std::string helpMsg = "Use the " + propKeys[tag] + " tag";

        if (aliases.size() == 2) {
            // if tag has both a short and long alias defined
            basicTagGroup.add_argument(aliases[0], aliases[1]).default_value("").help(helpMsg);
        } else {
            // tag probably only has the short or long alias defined
            basicTagGroup.add_argument(aliases[0]).default_value("").help(helpMsg);
        }
    }

    // for providing img path and reading/extracting
    app.add_argument("-p","--picture")
    .nargs(argparse::nargs_pattern::any)
    .help("Use the PICTURE tag. Use the option when reading to extract all tag data, or provide image paths when writing")
    .metavar("PATHS");

    // try parsing all cli args
    try {
        app.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        // crash and output error info + help message
        std::cerr << err.what() << "\n";
        std::cout << app;
        return 1;
    }

    // process passed arguments
    if (argc == 1) {
        // no arguments passed, print help info
        std::cout << app << std::endl;
        // can be used to launch some sort of interactive mode in the future
    } else {
        // arg mode
        // process all input file paths
        std::vector<std::string> inputFiles;

        try {
            inputFiles = FH::gatherAllFilesFromList(
                app.get<std::vector<std::string>>("--input"),
                true
            );
        } catch (const std::exception &e) {
            std::cerr << "Exception while parsing input files: " << e.what() << std::endl;
        }

        // loop through the defined basic tag aliases and check if each of them was used
        // did not find a good way to go through arguments passed to argparse instead
        std::unordered_set<int> requestedProps;
        std::map<int, std::string> providedVals;
        for (auto& [aliases, type]: basicTags) {
            if (app.is_used(aliases[0])) {
                requestedProps.insert(type);
                providedVals.insert({
                    type,
                    app.get<std::string>(aliases[0])
                });
            }
        }

        if (app["-r"] == true) {
            // read all requested tags
            for (auto& file : inputFiles) {
                // make a fileref out of each file to read the properties
                // the fileref constructor doesn't take std::string directly, only char*, so .data is used
                TagLib::FileRef f (file.data());

                if (app["--all"] == true) {
                    std::cout << "All defined properties of file: " << FH::getFilenameOf(file) << std::endl;
                    printProps(
                        readProps(f, findAllDefinedProps(f))
                    );
                } else {
                    std::cout << "Properties of file: " << FH::getFilenameOf(file) << std::endl;
                    printProps(
                        readProps(f, requestedProps)
                    );
                }

                // extracting images is a heavier operation so it should probably not be included in --all
                if (app.is_used("-p")) {
                    bool success = extractImgTags(f);
                    if (app["--verbose"] == true && success) {
                        std::cout << "Successfully extracted all picture data of " << file;
                    }
                }

                std::cout << std::endl;
            }
        } else if (app["-w"] == true) {
            // writing all passed tags
            // for safety it will only write to the first provided file unless --all is specified
            if (app["--verbose"] == true) {
                if (app["--all"] == true) {
                    std::cout << "Writing provided tags to ALL input files";
                } else {
                    std::cout << "Writing provided tags to first input file - " << inputFiles[0];
                }
                std::cout << std::endl;
            }

            // get all provided images if there are any
            // since the images stay the same, there's no need to re-read them for every input audio
            std::vector<std::string> imgList;
            if (app.is_used("-p")) {
                imgList = FH::gatherAllFilesFromList(
                    app.get<std::vector<std::string>>("--picture"),
                    false
                );
            }

            for (auto& file : inputFiles) {
                std::cout << "Writing properties to " << FH::getFilenameOf(file) << std::endl;

                TagLib::FileRef f (file.data());
                writeProps(f, providedVals);

                if (app["--verbose"] == true) {
                    std::cout << "Properties of file: " << FH::getFilenameOf(file) << std::endl;
                    printProps(
                        readProps(f, requestedProps)
                    );
                    std::cout << std::endl;
                }

                // picture tag used
                if (app.is_used("-p")) {
                    // reset file image data
                    f.setComplexProperties("PICTURE",{});

                    if (imgList.empty()) {
                        if (app["-v"] == true) std::cout << "All picture data removed from " << FH::getFilenameOf(file) << std::endl << std::endl;
                        f.save();
                    }

                    // loop over every image and add them to the input one by one
                    for (auto& imgPath : imgList) {
                        addImgTag(f, imgPath);
                        if (app["-v"] == true) std::cout << "Cover image " << FH::getFilenameOf(imgPath) << " added to " << FH::getFilenameOf(file) << std::endl << std::endl;
                    }
                }

                // stop writing data after the first file if --all has not been used
                if (app["--all"] != true) {
                    break;
                }
            }
        }
    }

    return 0;
}