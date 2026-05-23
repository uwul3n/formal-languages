#set document(title: "Спутники и группировки", author: "")
#set page(paper: "a4", margin: 2.2cm, numbering: "1")
#set text(lang: "ru", font: "New Computer Modern", size: 11pt)
#set par(justify: true, leading: 0.62em)
#show heading.where(level: 1): it => block(above: 1.4em, below: 0.6em, text(size: 16pt, weight: "bold", it.body))
#show raw.where(block: true): it => block(
  width: 100%,
  inset: 8pt,
  radius: 3pt,
  fill: rgb("#f5f5f5"),
  stroke: rgb("#ddd") + 0.5pt,
  it,
)
#show raw.where(block: false): it => box(inset: (x: 2pt), outset: (y: 1pt), fill: rgb("#f0f0f0"), radius: 2pt, it)
#show link: it => underline(text(fill: rgb("#1a5fb4"), it))

#align(center)[
  #text(size: 20pt, weight: "bold")[Спутники и группировки] \
  #v(0.3em) #text(size: 12pt)[Дубровин Никита, 23.Б07-мм]
]
#v(1em)

= Постановка задачи

Дан связный граф космических аппаратов и связей с длительностями взаимодействия
$tau$; заданы $k$ главных КА. КА распределяются по группировкам: пока остаются
нераспределённые КА, среди всех рёбер, соединяющих распределённый КА с
нераспределённым, выбирается ребро минимальной длительности, и его свободный конец
присоединяется к группировке распределённого конца.

= Решение

Многоисточниковый #link("https://ru.wikipedia.org/wiki/Алгоритм_Прима")[алгоритм Прима] с глобальным критерием выбора. Используется одна
общая min-куча с записями $("длительность", "КА", "группировка")$; на каждом шаге
извлекается глобально минимальное фронтирное ребро. Уже распределённые КА
отбрасываются (ленивое удаление).

= Псевдокод

```
function Distribute(V, E, k, mains):
    groupOf[1..n] <- -1
    PQ <- пустая min-куча по длительности
    for i = 1..k:
        groupOf[mains[i]] <- i
        for (mains[i], v, τ) in E:
            if groupOf[v] = -1: PQ.push((τ, v, i))

    while есть свободные КА:
        (τ, v, g) <- PQ.pop()
        if groupOf[v] ≠ -1: continue
        groupOf[v] <- g
        for (v, u, τ') in E:
            if groupOf[u] = -1: PQ.push((τ', u, g))

    return groups
```

= Сложность

$O(m log m)$ времени, $O(m)$ памяти.

= Пример

```
8 11
1 2 60
1 3 140
2 3 90
2 4 200
3 4 70
3 5 180
4 5 110
4 6 240
5 7 150
6 7 80
6 8 130
2
1 8
```

Главные КА: $c_1 = "КА1"$, $c_2 = "КА8"$. Распределение:
группировка 1 ${"КА1", "КА2", "КА3", "КА4", "КА5"}$,
группировка 2 ${"КА8", "КА6", "КА7"}$.

= Графический интерфейс

Интерфейс построен на Qt6.

#table(
  columns: (auto, 1fr),
  inset: 6pt,
  align: (left + top, left + top),
  stroke: 0.5pt + rgb("#999"),
  [*Элемент*], [*Класс Qt*],
  [Главное окно], [`QMainWindow` с `QStatusBar` для сообщений и `QLabel` со сводкой по загруженному файлу.],
  [Кнопка «Открыть файл»], [`QPushButton`; открывает `QFileDialog::getOpenFileName`. Чтение — `QFile` + `QTextStream`.],
  [Разделитель «граф / правая панель»], [`QSplitter` (горизонтальный).],
  [Переключение «Граф / Матрица»], [`QTabWidget` с двумя вкладками.],
  [Визуализация графа],
  [`QGraphicsView` поверх `QGraphicsScene`. Вершины — `QGraphicsEllipseItem` (через `scene->addEllipse`), рёбра — `QGraphicsLineItem` (`addLine`), подписи $tau$ и номеров КА — `QGraphicsTextItem` (`addText`). Цвет рёбер и заливки вершин кодирует группировку (`QColor`, `QBrush`, `QPen`).],

  [Матрица смежности], [`QTableWidget`; ячейки — `QTableWidgetItem`, заголовки задаются `QStringList`.],
  [Сообщения об ошибках], [`QMessageBox::warning`.],
)

КА размещаются равномерно по окружности (`QPointF`, `cos`/`sin` по углу
$2 pi (i - 1) / n$); главные КА выделяются увеличенным радиусом и более жирной обводкой.

= Скриншот

#figure(image("screenshot.png", width: 100%), caption: [Главное окно после загрузки `example2.txt`])
