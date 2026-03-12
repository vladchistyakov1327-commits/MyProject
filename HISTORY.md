# История проекта NestingPro

## Обзор

**NestingPro** — десктопное приложение на Qt 6 / C++17 для автоматической раскладки деталей из листового металла. Реализовано с нуля по архитектурному документу `NestingPro.md`.

---

## Хронология разработки

### Этап 1 — Фундамент проекта

Созданы базовые файлы сборки и точка входа:

- `CMakeLists.txt` — корневой build-файл (CMake 3.25+). Подключает зависимости через `FetchContent`: Clipper2, libdxfrw (форк LibreCAD), Catch2 v3.4.0. Определяет два таргета: `NestingApp` и `NestingTests`.
- `main.cpp` — точка входа, инициализирует `QApplication`, применяет стиль, запускает `MainWindow`.
- `AppStyle.h` / `AppStyle.cpp` — палитра Industrial Dark Theme (30+ цветовых констант в `namespace AppPalette`) и полный QSS-stylesheet с токен-подстановкой.

---

### Этап 2 — Модуль логирования (`core/logging/`)

- `LogChannel.h` — перечисление каналов: `IMPORT`, `GEOMETRY`, `NESTING`, `COEDGE`, `EXPORT`, `UI`, `SYSTEM`.
- `LogEntry.h` — структура записи лога: timestamp, уровень, канал, сообщение, детали, partId, sheetIndex, elapsedMs. Методы `levelString()` и `channelString()`.
- `AppLogger.h` / `AppLogger.cpp` — потокобезопасный singleton-логгер на `QMutex`. Сигнал `entryAdded(LogEntry)` для подключения UI. Удобные макросы: `LOG_INFO`, `LOG_WARN`, `LOG_ERR`, `LOG_CRIT`, `LOG_DEBUG`.

---

### Этап 3 — Модуль геометрии (`core/geometry/`)

- `PolyContour.h` / `PolyContour.cpp` — базовые структуры: `PolyContour` (вершины, признак дырки), `PartGeometry` (контур, дырки, bbox, площадь). Методы: `signedArea()`, `fixOrientation()`, `isValid()`.
- `GeomUtils.h` / `GeomUtils.cpp` — утилиты на базе Clipper2: смещение контура (`InflatePaths`), упрощение (Douglas–Peucker), детекция самопересечений, аппроксимация дуг/эллипсов/сплайнов (алгоритм де Бура), трансформации, зеркало.
- `DxfImporter.h` / `DxfImporter.cpp` — полный парсер DXF через libdxfrw. Класс `DxfListener : DRW_Interface` обрабатывает: LINE, ARC, CIRCLE, LWPOLYLINE (с bulge-конвертацией), POLYLINE, SPLINE, ELLIPSE, INSERT. Автодетектирование единиц (`$INSUNITS`), chain-building контуров с tolerance, автозакрытие, hole detection.
- `CoedgeDetector.h` / `CoedgeDetector.cpp` — детектор общих кромок. Алгоритм: извлечение рёбер → проверка обратной ориентации + расстояния < tolerance → вычисление смещения.

---

### Этап 4 — Модуль нестинга (`core/nesting/`)

- `PlacementStrategy.h` — 10 стратегий размещения (`BOTTOM_LEFT`, `GRAVITY_CENTER` и др.), `RotationMode` (NONE / STEP_90 / STEP_45 / CUSTOM), `NestAlgorithm`.
- `NestJob.h` — входные структуры: `PartEntry` (геометрия, количество, вращение, flip), `SheetDefinition` (RECTANGLE или CUSTOM_DXF), `NestParameters` (зазоры, стратегия, coedge, алгоритм).
- `NestResult.h` — выходные структуры: `PlacedPart` (позиция, угол, flip), `SheetResult` (отчёт по листу, утилизация), `NestResult` (все листы, неразмещённые детали).
- `RotationSampler.h` / `RotationSampler.cpp` — генерация углов по режиму. Flip кодируется как angle ≥ 360°.
- `NFPCalculator.h` / `NFPCalculator.cpp` — No-Fit Polygon через `Clipper2::MinkowskiSum`. IFP-приближение через `InflatePaths`. Кэш NFP по хешу вершин (round до 0.001 мм). Функция `findBestPosition` реализует голосование по стратегии.
- `SheetManager.h` / `SheetManager.cpp` — управление листами, проверка лимитов, крайняя геометрия листа с учётом margin.
- `NestEngine.h` / `NestEngine.cpp` — главный оркестратор. Алгоритм: сортировка деталей по убыванию площади → перебор углов → IFP → вычитание NFP union → `findBestPosition` → coedge. Асинхронный запуск через `QtConcurrent::run` + `QFuture<NestResult>`. Атомарный флаг отмены.

