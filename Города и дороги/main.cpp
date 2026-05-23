#include <QtWidgets>
#include <vector>
#include <queue>
#include <cmath>
#include <algorithm>

static const QColor STATE_COLORS[] = {
    Qt::red, Qt::blue, Qt::darkGreen, Qt::magenta,
    QColor(255,140,0), Qt::cyan, QColor(180,180,0), Qt::darkGray,
    QColor(128,0,128), QColor(165,42,42), QColor(0,128,128), QColor(70,130,180)
};
static const int NUM_COLORS = 12;

struct Edge { int u, v, len; };

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Города и дороги");
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
        scene = new QGraphicsScene(this);
        view = new QGraphicsView(scene);
        view->setRenderHint(QPainter::Antialiasing);
        tabs->addTab(view, "Граф");
        table = new QTableWidget;
        tabs->addTab(table, "Матрица смежности");
        splitter->addWidget(tabs);

        auto* rightPanel = new QWidget;
        rightPanel->setStyleSheet(
            "QLabel#section { padding: 2px 2px 4px 2px; font-weight: bold; }"
            "QTreeView::item { padding: 4px 2px; }"
        );
        auto* rvlay = new QVBoxLayout(rightPanel);
        rvlay->setContentsMargins(8, 8, 8, 8); rvlay->setSpacing(10);
        statsLbl = new QLabel("—");
        statsLbl->setStyleSheet("padding: 8px 10px; background: palette(base); border: 1px solid palette(mid);");
        rvlay->addWidget(statsLbl);
        auto* treeLbl = new QLabel("Распределение по государствам:");
        treeLbl->setObjectName("section");
        rvlay->addWidget(treeLbl);
        resultsTree = new QTreeWidget;
        resultsTree->setHeaderHidden(true);
        resultsTree->setRootIsDecorated(true);
        resultsTree->setIndentation(18);
        resultsTree->setAlternatingRowColors(true);
        rvlay->addWidget(resultsTree, 1);
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
        QTextStream in(&file);
        in >> n >> m; edges.clear(); adj.assign(n+1, {});
        for (int i = 0; i < m; i++) {
            int u, v, len; in >> u >> v >> len;
            edges.push_back({u, v, len});
            adj[u].push_back({v, len}); adj[v].push_back({u, len});
        }
        in >> k; capitals.resize(k);
        for (int i = 0; i < k; i++) in >> capitals[i];
        file.close();
        runAlgorithm(); buildMatrix(); buildGraph(); buildResults();
        infoLbl->setText(QString("  Городов: %1, дорог: %2, государств: %3").arg(n).arg(m).arg(k));
        statusBar()->showMessage("Загружено: " + path);
    }

