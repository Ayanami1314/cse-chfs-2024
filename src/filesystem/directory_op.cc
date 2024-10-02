#include <algorithm>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "filesystem/directory_op.h"
#include "common/config.h"
#include "common/error_code.h"
#include "common/result.h"

namespace chfs {

/**
 * Some helper functions
 */
auto string_to_inode_id(std::string &data) -> inode_id_t {
    std::stringstream ss(data);
    inode_id_t inode;
    ss >> inode;
    return inode;
}

auto inode_id_to_string(inode_id_t id) -> std::string {
    std::stringstream ss;
    ss << id;
    return ss.str();
}

// {Your code here}
auto dir_list_to_string(const std::list<DirectoryEntry> &entries)
    -> std::string {
    std::ostringstream oss;
    usize cnt = 0;
    for (const auto &entry : entries) {
        oss << entry.name << ':' << entry.id;
        if (cnt < entries.size() - 1) {
            oss << '/';
        }
        cnt += 1;
    }
    return oss.str();
}

// {Your code here}
auto append_to_directory(std::string src, std::string filename,
                         inode_id_t id) -> std::string {

    // TODO: Implement this function.
    //       Append the new directory entry to `src`.
    // UNIMPLEMENTED();
    std::string append = filename + ':' + inode_id_to_string(id) + '/';
    src += append;
    std::cout << src << std::endl;
    return src;
}

// {Your code here}
void parse_directory(std::string &src, std::list<DirectoryEntry> &list) {

    // TODO: Implement this function.
    // UNIMPLEMENTED();
    // ATTENTION src is 'const'
    auto idx = std::string::npos;
    std::string_view sv = src;
    while ((idx = sv.find_first_of('/')) != std::string::npos) {
        auto item = sv.substr(0, idx);
        auto item_str = std::string(item);
        auto delim = item.find_first_of(':');
        auto inode_id_str = item_str.substr(delim + 1);
        list.push_back(
            {item_str.substr(0, delim), string_to_inode_id(inode_id_str)});
        sv = sv.substr(idx + 1);
    }
}

// {Your code here}
auto rm_from_directory(std::string src, std::string filename) -> std::string {
    // TODO: Implement this function.
    //       Remove the directory entry from `src`.
    // UNIMPLEMENTED();
    auto start_idx = src.find(filename);
    if (start_idx != std::string::npos) {
        auto end_idx = src.find_first_of('/', start_idx);
        src = src.substr(0, start_idx) + src.substr(end_idx + 1);
    }
    return src;
}

/**
 * { Your implementation here }
 */
auto read_directory(FileOperation *fs, inode_id_t id,
                    std::list<DirectoryEntry> &list) -> ChfsNullResult {

    // TODO: Implement this function.
    // UNIMPLEMENTED();
    auto res = fs->read_file(id);
    if (res.is_err()) {
        return ChfsNullResult(res.unwrap_error());
    }
    std::vector<u8> content = res.unwrap();
    std::string src = std::string(content.begin(), content.end());
    parse_directory(src, list);
    return KNullOk;
}

// {Your code here}
auto FileOperation::lookup(inode_id_t id,
                           const char *name) -> ChfsResult<inode_id_t> {
    std::list<DirectoryEntry> list;

    // TODO: Implement this function.
    // UNIMPLEMENTED();
    auto content = this->read_file(id);
    if (content.is_err()) {
        return ChfsResult<inode_id_t>(content.unwrap_error());
    }
    auto content_str =
        std::string(content.unwrap().begin(), content.unwrap().end());
    std::list<DirectoryEntry> entries;
    parse_directory(content_str, entries);
    for (const auto &[entry_name, id] : entries) {
        if (entry_name == std::string(name)) {
            return ChfsResult<inode_id_t>(id);
        }
    }
    return ChfsResult<inode_id_t>(ErrorType::NotExist);
}

// {Your code here}
auto FileOperation::mk_helper(inode_id_t id, const char *name,
                              InodeType type) -> ChfsResult<inode_id_t> {

    // TODO:
    // 1. Check if `name` already exists in the parent.
    //    If already exist, return ErrorType::AlreadyExist.
    // 2. Create the new inode.
    // 3. Append the new entry to the parent directory.
    // UNIMPLEMENTED();
    std::list<DirectoryEntry> list;
    auto content = this->read_file(id);
    if (content.is_err()) {
        return ChfsResult<inode_id_t>(content.unwrap_error());
    }
    std::vector<u8> content_vec = content.unwrap();
    auto content_str = std::string(content_vec.begin(), content_vec.end());
    std::list<DirectoryEntry> entries;
    parse_directory(content_str, entries);
    for (const auto &[entry_name, id] : entries) {
        if (entry_name == std::string(name)) {
            return ChfsResult<inode_id_t>(ErrorType::AlreadyExist);
        }
    }

    auto new_inode = this->alloc_inode(type);
    if (new_inode.is_err()) {
        return ChfsResult<inode_id_t>(new_inode.unwrap_error());
    }
    auto new_inode_id = new_inode.unwrap();
    content_str = append_to_directory(content_str, name, new_inode_id);
    auto write_res = this->write_file(
        id, std::vector<u8>(content_str.begin(), content_str.end()));
    if (write_res.is_err()) {
        return ChfsResult<inode_id_t>(write_res.unwrap_error());
    }
    return ChfsResult<inode_id_t>(static_cast<inode_id_t>(new_inode_id));
}

// {Your code here}
auto FileOperation::unlink(inode_id_t parent,
                           const char *name) -> ChfsNullResult {

    // TODO:
    // 1. Remove the file, you can use the function `remove_file`
    // 2. Remove the entry from the directory.
    // UNIMPLEMENTED();
    auto target_inode_id = this->lookup(parent, name);
    if (target_inode_id.is_err()) {
        return ChfsNullResult(target_inode_id.unwrap_error());
    }
    auto dir = this->read_file(parent);
    if (dir.is_err()) {
        return ChfsNullResult(dir.unwrap_error());
    }
    auto dir_content = dir.unwrap();
    auto content_str = std::string(dir_content.begin(), dir_content.end());
    content_str = rm_from_directory(content_str, name);
    auto res = this->write_file(
        parent, std::vector<u8>(content_str.begin(), content_str.end()));
    return res;
}

} // namespace chfs
