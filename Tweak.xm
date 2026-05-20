#import <UIKit/UIKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>
#import <objc/runtime.h>

// MARK: - Global State
static CGFloat gResolutionScale = 1.0f;
static BOOL gMSAAEnabled = NO;
static NSInteger gMSAALevel = 4;
static BOOL gFXAAEnabled = NO;
static BOOL gVSyncEnabled = YES;

static dispatch_once_t menuOnceToken;
static UIButton *gFloatingButton = nil;
static UIView *gMenuPanel = nil;
static CGPoint gDragOffset;

// MARK: - FXAA Stub
static void applyFXAASetting(BOOL enabled) {
    // Placeholder: patch game engine FFlag or memory address
    // Example for Roblox: write to FFlagDisableFXAA memory offset
    // uintptr_t base = getBaseAddress("RobloxPlayer");
    // *(uint8_t *)(base + 0xDEADBEEF) = enabled ? 0x00 : 0x01;
    NSLog(@"[RenderTweak] FXAA %@", enabled ? @"ON" : @"OFF");
}

// MARK: - Menu Controller
@interface RenderMenuController : NSObject
+ (instancetype)shared;
- (void)setupFloatingButton;
- (void)showMenu;
- (void)hideMenu;
@end

@implementation RenderMenuController

+ (instancetype)shared {
    static RenderMenuController *inst;
    static dispatch_once_t t;
    dispatch_once(&t, ^{ inst = [RenderMenuController new]; });
    return inst;
}

- (void)setupFloatingButton {
    dispatch_async(dispatch_get_main_queue(), ^{
        UIWindow *win = [UIApplication sharedApplication].windows.firstObject;
        if (!win) return;

        gFloatingButton = [UIButton buttonWithType:UIButtonTypeSystem];
        gFloatingButton.frame = CGRectMake(20, 100, 54, 54);
        gFloatingButton.layer.cornerRadius = 27;
        gFloatingButton.clipsToBounds = YES;
        gFloatingButton.backgroundColor = [UIColor colorWithRed:0.1 green:0.1 blue:0.15 alpha:0.92];
        gFloatingButton.layer.borderColor = [UIColor colorWithRed:0.4 green:0.7 blue:1.0 alpha:1.0].CGColor;
        gFloatingButton.layer.borderWidth = 1.5;
        [gFloatingButton setTitle:@"⚙️" forState:UIControlStateNormal];
        gFloatingButton.titleLabel.font = [UIFont systemFontOfSize:26];

        [gFloatingButton addTarget:self action:@selector(floatingTapped) forControlEvents:UIControlEventTouchUpInside];

        UIPanGestureRecognizer *pan = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(handlePan:)];
        [gFloatingButton addGestureRecognizer:pan];

        gFloatingButton.layer.shadowColor = [UIColor colorWithRed:0.4 green:0.7 blue:1.0 alpha:1.0].CGColor;
        gFloatingButton.layer.shadowRadius = 8;
        gFloatingButton.layer.shadowOpacity = 0.8;
        gFloatingButton.layer.shadowOffset = CGSizeZero;

        [win addSubview:gFloatingButton];
    });
}

- (void)handlePan:(UIPanGestureRecognizer *)pan {
    UIWindow *win = [UIApplication sharedApplication].windows.firstObject;
    CGPoint loc = [pan locationInView:win];
    if (pan.state == UIGestureRecognizerStateBegan) {
        gDragOffset = CGPointMake(loc.x - gFloatingButton.center.x, loc.y - gFloatingButton.center.y);
    } else if (pan.state == UIGestureRecognizerStateChanged) {
        gFloatingButton.center = CGPointMake(loc.x - gDragOffset.x, loc.y - gDragOffset.y);
    }
}

- (void)floatingTapped {
    if (gMenuPanel && !gMenuPanel.hidden) {
        [self hideMenu];
    } else {
        [self showMenu];
    }
}

