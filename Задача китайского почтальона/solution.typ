#set document(title: "Задача китайского почтальона", author: "")
#set page(paper: "a4", margin: 2.2cm, numbering: "1")
#set text(lang: "ru", font: "New Computer Modern", size: 11pt)
#set par(justify: true, leading: 0.62em)
#show heading.where(level: 1): it => block(above: 1.4em, below: 0.6em, text(size: 16pt, weight: "bold", it.body))
#show raw.where(block: true): it => block(
  width: 100%, inset: 8pt, radius: 3pt,
  fill: rgb("#f5f5f5"), stroke: rgb("#ddd") + 0.5pt, it,
)
#show raw.where(block: false): it => box(inset: (x: 2pt), outset: (y: 1pt), fill: rgb("#f0f0f0"), radius: 2pt, it)
#show link: it => underline(text(fill: rgb("#1a5fb4"), it))

#align(center)[
  #text(size: 20pt, weight: "bold")[Задача китайского почтальона] \
  #v(0.3em) #text(size: 12pt)[Дубровин Никита, 23.Б07-мм]
]
#v(1em)

= Постановка задачи

Дан неориентированный связный взвешенный граф $G = (V, E)$. Требуется построить
замкнутый маршрут минимальной суммарной длины, проходящий по каждому ребру не менее
одного раза.

= Решение

По #link("https://ru.wikipedia.org/wiki/Эйлеров_цикл")[теореме Эйлера] эйлеров цикл
существует тогда и только тогда, когда все вершины имеют чётную степень.
Обозначим $O = {v : deg(v) "нечётна"}$, $|O| = 2t$.

Дублирование пути между двумя вершинами изменяет чётность только у его концов.
Минимизация суммарного веса дублей сводится к минимальному совершенному паросочетанию
на $O$ с весами $"dist"(u, v)$ — кратчайшими путями в $G$:

$ L^* = sum_(e in E) w(e) + sum_((u,v) in M) "dist"(u, v). $

Этапы: #link("https://ru.wikipedia.org/wiki/Алгоритм_Дейкстры")[алгоритм Дейкстры]
от каждой вершины $O$; ДП по битмаскам для паросочетания ($O(2^(2t) t)$, применимо при
$|O| <= 20$); дублирование рёбер вдоль восстановленных путей; обход полученного
мультиграфа #link("https://en.wikipedia.org/wiki/Eulerian_path#Hierholzer's_algorithm")[алгоритмом Хирхольцера].

= Псевдокод

```
function ChinesePostman(G):
    O <- {v : deg(v) нечётна}
    if O = ∅:
        return Σ w(e), Hierholzer(G, start=1)

    for v in O:
        (dist[v], prev[v]) <- Dijkstra(G, v)

    dp[full_mask] <- 0
    for mask = full_mask down to 0:
        i <- lowest_set_bit(mask)
        for j ∈ mask, j > i:
            new_mask <- mask \ {i, j}
            dp[new_mask] <- min(dp[new_mask], dp[mask] + dist[O[i]][O[j]])

    for (a, b) in восстановленные пары M:
        for ребро (x, y) на пути a -> b:
            добавить копию (x, y, w) в мультиграф

    return Σ w(e) + dp[0], Hierholzer(start=1)
```

= Сложность

$O(t (n + m) log n + 2^(2t) t)$ времени.

= Пример

```
6 9
1 2 4
1 3 2
2 3 1
2 4 5
3 4 8
3 5 10
4 5 2
4 6 6
5 6 3
```

Степени: $deg(1) = 2$, $deg(2) = 3$, $deg(3) = 4$, $deg(4) = 4$, $deg(5) = 3$, $deg(6) = 2$.
Нечётные — ${2, 5}$. Кратчайший путь $2 -> 5$ проходит через вершину 4
($2 -> 4 -> 5$, длина $5 + 2 = 7$). Сумма весов рёбер графа равна $41$;
$L^* = 41 + 7 = 48$.

= Скриншот

#figure(image("screenshot.png", width: 100%), caption: [Главное окно после загрузки `example2.txt`])