private:
    // Многоисточниковый Прим в режиме round-robin: каждая столица — отдельный источник,
    // государства по очереди забирают свободного соседа с минимальным ребром-кандидатом.
    void runAlgorithm() {
        // 1. Инициализация: столицы — стартовые вершины своих государств.
        stateOf.assign(n+1, -1); groups.assign(k, {});
        for (int i = 0; i < k; i++) { stateOf[capitals[i]] = i; groups[i].push_back(capitals[i]); }

        // 2. По одной min-куче на государство, заполняем рёбрами от столиц.
        using pii = std::pair<int,int>;  // (длина ребра, сосед)
        std::vector<std::priority_queue<pii,std::vector<pii>,std::greater<pii>>> pq(k);
        for (int i = 0; i < k; i++)
            for (auto [v, len] : adj[capitals[i]])
                if (stateOf[v] == -1) pq[i].push({len, v});

        // 3. Основной цикл: round-robin по государствам, пока есть свободные города.
        int unassigned = n - k;
        while (unassigned > 0) {
            bool progress = false;
            for (int i = 0; i < k && unassigned > 0; i++) {
                while (!pq[i].empty()) {
                    auto [d, v] = pq[i].top(); pq[i].pop();
                    if (stateOf[v] == -1) {  // ленивое удаление: занятых пропускаем
                        stateOf[v] = i; groups[i].push_back(v); unassigned--; progress = true;
                        // расширяем фронт: рёбра нового города к свободным соседям
                        for (auto [u, len] : adj[v]) if (stateOf[u] == -1) pq[i].push({len, u});
                        break;  // следующий ход — у следующего государства
                    }
                }
            }
            if (!progress) break;  // несвязный граф — невозможно при корректном входе
        }
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
        for (auto& e : edges) {
            table->item(e.u-1, e.v-1)->setText(QString::number(e.len));
            table->item(e.v-1, e.u-1)->setText(QString::number(e.len));
        }
        table->horizontalHeader()->setDefaultSectionSize(60);
        table->verticalHeader()->setDefaultSectionSize(36);
        QFont mf = table->font(); mf.setPointSize(11); table->setFont(mf);
    }

    void buildGraph() {
        scene->clear(); if (n == 0) return;
        const double cx = 400, cy = 300, R = 240, nodeR = 18;
        std::vector<QPointF> pos(n+1);
        for (int i = 1; i <= n; i++) {
            double a = 2*M_PI*(i-1)/n - M_PI/2;
            pos[i] = {cx + R*cos(a), cy + R*sin(a)};
        }
        for (auto& e : edges) {
            bool same = stateOf[e.u] == stateOf[e.v] && stateOf[e.u] >= 0;
            QPen pen = same ? QPen(STATE_COLORS[stateOf[e.u] % NUM_COLORS], 2)
                            : QPen(Qt::gray, 1);
            scene->addLine(QLineF(pos[e.u], pos[e.v]), pen);
            QPointF mid = (pos[e.u] + pos[e.v]) / 2;
            auto* t = scene->addText(QString::number(e.len));
            t->setPos(mid.x()-t->boundingRect().width()/2, mid.y()-t->boundingRect().height()/2);
        }
        for (int i = 1; i <= n; i++) {
            int s = stateOf[i];
            QColor c = s >= 0 ? STATE_COLORS[s % NUM_COLORS] : Qt::lightGray;
            bool isCap = std::find(capitals.begin(), capitals.end(), i) != capitals.end();
            double r = isCap ? nodeR*1.3 : nodeR;
            QPen pen(Qt::black, isCap ? 2.5 : 1);
            scene->addEllipse(pos[i].x()-r, pos[i].y()-r, 2*r, 2*r, pen, QBrush(c));
            auto* t = scene->addText(QString::number(i));
            QFont f = t->font(); f.setBold(true); t->setFont(f);
            t->setDefaultTextColor(Qt::black);
            t->setPos(pos[i].x()-t->boundingRect().width()/2, pos[i].y()-t->boundingRect().height()/2);
        }
        view->fitInView(scene->sceneRect().adjusted(-20,-20,20,20), Qt::KeepAspectRatio);
    }

    void buildResults() {
        statsLbl->setText(QString("Городов: <b>%1</b>   Дорог: <b>%2</b>   Государств: <b>%3</b>")
            .arg(n).arg(m).arg(k));
        resultsTree->clear();
        for (int i = 0; i < k; i++) {
            QPixmap swatch(14, 14); swatch.fill(STATE_COLORS[i % NUM_COLORS]);
            auto* top = new QTreeWidgetItem(resultsTree);
            top->setIcon(0, QIcon(swatch));
            top->setText(0, QString("Государство %1 — %2 городов")
                .arg(i+1).arg(groups[i].size()));
            QFont f = top->font(0); f.setBold(true); top->setFont(0, f);
            for (int ci : groups[i]) {
                auto* child = new QTreeWidgetItem(top);
                QString label = QString("Город %1").arg(ci);
                if (ci == capitals[i]) label += "   (столица)";
                child->setText(0, label);
            }
            top->setExpanded(true);
        }
    }

    int n=0, m=0, k=0;
    std::vector<Edge> edges;
    std::vector<std::vector<std::pair<int,int>>> adj;
    std::vector<int> capitals, stateOf;
    std::vector<std::vector<int>> groups;
    QTabWidget* tabs; QGraphicsView* view; QGraphicsScene* scene;
    QTableWidget* table; QTreeWidget* resultsTree; QLabel* statsLbl; QLabel* infoLbl;
};

#include "main.moc"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    MainWindow w; w.show();
    return app.exec();
}
