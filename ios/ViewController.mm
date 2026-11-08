#import "ViewController.h"
#import <WebKit/WebKit.h>
#import <sys/utsname.h>

static NSString* getIosDeviceModel() {
    struct utsname systemInfo;
    uname(&systemInfo);
    NSString* code = [NSString stringWithCString:systemInfo.machine encoding:NSUTF8StringEncoding];
    static NSDictionary* deviceNamesByCode = nil;
    if (!deviceNamesByCode) {
        deviceNamesByCode = @{
            @"iPhone12,1": @"iPhone 11",
            @"iPhone12,3": @"iPhone 11 Pro",
            @"iPhone12,5": @"iPhone 11 Pro Max",
            @"iPhone13,1": @"iPhone 12 mini",
            @"iPhone13,2": @"iPhone 12",
            @"iPhone13,3": @"iPhone 12 Pro",
            @"iPhone13,4": @"iPhone 12 Pro Max",
            @"iPhone14,4": @"iPhone 13 mini",
            @"iPhone14,5": @"iPhone 13",
            @"iPhone14,2": @"iPhone 13 Pro",
            @"iPhone14,3": @"iPhone 13 Pro Max",
            @"iPhone14,7": @"iPhone 14",
            @"iPhone14,8": @"iPhone 14 Plus",
            @"iPhone15,2": @"iPhone 14 Pro",
            @"iPhone15,3": @"iPhone 14 Pro Max",
            @"iPhone15,4": @"iPhone 15",
            @"iPhone15,5": @"iPhone 15 Plus",
            @"iPhone16,1": @"iPhone 15 Pro",
            @"iPhone16,2": @"iPhone 15 Pro Max",
            @"iPhone17,1": @"iPhone 16 Pro",
            @"iPhone17,2": @"iPhone 16 Pro Max",
            @"iPhone17,3": @"iPhone 16",
            @"iPhone17,4": @"iPhone 16 Plus"
        };
    }
    NSString* name = deviceNamesByCode[code];
    if (name) return name;
    if ([code hasPrefix:@"iPhone"]) return [NSString stringWithFormat:@"Apple %@", code];
    return [UIDevice currentDevice].model ?: @"Apple iPhone";
}

@interface ViewController () <WKNavigationDelegate, WKScriptMessageHandler>
@property (nonatomic, strong) WKWebView *webView;
@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = [UIColor colorWithRed:0.04 green:0.05 blue:0.07 alpha:1.0];

    WKWebViewConfiguration *config = [[WKWebViewConfiguration alloc] init];
    WKUserContentController *userContentController = [[WKUserContentController alloc] init];
    [userContentController addScriptMessageHandler:self name:@"iosBridge"];

    NSString *deviceModel = getIosDeviceModel();
    NSString *vendorId = [[[UIDevice currentDevice] identifierForVendor] UUIDString] ?: @"IOS-DEVICE-HWID";

    // Read all sensi and licensing permanent settings from NSUserDefaults
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    NSMutableDictionary *sensiStore = [NSMutableDictionary dictionary];
    NSDictionary *allDefaults = [defaults dictionaryRepresentation];
    for (NSString *key in allDefaults) {
        if ([key hasPrefix:@"sensi_"] || [key hasPrefix:@"sensee_"] || [key hasPrefix:@"gk_"]) {
            sensiStore[key] = allDefaults[key];
        }
    }
    NSData *storeData = [NSJSONSerialization dataWithJSONObject:sensiStore options:0 error:nil];
    NSString *storeJson = storeData ? [[NSString alloc] initWithData:storeData encoding:NSUTF8StringEncoding] : @"{}";

    NSString *jsInjection = [NSString stringWithFormat:
        @"window.iOSNativeBridge = {"
         "_store: %@,"
         "getDeviceModel: function() { return '%@'; },"
         "getHWID: function() { return 'IOS-%@'; },"
         "getUUID: function() { return '%@'; },"
         "savePermanentSetting: function(key, val) {"
         "  if (!this._store) this._store = {};"
         "  this._store[key] = val;"
         "  try { window.webkit.messageHandlers.iosBridge.postMessage({ action: 'savePref', key: key, value: val }); return true; }"
         "  catch(e) { return false; }"
         "},"
         "getPermanentSetting: function(key, defaultVal) {"
         "  if (this._store && this._store[key] !== undefined && this._store[key] !== null) return this._store[key];"
         "  return defaultVal || null;"
         "},"
         "removePermanentSetting: function(key) {"
         "  if (this._store) delete this._store[key];"
         "  try { window.webkit.messageHandlers.iosBridge.postMessage({ action: 'removePref', key: key }); return true; }"
         "  catch(e) { return false; }"
         "},"
         "clearAllPermanentSettings: function() {"
         "  this._store = {};"
         "  try { window.webkit.messageHandlers.iosBridge.postMessage({ action: 'clearAllPrefs' }); return true; }"
         "  catch(e) { return false; }"
         "},"
         "copyToClipboard: function(text) {"
         "  try { window.webkit.messageHandlers.iosBridge.postMessage({ action: 'copyToClipboard', text: text }); return true; }"
         "  catch(e) { return false; }"
         "}"
         "};",
        storeJson, deviceModel, vendorId, vendorId];

    WKUserScript *script = [[WKUserScript alloc] initWithSource:jsInjection injectionTime:WKUserScriptInjectionTimeAtDocumentStart forMainFrameOnly:YES];
    [userContentController addUserScript:script];

    config.userContentController = userContentController;

    self.webView = [[WKWebView alloc] initWithFrame:self.view.bounds configuration:config];
    self.webView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    self.webView.navigationDelegate = self;
    self.webView.opaque = NO;
    self.webView.backgroundColor = [UIColor clearColor];
    self.webView.scrollView.backgroundColor = [UIColor clearColor];
    [self.view addSubview:self.webView];

    // Load cross-platform bundle
    NSString *htmlPath = [[NSBundle mainBundle] pathForResource:@"index" ofType:@"html"];
    if (htmlPath) {
        NSURL *url = [NSURL fileURLWithPath:htmlPath];
        [self.webView loadFileURL:url allowingReadAccessToURL:[url URLByDeletingLastPathComponent]];
    } else {
        // Fallback embedded
        NSString *docs = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject];
        NSString *docHtml = [docs stringByAppendingPathComponent:@"index.html"];
        if ([[NSFileManager defaultManager] fileExistsAtPath:docHtml]) {
            [self.webView loadRequest:[NSURLRequest requestWithURL:[NSURL fileURLWithPath:docHtml]]];
        }
    }
}

