# DATA MODEL
**Live Arranger Platform — v0.9**

---

## 1. TỔNG QUAN

Data model chia làm 3 tầng:

| Tầng | Storage | Mô tả |
|---|---|---|
| **Runtime** | In-memory (C++ structs) | Engine state, event queue, active notes — không persist |
| **Library** | SQLite (GRDB) | Metadata của styles, performances, setlists — query nhanh |
| **Content** | File system (.uas, .json) | Actual style data, audio, JSON config — dereference qua ID |

---

## 2. SQLITE SCHEMA

### 2.1. `styles`

```sql
CREATE TABLE styles (
  id              TEXT PRIMARY KEY,    -- UUID v4
  name            TEXT NOT NULL,
  genre           TEXT,                -- JSON: ["pop","shuffle"]
  tags            TEXT,                -- JSON: ["wedding","slow"]
  author          TEXT,
  description     TEXT,

  -- File
  filePath        TEXT NOT NULL,       -- absolute path đến .uas trong Documents/styles/
  fileSizeBytes   INTEGER,
  sourceFormat    TEXT,                -- "yamaha-sff1" | "yamaha-sff2" | "user"
  sourceFileName  TEXT,               -- tên file gốc khi import

  -- Music info (cache từ manifest.json để không unzip khi browse)
  tempoBpm        REAL,
  timeSigNum      INTEGER,
  timeSigDen      INTEGER,
  sectionCount    INTEGER,
  hasIntro        INTEGER DEFAULT 0,  -- bool
  hasEnding       INTEGER DEFAULT 0,

  -- Import result
  importWarnings  INTEGER DEFAULT 0,  -- count của warnings
  importInfos     INTEGER DEFAULT 0,
  limitationsJson TEXT,               -- JSON array của ImportIssue

  -- User metadata
  isFavorite      INTEGER DEFAULT 0,
  rating          INTEGER,            -- 1–5, user-set
  notes           TEXT,               -- user's own notes

  -- Timestamps
  importedAt      INTEGER NOT NULL,   -- Unix timestamp
  lastUsedAt      INTEGER,
  lastModifiedAt  INTEGER,

  -- Pack info (phase 3)
  packId          TEXT,
  packVersion     TEXT,
  license         TEXT                -- "user-imported" | "cc0" | "commercial"
);

-- Full-text search
CREATE VIRTUAL TABLE styles_fts USING fts5(
  name, genre, tags, author,
  content='styles', content_rowid='rowid'
);

CREATE INDEX idx_styles_genre    ON styles(genre);
CREATE INDEX idx_styles_favorite ON styles(isFavorite);
CREATE INDEX idx_styles_lastUsed ON styles(lastUsedAt DESC);
CREATE INDEX idx_styles_pack     ON styles(packId);
```

### 2.2. `performances`

```sql
CREATE TABLE performances (
  id              TEXT PRIMARY KEY,
  name            TEXT NOT NULL,
  styleId         TEXT REFERENCES styles(id) ON DELETE SET NULL,

  -- Music settings
  tempoBpm        REAL,
  transpose       INTEGER DEFAULT 0,   -- -12 to +12
  autoFill        INTEGER DEFAULT 0,
  chordQuantize   TEXT DEFAULT 'off',  -- "off"|"beat"|"bar"

  -- Routing: JSON blob (per-track output assignment)
  routingJson     TEXT,

  -- Chord engine config: JSON
  chordEngineJson TEXT,

  -- Mixer state: JSON (volume/pan/mute/solo/sends per track)
  mixerJson       TEXT,

  -- Timestamps
  createdAt       INTEGER NOT NULL,
  lastUsedAt      INTEGER
);
```

`routingJson` example:
```json
[
  {"trackId":"drums",  "output":"internal", "channel":9},
  {"trackId":"bass",   "output":"device:USB-001", "channel":11},
  {"trackId":"phrase1","output":"virtual", "channel":15}
]
```

`mixerJson` example:
```json
[
  {"trackId":"drums",   "volume":110,"pan":64,"muted":false,"soloed":false,"reverbSend":20,"chorusSend":0},
  {"trackId":"bass",    "volume":100,"pan":64,"muted":false,"soloed":false,"reverbSend":8, "chorusSend":0}
]
```

