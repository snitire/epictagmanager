#pragma once
#include <fileref.h>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace TM {
    // property types for code readability
    enum propTypes : int {
        UNDEFINED = -1,
        ALBUM = 1,
        ARTIST,
        BPM,
        COMMENT,
        COMPOSER,
        YEAR,
        DISCNUMBER,
        GENRE,
        TITLE,
        TRACKNUMBER,
        LANGUAGE,
        LYRICIST,
        LYRICS,
        REMIXER
    };

    // maps propTypes to string keys to be used in audio PropertyMap maps
    // https://taglib.org/api/p_propertymapping.html
    extern std::unordered_map<int, std::string> propKeys;

    // get all properties of given file based on set of propTypes type IDs
    std::map<int, TagLib::StringList> readProps(const TagLib::FileRef &f, const std::unordered_set<int> &props);

    void printProps(const std::map<int, TagLib::StringList> &propList);

    // REPLACE properties of file based on given propList
    void writeProps(TagLib::FileRef &f, const std::map<int, std::string> &propList);

    // search propKeys and return the propType value of the provided key
    int findPropTypeByKey(const std::string& key);

    // find all non-empty properties in the given file
    std::unordered_set<int> findAllDefinedProps(const TagLib::FileRef &f);

    // adds a single PICTURE complex property (containing the file at imgPath) to the complex properties of f
    void addImgTag(TagLib::FileRef &f, const std::string& imgPath);

    // extracts PICTURE property data into a separate file
    bool extractImgTags(TagLib::FileRef &f);
}
