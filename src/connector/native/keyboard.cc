#include "point.h"
#include "keyboard.h"
#include "eventsource.h"

#include <CoreGraphics/CoreGraphics.h>
#include <iostream>
#include <forward_list>
#include <thread>
#include <string>
#include <map>
#include <string>

using std::forward_list;

bool threadSetup = false;
std::thread nativeThread;
CFRunLoopRef runLoop;
int nextId = 0;

std::map<int,std::string> macKeycodeMap = {
  // Layout independent - will break on non-qwerty :(
  {0x00, "a"},
  {0x01, "s"},
  {0x02, "d"},
  {0x03, "f"},
  {0x04, "h"},
  {0x05, "g"},
  {0x06, "z"},
  {0x07, "x"},
  {0x08, "c"},
  {0x09, "v"},
  {0x0B, "b"},
  {0x0C, "q"},
  {0x0D, "w"},
  {0x0E, "e"},
  {0x0F, "r"},
  {0x10, "y"},
  {0x11, "t"},
  {0x12, "1"},
  {0x13, "2"},
  {0x14, "3"},
  {0x15, "4"},
  {0x16, "6"},
  {0x17, "5"},
  {0x18, "="},
  {0x19, "9"},
  {0x1A, "7"},
  {0x1B, "-"},
  {0x1C, "8"},
  {0x1D, "0"},
  {0x1E, "]"},
  {0x1F, "O"},
  {0x20, "U"},
  {0x21, "["},
  {0x22, "i"},
  {0x23, "p"},
  {0x25, "l"},
  {0x26, "j"},
  {0x27, "\'"},
  {0x28, "k"},
  {0x29, ";"},
  {0x2A, "\\"},
  {0x2B, ","},
  {0x2C, "/"},
  {0x2D, "n"},
  {0x2E, "m"},
  {0x2F, "."},
  {0x32, "`"},
  {0x41, "."},
  {0x43, "*"},
  {0x45, "+"},
  {0x47, "Clear"},
  {0x4B, "/"},
  {0x4C, "Enter"},
  {0x4E, "-"},
  {0x51, "="},
  {0x52, "0"},
  {0x53, "1"},
  {0x54, "2"},
  {0x55, "3"},
  {0x56, "4"},
  {0x57, "5"},
  {0x58, "6"},
  {0x59, "7"},
  {0x5B, "8"},
  {0x5C, "9"},

  // Keyboard layout independent (won't break)  
  {0x24, "Enter"},
  {0x30, "Tab"},
  {0x31, "Space"},
  {0x33, "Backspace"},
  {0x35, "Escape"},
  {0x37, "Meta"},
  {0x38, "Shift"},
  {0x39, "CapsLock"},
  {0x3A, "Alt"},
  {0x3B, "Control"},
  {0x3C, "Shift"},
  {0x3D, "Alt"},
  {0x3E, "Control"},
  {0x3F, "Fn"},
  {0x40, "F17"},
  {0x48, "VolumeUp"},
  {0x49, "VolumeDown"},
  {0x4A, "Mute"},
  {0x4F, "F18"},
  {0x50, "F19"},
  {0x5A, "F20"},
  {0x60, "F5"},
  {0x61, "F6"},
  {0x62, "F7"},
  {0x63, "F3"},
  {0x64, "F8"},
  {0x65, "F9"},
  {0x67, "F11"},
  {0x69, "F13"},
  {0x6A, "F16"},
  {0x6B, "F14"},
  {0x6D, "F10"},
  {0x6F, "F12"},
  {0x71, "F15"},
  {0x72, "Help"},
  {0x73, "Home"},
  {0x74, "PageUp"},
  {0x75, "Delete"},
  {0x76, "F4"},
  {0x77, "End"},
  {0x78, "F2"},
  {0x79, "PageDown"},
  {0x7A, "F1"},
  {0x7B, "ArrowLeft"},
  {0x7C, "ArrowRight"},
  {0x7D, "ArrowDown"},
  {0x7E, "ArrowUp"}
};
std::map<std::string, int> macKeycodeMapReverse;

struct CallbackInfo {
    Napi::ThreadSafeFunction cb;
    Napi::Function bareCb;
    CGEventMask mask;
    int id;
    CFMachPortRef tap;
    CFRunLoopSourceRef runloopsrc;

    bool operator ==(const CallbackInfo& other) const {
        return other.cb == cb;
    }

    ~CallbackInfo() {
        cb.Release();

        if (CGEventTapIsEnabled(tap)) CGEventTapEnable(tap, false);
        CFMachPortInvalidate(tap);
        CFRunLoopRemoveSource(CFRunLoopGetMain(), runloopsrc, kCFRunLoopCommonModes);
        CFRelease(runloopsrc);
        CFRelease(tap);
    }
};

struct JSEvent {
    UInt16 nativeKeyCode;
    std::string lowerKey;
    bool meta, shift, ctrl, alt;
    int button, x, y;
};

forward_list<CallbackInfo*> callbacks; 

