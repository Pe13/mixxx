#include <QDebug>
#include <QtGlobal>

#import <AVFAudio/AVFAudio.h>

namespace mixxx {

void initializeAVAudioSession() {
    AVAudioSession* session = [AVAudioSession sharedInstance];
    AVAudioSessionCategory category = AVAudioSessionCategoryPlayback;
    AVAudioSessionMode mode = AVAudioSessionModeDefault;

    NSError* error = nil;

    // Set category first so the session knows we are doing playback
    [session setCategory:category mode:mode options:0 error:&error];
    if (error != nil) {
        qWarning() << "Could not initialize AVAudioSession:"
                   << error.localizedDescription;
        return;
    }

    // --- DYNAMIC CHANNEL QUERY ---
    // Ask the hardware how many channels it physically supports
    NSInteger maxChannels = [session maximumOutputNumberOfChannels];
    qInfo() << "Hardware supports up to" << maxChannels << "output channels.";

    // Request the maximum available channels
    [session setPreferredOutputNumberOfChannels:maxChannels error:&error];
    if (error != nil) {
        qWarning() << "Could not set preferred output channels:"
                   << error.localizedDescription;
        return;
    }

    [session setActive:true error:&error];
    if (error != nil) {
        qWarning() << "Could not activate AVAudioSession:"
                   << error.localizedDescription;
        return;
    }
}

int AVASOutChannelCount() {
    auto session = [AVAudioSession sharedInstance];
    return static_cast<int>([session maximumOutputNumberOfChannels]);
}

}; // namespace mixxx
