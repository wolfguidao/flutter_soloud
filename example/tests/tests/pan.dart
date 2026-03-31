import 'package:flutter_soloud/flutter_soloud.dart';

import 'common.dart';

/// Test instancing playing handles and their disposal.
Future<StringBuffer> testPan() async {
  /// Start audio isolate
  await initialize();

  final song =
      await SoLoud.instance.loadAsset('assets/audio/8_bit_mentality.mp3');

  final handle = SoLoud.instance.play(song, volume: 0.5);

  SoLoud.instance.setPan(handle, -0.8);
  var pan = SoLoud.instance.getPan(handle);
  assert(closeTo(pan, -0.8, 0.00001), 'setPan() or getPan() failed!');

  await delay(1000);

  SoLoud.instance.setPan(handle, 0.8);
  pan = SoLoud.instance.getPan(handle);
  assert(closeTo(pan, 0.8, 0.00001), 'setPan() or getPan() failed!');
  await delay(1000);

  deinit();
  return StringBuffer();
}