CGEventRef eventtap_callback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon) {
    if (CGEventGetIntegerValueField(event, kCGEventSourceUserData) == 42) {
        // Skip our own events
        return event;
    }

    CallbackInfo* e = (CallbackInfo*) refcon;    
    
    // hammerspoon says OS X disables eventtaps if it thinks they are slow or odd or just because the moon
    // is wrong in some way... but at least it's nice enough to tell us.
    if ((type == kCGEventTapDisabledByTimeout) || (type == kCGEventTapDisabledByUserInput)) {
        CGEventTapEnable(e->tap, true);
        return event;
    }

    JSEvent *jsEvent = new JSEvent();

    CGEventFlags flags = CGEventGetFlags(event);
    if ((flags & kCGEventFlagMaskAlphaShift) != 0) {
        jsEvent->shift = true;
    } else if ((flags & kCGEventFlagMaskShift) != 0) {
        jsEvent->shift = true;
    } else if ((flags & kCGEventFlagMaskControl) != 0) {
        jsEvent->ctrl = true;
    } else if ((flags & kCGEventFlagMaskAlternate) != 0) {
        jsEvent->alt = true;
    } else if ((flags & kCGEventFlagMaskCommand) != 0) {
        jsEvent->meta = true;
    }

    // TODO Check other thread access is 100% ok
    if (type == kCGEventKeyDown || 
        type == kCGEventKeyUp ||
        type == kCGEventFlagsChanged) {
        // Keyboard event

        auto callback = []( Napi::Env env, Napi::Function jsCallback, JSEvent* value ) {
        Napi::Object obj = Napi::Object::New(env);

            obj.Set(Napi::String::New(env, "nativeKeyCode"), Napi::Number::New(env, value->nativeKeyCode));
            obj.Set(Napi::String::New(env, "lowerKey"), Napi::String::New(env, value->lowerKey));
            obj.Set(Napi::String::New(env, "meta"), Napi::Boolean::New(env, value->meta));
            obj.Set(Napi::String::New(env, "shift"), Napi::Boolean::New(env, value->shift));
            obj.Set(Napi::String::New(env, "ctrl"), Napi::Boolean::New(env, value->ctrl));
            obj.Set(Napi::String::New(env, "alt"), Napi::Boolean::New(env, value->alt));

            jsCallback.Call( {obj} );

            delete value;
        };        

        jsEvent->nativeKeyCode = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
        jsEvent->lowerKey = macKeycodeMap[jsEvent->nativeKeyCode];
        e->cb.BlockingCall( jsEvent, callback );  
    } else {
        // Mouse event
        CGPoint point = CGEventGetLocation(event);
        jsEvent->x = (int) point.x;
        jsEvent->y = (int) point.y;
        if (type == kCGEventMouseMoved || type == kCGEventOtherMouseDragged) {
            // Mouse movement doesn't have a button (multiple buttons could theoretically be down)
            jsEvent->button = -1;
        } else if (type == kCGEventLeftMouseUp || type == kCGEventLeftMouseDown || type == kCGEventLeftMouseDragged) {
            jsEvent->button = 0;
        } else if (type == kCGEventRightMouseUp || type == kCGEventRightMouseDown || type == kCGEventRightMouseDragged) {
            jsEvent->button = 2;
        } else {
            jsEvent->button = 1;
        }

        auto callback = []( Napi::Env env, Napi::Function jsCallback, JSEvent* value ) {
            Napi::Object obj = Napi::Object::New(env);

            obj.Set(Napi::String::New(env, "meta"), Napi::Boolean::New(env, value->meta));
            obj.Set(Napi::String::New(env, "shift"), Napi::Boolean::New(env, value->shift));
            obj.Set(Napi::String::New(env, "ctrl"), Napi::Boolean::New(env, value->ctrl));
            obj.Set(Napi::String::New(env, "alt"), Napi::Boolean::New(env, value->alt));

            obj.Set(Napi::String::New(env, "x"), Napi::Number::New(env, value->x));
            obj.Set(Napi::String::New(env, "y"), Napi::Number::New(env, value->y));
            obj.Set(Napi::String::New(env, "button"), Napi::Number::New(env, value->button));

            jsCallback.Call( {obj} );

            delete value;
        };
        e->cb.BlockingCall( jsEvent, callback );  
    }
    // can return NULL to ignore event
    return event;
}

