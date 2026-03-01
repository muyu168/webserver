#ifndef MIMETYPE_H
#define MIMETYPE_H

#include <string>
#include <unordered_map>

class MimeType{

public:
    static void Init();
    static std::string GetMimeType(const std::string& file_path);

private:
    static std::unordered_map<std::string, std::string> mime_types_;
};

#endif