### 2.3. `setlists`

```sql
CREATE TABLE setlists (
  id          TEXT PRIMARY KEY,
  name        TEXT NOT NULL,
  description TEXT,
  createdAt   INTEGER NOT NULL,
  modifiedAt  INTEGER
);

CREATE TABLE setlist_songs (
  id            TEXT PRIMARY KEY,
  setlistId     TEXT NOT NULL REFERENCES setlists(id) ON DELETE CASCADE,
  position      INTEGER NOT NULL,       -- 0-based ordering
  title         TEXT,                   -- song title (override)
  performanceId TEXT REFERENCES performances(id) ON DELETE SET NULL,
  styleId       TEXT REFERENCES styles(id) ON DELETE SET NULL,  -- direct, nếu không có performance
  tempoBpm      REAL,
  transpose     INTEGER DEFAULT 0,
  notes         TEXT,                   -- nhạc công ghi chú riêng (key, capo, etc.)
  UNIQUE(setlistId, position)
);
```

### 2.4. `midi_devices`

```sql
CREATE TABLE midi_devices (
  id            TEXT PRIMARY KEY,   -- CoreMIDI device unique ID
  name          TEXT NOT NULL,
  type          TEXT,               -- "usb"|"ble"|"network"|"virtual"
  resetMode     TEXT DEFAULT 'gm', -- "gm"|"gs"|"xg"|"none"
  lastSeenAt    INTEGER
);
```

### 2.5. `sound_packs`

```sql
CREATE TABLE sound_packs (
  id          TEXT PRIMARY KEY,
  name        TEXT NOT NULL,
  packPath    TEXT NOT NULL,         -- path đến pack folder
  license     TEXT,
  version     TEXT,
  isBuiltIn   INTEGER DEFAULT 0,     -- 1 = GM bank built-in
  isActive    INTEGER DEFAULT 1,
  installedAt INTEGER NOT NULL
);

CREATE TABLE sound_presets (
  id          TEXT PRIMARY KEY,      -- "bass/fingered_standard"
  packId      TEXT REFERENCES sound_packs(id),
  name        TEXT,
  sfzPath     TEXT NOT NULL,
  role        TEXT,                  -- "bass"|"drums"|"pad"|"phrase"|"chord"|"other"
  gmProgram   INTEGER,               -- GM program number nếu có
  gmBankMsb   INTEGER,
  gmBankLsb   INTEGER
);
```

---

## 3. FILE SYSTEM STRUCTURES

### 3.1. Style file `.uas`

*(Đã định nghĩa đầy đủ trong UASF Spec v0.9 — tóm tắt tại đây)*

```
{uuid}.uas   (ZIP)
├── manifest.json
├── sections/
│   └── *.mid
├── samples/       (phase 2)
└── source.bin     (optional)
```

### 3.2. Performance file

Performances không lưu riêng file (lưu trong SQLite). Nhưng có thể export/import:

```json
// performance-export.json
{
  "exportVersion": "1.0",
  "performance": {
    "id": "...",
    "name": "Wedding Ballad Setup",
    "styleRef": { "sourceFileName": "CoolBallad.sty", "styleName": "Cool Ballad" },
    "tempoBpm": 76.0,
    "transpose": 0,
    "autoFill": false,
    "chordEngine": { "mode": "fingered", "splitPoint": 60 },
    "routing": [...],
    "mixer": [...]
  }
}
```

`styleRef` dùng `sourceFileName` thay vì `id` — vì ID khác nhau trên mỗi thiết bị.

### 3.3. Setlist file

```json
// setlist-export.json
{
  "exportVersion": "1.0",
  "setlist": {
    "name": "Show 15/6",
    "songs": [
      {
        "position": 0,
        "title": "Người Ở Lại Charlie",
        "styleRef": { "sourceFileName": "slow_rock.sty" },
        "tempoBpm": 72.0,
        "transpose": 0,
        "notes": "Key G, capo 0"
      }
    ]
  }
}
```

---

## 4. RUNTIME DATA MODEL (C++ — không persist)

