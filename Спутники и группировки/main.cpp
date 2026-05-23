#include <QtWidgets>
#include <vector>
#include <queue>
#include <cmath>
#include <algorithm>

static const QColor GROUP_COLORS[] = {
    Qt::red, Qt::blue, Qt::darkGreen, Qt::magenta,
    QColor(255,140,0), Qt::cyan, QColor(180,180,0), Qt::darkGray,
    QColor(128,0,128), QColor(165,42,42), QColor(0,128,128), QColor(70,130,180)
};
static const int NUM_COLORS = 12;

struct Link { int u, v, len; };

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Спутники и группировки");
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
        tabs->addTab(table, "Матрица взаимодействий (сек)");
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
        auto* treeLbl = new QLabel("Распределение по группировкам:");
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
        QTextStream in(&file); in >> n >> m;
        links.clear(); adj.assign(n+1, {});
        for (int i = 0; i < m; i++) {
            int u, v, len; in >> u >> v >> len;
            links.push_back({u, v, len});
            adj[u].push_back({v, len}); adj[v].push_back({u, len});
        }
        in >> k; mains.resize(k);
        for (int i = 0; i < k; i++) in >> mains[i];
        file.close();
        runAlgorithm(); buildMatrix(); buildGraph(); buildResults();
        infoLbl->setText(QString("  КА: %1, связей: %2, группировок: %3").arg(n).arg(m).arg(k));
        statusBar()->showMessage("Загружено: " + path);
    }

private:
    // Многоисточниковый Прим с глобальным критерием: одна общая куча на все группировки,
    // на каждом шаге берётся глобально минимальное фронтирное ребро.
    void runAlgorithm() {
        // 1. Главные КА — стартовые вершины своих группировок.
        groupOf.assign(n+1, -1); groups.assign(k, {});
        for (int i = 0; i < k; i++) { groupOf[mains[i]] = i; groups[i].push_back(mains[i]); }

        // 2. Единая min-куча: запись хранит длительность, КА-кандидат и его будущую группировку.
        struct Cand { int len, v, g; bool operator>(const Cand& o) const { return len > o.len; } };
        std::priority_queue<Cand, std::vector<Cand>, std::greater<Cand>> pq;
        for (int i = 0; i < k; i++)
            for (auto [v, len] : adj[mains[i]])
                if (groupOf[v] == -1) pq.push({len, v, i});

        // 3. Глобально жадный шаг: ближайший по tau из всех кандидатов, ленивое удаление занятых.
        int unassigned = n - k;
        while (unassigned > 0 && !pq.empty()) {
            auto [d, v, g] = pq.top(); pq.pop();
            if (groupOf[v] != -1) continue;
            groupOf[v] = g; groups[g].push_back(v); unassigned--;
            // расширяем фронт группировки g рёбрами нового КА
            for (auto [u, len] : adj[v]) if (groupOf[u] == -1) pq.push({len, u, g});
        }
    }

    void buildMatrix() {
        table->clear(); table->setRowCount(n); table->setColumnCount(n);
        QStringList labels; for (int i = 1; i <= n; i++) labels << QString("КА%1").arg(i);
        table->setHorizontalHeaderLabels(labels); table->setVerticalHeaderLabels(labels);
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++) {
                auto* item = new QTableWidgetItem(i == j ? "0" : "-");
                item->setTextAlignment(Qt::AlignCenter);
                table->setItem(i, j, item);
            }
        for (auto& l : links) {
            table->item(l.u-1, l.v-1)->setText(QString::number(l.len));
            table->item(l.v-1, l.u-1)->setText(QString::number(l.len));
        }
        table->horizontalHeader()->setDefaultSectionSize(70);
        table->verticalHeader()->setDefaultSectionSize(36);
        QFont mf = table->font(); mf.setPointSize(11); table->setFont(mf);
    }

    void buildGraph() {
        scene->clear(); if (n == 0) return;
        const double cx = 400, cy = 300, R = 240, nodeR = 20;
        std::vector<QPointF> pos(n+1);
        for (int i = 1; i <= n; i++) {
            double a = 2*M_PI*(i-1)/n - M_PI/2;
            pos[i] = {cx + R*cos(a), cy + R*sin(a)};
        }
        for (auto& l : links) {
            bool same = groupOf[l.u] == groupOf[l.v] && groupOf[l.u] >= 0;
            QPen pen = same ? QPen(GROUP_COLORS[groupOf[l.u] % NUM_COLORS], 2)
                            : QPen(Qt::gray, 1);
            scene->addLine(QLineF(pos[l.u], pos[l.v]), pen);
            QPointF mid = (pos[l.u] + pos[l.v]) / 2;
            auto* t = scene->addText(QString::number(l.len));
            t->setPos(mid.x()-t->boundingRect().width()/2, mid.y()-t->boundingRect().height()/2);
        }
        for (int i = 1; i <= n; i++) {
            int s = groupOf[i];
            QColor c = s >= 0 ? GROUP_COLORS[s % NUM_COLORS] : Qt::lightGray;
            bool isMain = std::find(mains.begin(), mains.end(), i) != mains.end();
            double r = isMain ? nodeR*1.3 : nodeR;
            QPen pen(Qt::black, isMain ? 2.5 : 1);
            scene->addEllipse(pos[i].x()-r, pos[i].y()-r, 2*r, 2*r, pen, QBrush(c));
            auto* t = scene->addText(QString("КА%1").arg(i));
            QFont f = t->font(); f.setBold(true); t->setFont(f);
            t->setDefaultTextColor(Qt::black);
            t->setPos(pos[i].x()-t->boundingRect().width()/2, pos[i].y()-t->boundingRect().height()/2);
        }
        view->fitInView(scene->sceneRect().adjusted(-20,-20,20,20), Qt::KeepAspectRatio);
    }

    void buildResults() {
        statsLbl->setText(QString("КА: <b>%1</b>   Связей: <b>%2</b>   Группировок: <b>%3</b>")
            .arg(n).arg(m).arg(k));
        resultsTree->clear();
        for (int i = 0; i < k; i++) {
            QPixmap swatch(14, 14); swatch.fill(GROUP_COLORS[i % NUM_COLORS]);
            auto* top = new QTreeWidgetItem(resultsTree);
            top->setIcon(0, QIcon(swatch));
            top->setText(0, QString("Группировка %1 — %2 КА")
                .arg(i+1).arg(groups[i].size()));
            QFont f = top->font(0); f.setBold(true); top->setFont(0, f);
            for (int ci : groups[i]) {
                auto* child = new QTreeWidgetItem(top);
                QString label = QString("КА%1").arg(ci);
                if (ci == mains[i]) label += "   (главный)";
                child->setText(0, label);
            }
            top->setExpanded(true);
        }
    }

    int n=0, m=0, k=0;
    std::vector<Link> links;
    std::vector<std::vector<std::pair<int,int>>> adj;
    std::vector<int> mains, groupOf;
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
