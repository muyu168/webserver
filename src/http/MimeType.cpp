#include "http/MimeType.h"

std::unordered_map<std::string, std::string> MimeType::mime_types_;

void MimeType::Init(){
    static bool initialized = false;
    if(!initialized){
        mime_types_["html"] = "text/html";
        mime_types_["css"] = "text/css";
        mime_types_["js"] = "application/javascript";
        mime_types_["png"] = "image/png";
        mime_types_["jpg"] = "image/jpeg";
        mime_types_["gif"] = "image/gif";
        mime_types_["svg"] = "image/svg+xml";
        mime_types_["ico"] = "image/x-icon";
        mime_types_["txt"] = "text/plain";
        mime_types_["xml"] = "text/xml";
        mime_types_["json"] = "application/json";
        initialized = true;
    }
}


std::string MimeType::GetMimeType(const std::string& file_path){
    Init();
    std::string ext = file_path.substr(file_path.find_last_of('.') + 1);
    if(mime_types_.find(ext) != mime_types_.end()){
        return mime_types_[ext];
    }else{
        return "application/octet-stream";
    }
}

