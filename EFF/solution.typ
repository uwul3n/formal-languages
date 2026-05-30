#set document(title: "Функция EFF_k", author: "")
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
  #text(size: 20pt, weight: "bold")[Функция $"EFF"_k^G$] \
  #v(0.3em) #text(size: 12pt)[Дубровин Никита, 23.Б07-мм] \
  #v(0.2em) #text(size: 10pt)[#link("https://github.com/uwul3n/formal-languages")[github.com/uwul3n/formal-languages]]
]
#v(1em)

= Постановка задачи

Для контекстно-свободной грамматики $G = (V_N, V_T, P, S)$ и параметра $k >= 1$
требуется вычислить функцию $"EFF"_k^G(alpha)$, где $alpha in (V_N union V_T)^*$:

$ "EFF"_k^G(alpha) = cases(
  "FIRST"_k^G(alpha)\, & "если" alpha "начинается на терминал",
  {w in V_T^(*k) | exists alpha arrow.r.double.long^*_("rm") beta arrow.r.double.long_("rm") w x\, beta != A w x}\, & "иначе"
) $

то есть $"EFF"_k^G(alpha)$ исключает терминальные цепочки, у которых последний шаг
правостороннего вывода имеет вид $A -> epsilon$ в позиции $beta = A w x$ (ε-правило
на ведущем нетерминале после полного раскрытия хвоста).

= Решение

Развернём определение в рекурсивную процедуру. Если $alpha = a beta$
(терминал в начале), $"EFF"_k = "FIRST"_k$ непосредственно. Если $alpha = A beta$,
то в правостороннем выводе хвост $beta$ полностью раскрывается до терминалов
раньше, чем затрагивается $A$; значит, последний шаг — это либо $A -> epsilon$
(запрещено определением), либо потомки $A$ в подвыводе из $A -> gamma$, $gamma != epsilon$.
В первом случае результат отбрасываем, во втором продолжаем тот же анализ
для строки $gamma beta$:

$ "EFF"_k^G(A beta) = union.big_(A -> gamma, gamma != epsilon) "EFF"_k^G(gamma beta). $

Рекурсия может зацикливаться (например, $A -> B$, $B -> A$); решается обходом
в ширину по строкам с множеством `visited`.

$"FIRST"_k^G$ для нетерминалов считается заранее как наименьшая неподвижная точка:
для каждого правила $A -> gamma$ добавляем $"FIRST"_k^G(gamma)$ в $"FIRST"_k^G(A)$
до стабилизации. Конкатенация с $k$-усечением — стандартная операция $plus.circle_k$.

= Псевдокод

```
function FirstK(α, k):
    R <- {ε}
    for X in α:
        F <- {X} если X терминал, иначе FIRST_k[X]
        R <- R ⊕_k F
        если все строки в R длины k: break
    return R

function EffK(α, k):
    если α пуста: return ∅
    если α[0] терминал: return FirstK(α, k)
    queue <- [α]; visited <- {α}; result <- ∅
    while queue непуста:
        form <- pop queue
        если form[0] терминал:
            result <- result ∪ FirstK(form, k); continue
        A <- form[0]; tail <- form[1:]
        for каждое правило A -> γ, γ ≠ ε:
            next <- γ ++ tail
            если next ∉ visited: visited.add(next); queue.push(next)
    return result
```

= Сложность

Пусть $|G|$ — суммарная длина правил, $N = |V_N|$, $|V_T| = t$.
Подсчёт $"FIRST"_k^G$ — стандартный, $O(|G| dot |V_T|^k dot N)$ в худшем случае.
$"EFF"_k^G(alpha)$ — BFS по строкам; для грамматик без бесполезных циклов
число посещаемых строк ограничено и обход терминируется за полиномиальное
время от $|G|$ и $k$.

= Формат входа

```
# терминалы
a b c
# нетерминалы
S A B
# стартовый символ
S
# количество правил
5
# правила: LHS -> X1 X2 ...; ε записывается как eps
S -> A B
A -> a
A -> eps
B -> b
B -> c
# k
1
# запросы (по одному α на строку)
S
A B
A
B
a B
```

= Пример

Для приведённой грамматики и $k = 1$:

```
EFF_1(S) = {a}
EFF_1(A B) = {a}
EFF_1(A) = {a}
EFF_1(B) = {b, c}
EFF_1(a B) = {a}
```

Из $"EFF"_1(S)$ исключены `b` и `c`: они достижимы только выводами
$S => A B =>_("rm") A b =>_("rm") b$ и $S => A B =>_("rm") A c =>_("rm") c$,
где последний шаг — $A -> epsilon$ при $beta = A w x$.

Для грамматики арифметических выражений (`example2.txt`,
$E -> T E_1$, $E_1 -> + T E_1 | epsilon$, $T -> F T_1$, $T_1 -> * F T_1 | epsilon$,
$F -> ( E ) | "id"$) и $k = 1$:

```
EFF_1(E)    = {(, id}
EFF_1(E1)   = {+}
EFF_1(T1)   = {*}
EFF_1(F)    = {(, id}
EFF_1(T E1) = {(, id}
```

В $"EFF"_1(E_1)$ нет $epsilon$ (хотя $epsilon in "FIRST"_1(E_1)$): единственный
способ получить пустой префикс — применить $E_1 -> epsilon$, что и есть
запрещённый последний шаг.

= Сборка и запуск

```
cd build && cmake .. && cmake --build .
./eff_app ../example.txt
./eff_app ../example2.txt
```
