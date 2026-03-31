import 'dart:async';
import 'dart:developer' as dev;

import 'package:flutter/foundation.dart';
import 'package:flutter_soloud/flutter_soloud.dart';
import 'package:logging/logging.dart';

import 'common.dart';

/// Test instancing playing handles and their disposal.
Future<StringBuffer> testStopFutures() async {
  final output = StringBuffer();
  final severeLogs = <LogRecord>[];

  Logger.root.level = kDebugMode ? Level.FINE : Level.INFO;
  Logger.root.onRecord.listen((record) {
    dev.log(
      record.message,
      time: record.time,
      level: record.level.value,
      name: record.loggerName,
      zone: record.zone,
      error: record.error,
      stackTrace: record.stackTrace,
    );

    if (record.level > Level.INFO) {
      output.writeln(record.message);
      if (record.level >= Level.SEVERE) {
        severeLogs.add(record);
      }
    }
  });

  /// Start audio isolate
  await initialize();

  /// Load sample
  final currentSound =
      await SoLoud.instance.loadAsset('assets/audio/explosion.mp3');

  /// Fast call to `stop` after `play`
  var handle = SoLoud.instance.play(currentSound);
  output
    ..writeln('fast play/stop')
    ..writeln('$handle started');
  unawaited(
    SoLoud.instance.stop(handle).then((_) => output.writeln('$handle stopped')),
  );

  await delay(500);

  /// Schedule a stop and call `stop` after the scheduled time
  handle = SoLoud.instance.play(currentSound);
  output
    ..writeln('\nscheduleStop')
    ..writeln('$handle started');
  SoLoud.instance.scheduleStop(handle, const Duration(milliseconds: 500));
  await delay(1000);
  unawaited(
    SoLoud.instance.stop(handle).then((_) => output.writeln('$handle stopped')),
  );

  /// Wait a bit.
  await delay(1000);

  deinit();

  if (severeLogs.isNotEmpty) {
    throw Exception('Severe logs produced:\n'
        '${severeLogs.map((r) => '[${r.level}] ${r.message}').join('\n')}');
  }

  return output;
}
