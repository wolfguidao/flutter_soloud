import 'all_instances_finished.dart' as all_instances_finished;
import 'async_multi_load.dart' as async_multi_load;
import 'asynchronous_deinit.dart' as asynchronous_deinit;
import 'auto_dispose.dart' as auto_dispose;
import 'buffer_stream_small_mp3.dart' as buffer_stream_small_mp3;
import 'create_notes.dart' as create_notes;
import 'global_filters.dart' as global_filters;
import 'handles.dart' as handles;
import 'looping.dart' as looping;
import 'pan.dart' as pan;
import 'play_seek_pause.dart' as play_seek_pause;
import 'protect_voice.dart' as protect_voice;
import 'sound_filters.dart' as sound_filters;
import 'stop_futures.dart' as stop_futures;
import 'synchronous_deinit.dart' as synchronous_deinit;
import 'voice_groups.dart' as voice_groups;

/// A single test entry.
class TestEntry {
  const TestEntry({
    required this.name,
    required this.run,
  });

  final String name;
  final Future<StringBuffer> Function() run;
}

/// The list of all available tests.
///
/// Add new tests here to make them available in the test runner UI.
final List<TestEntry> allTests = [
  const TestEntry(
    name: 'testProtectVoice',
    run: protect_voice.testProtectVoice,
  ),
  const TestEntry(
    name: 'testAllInstancesFinished',
    run: all_instances_finished.testAllInstancesFinished,
  ),
  const TestEntry(
    name: 'testCreateNotes',
    run: create_notes.testCreateNotes,
  ),
  const TestEntry(
    name: 'testPlaySeekPause',
    run: play_seek_pause.testPlaySeekPause,
  ),
  const TestEntry(
    name: 'testPan',
    run: pan.testPan,
  ),
  const TestEntry(
    name: 'testHandles',
    run: handles.testHandles,
  ),
  const TestEntry(
    name: 'testStopFutures',
    run: stop_futures.testStopFutures,
  ),
  const TestEntry(
    name: 'loopingTests',
    run: looping.loopingTests,
  ),
  const TestEntry(
    name: 'testSynchronousDeinit',
    run: synchronous_deinit.testSynchronousDeinit,
  ),
  const TestEntry(
    name: 'testAsynchronousDeinit',
    run: asynchronous_deinit.testAsynchronousDeinit,
  ),
  const TestEntry(
    name: 'testVoiceGroups',
    run: voice_groups.testVoiceGroups,
  ),
  const TestEntry(
    name: 'testSoundFilters',
    run: sound_filters.testSoundFilters,
  ),
  const TestEntry(
    name: 'testGlobalFilters',
    run: global_filters.testGlobalFilters,
  ),
  const TestEntry(
    name: 'testAsyncMultiLoad',
    run: async_multi_load.testAsyncMultiLoad,
  ),
  const TestEntry(
    name: 'testAutoDispose',
    run: auto_dispose.testAutoDispose,
  ),
  const TestEntry(
    name: 'testBufferStreamSmallMp3',
    run: buffer_stream_small_mp3.testBufferStreamSmallMp3,
  ),
];
