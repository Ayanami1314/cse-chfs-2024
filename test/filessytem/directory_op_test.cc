#include <cstdint>
#include <random>

#include "./common.h"
#include "filesystem/directory_op.h"
#include "gtest/gtest.h"
#include <sys/types.h>

namespace chfs {

TEST(FileSystemBase, Utilities) {
    std::vector<u8> content;
    std::list<DirectoryEntry> list;

    std::string input(content.begin(), content.end());

    parse_directory(input, list);
    ASSERT_TRUE(list.empty());

    input = append_to_directory(input, "test", 2);
    parse_directory(input, list);

    ASSERT_TRUE(list.size() == 1);

    for (uint i = 0; i < 100; i++) {
        input = append_to_directory(input, "test", i + 2);
    }
    list.clear();
    parse_directory(input, list);
    ASSERT_EQ(list.size(), 1 + 100);
}

TEST(FileSystemBase, UtilitiesRemove) {
    std::vector<u8> content;
    std::list<DirectoryEntry> list;

    std::string input(content.begin(), content.end());

    for (uint i = 0; i < 100; i++) {
        input = append_to_directory(input, "test" + std::to_string(i), i + 2);
    }

    input = rm_from_directory(input, "test0");
    // std::cout << input << std::endl;

    list.clear();
    parse_directory(input, list);
    ASSERT_EQ(list.size(), 99);

    input = rm_from_directory(input, "test12");
    list.clear();
    parse_directory(input, list);
    ASSERT_EQ(list.size(), 98);
}

TEST(FileSystemTest, DirectOperationAdd) {
    auto bm =
        std::shared_ptr<BlockManager>(new BlockManager(kBlockNum, kBlockSize));
    auto fs = FileOperation(bm, kTestInodeNum);

    auto res = fs.alloc_inode(InodeType::Directory);
    if (res.is_err()) {
        std::cerr << "Cannot allocate inode for root directory. " << std::endl;
        exit(1);
    }
    CHFS_ASSERT(res.unwrap() == 1, "The allocated inode number is incorrect ");

    std::list<DirectoryEntry> list;
    {
        auto res = read_directory(&fs, 1, list);
        ASSERT_TRUE(res.is_ok());
        ASSERT_TRUE(list.empty());
    }
}

TEST(FileSystemTest, mkdir) {
    auto bm =
        std::shared_ptr<BlockManager>(new BlockManager(kBlockNum, kBlockSize));
    auto fs = FileOperation(bm, kTestInodeNum);

    auto res = fs.alloc_inode(InodeType::Directory);
    if (res.is_err()) {
        std::cerr << "Cannot allocate inode for root directory. " << std::endl;
        exit(1);
    }

    for (uint i = 0; i < 100; i++) {
        auto res = fs.mkdir(1, ("test" + std::to_string(i)).c_str());
        ASSERT_TRUE(res.is_ok());
    }

    std::list<DirectoryEntry> list;
    {
        auto res = read_directory(&fs, 1, list);
        ASSERT_TRUE(res.is_ok());
    }
    ASSERT_EQ(list.size(), 100);
}
TEST(FileSystemTest, mkdir2) {
    auto bm =
        std::shared_ptr<BlockManager>(new BlockManager(kBlockNum, kBlockSize));
    auto fs = FileOperation(bm, kTestInodeNum);

    auto res = fs.alloc_inode(InodeType::Directory);
    if (res.is_err()) {
        std::cerr << "Cannot allocate inode for root directory. " << std::endl;
        exit(1);
    }

    for (uint i = 0; i < 100; i++) {
        std::string testname = std::string(i, 's');
        auto res = fs.mkdir(1, ("test-" + testname).c_str());
        ASSERT_TRUE(res.is_ok());
    }
    for (uint i = 0; i < 100; i++) {
        std::string testname = std::string(i, 's');
        auto res = fs.mkdir(1, ("test-" + testname).c_str());
        ASSERT_TRUE(res.is_err());
    }
    std::list<DirectoryEntry> list;
    {
        auto res = read_directory(&fs, 1, list);
        ASSERT_TRUE(res.is_ok());
    }
    ASSERT_EQ(list.size(), 100);
}

TEST(FileSystemTest, createAlot) {
    /**
     *  sub createone {
            # my $name = "file -\n-\t-";
            my $name = "file-";
            # for(my $i = 0; $i < 40; $i++){
            for(my $i = 0; $i < 20; $i++){
            $name .= sprintf("%c", ord('a') + int(rand(26)));
            }
            $name .= "-$$-" . $seq;
            $seq = $seq + 1;
            my $contents = rand();
            print "create $name\n";
            if(!open(F, ">$dir/$name")){
                print STDERR "test-lab1-part2-a: cannot create $dir/$name :
     $!\n"; exit(1);
            }
            close(F);
            $files->{$name} = $contents;
        }
     */
    auto bm =
        std::shared_ptr<BlockManager>(new BlockManager(kBlockNum, kBlockSize));
    auto fs = FileOperation(bm, kTestInodeNum);
    auto res = fs.alloc_inode(InodeType::Directory);
    if (res.is_err()) {
        std::cerr << "Cannot allocate inode for root directory. " << std::endl;
        exit(1);
    }
    for (uint i = 0; i < 200; i++) {
        std::string testname = std::string(40, 0);
        for (uint j = 0; j < 40; j++) {
            testname[j] = 'a' + rand() % 26;
        }
        testname = "file-" + testname + "-" + std::to_string(i);
        auto file_res = fs.mkfile(res.unwrap(), testname.c_str());
        if (file_res.is_err()) {
            std::cerr << "Error: " << static_cast<int>(res.unwrap_error())
                      << std::endl;
        }
        ASSERT_TRUE(file_res.is_ok());
    }
}

TEST(FileSystemTest, Unlink) {
    auto bm =
        std::shared_ptr<BlockManager>(new BlockManager(kBlockNum, kBlockSize));
    auto fs = FileOperation(bm, kTestInodeNum);
    auto dir_res = fs.alloc_inode(InodeType::Directory);
    if (dir_res.is_err()) {
        std::cerr << "Cannot allocate inode for root directory. " << std::endl;
        exit(1);
    }
    std::vector<std::string> test_files;
    for (uint i = 0; i < 20; i++) {
        std::string testname = std::string(5, 'a' + i);
        testname = "file-" + testname + "-" + std::to_string(i);
        test_files.push_back(testname);

        auto file_res = fs.mkfile(dir_res.unwrap(), testname.c_str());
        if (file_res.is_err()) {
            std::cerr << "Error: " << static_cast<int>(file_res.unwrap_error())
                      << std::endl;
        }
        ASSERT_TRUE(file_res.is_ok());
    }
    for (const auto &s : test_files) {
        auto rm_res = fs.unlink(dir_res.unwrap(), s.c_str());
        if (rm_res.is_err()) {
            std::cerr << "Error: " << static_cast<int>(rm_res.unwrap_error())
                      << std::endl;
        }
        ASSERT_TRUE(rm_res.is_ok());
    }
}
} // namespace chfs