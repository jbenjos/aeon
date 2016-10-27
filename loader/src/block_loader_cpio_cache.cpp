/*
 Copyright 2016 Nervana Systems Inc.
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/

#include <errno.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ftw.h>

#include "cpio.hpp"
#include "block_loader_cpio_cache.hpp"

using namespace std;
using namespace nervana;

// maximum number of files opened by nftw file enumeration function
// For some platforms (older linux), OPEN_MAX needs to be defined
#ifndef OPEN_MAX
#define OPEN_MAX 128
#endif

block_loader_cpio_cache::block_loader_cpio_cache(const string& rootCacheDir,
                                                 const string& cache_id,
                                                 const string& version,
                                                 shared_ptr<block_loader> loader)
: block_loader(loader->blockSize()), _loader(loader)
{
    invalidateOldCache(rootCacheDir, cache_id, version);

    _cacheDir = rootCacheDir + "/" + cache_id + "_" + version;

    makeDirectory(_cacheDir);
}

void block_loader_cpio_cache::loadBlock(buffer_in_array& dest, uint32_t block_num)
{
    if(loadBlockFromCache(dest, block_num)) {
        return;
    } else {
        _loader->loadBlock(dest, block_num);

        try {
            writeBlockToCache(dest, block_num);
        } catch (std::exception& e) {
            // failure to write block to cache doesn't stop execution, only print an error
            cerr << "ERROR writing block to cache: " << e.what() << endl;
        }
    }
}

bool block_loader_cpio_cache::loadBlockFromCache(buffer_in_array& dest, uint32_t block_num)
{
    // load a block from cpio cache into dest.  If file doesn't exist, return false.
    //  If loading from cpio cache was successful return true.
    cpio::file_reader reader;

    if(!reader.open(blockFilename(block_num))) {
        // couldn't load the file
        return false;
    }
    // load cpio file into dest one item at a time
    for(int i=0; i < reader.itemCount(); ++i) {
        for (auto d : dest) {
            try {
                reader.read(*d);
            } catch (std::exception& e) {
                d->add_exception(std::current_exception());
            }
        }
    }

    reader.close();

    // cpio file was read successfully, no need to hit primary data
    // source
    return true;
}

void block_loader_cpio_cache::writeBlockToCache(buffer_in_array& buff, uint32_t block_num)
{
    cpio::file_writer writer;
    writer.open(blockFilename(block_num));
    writer.write_all_records(buff);
    writer.close();
}

void block_loader_cpio_cache::invalidateOldCache(const string& rootCacheDir,
                                                 const string& cache_id,
                                                 const string& version)
{
    // remove cache directories that match rootCacheDir and cache_id but not version

    DIR *dir;
    struct dirent *ent;
    if((dir = opendir(rootCacheDir.c_str())) != NULL) {
        while((ent = readdir(dir)) != NULL) {
            if(filenameHoldsInvalidCache(ent->d_name, cache_id, version)) {
                removeDirectory(rootCacheDir + "/" + ent->d_name);
            }
        }
        closedir(dir);
    }
    else {
        throw std::runtime_error("error enumerating old cache in " + rootCacheDir);
    }
}

bool block_loader_cpio_cache::filenameHoldsInvalidCache(const string& filename,
                                                        const string& cache_id,
                                                        const string& version)
{
    // in order for `filename` to hold invalid cache, it must begin with
    // `cache_id`, but not contain `version`

    if(filename.find(cache_id) != 0) {
        // filename doesn't start with cache_id, dont remove it
        return false;
    }
    if(filename.find(version) == string::npos) {
        // filename does start with cache_id, but doesnt have version, invalidate
        return true;
    }
    // filename does start with cache_id and does have version, keep, its valid
    return false;
}

int block_loader_cpio_cache::rm(const char *path, const struct stat *s, int flag, struct FTW *f)
{
    // see http://stackoverflow.com/a/1149837/2093984
    // Call unlink or rmdir on the path, as appropriate.
    int status;

    switch(flag) {
        default:     status = unlink(path); break;
        case FTW_DP: status = rmdir (path);
    }

    if(status != 0) {
        stringstream message;
        message << "error deleting file " << path;
        throw std::runtime_error(message.str());
    }

    return status;
}

void block_loader_cpio_cache::removeDirectory(const string& dir)
{
    // see http://stackoverflow.com/a/1149837/2093984
    // FTW_DEPTH: handle directories after its contents
    // FTW_PHYS: do not follow symbolic links
    if(nftw(dir.c_str(), rm, OPEN_MAX, FTW_DEPTH | FTW_PHYS)) {
        throw std::runtime_error("error deleting directory " + dir);
    }
}

void block_loader_cpio_cache::makeDirectory(const string& dir)
{
    if(mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
        if(errno == EEXIST) {
            // not really an error, the directory already exists
            return;
        }
        throw std::runtime_error("error making directory " + dir + " " + strerror(errno));
    }
}

string block_loader_cpio_cache::blockFilename(uint32_t block_num)
{
     return _cacheDir + "/" + to_string(block_num) + "-" + to_string(_block_size) + ".cpio";
}

uint32_t block_loader_cpio_cache::objectCount()
{
    return _loader->objectCount();
}
