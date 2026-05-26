#import "AppDelegate.h"
#import "MacHookManager.h"
#import "MacInputInjector.h"
#import "CayEngine.h"
#import <ServiceManagement/ServiceManagement.h>

namespace CayIME {
    Cay::TelexEngine g_engine;
    extern bool g_enabled;
}

@implementation AppDelegate {
    NSStatusItem *statusItem;
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    // Single Instance Check
    NSArray *apps = [NSRunningApplication runningApplicationsWithBundleIdentifier:[[NSBundle mainBundle] bundleIdentifier]];
    if ([apps count] > 1) {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"CayIME đang chạy!"];
        [alert runModal];
        [NSApp terminate:nil];
        return;
    }

    // Gắn Callback Inject Text
    CayIME::g_engine.OnInjectText = CayIME::MacInputInjector::ReplaceText;

    // Chạy Hook
    if (!CayIME::MacHookManager::Initialize()) {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"Không thể khởi tạo Hook. Vui lòng cấp quyền Accessibility trong System Preferences."];
        [alert runModal];
        [NSApp terminate:nil];
        return;
    }

    [self setupMenu];
    
    // Lần chạy đầu tiên
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    if (![defaults boolForKey:@"CayHasLaunchedBefore"]) {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"Cay - Lần chạy đầu tiên"];
        [alert setInformativeText:@"Bạn có muốn Cay tự động chạy khi khởi động máy không?"];
        [alert addButtonWithTitle:@"Có"];
        [alert addButtonWithTitle:@"Không"];
        
        [NSApp activateIgnoringOtherApps:YES];
        
        if ([alert runModal] == NSAlertFirstButtonReturn) {
            if (statusItem.menu) {
                for (NSMenuItem *item in statusItem.menu.itemArray) {
                    if (item.action == @selector(toggleAutoStart:)) {
                        if (item.state == NSControlStateValueOff) {
                            [self toggleAutoStart:item];
                        }
                        break;
                    }
                }
            }
        }
        
        [defaults setBool:YES forKey:@"CayHasLaunchedBefore"];
    }

    // Lắng nghe Notification từ HookManager để cập nhật giao diện khi toggle bằng phím tắt
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(handleToggleNotification:) name:@"ToggleIMENotification" object:nil];
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
    CayIME::MacHookManager::Shutdown();
}

- (void)setupMenu {
    statusItem = [[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength];
    [self updateIcon];

    NSMenu *menu = [[NSMenu alloc] init];
    menu.delegate = self;

    NSMenuItem *toggleItem = [[NSMenuItem alloc] initWithTitle:@"Bật / Tắt (Cmd + Shift)" action:@selector(toggleFromMenu) keyEquivalent:@""];
    [menu addItem:toggleItem];
    
    [menu addItem:[NSMenuItem separatorItem]];

    NSMenuItem *autoStartItem = [[NSMenuItem alloc] initWithTitle:@"Khởi động cùng macOS" action:@selector(toggleAutoStart:) keyEquivalent:@""];
    NSString *bundleID = [[NSBundle mainBundle] bundleIdentifier];
    NSString *plistPath = [NSString stringWithFormat:@"%@/Library/LaunchAgents/%@.plist", NSHomeDirectory(), bundleID];
    if ([[NSFileManager defaultManager] fileExistsAtPath:plistPath]) {
        [autoStartItem setState:NSControlStateValueOn];
    } else {
        [autoStartItem setState:NSControlStateValueOff];
    }
    [menu addItem:autoStartItem];

    NSMenuItem *aboutItem = [[NSMenuItem alloc] initWithTitle:@"Giới thiệu" action:@selector(showAbout) keyEquivalent:@""];
    [menu addItem:aboutItem];

    NSMenuItem *quitItem = [[NSMenuItem alloc] initWithTitle:@"Thoát" action:@selector(quitApp) keyEquivalent:@"q"];
    [menu addItem:quitItem];

    statusItem.menu = menu;
}

- (void)updateIcon {
    NSString *text = CayIME::g_enabled ? @"Cay: V" : @"Cay: E";
    if (statusItem.button) {
        statusItem.button.title = text;
    } else {
        statusItem.title = text;
    }
}

- (void)toggleFromMenu {
    [self toggleIME];
}

- (void)handleToggleNotification:(NSNotification*)notif {
    [self toggleIME];
}

- (void)toggleIME {
    CayIME::g_enabled = !CayIME::g_enabled;
    CayIME::MacHookManager::ResetEngine();
    [self updateIcon];
}

- (void)toggleAutoStart:(NSMenuItem *)item {
    NSString *bundleID = [[NSBundle mainBundle] bundleIdentifier];
    NSString *execPath = [[NSBundle mainBundle] executablePath];
    NSString *launchAgentsDir = [NSString stringWithFormat:@"%@/Library/LaunchAgents", NSHomeDirectory()];
    NSString *plistPath = [NSString stringWithFormat:@"%@/%@.plist", launchAgentsDir, bundleID];
    
    NSFileManager *fm = [NSFileManager defaultManager];
    
    if ([fm fileExistsAtPath:plistPath]) {
        [fm removeItemAtPath:plistPath error:nil];
        [item setState:NSControlStateValueOff];
    } else {
        if (![fm fileExistsAtPath:launchAgentsDir]) {
            [fm createDirectoryAtPath:launchAgentsDir withIntermediateDirectories:YES attributes:nil error:nil];
        }
        
        NSDictionary *plistDict = @{
            @"Label": bundleID,
            @"ProgramArguments": @[execPath],
            @"RunAtLoad": @YES
        };
        
        [plistDict writeToFile:plistPath atomically:YES];
        [item setState:NSControlStateValueOn];
    }
}

- (void)showAbout {
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:@"Cay – Bộ gõ tiếng Việt Telex v1.0.1"];
    [alert setInformativeText:@"aa→â  aw→ă  dd→đ  ee→ê  oo→ô  ow→ơ  uw→ư\n"
                              @"s=sắc  f=huyền  r=hỏi  x=ngã  j=nặng\n\n"
                              @"License: GPL-3.0\nSource: github.com/tctvn/cay"];
    [alert runModal];
}

- (void)quitApp {
    [NSApp terminate:nil];
}

@end

AppDelegate *g_delegate = nil;

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyAccessory];
        g_delegate = [[AppDelegate alloc] init];
        app.delegate = g_delegate;
        [app run];
    }
    return 0;
}
