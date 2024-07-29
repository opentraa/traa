#include "base/folder/folder.h"
#include "base/platform.h"

#if defined(TRAA_OS_MAC)
#import <Foundation/Foundation.h>
#endif

#if defined(TARGET_OS_IPHONE)
#import <AVFoundation/AVFoundation.h>
#endif

namespace traa {
namespace base {

std::string folder::get_current_folder() { return std::string(); }

std::string folder::get_config_folder() {
#if defined(TRAA_OS_MAC)
  NSString *home = NSHomeDirectory();
  NSString *dir = [NSString stringWithFormat:@"%@/Library/Logs/", home];
  NSFileManager *fileManager = [NSFileManager defaultManager];
  if (![fileManager fileExistsAtPath:dir]) {
    [fileManager createDirectoryAtPath:dir
           withIntermediateDirectories:YES
                            attributes:nil
                                 error:nil];
  }
  return [dir UTF8String];
#else
  NSString *dir =
      NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES).firstObject;
  return [dir UTF8String];
#endif
}

std::string folder::get_temp_folder() { return std::string(); }

bool folder::create_folder(const std::string &folder) {
  if (folder.empty()) {
    return false;
  }

  NSFileManager *fileManager = [NSFileManager defaultManager];
  if (![fileManager fileExistsAtPath:[NSString stringWithUTF8String:folder.c_str()]]) {
    return [fileManager createDirectoryAtPath:[NSString stringWithUTF8String:folder.c_str()]
                  withIntermediateDirectories:YES
                                   attributes:nil
                                        error:nil];
  }

  return true;
}

} // namespace base
} // namespace traa