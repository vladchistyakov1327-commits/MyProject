# АРХИТЕКТУРНЫЙ ПРОМТ — NestingApp
## Профессиональная программа раскладки деталей из листового металла
### Платформа: Qt 6 / C++17 / Desktop

---

## 🎯 ОБЗОР ПРОЕКТА

Создай полноценное desktop-приложение **NestingApp** — профессиональный инструмент для оптимальной раскладки (нестинга) деталей из листового металла на плоских листах произвольной формы.

**Ключевые принципы разработки:**
- Каждый модуль — независимый, тестируемый, с собственным логгером
- Ни одна операция не падает молча — всё логируется с контекстом
- UI строго отделён от бизнес-логики (core не зависит от Qt Widgets)
- Производительность: 500 типов деталей × 10 000 шт. должны раскладываться без зависания UI
- Стиль UI — строго по предоставленному `AppStyle.h` (индустриальный тёмный, металлические акценты)

---

## 🏗️ ТЕХНОЛОГИЧЕСКИЙ СТЕК

```
Язык:           C++17
UI фреймворк:   Qt 6.5+ (QtWidgets + QtConcurrent)
Геометрия:      Clipper2 (https://github.com/AngusJohnson/Clipper2)
                — polygon offset, boolean ops, NFP-вычисления
Нестинг-ядро:   libnest2d (https://github.com/tamasmeszaros/libnest2d)
                — True Shape нестинг с NFP, интерфейс над Clipper2
                — fallback: собственный Bottom-Left + NFP если libnest2d не подходит
DXF парсер:     libdxfrw (https://github.com/codelibs/libdxfrw)
                — поддержка форматов AC1009 (R12) .. AC1032 (2018+)
                — все entity: LINE, ARC, CIRCLE, LWPOLYLINE, POLYLINE, SPLINE, ELLIPSE, INSERT
Многопоток:     QThreadPool + QFuture<NestResult> (QtConcurrent::run)
Сборка:         CMake 3.25+ с FetchContent для зависимостей
Тесты:          Catch2 для core/, Qt Test для UI-компонентов
```

---

## 📁 ПОЛНАЯ СТРУКТУРА ПРОЕКТА

```
NestingApp/
├── CMakeLists.txt                        ← корневой, подключает все модули
├── AppStyle.h                            ← ПРЕДОСТАВЛЕН, не изменять
├── AppStyle.cpp                          ← реализация appStyleSheet() — см. раздел UI
│
├── core/                                 ← НУЛЕВАЯ зависимость от QtWidgets
│   │
│   ├── geometry/
│   │   ├── DxfImporter.h/.cpp            ← парсинг DXF всех версий
│   │   ├── PolyContour.h/.cpp            ← нормализованный контур детали
│   │   ├── GeomUtils.h/.cpp              ← offset, simplify, bbox, area
│   │   └── CoedgeDetector.h/.cpp         ← алгоритм объединения общих кромок
│   │
│   ├── nesting/
│   │   ├── NestJob.h                     ← структура задания (входные данные)
│   │   ├── NestResult.h                  ← результат (PlacedPart по листам)
│   │   ├── NestEngine.h/.cpp             ← главный оркестратор нестинга
│   │   ├── NFPCalculator.h/.cpp          ← No-Fit Polygon через Clipper2
│   │   ├── PlacementStrategy.h           ← enum всех стратегий направления
│   │   ├── RotationSampler.h/.cpp        ← генерация шагов вращения
│   │   └── SheetManager.h/.cpp           ← управление листами, остатки
│   │
│   ├── export/
│   │   ├── LxdsExporter.h/.cpp           ← экспорт в .lxds (CypCut XML)
│   │   ├── DxfExporter.h/.cpp            ← экспорт раскладки в DXF
│   │   └── ReportGenerator.h/.cpp        ← CSV/текстовый отчёт по листам
│   │
│   └── logging/
│       ├── AppLogger.h/.cpp              ← центральный логгер (singleton)
│       ├── LogEntry.h                    ← структура записи лога
│       └── LogChannel.h                  ← enum каналов: IMPORT/NEST/EXPORT/UI/SYSTEM
│
├── ui/
│   ├── MainWindow.h/.cpp                 ← главное окно, меню, статусбар
│   ├── TopBar.h/.cpp                     ← верхняя панель с кнопками действий
│   │
│   ├── tabs/
│   │   ├── PartListTab.h/.cpp            ← вкладка "Детали"
│   │   ├── NestSettingsTab.h/.cpp        ← вкладка "Настройки раскладки"
│   │   ├── SheetsTab.h/.cpp              ← вкладка "Листы" (результат)
│   │   └── LogTab.h/.cpp                 ← вкладка "Лог"
│   │
│   ├── canvas/
│   │   ├── NestCanvas.h/.cpp             ← QGraphicsView — отображение листа
│   │   ├── PartGraphicsItem.h/.cpp       ← QGraphicsItem для одной детали
│   │   └── SheetGraphicsItem.h/.cpp      ← QGraphicsItem для листа/заготовки
│   │
│   ├── dialogs/
│   │   ├── ImportDxfDialog.h/.cpp        ← диалог импорта DXF с превью
│   │   ├── SheetSetupDialog.h/.cpp       ← настройка листа (размер или DXF)
│   │   └── ExportDialog.h/.cpp           ← настройки экспорта .lxds / DXF
│   │
│   └── widgets/
│       ├── ProgressOverlay.h/.cpp        ← оверлей прогресса нестинга
│       ├── UtilizationBar.h/.cpp         ← виджет % утилизации листа
│       └── LogWidget.h/.cpp              ← таблица логов с фильтрами
│
└── tests/
    ├── test_dxf_importer.cpp
    ├── test_nfp_calculator.cpp
    ├── test_nest_engine.cpp
    └── test_lxds_exporter.cpp
```