- (UIStatusBarStyle)preferredStatusBarStyle {
    return UIStatusBarStyleLightContent;
}

#pragma mark - Script Message Handler
- (void)userContentController:(WKUserContentController *)userContentController didReceiveScriptMessage:(WKScriptMessage *)message {
    if ([message.name isEqualToString:@"iosBridge"]) {
        NSDictionary *dict = (NSDictionary *)message.body;
        NSString *action = dict[@"action"];

        if ([action isEqualToString:@"savePref"]) {
            NSString *key = dict[@"key"];
            NSString *val = dict[@"value"];
            if (key && val) {
                [[NSUserDefaults standardUserDefaults] setObject:val forKey:key];
                [[NSUserDefaults standardUserDefaults] synchronize];
            }
        } else if ([action isEqualToString:@"removePref"]) {
            NSString *key = dict[@"key"];
            if (key) {
                [[NSUserDefaults standardUserDefaults] removeObjectForKey:key];
                [[NSUserDefaults standardUserDefaults] synchronize];
            }
        } else if ([action isEqualToString:@"clearAllPrefs"]) {
            NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
            NSDictionary *all = [defaults dictionaryRepresentation];
            for (NSString *k in all) {
                if ([k hasPrefix:@"sensi_"] || [k hasPrefix:@"sensee_"]) {
                    [defaults removeObjectForKey:k];
                }
            }
            [defaults synchronize];
        } else if ([action isEqualToString:@"launchGame"]) {
            NSString *target = dict[@"target"];
            NSURL *targetUrl = nil;
            if ([target isEqualToString:@"max"]) {
                targetUrl = [NSURL URLWithString:@"freefiremax://"];
            } else {
                targetUrl = [NSURL URLWithString:@"freefire://"];
            }
            if ([[UIApplication sharedApplication] canOpenURL:targetUrl]) {
                [[UIApplication sharedApplication] openURL:targetUrl options:@{} completionHandler:nil];
            } else {
                NSURL *altUrl = [NSURL URLWithString:([target isEqualToString:@"max"] ? @"freefire://" : @"freefiremax://")];
                if ([[UIApplication sharedApplication] canOpenURL:altUrl]) {
                    [[UIApplication sharedApplication] openURL:altUrl options:@{} completionHandler:nil];
                } else {
                    NSString *appName = [target isEqualToString:@"max"] ? @"Free Fire MAX" : @"Free Fire";
                    NSString *toastJs = [NSString stringWithFormat:@"if (typeof showToast === 'function') showToast('%@ not installed on this iOS device.', '⚠️');", appName];
                    [self.webView evaluateJavaScript:toastJs completionHandler:nil];
                }
            }
        } else if ([action isEqualToString:@"vibrate"]) {
            UIImpactFeedbackGenerator *gen = [[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleMedium];
            [gen impactOccurred];
        } else if ([action isEqualToString:@"copyToClipboard"]) {
            NSString *text = dict[@"text"];
            if (text) {
                [UIPasteboard generalPasteboard].string = text;
            }
        }
    }
}

#pragma mark - Navigation Delegate
- (void)webView:(WKWebView *)webView decidePolicyForNavigationAction:(WKNavigationAction *)navigationAction decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler {
    NSURL *url = navigationAction.request.URL;
    if ([url.scheme isEqualToString:@"http"] || [url.scheme isEqualToString:@"https"]) {
        [[UIApplication sharedApplication] openURL:url options:@{} completionHandler:nil];
        decisionHandler(WKNavigationActionPolicyCancel);
        return;
    }
    decisionHandler(WKNavigationActionPolicyAllow);
}

@end
