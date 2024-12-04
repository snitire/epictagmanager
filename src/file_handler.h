#pragma once
#include <string>
#include <tbytevector.h>
#include <vector>

namespace FH {
    // clean up cli input path to be usable further
    std::string cleanPath(const std::string& path);

    // check if input path is a directory
    bool pathIsDir(const std::string& path);

    // recursively find all files in the given directory path
    std::vector<std::string> getAllFilesInDir(const std::string& dirPath);

    // make a list of all individual files from list of paths (potentially containing both file and directory paths)
    std::vector<std::string> gatherAllFilesFromList(const std::vector<std::string> &, bool forTaglib);

    // get the filename of a given path (the part at the very end with the extension)
    std::string getFilenameOf(const std::string& path);

    // get the extension of a given path
    std::string getExtOf(const std::string& path);

    // get the parent folder of a given path
    std::string getDirOf(const std::string& path);

    // create a ByteVector from an image for embedding
    TagLib::ByteVector getImgByteVector(const std::string& path);

    // export the data as a file to the system
    bool exportFile(TagLib::ByteVector data, const std::string &filename);
}