---

## 🔷 МОДУЛЬ 1: DXF ИМПОРТ (`core/geometry/DxfImporter`)

### Требования
- Поддержка **всех версий DXF**: AC1009 (R12), AC1015 (2000), AC1018 (2004), AC1021 (2007), AC1024 (2010), AC1027 (2013), AC1032 (2018+)
- Entity для извлечения контуров: `LINE`, `ARC`, `CIRCLE`, `LWPOLYLINE`, `POLYLINE` (с дугами), `SPLINE` (аппроксимация полигоном), `ELLIPSE` (аппроксимация), `INSERT` (блоки с трансформацией)
- Автосборка разрозненных сегментов в замкнутые контуры (chain-building с tolerance)
- Определение: что является **внешним** контуром детали, что — **отверстием** (по направлению обхода + вложенности)
- Единицы: автодетектирование (мм / дюймы / метры) по заголовку DXF `$INSUNITS`
- Обработка слоёв (LAYER): опциональная фильтрация по имени слоя

### Структура `PolyContour`
```cpp
struct PolyContour {
    std::vector<QPointF>  vertices;      // полигон (аппроксимация всех кривых)
    bool                  isHole;        // true = отверстие внутри детали
    double                approximationTolerance; // мм, использованный при аппроксимации
};

struct PartGeometry {
    QString               partId;        // уникальный ID
    QString               sourceName;    // имя файла DXF
    PolyContour           outerContour;  // внешний контур
    std::vector<PolyContour> holes;      // внутренние отверстия
    QRectF                boundingBox;
    double                areaMm2;
    QString               sourceLayer;  // из какого слоя DXF
};
```

### Обработка ошибок
Каждая из следующих ситуаций должна логироваться отдельной записью в `LogChannel::IMPORT`:
- Незамкнутый контур (gap > tolerance) → предупреждение + попытка автозакрытия
- Самопересечение контура → предупреждение + попытка исправления через Clipper2 SimplifyPolygon
- Нулевая площадь детали → ошибка, деталь пропускается
- Неизвестный entity type → информационное сообщение
- Версия DXF не определена → предупреждение, попытка парсинга как AC1015
- Блок INSERT с трансформацией → информационное сообщение с матрицей

---

## 🔷 МОДУЛЬ 2: СТРУКТУРЫ ЗАДАНИЯ И РЕЗУЛЬТАТА

### `NestJob` — входное задание
```cpp
struct PartEntry {
    PartGeometry  geometry;        // геометрия детали
    int           quantity;        // количество (1 .. 10 000)
    QString       name;            // пользовательское имя
    bool          allowFlip;       // разрешён ли переворот (зеркало)
    RotationMode  rotationMode;    // NONE / STEP_90 / STEP_45 / CUSTOM
    double        customRotStep;   // шаг в градусах если CUSTOM (1..360)
};

struct SheetDefinition {
    enum Type { RECTANGLE, CUSTOM_DXF };
    Type          type;
    double        widthMm;          // для RECTANGLE
    double        heightMm;         // для RECTANGLE
    PartGeometry  customShape;      // для CUSTOM_DXF — контур листа
    int           quantity;         // кол-во листов (или 0 = бесконечно)
    QString       materialId;
    double        thicknessMm;
};

struct NestParameters {
    // Зазоры
    double        partSpacingMm;    // расстояние между деталями (0.1 .. 25.0 мм)
    double        sheetMarginMm;    // отступ от края листа (0.1 .. 25.0 мм)

    // Стратегия размещения
    PlacementStrategy strategy;     // направление обхода листа

    // Вращение
    bool          enableRotation;   // глобальный флаг
    double        globalRotStepDeg; // шаг (если не переопределён на детали)
    // Значения: 1, 5, 10, 15, 22.5, 30, 45, 90, 180, 360 — или произвольный от 1 до 360

    // Coedge
    bool          enableCoedge;     // объединение общих кромок
    double        coedgeTolerance;  // допуск совпадения кромок в мм

    // Алгоритм
    NestAlgorithm algorithm;        // BOTTOM_LEFT_NFP / GENETIC / SIMULATED_ANNEALING
    int           maxIterations;    // для эвристических методов
    int           timeLimitSec;     // таймаут расчёта (0 = без ограничения)
    int           threadCount;      // число потоков (0 = auto = QThread::idealThreadCount())
};

struct NestJob {
    std::vector<PartEntry>   parts;
    SheetDefinition          sheet;
    NestParameters           params;
    QString                  jobId;      // UUID
    QDateTime                createdAt;
};
```