### 4.1. UASFStyle (loaded style)

```cpp
struct UASFStyle {
  std::string   id;
  StyleManifest manifest;              // parsed manifest.json

  // Sections: pre-loaded, immutable
  std::vector<SectionData> sections;

  // Pre-computed:
  NTTLookupTable nttTable;             // [mode][chordType][srcPC] → dstPC
  DrumNoteMap    drumMap;

  // Index for fast lookup
  std::unordered_map<std::string, size_t> sectionIndex;  // sectionId → index
};

struct SectionData {
  std::string   sectionId;
  SectionType   type;        // enum
  int           ordinal;
  int           lengthTicks;
  bool          loop;
  std::string   fillTarget;
  std::vector<MidiEvent> events;       // sorted by tick
  std::vector<TrackRule> trackRules;   // indexed by trackId
};
```

### 4.2. EngineState (atomic snapshot cho UI)

```cpp
struct EngineState {
  // Transport
  PlayState     playState;    // STOPPED | COUNT_IN | PLAYING
  double        currentBpm;
  int           currentBar;
  int           currentBeat;
  int           ppq;

  // Section
  std::string   currentSectionId;
  std::string   pendingSectionId;   // empty = không có pending
  bool          autoFill;

  // Chord
  Chord         currentChord;

  // Tracks
  struct TrackState {
    bool  muted;
    bool  soloed;
    float volume;
    float pan;
  };
  std::array<TrackState, 16> tracks;

  // Alerts
  bool          midiDeviceDisconnected;
  std::string   loadedStyleName;

  // Timestamp
  uint64_t      snapshotTime;         // host time khi snapshot được tạo
};
```

### 4.3. Chord struct

```cpp
struct Chord {
  uint8_t    root;           // 0-11, C=0; 255 = NONE
  ChordType  type;           // enum
  uint8_t    bassNote;       // 0-127
  uint64_t   hostTimestamp;
  InputSource source;
};
```

### 4.4. MidiEvent

```cpp
struct MidiEvent {
  int64_t   tick;         // absolute tick trong section
  uint8_t   status;       // MIDI status byte
  uint8_t   data1;
  uint8_t   data2;
  uint8_t   channel;      // 0-15

  // Runtime only (không trong file):
  int       frameOffset;  // computed per render block
  TrackId   trackId;
};
```

---

## 5. MIGRATION STRATEGY

### SQLite migration

GRDB `DatabaseMigrator`:
```swift
var migrator = DatabaseMigrator()
migrator.registerMigration("v1") { db in
  try db.create(table: "styles") { ... }
  try db.create(table: "performances") { ... }
  // ...
}
migrator.registerMigration("v2") { db in
  try db.alter(table: "styles") { t in
    t.add(column: "rating", .integer)
  }
}
// thêm migration khi schema thay đổi
try migrator.migrate(dbQueue)
```

### UASF migration

`formatVersion` trong manifest.json. Engine kiểm tra version khi load:
```swift
func migrateIfNeeded(_ style: UASFBundle) -> UASFBundle {
  switch style.manifest.formatVersion {
  case "1.0": return style  // current
  case "0.9": return migrate_0_9_to_1_0(style)
  default:
    if style.manifest.minEngineVersion > currentEngineVersion:
      throw UASFError.engineTooOld
    return style  // ignore-unknown policy
  }
}
```

Migration giữ nguyên `source.bin` — nếu migration có bug, user có thể re-import.

---

## 6. DATA INTEGRITY RULES

| Rule | Mức | Enforce ở đâu |
|---|---|---|
| `styles.filePath` phải tồn tại trên disk | Warn (không delete tự động) | Library scan khi app start |
| `setlist_songs.styleId` orphan khi style deleted | Set NULL, hiển thị "Style missing" | SQLite FK + UI |
| Không có 2 style trùng `sourceHash` | Warn user "duplicate detected" | Import Adapter |
| `performance.styleId` có thể NULL | Cho phép (performance không gắn style) | — |
| `tempoBpm` trong [20, 300] | Validate trước save | UI + model |
| `transpose` trong [-12, 12] | Validate trước save | UI + model |
