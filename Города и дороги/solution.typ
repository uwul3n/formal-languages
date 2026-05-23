#set document(title: "Города и дороги", author: "")
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
  #text(size: 20pt, weight: "bold")[Города и дороги] \
  #v(0.3em) #text(size: 12pt)[Дубровин Никита, 23.Б07-мм]
]
#v(1em)

= Постановка задачи

Дан связный взвешенный граф городов и дорог; заданы $k$ городов-столиц. Города
распределяются по государствам: по очереди для каждого государства $i = 1, ..., k$
к нему присоединяется ещё не распределённый город, соединённый прямым ребром
минимальной длины с одним из городов государства.

= Решение

Многоисточниковая модификация #link("https://ru.wikipedia.org/wiki/Алгоритм_Прима")[алгоритма Прима] в режиме round-robin: каждое государство
ведёт свою min-кучу пар $("длина", "город")$ и на своём ходе извлекает минимум.
Уже занятые города отбрасываются (ленивое удаление). Критерий — длина прямого ребра,
не кратчайшего пути.

= Псевдокод

```
function Distribute(V, E, k, capitals):
    stateOf[1..n] <- -1
    PQ[1..k] <- пустые min-кучи
    for i = 1..k:
        stateOf[capitals[i]] <- i
        for (capitals[i], v, ℓ) in E:
            if stateOf[v] = -1: PQ[i].push((ℓ, v))

    while есть свободные города:
        for i = 1..k:
            while PQ[i] не пуста:
                (ℓ, v) <- PQ[i].pop()
                if stateOf[v] = -1:
                    stateOf[v] <- i
                    for (v, u, w) in E:
                        if stateOf[u] = -1: PQ[i].push((w, u))
                    break

    return groups
```

= Сложность

$O(k m log m)$ времени, $O(k m)$ памяти.

= Пример

```
9 12
1 2 4
1 3 7
2 3 2
2 4 6
3 5 3
4 5 5
4 6 8
5 6 4
5 7 9
6 8 2
7 8 6
7 9 5
3
1 6 9
```

Столицы $c_1 = 1$, $c_2 = 6$, $c_3 = 9$. Распределение:
государство 1 ${1, 2, 3, 4}$, государство 2 ${6, 8, 5}$, государство 3 ${9, 7}$.

= Скриншот

#figure(image("screenshot.png", width: 100%), caption: [Главное окно после загрузки `example2.txt`])