---

### Этап 5 — Модуль экспорта (`core/export/`)

- `LxdsExporter.h` / `LxdsExporter.cpp` — экспорт в формат `.lxds` (ZIP-архив с `layout.xml` + DXF-файлами деталей). Кастомный ZIP-писатель через zlib `compress2`. Y-инверсия: `exportY = sheetH − posY`. Rotation CW-конвертация: `exportRot = (360 − rotDeg) % 360`. XML через `QXmlStreamWriter`.
- `DxfExporter.h` / `DxfExporter.cpp` — экспорт раскладки в DXF (AC1015, LWPOLYLINE). Методы `exportGeometryToBytes()` и `exportLayoutToFile()`.
- `ReportGenerator.h` / `ReportGenerator.cpp` — генерация CSV и текстового отчёта о результатах раскладки.

---

### Этап 6 — Холст (`ui/canvas/`)

- `PartGraphicsItem.h` / `PartGraphicsItem.cpp` — `QGraphicsItem` для одной детали. LOD: при zoom < 0.3 — только bbox; при zoom > 2.0 — показывает ID. Состояния: selected, highlighted.
- `SheetGraphicsItem.h` / `SheetGraphicsItem.cpp` — фон листа + рамка + штриховой margin.
- `NestCanvas.h` / `NestCanvas.cpp` — `QGraphicsView` с управлением: колесо мыши — zoom, ПКМ drag — pan, ЛКМ — выбор детали. Добавлены методы `setResult()` и `clearLayout()` для интеграции с `MainWindow`. Сигналы: `partSelected`, `zoomChanged`.

---

### Этап 7 — Виджеты (`ui/widgets/`)

- `UtilizationBar.h` / `UtilizationBar.cpp` — кастомный прогресс-бар утилизации. Цвет зависит от значения: зелёный → синий → янтарный.
- `LogWidget.h` / `LogWidget.cpp` — таблица логов на `QAbstractTableModel` + `QSortFilterProxyModel`. Фильтры: канал, уровень, поиск по тексту. Автоскролл. Двойной клик — диалог с деталями записи. Сохранение в файл (TSV).
- `ProgressOverlay.h` / `ProgressOverlay.cpp` — полупрозрачный оверлей поверх `NestCanvas` во время расчёта. Прогресс-бар + статус + кнопка «Отмена».

---

### Этап 8 — Вкладки (`ui/tabs/`)

- `PartListTab.h` / `PartListTab.cpp` — таблица деталей на `QStandardItemModel`. Импорт DXF через диалог, редактирование количества и флага переворота inline.
- `NestSettingsTab.h` / `NestSettingsTab.cpp` — все параметры алгоритма в сгруппированных `QGroupBox`: зазоры, стратегия, вращение, coedge, алгоритм + ограничения.
- `SheetsTab.h` / `SheetsTab.cpp` — выбор типа листа (прямоугольник / DXF-контур), параметры листа. После расчёта — карточки с `UtilizationBar` для каждого листа.
- `LogTab.h` / `LogTab.cpp` — обёртка над `LogWidget` со строкой статуса (суммарное количество предупреждений и ошибок).

---

### Этап 9 — Диалоги (`ui/dialogs/`)

- `ImportDxfDialog.h` / `ImportDxfDialog.cpp` — мультифайловый импорт DXF. Список файлов с дедупликацией, настройки количества и режима вращения для каждого файла, предпросмотр геометрии (площадь, bbox, количество контуров).
- `SheetSetupDialog.h` / `SheetSetupDialog.cpp` — создание/редактирование `SheetDefinition`. Переключение между прямоугольным листом и контуром из DXF.
- `ExportDialog.h` / `ExportDialog.cpp` — выбор формата (.lxds / .dxf / .csv / .txt), путь сохранения, автоподстановка расширения при смене формата.

---

### Этап 10 — Главное окно (`ui/`)

- `TopBar.h` / `TopBar.cpp` — верхняя панель высотой 48 px. Кнопки: «↓ Импорт DXF», «⬛ Лист», «▶ Рассчитать» (зелёная), «✖ Отмена» (красная, видна только во время расчёта), «⬆ Экспорт». Метод `setRunning(bool)`.
- `MainWindow.h` / `MainWindow.cpp` — главное окно с двойным splitter-ом:
  - горизонтальный: боковая панель (340 px) + холст;
  - вертикальный: основная область + нижняя вкладка лога.
  
  Интеграция: `NestEngine` ↔ `QFutureWatcher` ↔ `ProgressOverlay` ↔ `NestCanvas` ↔ вкладки. Подтверждение отмены при закрытии во время расчёта.

