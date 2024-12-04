#include "file_handler.h"

#include <fileref.h>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

std::string FH::cleanPath(const std::string& path) {
    if (path.empty()) return "";

    // remove double quotes at the start and end if present
    if (path.front() == '"' && path.back() == '"') {
        // ignore 1st character, get length-2 chars
        return path.substr(1, path.size() - 2);
    }
    return path;
}

bool FH::pathIsDir(const std::string& path) {
    return fs::is_directory(fs::status(path));
}

std::vector<std::string> FH::getAllFilesInDir(const std::string& dirPath) {
    std::vector<std::string> result;

    for (const auto& path : fs::recursive_directory_iterator(dirPath)) {
        if (fs::is_regular_file(path.status())) {
            // get all regular files in the dirPath folder (ignore paths of subdirs)
            result.push_back(path.path().string());
        }
    }

    return result;
}

std::vector<std::string> FH::gatherAllFilesFromList(const std::vector<std::string>& paths, bool forTaglib) {
    std::vector<std::string> result;

    for (auto& path : paths) {
        std::string cleanPath = FH::cleanPath(path);

        // skip empty paths
        if (cleanPath.empty()) continue;

        try {
            if (FH::pathIsDir(cleanPath)) {
                // path is a directory, search for all files inside
                auto dirPaths = FH::getAllFilesInDir(cleanPath);
                // copies over dirPaths into resulting list
                result.insert(result.end(), dirPaths.begin(), dirPaths.end());
            } else {
                result.push_back(cleanPath);
            }
        } catch (const std::exception &e) {
            std::cerr << "Exception while gathering files: " << e.what() << std::endl;
        }
    }

    // filter the file list and remove any files not supported by TagLib
    // it might be slower than just checking the extension, but it should be more robust and ensures that
    // the file will be usable later on in the code
    if (forTaglib)
    std::erase_if(
        result,
        [](const std::string& path) {
            TagLib::FileRef f (path.data());
            if (f.isNull()) {
                std::cerr << "WARN: Unsupported file provided as input: " << getFilenameOf(path) << std::endl;
                return true;
            } else {
                return false; // tells it to NOT delete this file
            }
        }
    );

    return result;
}

std::string FH::getFilenameOf(const std::string &path) {
    return fs::path(path).filename().string();
}

TagLib::ByteVector FH::getImgByteVector(const std::string &imgPath) {
    TagLib::ByteVector result;

    // return empty vector if no path was provided
    if (fs::path(imgPath).empty()) {
        return result;
    }

    std::ifstream imgData (imgPath, std::ios_base::in | std::ios_base::binary);

    // read the image data per byte
    char byte;
    imgData.read(&byte,1);
    while (imgData) {
        result.append(byte);
        imgData.read(&byte,1);
    }

    imgData.close();
    return result;
}

std::string FH::getExtOf(const std::string &path) {
    return fs::path(path).extension().string();
}

std::string FH::getDirOf(const std::string &path) {
    return fs::path(path).parent_path().string();
}

bool FH::exportFile(TagLib::ByteVector data, const std::string &filename) {
    fs::path outPath (filename);

    try {
        std::ofstream of (outPath, std::ios_base::out | std::ios_base::binary);
        of << data;
        of.close();
        return true;
    } catch (const std::exception &e) {
        std::cerr << "Exception while exporting data to " << outPath << ":" << e.what() << std::endl;
        return false;
    }
}
