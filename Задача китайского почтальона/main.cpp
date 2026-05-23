#include <QtWidgets>
#include <vector>
#include <queue>
#include <stack>
#include <set>
#include <cmath>
#include <algorithm>

using ll = long long;
const ll INF = 1e18;

struct Edge { int u, v; ll w; bool duplicate = false; };

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Задача китайского почтальона");
        resize(1000, 700);

        auto* central = new QWidget; setCentralWidget(central);
        auto* vlay = new QVBoxLayout(central);
        vlay->setContentsMargins(8, 8, 8, 8);
        vlay->setSpacing(6);

        auto* hlay = new QHBoxLayout;
        hlay->setSpacing(8);
        auto* loadBtn = new QPushButton("Открыть файл...");
        connect(loadBtn, &QPushButton::clicked, this, &MainWindow::loadFile);
        hlay->addWidget(loadBtn);
        infoLbl = new QLabel("Файл не загружен");
        infoLbl->setStyleSheet("padding: 4px 8px;");
        hlay->addWidget(infoLbl);
        hlay->addStretch();
        vlay->addLayout(hlay, 0);

        auto* splitter = new QSplitter(Qt::Horizontal);
        tabs = new QTabWidget;

        auto* graphTab = new QWidget;
        auto* gvlay = new QVBoxLayout(graphTab);
        gvlay->setContentsMargins(4, 4, 4, 4);
        gvlay->setSpacing(4);
        scene = new QGraphicsScene(this);
        view = new QGraphicsView(scene);
        view->setRenderHint(QPainter::Antialiasing);
        gvlay->addWidget(view);
        auto* rl = new QLabel("Маршрут:");
        rl->setStyleSheet("padding: 2px 4px;");
        gvlay->addWidget(rl);
        routeText = new QTextEdit; routeText->setReadOnly(true);
        routeText->setMaximumHeight(70);
        routeText->document()->setDocumentMargin(8);
        gvlay->addWidget(routeText);
        tabs->addTab(graphTab, "Граф");

        table = new QTableWidget;
        tabs->addTab(table, "Матрица смежности");
        splitter->addWidget(tabs);

        auto* rightPanel = new QWidget;
        rightPanel->setStyleSheet(
            "QGroupBox { margin-top: 14px; padding: 10px 4px 4px 4px; }"
            "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }"
            "QListView::item { padding: 4px 6px; }"
        );
        auto* rvlay = new QVBoxLayout(rightPanel);
        rvlay->setContentsMargins(8, 8, 8, 8); rvlay->setSpacing(10);

        auto* statsBox = new QGroupBox("Статистика");
        auto* form = new QFormLayout(statsBox);
        form->setLabelAlignment(Qt::AlignRight);
        form->setContentsMargins(12, 8, 12, 8);
        form->setHorizontalSpacing(14); form->setVerticalSpacing(8);
        vCountLbl    = new QLabel("—"); form->addRow("Вершин:",          vCountLbl);
        eCountLbl    = new QLabel("—"); form->addRow("Рёбер:",           eCountLbl);
        eulerianLbl  = new QLabel("—"); form->addRow("Граф эйлеров:",    eulerianLbl);
        oddCountLbl  = new QLabel("—"); form->addRow("Нечётных вершин:", oddCountLbl);
        dupCountLbl  = new QLabel("—"); form->addRow("Добавлено дублей:", dupCountLbl);
        distanceLbl  = new QLabel("—");
        QFont bf = distanceLbl->font(); bf.setBold(true); bf.setPointSize(bf.pointSize()+2);
        distanceLbl->setFont(bf);
        form->addRow("Длина маршрута:", distanceLbl);
        rvlay->addWidget(statsBox);

        auto* oddBox = new QGroupBox("Нечётные вершины");
        auto* ovl = new QVBoxLayout(oddBox); ovl->setContentsMargins(12, 8, 12, 10);
        oddListLbl = new QLabel("—"); oddListLbl->setWordWrap(true);
        ovl->addWidget(oddListLbl);
        rvlay->addWidget(oddBox);

        auto* matchBox = new QGroupBox("Паросочетание");
        auto* mvl = new QVBoxLayout(matchBox); mvl->setContentsMargins(12, 8, 12, 10);
        matchingList = new QListWidget;
        mvl->addWidget(matchingList);
        rvlay->addWidget(matchBox, 1);

        splitter->addWidget(rightPanel);
        splitter->setStretchFactor(0, 3);
        splitter->setStretchFactor(1, 1);
        vlay->addWidget(splitter, 1);

        statusBar()->showMessage("Готов");
    }