- (void)showMenu {
    dispatch_async(dispatch_get_main_queue(), ^{
        UIWindow *win = [UIApplication sharedApplication].windows.firstObject;
        if (!win) return;

        if (gMenuPanel) {
            gMenuPanel.hidden = NO;
            [win bringSubviewToFront:gMenuPanel];
            [win bringSubviewToFront:gFloatingButton];
            return;
        }

        CGFloat W = 310, H = 420;
        CGFloat X = (win.bounds.size.width - W) / 2;
        CGFloat Y = (win.bounds.size.height - H) / 2;

        gMenuPanel = [[UIView alloc] initWithFrame:CGRectMake(X, Y, W, H)];
        gMenuPanel.backgroundColor = [UIColor colorWithRed:0.07 green:0.07 blue:0.12 alpha:0.95];
        gMenuPanel.layer.cornerRadius = 18;
        gMenuPanel.layer.borderColor = [UIColor colorWithRed:0.3 green:0.6 blue:1.0 alpha:0.6].CGColor;
        gMenuPanel.layer.borderWidth = 1.0;
        gMenuPanel.layer.shadowColor = [UIColor blackColor].CGColor;
        gMenuPanel.layer.shadowRadius = 20;
        gMenuPanel.layer.shadowOpacity = 0.7;
        gMenuPanel.layer.shadowOffset = CGSizeZero;
        gMenuPanel.clipsToBounds = NO;

        // Title
        UILabel *title = [[UILabel alloc] initWithFrame:CGRectMake(0, 12, W, 30)];
        title.text = @"⚙ Render Tweaks";
        title.textAlignment = NSTextAlignmentCenter;
        title.textColor = [UIColor colorWithRed:0.5 green:0.8 blue:1.0 alpha:1.0];
        title.font = [UIFont boldSystemFontOfSize:16];
        [gMenuPanel addSubview:title];

        // Separator
        UIView *sep = [[UIView alloc] initWithFrame:CGRectMake(16, 48, W - 32, 1)];
        sep.backgroundColor = [UIColor colorWithWhite:1.0 alpha:0.1];
        [gMenuPanel addSubview:sep];

        CGFloat yOff = 58;

        // --- Resolution Scale ---
        yOff = [self addSectionLabel:@"Resolution Scale" y:yOff width:W];

        UILabel *scaleValLabel = [[UILabel alloc] initWithFrame:CGRectMake(W - 60, yOff - 22, 52, 20)];
        scaleValLabel.textColor = [UIColor colorWithRed:0.5 green:0.8 blue:1.0 alpha:1.0];
        scaleValLabel.font = [UIFont monospacedDigitSystemFontOfSize:13 weight:UIFontWeightMedium];
        scaleValLabel.textAlignment = NSTextAlignmentRight;
        scaleValLabel.text = [NSString stringWithFormat:@"%.2f", gResolutionScale];
        scaleValLabel.tag = 101;
        [gMenuPanel addSubview:scaleValLabel];

        UISlider *scaleSlider = [[UISlider alloc] initWithFrame:CGRectMake(16, yOff, W - 32, 28)];
        scaleSlider.minimumValue = 1.0;
        scaleSlider.maximumValue = 3.0;
        scaleSlider.value = gResolutionScale;
        scaleSlider.minimumTrackTintColor = [UIColor colorWithRed:0.3 green:0.6 blue:1.0 alpha:1.0];
        scaleSlider.tag = 100;
        [scaleSlider addTarget:self action:@selector(scaleChanged:) forControlEvents:UIControlEventValueChanged];
        [gMenuPanel addSubview:scaleSlider];
        yOff += 36;

        // --- MSAA ---
        yOff += 6;
        [gMenuPanel addSubview:[self makeSeparator:yOff width:W]]; yOff += 10;
        yOff = [self addSectionLabel:@"Hardware AA (MSAA)" y:yOff width:W];

        UISwitch *msaaSwitch = [[UISwitch alloc] init];
        msaaSwitch.frame = CGRectMake(W - 68, yOff - 24, 0, 0);
        msaaSwitch.on = gMSAAEnabled;
        msaaSwitch.onTintColor = [UIColor colorWithRed:0.3 green:0.6 blue:1.0 alpha:1.0];
        msaaSwitch.tag = 200;
        [msaaSwitch addTarget:self action:@selector(msaaToggled:) forControlEvents:UIControlEventValueChanged];
        [gMenuPanel addSubview:msaaSwitch];

        UILabel *msaaLvlLabel = [[UILabel alloc] initWithFrame:CGRectMake(16, yOff, 60, 24)];
        msaaLvlLabel.text = @"Level:";
        msaaLvlLabel.textColor = [UIColor colorWithWhite:0.6 alpha:1.0];
        msaaLvlLabel.font = [UIFont systemFontOfSize:13];
        msaaLvlLabel.tag = 201;
        [gMenuPanel addSubview:msaaLvlLabel];

        UISegmentedControl *msaaSeg = [[UISegmentedControl alloc] initWithItems:@[@"x2", @"x4", @"x8"]];
        msaaSeg.frame = CGRectMake(75, yOff, W - 91, 28);
        msaaSeg.selectedSegmentIndex = (gMSAALevel == 2) ? 0 : (gMSAALevel == 8) ? 2 : 1;
        msaaSeg.tag = 202;
        msaaSeg.enabled = gMSAAEnabled;
        msaaSeg.alpha = gMSAAEnabled ? 1.0 : 0.35;
        [msaaSeg setTitleTextAttributes:@{NSForegroundColorAttributeName: [UIColor whiteColor]} forState:UIControlStateNormal];
        [msaaSeg addTarget:self action:@selector(msaaLevelChanged:) forControlEvents:UIControlEventValueChanged];

        UIColor *segTint = [UIColor colorWithRed:0.3 green:0.6 blue:1.0 alpha:1.0];
        msaaSeg.selectedSegmentTintColor = segTint;
        [gMenuPanel addSubview:msaaSeg];
        yOff += 36;

        // --- FXAA ---
        yOff += 4;
        [gMenuPanel addSubview:[self makeSeparator:yOff width:W]]; yOff += 10;
        yOff = [self addSectionLabel:@"Software AA (FXAA)" y:yOff width:W];

        UISwitch *fxaaSwitch = [[UISwitch alloc] init];
        fxaaSwitch.frame = CGRectMake(W - 68, yOff - 24, 0, 0);
        fxaaSwitch.on = gFXAAEnabled;
        fxaaSwitch.onTintColor = [UIColor colorWithRed:0.3 green:0.6 blue:1.0 alpha:1.0];
        [fxaaSwitch addTarget:self action:@selector(fxaaToggled:) forControlEvents:UIControlEventValueChanged];
        [gMenuPanel addSubview:fxaaSwitch];

        // --- VSync ---
        yOff += 4;
        [gMenuPanel addSubview:[self makeSeparator:yOff width:W]]; yOff += 10;
        yOff = [self addSectionLabel:@"VSync / FPS Lock" y:yOff width:W];

        UISwitch *vsyncSwitch = [[UISwitch alloc] init];
        vsyncSwitch.frame = CGRectMake(W - 68, yOff - 24, 0, 0);
        vsyncSwitch.on = gVSyncEnabled;
        vsyncSwitch.onTintColor = [UIColor colorWithRed:0.3 green:0.6 blue:1.0 alpha:1.0];
        [vsyncSwitch addTarget:self action:@selector(vsyncToggled:) forControlEvents:UIControlEventValueChanged];
        [gMenuPanel addSubview:vsyncSwitch];

        UILabel *vsyncNote = [[UILabel alloc] initWithFrame:CGRectMake(16, yOff, W - 90, 18)];
        vsyncNote.text = @"OFF = Uncap to 120fps";
        vsyncNote.textColor = [UIColor colorWithWhite:0.45 alpha:1.0];
        vsyncNote.font = [UIFont systemFontOfSize:11];
        [gMenuPanel addSubview:vsyncNote];
        yOff += 26;

        // Close button
        UIButton *closeBtn = [UIButton buttonWithType:UIButtonTypeSystem];
        closeBtn.frame = CGRectMake((W - 120) / 2, H - 46, 120, 32);
        closeBtn.layer.cornerRadius = 10;
        closeBtn.clipsToBounds = YES;
        closeBtn.backgroundColor = [UIColor colorWithRed:0.2 green:0.35 blue:0.6 alpha:0.8];
        [closeBtn setTitle:@"✕  Close" forState:UIControlStateNormal];
        [closeBtn setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
        closeBtn.titleLabel.font = [UIFont boldSystemFontOfSize:14];
        [closeBtn addTarget:self action:@selector(hideMenu) forControlEvents:UIControlEventTouchUpInside];
        [gMenuPanel addSubview:closeBtn];

        [win addSubview:gMenuPanel];
        [win bringSubviewToFront:gFloatingButton];

        gMenuPanel.alpha = 0;
        gMenuPanel.transform = CGAffineTransformMakeScale(0.85, 0.85);
        [UIView animateWithDuration:0.22 delay:0 usingSpringWithDamping:0.75 initialSpringVelocity:0.5 options:0 animations:^{
            gMenuPanel.alpha = 1;
            gMenuPanel.transform = CGAffineTransformIdentity;
        } completion:nil];
    });
}

- (void)hideMenu {
    dispatch_async(dispatch_get_main_queue(), ^{
        [UIView animateWithDuration:0.18 animations:^{
            gMenuPanel.alpha = 0;
            gMenuPanel.transform = CGAffineTransformMakeScale(0.88, 0.88);
        } completion:^(BOOL done) {
            gMenuPanel.hidden = YES;
            gMenuPanel.alpha = 1;
            gMenuPanel.transform = CGAffineTransformIdentity;
        }];
    });
}

- (CGFloat)addSectionLabel:(NSString *)text y:(CGFloat)y width:(CGFloat)W {
    UILabel *lbl = [[UILabel alloc] initWithFrame:CGRectMake(16, y, W - 90, 22)];
    lbl.text = text;
    lbl.textColor = [UIColor colorWithWhite:0.85 alpha:1.0];
    lbl.font = [UIFont systemFontOfSize:14 weight:UIFontWeightSemibold];
    [gMenuPanel addSubview:lbl];
    return y + 28;
}

- (UIView *)makeSeparator:(CGFloat)y width:(CGFloat)W {
    UIView *v = [[UIView alloc] initWithFrame:CGRectMake(16, y, W - 32, 1)];
    v.backgroundColor = [UIColor colorWithWhite:1.0 alpha:0.07];
    return v;
}

// MARK: - Actions

- (void)scaleChanged:(UISlider *)slider {
    gResolutionScale = slider.value;
    UILabel *lbl = (UILabel *)[gMenuPanel viewWithTag:101];
    lbl.text = [NSString stringWithFormat:@"%.2f", gResolutionScale];
}

- (void)msaaToggled:(UISwitch *)sw {
    gMSAAEnabled = sw.on;
    UISegmentedControl *seg = (UISegmentedControl *)[gMenuPanel viewWithTag:202];
    seg.enabled = gMSAAEnabled;
    [UIView animateWithDuration:0.2 animations:^{ seg.alpha = gMSAAEnabled ? 1.0 : 0.35; }];
}

- (void)msaaLevelChanged:(UISegmentedControl *)seg {
    NSInteger levels[] = {2, 4, 8};
    gMSAALevel = levels[seg.selectedSegmentIndex];
}

- (void)fxaaToggled:(UISwitch *)sw {
    gFXAAEnabled = sw.on;
    applyFXAASetting(gFXAAEnabled);
}

- (void)vsyncToggled:(UISwitch *)sw {
    gVSyncEnabled = sw.on;
}

@end

// MARK: - Logos Hooks

// --- UIApplication lifecycle bootstrap ---
%hook UIApplication
- (void)applicationDidBecomeActive:(UIApplication *)application {
    %orig;
    dispatch_once(&menuOnceToken, ^{
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(2.0 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
            [[RenderMenuController shared] setupFloatingButton];
        });
    });
}
%end

