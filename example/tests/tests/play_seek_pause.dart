import 'package:flutter_soloud/flutter_soloud.dart';

import 'common.dart';

/// Test play, pause, seek, position.
Future<StringBuffer> testPlaySeekPause() async {
  /// Start audio isolate
  await initialize();

  /// Load sample
  final currentSound =
      await SoLoud.instance.loadAsset('assets/audio/explosion.mp3');

  /// pause, seek test
  {
    SoLoud.instance.play(currentSound);
    final length = SoLoud.instance.getLength(currentSound);
    assert(
      closeTo(length.inMilliseconds, 3773, 100),
      'getLength() failed: ${length.inMilliseconds}!\n',
    );
    await delay(1000);
    SoLoud.instance.pauseSwitch(currentSound.handles.first);
    final paused = SoLoud.instance.getPause(currentSound.handles.first);
    assert(paused, 'pauseSwitch() failed!');

    /// seek
    const wantedPosition = Duration(seconds: 2);
    SoLoud.instance.seek(currentSound.handles.first, wantedPosition);
    final position = SoLoud.instance.getPosition(currentSound.handles.first);
    assert(position == wantedPosition, 'getPosition() failed!');
  }

  deinit();
  return StringBuffer();
}