### `NestResult` — результат
```cpp
struct PlacedPart {
    QString       partId;
    QString       instanceId;       // partId + "_" + порядковый номер
    QPointF       positionMm;       // позиция origin на листе
    double        rotationDeg;      // применённый угол поворота
    bool          flipped;          // зеркало
    int           sheetIndex;       // на каком листе (0-based)
    QPolygonF     placedContour;    // трансформированный контур (для отрисовки)
};

struct SheetResult {
    int           sheetIndex;
    double        utilizationPct;   // % использования площади
    double        usedAreaMm2;
    double        totalAreaMm2;
    double        wasteAreaMm2;
    int           partsCount;
    std::vector<PlacedPart> placedParts;
    QRectF        sheetBounds;
};

struct NestResult {
    QString                    jobId;
    bool                       success;
    QString                    errorMessage;
    std::vector<SheetResult>   sheets;
    int                        totalPartsPlaced;
    int                        totalPartsRequested;
    int                        unplacedPartsCount;
    std::vector<QString>       unplacedPartIds;    // что не влезло
    double                     avgUtilizationPct;
    QDateTime                  calculatedAt;
    qint64                     elapsedMs;
    std::vector<LogEntry>      nestLog;             // лог именно этого расчёта
};
```

---

## 🔷 МОДУЛЬ 3: АЛГОРИТМ НЕСТИНГА (`core/nesting/NestEngine`)

### Стратегии размещения (`PlacementStrategy`)
```cpp
enum class PlacementStrategy {
    BOTTOM_LEFT,      // снизу-вверх, слева-направо (классика)
    BOTTOM_RIGHT,     // снизу-вверх, справа-налево
    TOP_LEFT,         // сверху-вниз, слева-направо
    TOP_RIGHT,        // сверху-вниз, справа-налево
    LEFT_BOTTOM,      // слева-направо, снизу-вверх
    LEFT_TOP,         // слева-направо, сверху-вниз
    RIGHT_BOTTOM,     // справа-налево, снизу-вверх
    RIGHT_TOP,        // справа-налево, сверху-вниз
    GRAVITY_CENTER,   // к центру листа
    GRAVITY_EDGE,     // к ближайшему краю
};
```
Каждая стратегия определяет **sorting key** для кандидатных позиций при NFP-обходе.

### NFP (No-Fit Polygon) — ключевой алгоритм
```
Для каждой пары (фиксированная деталь A, размещаемая деталь B):
  1. Построить NFP(A, B) через Clipper2 MinkowskiSum
  2. NFP определяет все недопустимые позиции origin детали B
  3. Допустимые позиции = пространство листа МИНУС union всех NFP
  4. Выбрать лучшую позицию согласно PlacementStrategy
  5. Добавить B к размещённым → обновить union NFP для следующей детали

ВАЖНО: NFP кэшировать по парам (hashA, hashB, rotation) — для 500 типов деталей
без кэша будет O(n²) пересчётов. Кэш: QHash<NFPKey, QPolygonF>.
```

### Зазоры
Зазор между деталями реализуется через **offset контуров** перед нестингом:
- `offsetContour = Clipper2::InflatePaths(contour, partSpacingMm / 2)`
- Отступ от края листа: дефлятировать контур листа на `sheetMarginMm`
- После размещения — для экспорта использовать **оригинальные** (не offset) контуры

### Coedge — объединение общих кромок
```
Алгоритм CoedgeDetector:
  1. Для двух уже размещённых деталей проверить все пары рёбер
  2. Ребро считается "общим" если:
     - обратная ориентация (A→B у первой, B→A у второй)
     - расстояние между рёбрами < coedgeTolerance
  3. При совпадении — сместить вторую деталь вплотную (зазор = 0 для этой кромки)
  4. Пересчитать NFP только для затронутых деталей
  5. В лог записать: какие детали, какие рёбра объединены, экономия площади

Coedge особенно полезен для прямолинейных деталей (прямоугольники, трапеции)
```

### Многопоточность
```cpp
// NestEngine::runAsync() возвращает QFuture<NestResult>
// Внутри: QtConcurrent::run на QThreadPool
// Прогресс: через QPromise<NestResult> с promse.setProgressValue()
// Отмена: std::atomic<bool> cancelRequested — проверять в каждой итерации NFP
// UI поток: NestProgressWorker слушает QFutureWatcher<NestResult>
```

### Порядок сортировки деталей перед раскладкой
Детали сортировать по убыванию площади (`areaMm2`) — крупные сначала. Это критично для качества результата. Опционально: сортировка по периметру, по bounding box diagonal.

---

## 🔷 МОДУЛЬ 4: ЭКСПОРТ В .LXDS (CypCut формат)

### Что такое .lxds
Файл `.lxds` — это **ZIP-архив** (переименованный .zip) содержащий:
```
layout.xml          ← основной файл раскладки
parts/
  part_001.dxf      ← геометрия каждой уникальной детали
  part_002.dxf
  ...
thumbnail.png       ← превью (опционально)
```

### Структура `layout.xml`
```xml
<?xml version="1.0" encoding="UTF-8"?>
<Layout version="2.0" software="NestingApp" created="2025-01-01T00:00:00">
  <Sheet width="3000" height="1500" unit="mm" material="Steel" thickness="3.0">
    <Contour>
      <!-- для нестандартных листов: список точек контура -->
    </Contour>
  </Sheet>
  <Parts>
    <Part id="part_001" name="ДетальА" quantity_placed="47" file="parts/part_001.dxf">
      <Instance id="1" x="125.3" y="47.8" rotation="90.0" flipped="false"/>
      <Instance id="2" x="230.1" y="47.8" rotation="0.0"  flipped="false"/>
      <!-- ... -->
    </Part>
  </Parts>
  <Statistics total_parts="500" sheets_used="3" avg_utilization="87.4"/>
</Layout>
```

