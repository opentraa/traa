#include "base/devices/screen/test/simple_window/simple_window.h"

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#include <objc/NSObject.h>

static void dispatch_ui_thread_sync(dispatch_block_t block) {
  if ([NSThread isMainThread]) {
    block();
  } else {
    dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
    dispatch_async(dispatch_get_main_queue(), ^{
      block();
      dispatch_semaphore_signal(semaphore);
    });
    const dispatch_time_t timeout = dispatch_time(DISPATCH_TIME_NOW, 15 * NSEC_PER_SEC);
    dispatch_semaphore_wait(semaphore, timeout);
  }
}

@interface SimpleWindowDelegate : NSObject <NSWindowDelegate> {
}
@end

@implementation SimpleWindowDelegate
- (void)windowWillClose:(NSNotification *)notification {
}
@end

@interface SimpleAppDelegate : NSObject <NSApplicationDelegate> {
  NSWindow *window;
  SimpleWindowDelegate *window_delegate_;
}
@end

@implementation SimpleAppDelegate

- (NSWindow *)getWindow {
  return window;
}

- (instancetype)initWith:(NSString *)title width:(uint32_t)width height:(uint32_t)height {
  window_delegate_ = [[SimpleWindowDelegate alloc] init];
  self = [super init];
  if (self) {
    NSRect mainDisplayRect = [[NSScreen mainScreen] frame];

    NSRect windowRect = NSMakeRect(mainDisplayRect.size.width / 4, mainDisplayRect.size.height / 4,
                                   mainDisplayRect.size.width / 2, mainDisplayRect.size.height / 2);

    NSUInteger windowStyle =
        NSWindowStyleMaskTitled | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskClosable;

    NSInteger windowLevel = NSNormalWindowLevel + 1;

    dispatch_ui_thread_sync(^{
      window = [[NSWindow alloc] initWithContentRect:windowRect
                                           styleMask:windowStyle
                                             backing:NSBackingStoreBuffered
                                               defer:NO];
      [window setLevel:windowLevel];
      [window setTitle:title];
      [window setOpaque:YES];
      [window setHasShadow:YES];
      [window setHidesOnDeactivate:NO];
      [window setDelegate:window_delegate_];
    });
  }
  return self;
}

- (void)showWindow {
  dispatch_ui_thread_sync(^{
    [window makeKeyAndOrderFront:self];
  });
}

- (void)applicationWillFinishLaunching:(NSNotification *)notification {
  dispatch_ui_thread_sync(^{
    [window makeKeyAndOrderFront:self];
  });
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
}

- (void)applicationWillTerminate:(NSNotification *)notification {
}

- (void)dealloc {
  [window close];
  window = nil;
}

@end

namespace traa {
namespace base {

class simple_window_mac : public simple_window {
public:
  simple_window_mac(const std::string &title, uint32_t width, uint32_t height) {
    title_ = title;
    @autoreleasepool {
      app_delegate_ =
          [[SimpleAppDelegate alloc] initWith:[NSString stringWithUTF8String:title_.c_str()]
                                        width:width
                                       height:height];
      [app_delegate_ showWindow];
    }
  }

  ~simple_window_mac() {}

  intptr_t get_source_id() const override {
    __block intptr_t result = 0;
    dispatch_ui_thread_sync(^{
      result = static_cast<intptr_t>([app_delegate_ getWindow].windowNumber);
    });
    return result;
  }

  intptr_t get_view() const override {
    __block intptr_t result = 0;
    dispatch_ui_thread_sync(^{
      NSView *contentView = [[app_delegate_ getWindow] contentView];
      result = static_cast<intptr_t>([contentView hash]);
    });
    return result;
  }

  void minimized() override {}

  void unminimized() override {}

  void resize(uint32_t width, uint32_t height) override {
    dispatch_ui_thread_sync(^{
      [[app_delegate_ getWindow] setContentSize:NSMakeSize(width, height)];
    });
  }

  void move(uint32_t x, uint32_t y) override {
    dispatch_ui_thread_sync(^{
      [[app_delegate_ getWindow] setFrameOrigin:NSMakePoint(x, y)];
    });
  }

private:
  SimpleAppDelegate *app_delegate_;
};

std::shared_ptr<simple_window> simple_window::create(const std::string &title, uint32_t width,
                                                     uint32_t height, uint64_t style) {
  return std::make_shared<simple_window_mac>(title, width, height);
}

} // namespace base
} // namespace traa