// --- Resolution Scale: UIScreen ---
%hook UIScreen
- (CGFloat)scale {
    return gResolutionScale;
}
- (CGFloat)nativeScale {
    return gResolutionScale;
}
%end

// --- Resolution Scale: CAMetalLayer ---
%hook CAMetalLayer
- (void)setContentsScale:(CGFloat)scale {
    %orig(gResolutionScale);
}
%end

// --- MSAA: MTLRenderPipelineDescriptor ---
%hook MTLRenderPipelineDescriptor
- (void)setRasterSampleCount:(NSUInteger)count {
    if (!gMSAAEnabled) {
        %orig(1);
    } else {
        %orig((NSUInteger)gMSAALevel);
    }
}
%end

// --- MSAA: MTLTextureDescriptor ---
%hook MTLTextureDescriptor
- (void)setSampleCount:(NSUInteger)count {
    if (!gMSAAEnabled) {
        %orig(1);
    } else {
        %orig((NSUInteger)gMSAALevel);
    }
}
%end

// --- VSync / FPS Unlock: CADisplayLink ---
%hook CADisplayLink
- (void)setPreferredFramesPerSecond:(NSInteger)fps {
    if (!gVSyncEnabled) {
        %orig(0);
    } else {
        %orig(fps);
    }
}
%end