### Требования к экспорту
- Координаты в мм, float с точностью 4 знака после запятой
- Угол поворота: 0..360, по часовой стрелке (конвенция CypCut)
- Каждая уникальная деталь сохраняется как отдельный DXF в папке `parts/`
- DXF для деталей: версия AC1015 (R2000), LWPOLYLINE entity, слой "0"
- Листовой DXF (если нестандартный лист): аналогично
- Сжатие ZIP: уровень 6 (Deflate)
- Использовать `QZipWriter` (Qt 6) или `miniz` (если QZipWriter недоступен)
- В лог записать: путь к файлу, количество листов, деталей, размер архива

---

## 🔷 МОДУЛЬ 5: СИСТЕМА ЛОГИРОВАНИЯ (`core/logging/AppLogger`)

### Принципы
Это **самый важный отладочный инструмент**. Лог должен давать полную картину происходящего.

```cpp
enum class LogChannel {
    IMPORT,     // парсинг DXF, ошибки геометрии
    GEOMETRY,   // offset, NFP, упрощение
    NESTING,    // итерации алгоритма, размещение деталей
    COEDGE,     // обнаружение и применение общих кромок
    EXPORT,     // запись .lxds, .dxf
    UI,         // действия пользователя, навигация
    SYSTEM      // старт/стоп, память, производительность
};

enum class LogLevel { DEBUG, INFO, WARNING, ERROR, CRITICAL };

struct LogEntry {
    QDateTime   timestamp;
    LogLevel    level;
    LogChannel  channel;
    QString     message;
    QString     detail;        // расширенный контекст (stacktrace, значения)
    QString     partId;        // если относится к конкретной детали
    int         sheetIndex;    // если относится к конкретному листу (-1 = общий)
    qint64      elapsedMs;     // время от начала операции
};
```

### AppLogger API
```cpp
class AppLogger : public QObject {
    Q_OBJECT
public:
    static AppLogger& instance();
    
    void log(LogLevel level, LogChannel channel, const QString& message,
             const QString& detail = {}, const QString& partId = {},
             int sheetIndex = -1);
    
    // Удобные методы
    void info   (LogChannel ch, const QString& msg, const QString& detail = {});
    void warning(LogChannel ch, const QString& msg, const QString& detail = {});
    void error  (LogChannel ch, const QString& msg, const QString& detail = {});
    
    // Для нестинга — замер времени
    void beginOperation(const QString& name);   // записывает START + время
    void endOperation  (const QString& name);   // записывает END + elapsed
    
    // Для UI
    std::vector<LogEntry> entries() const;
    void clear();
    
signals:
    void entryAdded(LogEntry entry);    // UI подключается к этому сигналу
};

// Макросы для удобства
#define LOG_INFO(ch, msg)     AppLogger::instance().info(ch, msg)
#define LOG_WARN(ch, msg, d)  AppLogger::instance().warning(ch, msg, d)
#define LOG_ERR(ch, msg, d)   AppLogger::instance().error(ch, msg, d)
```

### Что логировать ОБЯЗАТЕЛЬНО
**IMPORT канал:**
- Начало/конец парсинга каждого DXF (имя файла, версия, elapsed)
- Количество найденных entity по типам
- Каждый незамкнутый контур: ID, gap в мм, были ли автозакрыты
- Каждое самопересечение: ID детали, координаты пересечения
- Итог: N деталей импортировано, M пропущено (с причинами)

**NESTING канал:**
- Старт расчёта: параметры задания (N типов, M штук, лист XxY, стратегия, шаг вращения)
- Каждые 5% прогресса: сколько деталей размещено / всего
- Каждый лист: номер, % утилизации, количество деталей
- Детали, которые не удалось разместить: ID, причина (не влезает / нет позиции)
- Итог: N листов, средняя утилизация, elapsed

**COEDGE канал:**
- Каждая найденная общая кромка: детали A и B, рёбра, смещение
- Итоговая экономия площади от coedge

**EXPORT канал:**
- Путь экспорта, формат
- Количество листов / деталей в экспорте
- Размер файла, elapsed

---

## 🔷 МОДУЛЬ 6: ГЛАВНОЕ ОКНО И UI (`ui/MainWindow`)

### Компоновка главного окна

