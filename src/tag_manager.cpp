#include "tag_manager.h"

#include <fileref.h>
#include <iostream>
#include <tpropertymap.h>
#include <unordered_map>
#include <unordered_set>

#include "file_handler.h"

std::unordered_map<int, std::string> TM::propKeys = {
    {ALBUM, "ALBUM"},
    {ARTIST, "ARTIST"},
    {BPM, "BPM"},
    {COMMENT, "COMMENT"},
    {COMPOSER, "COMPOSER"},
    {YEAR, "DATE"},
    {DISCNUMBER, "DISCNUMBER"},
    {GENRE, "GENRE"},
    {TITLE, "TITLE"},
    {TRACKNUMBER, "TRACKNUMBER"},
    {LANGUAGE, "LANGUAGE"},
    {LYRICIST, "LYRICIST"},
    {LYRICS, "LYRICS"},
    {REMIXER, "REMIXER"},
};

std::map<int, TagLib::StringList> TM::readProps(const TagLib::FileRef& f, const std::unordered_set<int>& props) {
    // outputs a map of propTypes and their respective values in the passed file f
    std::map<int, TagLib::StringList> result;
    TagLib::PropertyMap propMap = f.properties();

    for (auto& type : props) {
        // propKeys[type] corresponds to the relevant TagLib property key
        TagLib::StringList propVal = propMap[propKeys[type]];
        result.insert({type,propVal});
    }

    return result;
}

void TM::printProps(const std::map<int, TagLib::StringList> &propList) {
    // type is from propTypes, val is the actual tag value
    for (auto& [type, val] : propList) {
        std::cout << "    " << propKeys[type] << ": " << val << std::endl;
    }
}

void TM::writeProps(TagLib::FileRef& f, const std::map<int, std::string> &propList) {
    TagLib::PropertyMap propMap = f.properties();

    for (auto& [type, val] : propList) {
        // .replace explicitly requires a StringList even if only one val is used
        TagLib::StringList tag;
        tag.append(val);
        propMap.replace(propKeys[type],tag);
    }

    // writes the changes to the fileref
    f.setProperties(propMap);
    f.save();
}

int TM::findPropTypeByKey(const std::string& key) {
    for (auto& [type, propKey]: propKeys) {
        if (key == propKey) return type;
    }
    // if no suitable or defined key was found
    return UNDEFINED;
}

std::unordered_set<int> TM::findAllDefinedProps(const TagLib::FileRef& f) {
    std::unordered_set<int> result;
    TagLib::PropertyMap propMap = f.properties();

    // clean out any empty values so those don't get returned
    propMap.removeEmpty();

    // go through all file properties and pick out the ones that are defined in propTypes
    for (auto& [key, val]: propMap) {
        int type = findPropTypeByKey(key.to8Bit()); // convert TagLib::String to std::string before passing as key
        if (type != UNDEFINED) result.insert(type);
    }

    return result;
}

void TM::addImgTag(TagLib::FileRef &f, const std::string &imgPath) {
    if (imgPath.empty()) {
        return;
    }

    // I'm not treating it as an error because it technically still works with any file type
    // which might be cool if you want to hide something in the image data tag
    if (auto ext = FH::getExtOf(imgPath); ext != ".png" && ext != ".jpg" && ext != ".jpeg") {
        std::cerr << "WARN: Provided image " << imgPath << " has an unusual extension: " << ext << std::endl;
    }

    // static so they don't reset in every function call
    static auto lastImgPath = imgPath;
    static auto imgData = FH::getImgByteVector(imgPath);

    // optimization for multiple files in a row having the same img being set (e.g. in an album)
    if (imgPath != lastImgPath) {
        imgData = FH::getImgByteVector(imgPath);
    }

    TagLib::String mimeType;
    // png file magic number
    if (imgData.startsWith("\x89PNG\x0d\x0a\x1a\x0a")) {
        mimeType = "image/png";
    } else {
        mimeType = "image/jpeg";
    }

    // Make the property list for the provided image
    TagLib::VariantMap imgPropMap = {
        {"data", imgData},
        {"pictureType", "Front Cover"},
        {"mimeType", mimeType}
    };

    // extract the current picture data, append the new data
    auto complexProps = f.complexProperties("PICTURE");
    complexProps.append(imgPropMap);
    f.setComplexProperties("PICTURE",complexProps);

    f.save();
}

bool TM::extractImgTags(TagLib::FileRef &f) {
    TagLib::ByteVector imgData;
    TagLib::String mimeType = "image/jpeg";
    std::string filename = f.file()->name().toString().to8Bit(); // file -> FileName -> TagLib string -> std string
    int imgCount = 0;

    // find all embedded pictures
    auto imgList = f.complexProperties("PICTURE");
    // go through all pictures
    for (auto& prop : imgList) {
        // individual properties of each picture
        for (auto& [key, val] : prop) {
            if (key == "data" && val.type() == TagLib::Variant::ByteVector) {
                imgData = val.toByteVector();
            } else if (key == "mimeType" && val.type() == TagLib::Variant::String) {
                mimeType = val.toString();
            }
        }

        // attempt to write the image data
        try {
            // if one file has multiple images, enumerate them
            std::string imgName = filename;
            if (imgCount > 0) imgName += "_" + std::to_string(imgCount);

            if (mimeType == "image/png") {
                imgName += ".png";
            } else {
                imgName += ".jpg";
            }

            // if exporting failed, return a fail from this function too
            if (FH::exportFile(imgData, imgName) == false) {
                std::cerr << "Could not extract picture data of " << imgName;
                return false;
            }

            imgCount++;
        } catch (const std::exception &e) {
            std::cerr << "Exception while extracting image data from " << f.file()->name().toString() << ":" << e.what() << std::endl;
            return false;
        }
    }
    return true;
}
