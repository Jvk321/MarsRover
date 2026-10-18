# Audio replacements

Optional WAV files loaded once at startup. Missing/invalid files use generated PCM placeholders.

| File | Use |
| --- | --- |
| ambient_wind.wav | Loop: Mars ambience and sandstorm wind |
| rover_engine.wav | Loop: quiet electric motor |
| ui_click.wav | UI click |
| lidar.wav | Scan ping |
| path_found.wav | Route found, including replanning |
| route_blocked.wav | Obstruction alert |
| recalculating.wav | Technical replanning cue |
| success.wav | Mission complete |
| error.wav | No route / invalid endpoints |

PCM WAV mono/stereo recommended; loop files should have seamless boundaries and conservative levels.
No files are downloaded. Runtime path is supplied by ROVER_AUDIO_DIR in src/CMakeLists.txt.
Defaults and gains: src/audio_system.h (AudioConfig); generated tones: src/audio_system.cpp.