```
┌─────────────────────────────────────────────────────────────────┐
│  TOPBAR: [📂 Импорт DXF] [⚙ Настройки листа] [▶ Рассчитать]   │
│          [⏹ Остановить]  [💾 Экспорт .lxds]  [📄 Экспорт DXF] │
│          ──────────────────── статус: "Готово | 3 листа | 87%" │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────────────────┐  ┌───────────────────────────────┐ │
│  │  LEFT PANEL (340px)     │  │  CENTER: NestCanvas           │ │
│  │                         │  │                               │ │
│  │  QTabWidget:            │  │  QGraphicsView с отображением │ │
│  │  [Детали][Настройки]    │  │  текущего листа               │ │
│  │                         │  │                               │ │
│  │  ── Вкладка Детали ──   │  │  • Колёсо = zoom             │ │
│  │  Таблица: имя / кол-во  │  │  • ПКМ drag = pan            │ │
│  │  / тип / статус         │  │  • Клик на деталь = выделить │ │
│  │  [+ Добавить DXF]       │  │  • Показать: контур + fill    │ │
│  │  [✕ Удалить выбранные]  │  │    цвет по типу детали        │ │
│  │                         │  └───────────────────────────────┘ │
│  │  ── Вкладка Настройки ──│                                    │
│  │  (см. NestSettingsTab)  │                                    │
│  └─────────────────────────┘                                    │
│                                                                 │
├─────────────────────────────────────────────────────────────────┤
│  BOTTOM TABS: [Листы (3)] [Лог (47 записей)]                   │
│                                                                 │
│  ── Вкладка Листы ──                                           │
│  Карточки листов: Лист 1 [██████████ 91%] 234 дет.            │
│                   Лист 2 [████████░░ 82%] 198 дет.            │
│                   Лист 3 [████░░░░░░ 43%]  68 дет.            │
│  Клик на карточку → показать лист в NestCanvas                 │
│                                                                 │
│  ── Вкладка Лог ──                                             │
│  Фильтры: [Все каналы ▼] [Все уровни ▼] [🔍 поиск]           │
│  Таблица: время | уровень | канал | сообщение | деталь         │
└─────────────────────────────────────────────────────────────────┘
```

---

## 🔷 МОДУЛЬ 7: ВКЛАДКА НАСТРОЙКИ РАСКЛАДКИ (`NestSettingsTab`)

Все настройки применяются немедленно к `NestJob::params`. Группировать визуально через `QGroupBox`.

### Группа "Зазоры"
```
Расстояние между деталями:  [___2.0___] мм  (QDoubleSpinBox, 0.1..25.0, шаг 0.1)
Отступ от края листа:       [___5.0___] мм  (QDoubleSpinBox, 0.1..25.0, шаг 0.1)
```

### Группа "Направление раскладки"
```
8 кнопок-стрелок в виде компаса (QButtonGroup, exclusive):

        ↖ TB_LEFT    ↑ TOP_LEFT   ↗ TB_RIGHT
        ← LEFT_TOP   [✦ CENTER]   → RIGHT_TOP
        ↙ BL_LEFT    ↓ BOTTOM_LEFT ↘ BR_RIGHT

Подпись под выбранной кнопкой: "Снизу-вверх, слева-направо"
```

### Группа "Вращение"
```
☑ Разрешить вращение деталей

Шаг вращения:  [──●──────] (QSlider 1..360 + QSpinBox)
Предустановки: [1°] [5°] [10°] [22.5°] [30°] [45°] [90°]

☐ Переопределить для конкретных деталей (→ кнопка "Настроить...")
```

### Группа "Coedge (объединение кромок)"
```
☐ Включить Coedge

Допуск совпадения: [___0.5___] мм

ℹ Coedge позволяет двум деталям разделять одну линию реза,
  экономя материал на прямых кромках.
```

### Группа "Алгоритм"
```
Метод:  ● NFP + Bottom-Left  ○ NFP + Genetic  ○ Simulated Annealing
Потоков: [auto ▼]  (1, 2, 4, 8, auto)
Таймаут: [___0___] сек  (0 = без ограничения)
```

---

## 🔷 МОДУЛЬ 8: ВКЛАДКА ЛИСТЫ (`SheetsTab`)

### Виджет карточки листа
```
┌────────────────────────────────────────────────────────┐
│  Лист 1 из 3                              [Просмотр ▶] │
│                                                        │
│  ████████████████████████░░░░  87.4%                  │
│                                                        │
│  Деталей: 234    Площадь: 3 000 × 1 500 мм            │
│  Использовано: 3.92 м²   Отходы: 0.58 м²              │
└────────────────────────────────────────────────────────┘
```

- Карточки в `QScrollArea` (вертикальный список)
- Клик → переключить `NestCanvas` на этот лист
- Активная карточка подсвечена `BLUE_BORDER` из палитры
- Кнопка "Просмотр" → развернуть/свернуть мини-превью прямо в карточке
- Внизу вкладки: суммарная статистика (все листы, среднее %, итого деталей)

---

## 🔷 МОДУЛЬ 9: ВКЛАДКА ЛОГ (`LogTab`)

### Фильтры (QToolBar над таблицей)
```
[Канал: Все ▼]  [Уровень: Все ▼]  [🔍 Поиск...________]  [🗑 Очистить]  [💾 Сохранить в файл]
```

### Таблица логов (`QTableView` + кастомная модель)
```
Колонки:
  Время     │ Уровень  │ Канал    │ Сообщение                        │ Деталь   │ Лист
  ──────────┼──────────┼──────────┼──────────────────────────────────┼──────────┼──────
  12:34:56  │ ⚠ WARN   │ IMPORT   │ Незамкнутый контур, gap=0.3мм   │ part_007 │  —
  12:34:56  │ ✓ INFO   │ IMPORT   │ Автозакрыт контур               │ part_007 │  —
  12:35:01  │ ✓ INFO   │ NESTING  │ Лист 1: 87.4% утилизация        │  —       │  1
  12:35:02  │ ✗ ERROR  │ NESTING  │ Деталь не размещена: нет места  │ part_143 │  —
```

### Цветовая кодировка строк (из Palette):
- `ERROR` / `CRITICAL` → фон `RED_DIM`, текст `RED_TEXT`
- `WARNING` → фон `AMBER_TEXT` с opacity 10%, текст `AMBER_TEXT`
- `INFO` → фон прозрачный, текст `TEXT_NORMAL`
- `DEBUG` → фон прозрачный, текст `TEXT_MUTED`

