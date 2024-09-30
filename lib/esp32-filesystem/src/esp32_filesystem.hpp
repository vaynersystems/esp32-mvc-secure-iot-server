#ifndef _ESP32_FILESYSTEM_H_
#define _ESP32_FILESYSTEM_H_
#include "esp32_filesystem_objects.h"
#include "esp32_file_drive.hpp"
#include "string_helper.h"
#include "FS.h"
#include "vfs_api.h"
#include <vector>
#include <SPIFFS.h>
#include "SD.h"


using namespace std;
using namespace fs;


// template <class T>
// struct PSallocator {
//   typedef T value_type;
//   PSallocator() = default;
//   template <class U> constexpr PSallocator(const PSallocator<U>&) noexcept {}
//   [[nodiscard]] T* allocate(std::size_t n) {
//     if(n > std::size_t(-1) / sizeof(T)) throw std::bad_alloc();
//     if(auto p = static_cast<T*>(heap_caps_malloc(n*sizeof(T), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT))) return p;
//     throw std::bad_alloc();
//   }
//   void deallocate(T* p, std::size_t) noexcept { std::free(p); }
// };
// template <class T, class U>
// bool operator==(const PSallocator<T>&, const PSallocator<U>&) { return true; }
// template <class T, class U>
// bool operator!=(const PSallocator<T>&, const PSallocator<U>&) { return false; }


class esp32_fs_impl : public VFSImpl
{
public:
    inline esp32_fs_impl(){};
    virtual ~esp32_fs_impl() { }
    inline bool exists(const char* path)
    {
        File f = open(path, FILE_READ,false);
        bool valid = (f == true);// && !f.isDirectory();
        f.close();
        return valid;
    }
};





class esp32_file_system
{
public:
    void addDisk(FS &disk, const char* label, esp32_drive_type type = dt_SPIFFS);
    int driveCount();
    esp32_file_drive* getDisk(int index);
    esp32_file_drive* getDisk(const char* driveName);
private:
    vector<esp32_file_drive> _disks;
    esp32_file_drive _selectedDisk;
};
#endif