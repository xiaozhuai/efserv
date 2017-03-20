/**
 * @author : xiaozhuai
 * @date   : 17/3/19
 */

#include "Response.h"

string Response::dir_indexs_html;
string Response::err_html;


void Response::loadTpl() {
    FileHandler dirIndexsTpl(SERV_ENV.getConfig(KEY_DIR_INDEXS_TPL, DEFAULT_DIR_INDEXS_TPL));
    FileHandler errTpl(SERV_ENV.getConfig(KEY_ERR_TPL, DEFAULT_ERR_TPL));
    if(dirIndexsTpl.exist()){
        dir_indexs_html = dirIndexsTpl.readAsText();
    }else{
        dir_indexs_html = "{{file_list}}";
    }
    if(errTpl.exist()){
        err_html = errTpl.readAsText();
    }else{
        err_html = "{{status_code}} {{msg}}";
    }
}

void Response::respondHeader(int fd, string mimetype, size_t content_length) {
    header(fd, "HTTP/1.1 200 OK");
    header(fd, "Server", HEADER_SERVER);
    header(fd, "X-Powered-By", HEADER_SERVER);
    header(fd, "Content-Type", mimetype);
    header(fd, "Content-Length", to_string(content_length));
    header_end(fd);
}

void Response::respondContent(int fd, const char *content, size_t length) {
    send(fd, content, length, 0);
}

string assign(string format, string key, string value){
    key = "{{" + key + "}}";

    size_t start_pos = 0;
    while((start_pos = format.find(key, start_pos)) != std::string::npos) {
        format = format.replace(start_pos, key.length(), value);
        start_pos += value.length(); // Handles case where 'to' is a substring of 'from'
    }

    return format;
}

void Response::respondErr(int fd, int status_code) {
    string msg;
    switch (status_code){
        case 403:
            msg = "Forbidden";
            break;
        case 404:
            msg = "Not Found";
            break;
        default:
            status_code = 500;
        case 500:
            msg = "Internal Server Error";
            break;
    }

    header(fd, "HTTP/1.1 "+to_string(status_code)+" "+msg);


    string content;
    content = assign(err_html, "status_code", to_string(status_code));
    content = assign(content, "msg", msg);
    size_t len = content.length();

    header(fd, "Server", HEADER_SERVER);
    header(fd, "X-Powered-By", HEADER_SERVER);
    header(fd, "Content-Type", "text/html");
    header(fd, "Content-Length", to_string(len));
    header_end(fd);
    respondContent(fd, content.c_str(), len);
}

void Response::respondIndexs(int fd, vector<FileHandler> files, string url) {

    header(fd, "HTTP/1.1 200 OK");
    header(fd, "Server", HEADER_SERVER);
    header(fd, "X-Powered-By", HEADER_SERVER);
    header(fd, "Content-Type", "text/html");

    size_t fileCount = files.size();
    string fileListJson = "[\n";
    for(int i=0; i<fileCount; i++){
        FileHandler file = files[i];
        string obj = tfm::format(
                "    {\n"
                "        \"is_file\":%s,\n"
                "        \"is_link\":%s,\n"
                "        \"name\":\"%s\",\n"
                "        \"ext\":\"%s\",\n"
                "        \"mime_type\":\"%s\",\n"
                "        \"ctime\":%ld,\n"
                "        \"mtime\":%ld,\n"
                "        \"atime\":%ld\n"
                "    }",
                file.isFile() ? "true" : "false",
                file.isLink() ? "true" : "false",
                file.getName(),
                file.getExt(),
                file.getMimeType(),
                file.getCreateTime(),
                file.getModifyTime(),
                file.getAccessTime()
        );
        if(i==fileCount-1)  obj += "\n";
        else                obj += ",\n";
        fileListJson += obj;
    }
    fileListJson += "]";

    string content;
    content = assign(dir_indexs_html, "file_list", fileListJson);
    content = assign(content, "url", url);
    size_t len = content.length();

    header(fd, "Content-Length", to_string(len));
    header_end(fd);

    respondContent(fd, content.c_str(), len);
}