### Двойной клик на строку → `QDialog` с полным `detail` текстом

### Автоскролл
- Включён по умолчанию, отключается если пользователь вручную прокрутил вверх
- Кнопка "↓ К концу" появляется когда autoscroll отключён

---

## 🔷 МОДУЛЬ 10: CANVAS (`ui/canvas/NestCanvas`)

### Основа: `QGraphicsView` + `QGraphicsScene`

```cpp
class NestCanvas : public QGraphicsView {
    // Обязательно реализовать:
    void setSheetResult(const SheetResult& result);  // загрузить лист для отображения
    void setZoom(double factor);
    void fitToWindow();
    void highlightPart(const QString& instanceId);  // подсветить деталь

    // Управление
    // Колёсо мыши → zoom (ctrl+wheel или просто wheel)
    // ПКМ + drag  → pan
    // ЛКМ click   → выбор детали → сигнал partSelected(instanceId)

signals:
    void partSelected(QString instanceId);
    void zoomChanged(double factor);
};
```

### Отображение деталей
- Каждый тип детали — свой цвет заливки (генерировать из HSV palette, 500 различимых цветов)
- Контур детали: `BORDER_MID` из палитры, толщина 1px в мировых координатах
- Выделенная деталь: контур `BLUE_BRIGHT`, заливка чуть ярче
- Отверстия внутри деталей: заливка цветом фона (`BG_VOID`)
- Лист: рамка `BORDER_LIGHT`, фон `BG_DEEP`
- Отступ от края (margin): штриховая линия `TEXT_GHOST`

### Производительность
- До 10 000 деталей на листе: использовать `QGraphicsItem` с `ItemUsesExtendedStyleOption`
- LOD (Level of Detail): при zoom < 0.3 рисовать только bounding rect (без контура)
- При zoom > 2.0 показывать ID детали текстом внутри

---

## 🔷 МОДУЛЬ 11: `AppStyle.cpp` — реализация стиля

Реализуй функцию `appStyleSheet()` используя **все** константы из предоставленного `AppStyle.h`.

### Обязательные элементы stylesheet:

```css
/* QMainWindow */
QMainWindow { background: BG_BASE; }

/* QTabWidget */
QTabWidget::pane { border: 1px solid BORDER_MID; background: BG_SURFACE; }
QTabBar::tab { background: BG_OVERLAY; color: TEXT_DIM; padding: 8px 16px;
               border-bottom: 2px solid transparent; }
QTabBar::tab:selected { color: TEXT_BRIGHT; border-bottom: 2px solid BLUE_BORDER; }
QTabBar::tab:hover { color: TEXT_NORMAL; background: BG_RAISED; }

/* QPushButton — primary (нестинг) */
QPushButton[accent="green"] {
  background: GREEN_DARK; color: GREEN_TEXT; border: 1px solid GREEN_BORDER; }
QPushButton[accent="green"]:hover { background: GREEN_MID; }
QPushButton[accent="green"]:pressed { background: GREEN_DIM; }

/* QPushButton — action (экспорт, импорт) */
QPushButton[accent="blue"] { background: BLUE_DARK; color: BLUE_TEXT; border: 1px solid BLUE_BORDER; }

/* QPushButton — danger (отмена, удаление) */
QPushButton[accent="red"] { background: RED_DARK; color: RED_TEXT; border: 1px solid RED_BORDER; }

/* QTableView */
QTableView { background: BG_DEEP; color: TEXT_NORMAL; gridline-color: BORDER_DARK;
             selection-background-color: BLUE_MID; selection-color: TEXT_WHITE; }
QHeaderView::section { background: BG_OVERLAY; color: TEXT_DIM;
                        border-bottom: 1px solid BORDER_MID; padding: 4px 8px; }

/* QDoubleSpinBox, QSpinBox */
QDoubleSpinBox, QSpinBox {
  background: BG_RAISED; color: TEXT_BRIGHT; border: 1px solid BORDER_MID;
  padding: 4px 8px; border-radius: 3px; }
QDoubleSpinBox:focus, QSpinBox:focus { border: 1px solid BORDER_LIGHT; }

/* QGroupBox */
QGroupBox { color: TEXT_DIM; border: 1px solid BORDER_DARK;
            border-radius: 4px; margin-top: 8px; padding-top: 16px; }
QGroupBox::title { color: CYAN_TEXT; padding: 0 6px; }

/* QScrollBar */
QScrollBar:vertical { background: BG_DEEP; width: 8px; }
QScrollBar::handle:vertical { background: BORDER_MID; border-radius: 4px; min-height: 20px; }
QScrollBar::handle:vertical:hover { background: BORDER_LIGHT; }
QScrollBar::add-line, QScrollBar::sub-line { height: 0; }

/* TopBar */
#topBar { background: TOPBAR_BG; border-bottom: 1px solid TOPBAR_BORD; padding: 6px 12px; }

/* LogWidget — уровни */
QTableView[role="log"] QModelIndex[level="ERROR"] { background: RED_DIM; color: RED_TEXT; }
QTableView[role="log"] QModelIndex[level="WARNING"] { color: AMBER_TEXT; }
```

---

## 🔷 ДИАЛОГ ИМПОРТА DXF (`ImportDxfDialog`)