---

### Этап 11 — Тесты (`tests/`)

Написаны с использованием Catch2 v3.4.0:

- `test_dxf_importer.cpp` — 4 теста: пустой DXF, прямоугольник LWPOLYLINE, несуществующий файл, прямоугольник из отдельных LINE.
- `test_nfp_calculator.cpp` — 3 теста: NFP двух квадратов (площадь), IFP листа с деталью, корректность кэша.
- `test_nest_engine.cpp` — 4 теста: 2 детали на 1 листе, деталь больше листа, многолистный расчёт, отмена асинхронного расчёта.
- `test_lxds_exporter.cpp` — 4 теста: создание непустого .lxds, структура DXF-файла, содержимое CSV, содержимое TXT-отчёта.

---

## Технический стек

| Компонент | Версия / Источник |
|-----------|-------------------|
| Qt | 6.5+ (Widgets, Concurrent) |
| C++ | C++17 |
| CMake | 3.25+ |
| Clipper2 | `main` branch (AngusJohnson/Clipper2) |
| libdxfrw | `master` branch (LibreCAD/libdxfrw) |
| Catch2 | v3.4.0 |
| zlib | системная (через Qt) |

---

## Архитектурные решения

### NFP-кэш

`QHash<NFPKey, PathsD>` с хешем по скруглённым (до 0.001 мм) координатам вершин и паре углов. Позволяет избежать повторного вычисления MinkowskiSum для одинаковых пар деталей.

### IFP-приближение

Точный Inner Fit Polygon — сложная операция. Использовано приближение через `Clipper2::InflatePaths` с радиусом описанной окружности детали, что достаточно для практических задач.

### ZIP без внешних зависимостей

Формат `.lxds` — ZIP-архив. Реализован минимальный ZIP-писатель (~100 строк) на основе zlib `compress2`, уже присутствующего в Qt.

### Y-инверсия для CypCut/HypCut

Системы CypCut/HypCut используют Y-ось направленную вниз, Qt — вверх. При экспорте применяется:
```
exportY = sheetHeight - placedY
exportRot = (360 - rotDeg) % 360   // CCW → CW
```

### Многопоточность

`NestEngine::runAsync()` запускает `QtConcurrent::run`. Прогресс передаётся через сигналы Qt (с `Qt::QueuedConnection`). Отмена — через `std::atomic<bool>`.

---

## Структура файлов

```
NestingPro/
├── CMakeLists.txt
├── main.cpp
├── AppStyle.h / .cpp
├── core/
│   ├── logging/
│   │   ├── LogChannel.h
│   │   ├── LogEntry.h
│   │   └── AppLogger.h / .cpp
│   ├── geometry/
│   │   ├── PolyContour.h / .cpp
│   │   ├── GeomUtils.h / .cpp
│   │   ├── DxfImporter.h / .cpp
│   │   └── CoedgeDetector.h / .cpp
│   ├── nesting/
│   │   ├── PlacementStrategy.h
│   │   ├── NestJob.h
│   │   ├── NestResult.h
│   │   ├── RotationSampler.h / .cpp
│   │   ├── NFPCalculator.h / .cpp
│   │   ├── SheetManager.h / .cpp
│   │   └── NestEngine.h / .cpp
│   └── export/
│       ├── LxdsExporter.h / .cpp
│       ├── DxfExporter.h / .cpp
│       └── ReportGenerator.h / .cpp
├── ui/
│   ├── TopBar.h / .cpp
│   ├── MainWindow.h / .cpp
│   ├── canvas/
│   │   ├── NestCanvas.h / .cpp
│   │   ├── PartGraphicsItem.h / .cpp
│   │   └── SheetGraphicsItem.h / .cpp
│   ├── widgets/
│   │   ├── UtilizationBar.h / .cpp
│   │   ├── LogWidget.h / .cpp
│   │   └── ProgressOverlay.h / .cpp
│   ├── tabs/
│   │   ├── PartListTab.h / .cpp
│   │   ├── NestSettingsTab.h / .cpp
│   │   ├── SheetsTab.h / .cpp
│   │   └── LogTab.h / .cpp
│   └── dialogs/
│       ├── ImportDxfDialog.h / .cpp
│       ├── SheetSetupDialog.h / .cpp
│       └── ExportDialog.h / .cpp
└── tests/
    ├── test_dxf_importer.cpp
    ├── test_nfp_calculator.cpp
    ├── test_nest_engine.cpp
    └── test_lxds_exporter.cpp
```

---

## Сборка проекта

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# Запуск приложения
./build/NestingApp

# Запуск тестов
cd build && ctest --output-on-failure
```