Napi::Value addEventListener(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    auto eventType = info[0].As<Napi::String>().Utf8Value();
    auto cb = info[1].As<Napi::Function>();

    CGEventMask mask = kCGEventMaskForAllEvents;

    if ("keyup" == eventType) {
        mask = CGEventMaskBit(kCGEventKeyUp);
    } else if ("keydown" == eventType) {
        mask = CGEventMaskBit(kCGEventKeyDown);
    } else if ("mousemoved" == eventType) {
        mask = CGEventMaskBit(kCGEventMouseMoved) | CGEventMaskBit(kCGEventOtherMouseDragged);
    } else if ("mousedown" == eventType) {
        mask = CGEventMaskBit(kCGEventLeftMouseDown) | CGEventMaskBit(kCGEventRightMouseDown) | CGEventMaskBit(kCGEventOtherMouseDown);
    } else if ("mouseup" == eventType) {
        mask = CGEventMaskBit(kCGEventLeftMouseUp) | CGEventMaskBit(kCGEventRightMouseUp) | CGEventMaskBit(kCGEventOtherMouseUp);
    }

    // TODO FREE
    CallbackInfo *ourInfo = new CallbackInfo;
    ourInfo->bareCb = cb;
    ourInfo->id = nextId++;
    ourInfo->cb = Napi::ThreadSafeFunction::New(
      env,
      cb,                      // JavaScript function called asynchronously
      "Resource Name",         // Name
      0,                       // Unlimited queue
      1);                      // Initial thread count 

    ourInfo->tap = CGEventTapCreate(
        kCGSessionEventTap,
        kCGHeadInsertEventTap,
        kCGEventTapOptionDefault,
        mask,
        eventtap_callback,
        ourInfo);

    if (!ourInfo->tap) {
        std::cout << "Could not create event tap.";
    } else {
        CGEventTapEnable(ourInfo->tap, true);
        ourInfo->runloopsrc = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, ourInfo->tap, 0);
        if (!threadSetup) {
            threadSetup = true;
            nativeThread = std::thread( [=] {
                runLoop = CFRunLoopGetCurrent();
                CFRunLoopAddSource(runLoop, ourInfo->runloopsrc, kCFRunLoopCommonModes);
                CFRunLoopRun();
            } );
        } else {    
            CFRunLoopAddSource(runLoop, ourInfo->runloopsrc, kCFRunLoopCommonModes);
        }
    }
    callbacks.push_front(ourInfo);
    return Napi::Number::New(env, ourInfo->id);
}

Napi::Value removeEventListener(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    int id = info[0].As<Napi::Number>();
    callbacks.remove_if([=](CallbackInfo *e){ 
        bool willRemove = e->id == id;     
        if (willRemove) {
            // free it
            delete e;
        }
        return willRemove;
    });
    return Napi::Boolean::New(env, true);
}

Napi::Value isEnabled(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    int id = info[0].As<Napi::Number>();

    auto it = std::find_if (callbacks.begin(), callbacks.end(), [=](CallbackInfo *e){ 
        return e->id == id;
    });
    if (it != callbacks.end()) {
        CallbackInfo* info = *it;
        return Napi::Boolean::New(env, CGEventTapIsEnabled(info->tap));
    }
    return Napi::Boolean::New(env, false);
}

Napi::Value keyPresser(const Napi::CallbackInfo &info, bool down) {
    Napi::Env env = info.Env();

    if (macKeycodeMapReverse.size() == 0) {
        // Initalise our reverse map
        auto it = macKeycodeMap.begin();
        while(it != macKeycodeMap.end()) {
            macKeycodeMapReverse[it->second] = it->first;
            it++;
        }
    }

    CGKeyCode keyCode = (CGKeyCode)macKeycodeMapReverse[info[0].As<Napi::String>()];    
    CGEventFlags flags = (CGEventFlags)0;
    if (info[1].IsObject()) {
        Napi::Object obj = info[1].As<Napi::Object>();
        if (obj.Has("meta")) {
            flags |= kCGEventFlagMaskCommand;
        } else if (obj.Has("shift")) {
            flags |= kCGEventFlagMaskShift;
        } else if (obj.Has("alt")) {
            flags |= kCGEventFlagMaskAlternate;
        } else if (obj.Has("ctrl")) {
            flags |= kCGEventFlagMaskControl;
        }
    }
    CGEventRef keyevent = CGEventCreateKeyboardEvent(getCGEventSource(), keyCode, down);
    CGEventSetFlags(keyevent, flags);
    CGEventPost(kCGSessionEventTap, keyevent);
    CFRelease(keyevent);
    return Napi::Boolean::New(env, true);
}

Napi::Value keyDown(const Napi::CallbackInfo &info) {
    return keyPresser(info, true);
}

Napi::Value keyUp(const Napi::CallbackInfo &info) {
    return keyPresser(info, false);
}

Napi::Value keyPress(const Napi::CallbackInfo &info) {
    keyDown(info);
    usleep(10000);
    return keyUp(info);
}

Napi::Value InitKeyboard(Napi::Env env, Napi::Object exports)
{
    Napi::Object obj = Napi::Object::New(env);
    obj.Set(Napi::String::New(env, "addEventListener"), Napi::Function::New(env, addEventListener));
    obj.Set(Napi::String::New(env, "removeEventListener"), Napi::Function::New(env, removeEventListener));
    obj.Set(Napi::String::New(env, "isEnabled"), Napi::Function::New(env, isEnabled));
    obj.Set(Napi::String::New(env, "keyDown"), Napi::Function::New(env, keyDown));
    obj.Set(Napi::String::New(env, "keyUp"), Napi::Function::New(env, keyUp));
    obj.Set(Napi::String::New(env, "keyPress"), Napi::Function::New(env, keyPress));
    exports.Set("Keyboard", obj);
    return exports;
}