```
┌──────────────────────────────────────────────────────┐
│  Импорт деталей из DXF файлов                    [✕] │
├──────────────────────────────────────────────────────┤
│  Файлы:  [__________________________] [Добавить...]  │
│  ┌────────────────────────────────────────────────┐  │
│  │ ✓  flanets_v2.dxf          — 1 контур         │  │
│  │ ⚠  bracket.dxf             — 3 контура, 1 warn│  │
│  │ ✗  corrupted.dxf           — ошибка парсинга  │  │
│  └────────────────────────────────────────────────┘  │
│                                                      │
│  Превью выбранного файла: [QGraphicsView 200x200]    │
│                                                      │
│  Количество: [____100____]  Слой: [Все слои ▼]       │
│  ☐ Разрешить переворот (зеркало)                     │
│  Шаг вращения: [45°]                                 │
│                                                      │
│  Единицы: ● Авто  ○ мм  ○ дюймы                     │
│                                                      │
│              [Отмена]  [Добавить детали]             │
└──────────────────────────────────────────────────────┘
```

---

## 🔷 ДИАЛОГ НАСТРОЙКИ ЛИСТА (`SheetSetupDialog`)

```
┌──────────────────────────────────────────────────────┐
│  Настройка листа                                 [✕] │
├──────────────────────────────────────────────────────┤
│  Тип листа:  ● Прямоугольник  ○ Произвольная форма   │
│                                                      │
│  — Прямоугольник —                                   │
│  Ширина:  [__3000__] мм    Высота: [__1500__] мм     │
│                                                      │
│  — Произвольная форма —                              │
│  DXF файл листа: [________________________] [Обзор] │
│  Превью: [QGraphicsView]                             │
│                                                      │
│  Количество листов: [___∞___]  (0 = бесконечно)     │
│  Материал: [__Сталь__________]                       │
│  Толщина:  [___3.0___] мм                            │
│                                                      │
│                    [Отмена]  [Сохранить]             │
└──────────────────────────────────────────────────────┘
```

---

## 🔷 ПРОГРЕСС НЕСТИНГА (`ProgressOverlay`)

Полупрозрачный оверлей поверх `NestCanvas` во время расчёта:

```
┌────────────────────────────────────────┐
│                                        │
│    ⚙  Расчёт раскладки...             │
│                                        │
│    ████████████████░░░░░░  68%        │
│                                        │
│    Размещено: 3 400 / 5 000           │
│    Текущий лист: 2                     │
│    Elapsed: 00:01:23                   │
│                                        │
│              [⏹ Отмена]               │
└────────────────────────────────────────┘
```

Обновляется через `QFutureWatcher::progressValueChanged` — не блокирует UI поток.

---

## 🔷 ОБРАБОТКА ОШИБОК — СВОДНЫЕ ПРАВИЛА

Каждый метод в `core/` должен следовать одному из двух паттернов:

**Паттерн A — возврат Result типа:**
```cpp
struct ImportResult {
    bool                        success;
    std::vector<PartGeometry>   parts;
    std::vector<LogEntry>       warnings;   // не фатальные проблемы
    QString                     errorMessage; // если !success
};
ImportResult DxfImporter::importFile(const QString& path, const ImportOptions& opts);
```

**Паттерн B — исключения только для фатальных ошибок:**
```cpp
// Бросать только std::runtime_error для ситуаций типа "файл не существует"
// Для геометрических проблем — использовать Паттерн A
```

**Запрещено:**
- Silent fail (вернуть пустой результат без объяснения)
- `qDebug()` без дублирования в `AppLogger`
- Catch-all `catch(...)` без перелогирования

---

## 🔷 CMAKE — ЗАВИСИМОСТИ

```cmake
cmake_minimum_required(VERSION 3.25)
project(NestingApp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent)

include(FetchContent)

# Clipper2
FetchContent_Declare(clipper2
  GIT_REPOSITORY https://github.com/AngusJohnson/Clipper2.git
  GIT_TAG        main)
FetchContent_MakeAvailable(clipper2)

# libdxfrw
FetchContent_Declare(libdxfrw
  GIT_REPOSITORY https://github.com/codelibs/libdxfrw.git
  GIT_TAG        master)
FetchContent_MakeAvailable(libdxfrw)

# libnest2d (опционально, если интеграция возможна)
# Альтернатива: реализовать собственный NFP движок поверх Clipper2

target_link_libraries(NestingApp PRIVATE
  Qt6::Widgets Qt6::Concurrent
  Clipper2 dxfrw)
```

---

## 🔷 ПОРЯДОК РЕАЛИЗАЦИИ (для Claude Code)

Реализовывай строго в этом порядке — каждый шаг должен компилироваться и быть проверяемым:

