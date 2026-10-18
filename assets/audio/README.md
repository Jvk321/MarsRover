# Audio replacements

Optional one-shot WAV files loaded once at startup. Missing/invalid files use generated PCM placeholders.
Wind and engine are now continuous procedural streams (no short-file looping).

| File | Use |
| --- | --- |
| ui_click.wav | UI click |
| lidar.wav | Sonar ping after a full antenna revolution (include any desired echo) |
| path_found.wav | Route found, including replanning |
| route_blocked.wav | Obstruction alert |
| recalculating.wav | Technical replanning cue |
| success.wav | Mission complete |
| error.wav | No route / invalid endpoints |

PCM WAV mono/stereo recommended; keep conservative levels.
No files are downloaded. CMake copies assets next to the executable;
runtime files are read from `<executable directory>/assets/audio/`, independently of the working directory.
Wind/engine parameters: src/drive_audio_synth.h (DriveAudioConfig).
Sonar timing, sweep and ping: src/sonar.h (SonarConfig).
Other defaults: src/audio_system.h (AudioConfig); generated one-shots: src/audio_system.cpp.
Legacy ambient_wind.wav / rover_engine.wav are intentionally not loaded, so a short old asset
cannot bring back the audible repeat. To replace those streams later, swap the two synthesizer
methods in drive_audio_synth.cpp; keep their state continuous across blocks.