private slots:
    void loadFile() {
        QString path = QFileDialog::getOpenFileName(this, "Открыть", "", "*.txt");
        if (path.isEmpty()) return;
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::warning(this, "Ошибка", "Не могу открыть файл"); return;
        }
        QTextStream in(&file); in >> n >> m;
        edges.clear(); adj.assign(n+1, {});
        for (int i = 0; i < m; i++) {
            int u, v; ll w; in >> u >> v >> w;
            edges.push_back({u, v, w, false});
            adj[u].push_back({v, i}); adj[v].push_back({u, i});
        }
        file.close();
        solve(); buildMatrix(); buildGraph(); buildResults();
        infoLbl->setText(QString("  Вершин: %1, рёбер: %2, длина маршрута: %3")
            .arg(n).arg(m).arg(totalDist));
        statusBar()->showMessage("Загружено: " + path);
    }

private:
    // Дейкстра на min-куче. Возвращает (расстояния, предки).
    std::pair<std::vector<ll>, std::vector<int>> dijkstra(int src) {
        std::vector<ll> d(n+1, INF); std::vector<int> p(n+1, -1);
        using pli = std::pair<ll,int>;
        std::priority_queue<pli,std::vector<pli>,std::greater<pli>> pq;
        d[src] = 0; pq.push({0, src});
        while (!pq.empty()) {
            auto [dd, u] = pq.top(); pq.pop();
            if (dd > d[u]) continue;  // ленивое удаление
            for (auto [v, ei] : adj[u]) {
                ll nd = d[u] + edges[ei].w;
                if (nd < d[v]) { d[v] = nd; p[v] = u; pq.push({nd, v}); }
            }
        }
        return {d, p};
    }

    void solve() {
        // 1. Вершины нечётной степени O — кандидаты на «спаривание».
        std::vector<int> deg(n+1, 0);
        for (auto& e : edges) { deg[e.u]++; deg[e.v]++; }
        oddVertices.clear();
        for (int i = 1; i <= n; i++) if (deg[i] & 1) oddVertices.push_back(i);

        ll baseTotal = 0; for (auto& e : edges) baseTotal += e.w;
        addedCount = 0; circuit.clear(); matchingPairs.clear();

        if (!oddVertices.empty()) {
            // 2. Дейкстра от каждой вершины из O: D[i][j] — кратчайший путь, Prev — для восстановления.
            int sz = oddVertices.size();
            std::vector<std::vector<ll>> D(sz); std::vector<std::vector<int>> Prev(sz);
            for (int i = 0; i < sz; i++) {
                auto [d, p] = dijkstra(oddVertices[i]);
                D[i].resize(sz); for (int j = 0; j < sz; j++) D[i][j] = d[oddVertices[j]];
                Prev[i] = p;
            }

            // 3. Минимальное совершенное паросочетание на O — ДП по битмаскам.
            // dp[mask] = минимальная стоимость спаривания вершин из mask. Идём от полной к пустой.
            int S = 1 << sz;
            std::vector<ll> dp(S, INF); std::vector<int> par(S,-1), parPair(S,-1);
            dp[S-1] = 0;
            for (int mask = S-1; mask > 0; mask--) {
                if (dp[mask] == INF) continue;
                int i = __builtin_ctz(mask);  // младший бит — обязательно с кем-то спариваем
                for (int j = i+1; j < sz; j++) {
                    if (!(mask & (1<<j))) continue;
                    int nm = mask ^ (1<<i) ^ (1<<j);
                    ll nc = dp[mask] + D[i][j];
                    if (nc < dp[nm]) { dp[nm] = nc; par[nm] = mask; parPair[nm] = i*sz+j; }
                }
            }
            totalDist = baseTotal + dp[0];

            // 4. Восстанавливаем пары и дублируем рёбра вдоль их путей.
            int cur = 0;
            while (cur != S-1) {
                int pp = parPair[cur], i = pp/sz, j = pp%sz;
                matchingPairs.push_back({oddVertices[i], oddVertices[j]});
                std::vector<int> path; int v = oddVertices[j];
                while (v != oddVertices[i] && v != -1) { path.push_back(v); v = Prev[i][v]; }
                path.push_back(oddVertices[i]); std::reverse(path.begin(), path.end());
                for (int pi = 0; pi+1 < (int)path.size(); pi++) {
                    int a = path[pi], b = path[pi+1]; ll w = 0;
                    for (auto [nb, ei] : adj[a]) if (nb == b) { w = edges[ei].w; break; }
                    int eid = edges.size();
                    edges.push_back({a, b, w, true});
                    adj[a].push_back({b, eid}); adj[b].push_back({a, eid});
                    addedCount++;
                }
                cur = par[cur];
            }
        } else { totalDist = baseTotal; }

        // 5. Эйлеров цикл (Хирхольцер, итеративная версия на стеке).
        // ptr[u] — следующее непросмотренное ребро, благодаря ему всё O(n+m).
        std::vector<std::vector<std::pair<int,int>>> eadj(n+1);
        for (int i = 0; i < (int)edges.size(); i++) {
            eadj[edges[i].u].push_back({edges[i].v, i});
            eadj[edges[i].v].push_back({edges[i].u, i});
        }
        std::vector<bool> used(edges.size(), false); std::vector<int> ptr(n+1, 0);
        std::stack<int> st; st.push(1);
        while (!st.empty()) {
            int u = st.top(); bool found = false;
            while (ptr[u] < (int)eadj[u].size()) {
                auto [v, ei] = eadj[u][ptr[u]++];
                if (!used[ei]) { used[ei] = true; st.push(v); found = true; break; }
            }
            if (!found) { circuit.push_back(u); st.pop(); }
        }
        std::reverse(circuit.begin(), circuit.end());

        QString route;
        for (int ci = 0; ci < (int)circuit.size(); ci++) {
            if (ci > 0) route += " -> "; route += QString::number(circuit[ci]);
        }
        routeText->setPlainText(route);
    }

    void buildMatrix() {
        table->clear(); table->setRowCount(n); table->setColumnCount(n);
        QStringList labels; for (int i = 1; i <= n; i++) labels << QString::number(i);
        table->setHorizontalHeaderLabels(labels); table->setVerticalHeaderLabels(labels);
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++) {
                auto* item = new QTableWidgetItem(i == j ? "0" : "-");
                item->setTextAlignment(Qt::AlignCenter);
                table->setItem(i, j, item);
            }
        for (int i = 0; i < m; i++) {
            auto& e = edges[i];
            table->item(e.u-1, e.v-1)->setText(QString::number(e.w));
            table->item(e.v-1, e.u-1)->setText(QString::number(e.w));
        }
        table->horizontalHeader()->setDefaultSectionSize(60);
        table->verticalHeader()->setDefaultSectionSize(36);
        QFont mf = table->font(); mf.setPointSize(11); table->setFont(mf);
    }

    void buildGraph() {
        scene->clear(); if (n == 0) return;
        const double cx = 380, cy = 280, R = 220, nodeR = 18;
        std::vector<QPointF> pos(n+1);
        for (int i = 1; i <= n; i++) {
            double a = 2*M_PI*(i-1)/n - M_PI/2;
            pos[i] = {cx + R*cos(a), cy + R*sin(a)};
        }
        // Исходные рёбра — синие
        for (int i = 0; i < m; i++) {
            auto& e = edges[i];
            scene->addLine(QLineF(pos[e.u], pos[e.v]), QPen(Qt::blue, 2));
            QPointF mid = (pos[e.u] + pos[e.v]) / 2;
            auto* t = scene->addText(QString::number(e.w));
            t->setPos(mid.x()-t->boundingRect().width()/2, mid.y()-t->boundingRect().height()/2);
        }
        // Дублирующие рёбра — красные, со смещением для наглядности
        for (int i = m; i < (int)edges.size(); i++) {
            auto& e = edges[i];
            QLineF line(pos[e.u], pos[e.v]);
            QPointF dir = line.p2() - line.p1();
            double len = std::sqrt(dir.x()*dir.x() + dir.y()*dir.y());
            if (len > 0) { QPointF perp(-dir.y()/len*5, dir.x()/len*5); line.translate(perp); }
            scene->addLine(line, QPen(Qt::red, 3));
        }
        // Вершины: жёлтые — нечётные, зелёные — чётные
        std::set<int> oddSet(oddVertices.begin(), oddVertices.end());
        for (int i = 1; i <= n; i++) {
            QColor c = oddSet.count(i) ? QColor(255,200,0) : Qt::green;
            scene->addEllipse(pos[i].x()-nodeR, pos[i].y()-nodeR, 2*nodeR, 2*nodeR,
                QPen(Qt::black, 1.5), QBrush(c));
            auto* t = scene->addText(QString::number(i));
            QFont f = t->font(); f.setBold(true); t->setFont(f);
            t->setPos(pos[i].x()-t->boundingRect().width()/2, pos[i].y()-t->boundingRect().height()/2);
        }
        view->fitInView(scene->sceneRect().adjusted(-20,-20,20,20), Qt::KeepAspectRatio);
    }

    void buildResults() {
        vCountLbl->setText(QString::number(n));
        eCountLbl->setText(QString::number(m));
        eulerianLbl->setText(oddVertices.empty() ? "да" : "нет");
        oddCountLbl->setText(QString::number(oddVertices.size()));
        dupCountLbl->setText(QString::number(addedCount));
        distanceLbl->setText(QString::number(totalDist));

        if (oddVertices.empty()) {
            oddListLbl->setText("(нет — граф уже эйлеров)");
        } else {
            QStringList parts;
            for (int v : oddVertices) parts << QString::number(v);
            oddListLbl->setText(parts.join(", "));
        }

        matchingList->clear();
        if (matchingPairs.empty()) {
            matchingList->addItem("(не требуется)");
        } else {
            for (auto [a, b] : matchingPairs)
                matchingList->addItem(QString("%1  —  %2").arg(a).arg(b));
        }
    }

    int n=0, m=0; ll totalDist=0; int addedCount=0;
    std::vector<Edge> edges;
    std::vector<std::vector<std::pair<int,int>>> adj;
    std::vector<int> circuit, oddVertices;
    std::vector<std::pair<int,int>> matchingPairs;
    QTabWidget* tabs; QGraphicsView* view; QGraphicsScene* scene;
    QTableWidget* table; QTextEdit* routeText; QLabel* infoLbl;
    QLabel* vCountLbl; QLabel* eCountLbl; QLabel* eulerianLbl;
    QLabel* oddCountLbl; QLabel* dupCountLbl; QLabel* distanceLbl;
    QLabel* oddListLbl; QListWidget* matchingList;
};

#include "main.moc"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    MainWindow w; w.show();
    return app.exec();
}