```
Шаг 1:  CMakeLists.txt + зависимости + пустые заглушки всех классов
Шаг 2:  AppStyle.cpp — полный stylesheet по AppStyle.h
Шаг 3:  core/logging/ — AppLogger, LogEntry, LogChannel (тесты)
Шаг 4:  core/geometry/DxfImporter — парсинг LINE/ARC/LWPOLYLINE (тесты)
Шаг 5:  core/geometry/DxfImporter — SPLINE/ELLIPSE/INSERT/сборка контуров (тесты)
Шаг 6:  core/geometry/GeomUtils — offset через Clipper2 (тесты)
Шаг 7:  NestJob / NestResult / PlacementStrategy — структуры данных
Шаг 8:  core/nesting/NFPCalculator — через Clipper2 (тесты)
Шаг 9:  core/nesting/NestEngine — Bottom-Left + NFP (тесты с простыми прямоугольниками)
Шаг 10: core/nesting/NestEngine — многопоточность + отмена
Шаг 11: core/nesting/NestEngine — все PlacementStrategy (тесты)
Шаг 12: core/nesting/CoedgeDetector (тесты)
Шаг 13: core/export/LxdsExporter (тесты с mock-данными)
Шаг 14: core/export/DxfExporter
Шаг 15: ui/canvas/NestCanvas + PartGraphicsItem (можно тестировать отдельно)
Шаг 16: ui/MainWindow — скелет с вкладками
Шаг 17: ui/tabs/PartListTab + ImportDxfDialog
Шаг 18: ui/tabs/NestSettingsTab — все виджеты настроек
Шаг 19: ui/tabs/SheetsTab — карточки листов
Шаг 20: ui/tabs/LogTab — таблица + фильтры + автоскролл
Шаг 21: ui/widgets/ProgressOverlay + подключение к QFutureWatcher
Шаг 22: Интеграция: MainWindow ↔ NestEngine ↔ Canvas ↔ LogTab
Шаг 23: SheetSetupDialog + поддержка DXF-листов
Шаг 24: ExportDialog + полный экспорт .lxds
Шаг 25: Финальное тестирование: 500 типов деталей × 10 000 шт.
```

---

## 🔷 ТЕСТЫ — МИНИМАЛЬНЫЙ НАБОР

```cpp
// test_dxf_importer.cpp
TEST_CASE("Парсинг LWPOLYLINE замкнутого контура")
TEST_CASE("Парсинг ARC и сборка в полигон")
TEST_CASE("Автозакрытие незамкнутого контура (gap < tolerance)")
TEST_CASE("Определение отверстий внутри детали")
TEST_CASE("Единицы: дюймы конвертируются в мм")
TEST_CASE("Блок INSERT с трансформацией")

// test_nfp_calculator.cpp
TEST_CASE("NFP двух прямоугольников")
TEST_CASE("NFP L-образных деталей")
TEST_CASE("Кэширование NFP по хешу")

// test_nest_engine.cpp
TEST_CASE("Раскладка 10 одинаковых квадратов — ожидаемая утилизация > 90%")
TEST_CASE("Стратегия BOTTOM_LEFT vs TOP_RIGHT — разная компоновка")
TEST_CASE("Деталь не влезает — в unplacedPartIds")
TEST_CASE("Coedge двух прямоугольников по длинной стороне")
TEST_CASE("Отмена расчёта через cancelRequested")

// test_lxds_exporter.cpp
TEST_CASE("Экспортированный .lxds — валидный ZIP")
TEST_CASE("layout.xml содержит все размещённые детали с координатами")
TEST_CASE("Координаты в layout.xml совпадают с NestResult")
```

---

## 🔷 КРИТИЧНЫЕ МЕСТА — ОСОБОЕ ВНИМАНИЕ

1. **DXF SPLINE → полигон**: использовать De Boor аппроксимацию с adaptive tolerance (меньше точек для прямых участков, больше для кривых). Ошибка аппроксимации не должна превышать 0.01мм.

2. **NFP кэш**: ключ `{hashA, hashB, rotationDeg}`. Hash детали = hash от вершин с округлением до 0.001мм. Без кэша при 500 типах деталей и 360 углах = 500×500×360 = 90 млн операций.

3. **Зазор через offset**: применять ПЕРЕД нестингом к копиям контуров, экспортировать ОРИГИНАЛЬНЫЕ контуры. Никогда не модифицировать `PartGeometry` постоянно.

4. **lxds координаты**: CypCut ожидает Y-ось направленную ВВЕРХ (математическая). Qt использует Y вниз. Инвертировать Y при экспорте: `exportY = sheetHeight - placedY`.

5. **Многопоточность**: весь `NestEngine::run()` выполняется в рабочем потоке. Сигналы прогресса через `Qt::QueuedConnection`. Никогда не трогать UI из `NestEngine`.

6. **500 типов × 10 000 штук = 5 000 000 объектов PlacedPart**: использовать `std::vector` с `reserve()`, не `QList`. Для `NestCanvas` при таком количестве использовать GPU-ускорение через `QOpenGLWidget` если zoom позволяет.

7. **Произвольный лист (DXF)**: контур листа также offset-уменьшается на `sheetMarginMm`. NFP для произвольного листа = инвертированный NFP (inner fit polygon).

---

## ✅ ОПРЕДЕЛЕНИЕ "ГОТОВО"

Проект считается готовым когда:
- [ ] Импортируется DXF с 500 различными деталями без ошибок
- [ ] Нестинг 5 000 000 деталей завершается за < 5 минут
- [ ] Все 8 стратегий направления дают визуально разный результат
- [ ] Coedge работает и виден в логе
- [ ] Экспорт .lxds открывается в CypCut без ошибок
- [ ] Вкладка Лог показывает КАЖДУЮ операцию с деталями
- [ ] Ни одна операция не падает молча — всё в логе
- [ ] UI не зависает во время расчёта (ProgressOverlay работает)
- [ ] Стиль строго соответствует AppStyle.h
