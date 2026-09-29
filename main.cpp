/**
 * easysearch — 文件快速搜索/打开/删除工具
 *
 * 在当前目录下按文件名关键词搜索，支持打开和回收站删除。
 * 优先从 tree.tree 索引检索（若存在），否则直接遍历目录。
 * 原 Python (ck.pyw) 的 C++/Qt6 重构版。
 */

#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>
#include <QFile>
#include <QStatusBar>
#include <QTextStream>
#include <QDirIterator>
#include <QVector>
#include <algorithm>
#include <cstdio>

struct FileEntry {
    QString filename;
    QString rel_path;   // 相对于当前目录
};

// ── 读取 tree.tree 索引 ──────────────────────────────────────────
// 格式: filename|rel_path   （每行一个，忽略头尾标记行）
static QVector<FileEntry> read_tree_file(const QString& path)
{
    QVector<FileEntry> result;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return result;

    QTextStream in(&f);
    bool in_table = false;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line == "=== FILE TABLE ===") {
            in_table = true;
            continue;
        }
        if (line == "--- END OF FILE TABLE ---")
            break;
        if (!in_table || line.isEmpty())
            continue;

        // 解析 "filename|rel_path"
        int sep = line.lastIndexOf('|');
        if (sep <= 0) continue;
        QString filename = line.left(sep);
        QString rel_path = line.mid(sep + 1);
        result.push_back({filename, rel_path});
    }
    return result;
}

static bool tree_file_exists()
{
    return QFileInfo::exists(QDir::current().absoluteFilePath("tree.tree"));
}

class EasySearchWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit EasySearchWindow(QWidget* parent = nullptr)
        : QMainWindow(parent)
    {
        setWindowTitle("文件查询器");
        resize(520, 420);

        auto* central = new QWidget(this);
        setCentralWidget(central);
        auto* layout = new QVBoxLayout(central);
        layout->setSpacing(8);

        layout->addWidget(new QLabel("请输入查询的字符:", this));

        search_input_ = new QLineEdit(this);
        search_input_->setPlaceholderText("输入文件名关键词...");
        connect(search_input_, &QLineEdit::returnPressed,
                this, &EasySearchWindow::on_search);
        layout->addWidget(search_input_);

        auto* search_btn = new QPushButton("查询文件", this);
        connect(search_btn, &QPushButton::clicked,
                this, &EasySearchWindow::on_search);
        layout->addWidget(search_btn);

        file_list_ = new QListWidget(this);
        file_list_->setSelectionMode(QAbstractItemView::ExtendedSelection);
        layout->addWidget(file_list_, 1);

        auto* btn_layout = new QHBoxLayout;
        auto* open_btn = new QPushButton("打开选中文件", this);
        auto* del_btn  = new QPushButton("删除选中文件", this);
        auto* tree_btn = new QPushButton("生成 tree.tree", this);

        connect(open_btn, &QPushButton::clicked,
                this, &EasySearchWindow::on_open);
        connect(del_btn, &QPushButton::clicked,
                this, &EasySearchWindow::on_delete);
        connect(tree_btn, &QPushButton::clicked,
                this, &EasySearchWindow::on_build_tree);

        btn_layout->addWidget(open_btn);
        btn_layout->addWidget(del_btn);
        btn_layout->addWidget(tree_btn);
        layout->addLayout(btn_layout);

        QString msg = "就绪 | 当前目录: " + QDir::currentPath();
        if (tree_file_exists())
            msg += " [tree.tree 索引可用]";
        statusBar()->showMessage(msg);
    }

private slots:
    void on_search()
    {
        QString keyword = search_input_->text().trimmed();
        if (keyword.isEmpty()) {
            QMessageBox::warning(this, "错误", "请输入查询的字符");
            return;
        }

        file_list_->clear();
        QDir base(QDir::current());

        // 收集匹配结果: {filename, rel_path, full_path}
        struct Match { QString name; QString rel; QString full; };
        QVector<Match> matches;
        bool from_tree = false;

        // 优先从 tree.tree 检索
        auto tree = read_tree_file(base.absoluteFilePath("tree.tree"));
        if (!tree.empty()) {
            from_tree = true;
            for (const auto& e : tree) {
                if (e.filename.contains(keyword, Qt::CaseInsensitive))
                    matches.push_back({e.filename, e.rel_path,
                                       base.absoluteFilePath(e.rel_path)});
            }
        }

        // 回退：直接遍历目录
        if (!from_tree) {
            for (const QString& name : base.entryList(
                     QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot)) {
                if (name.contains(keyword, Qt::CaseInsensitive))
                    matches.push_back({name, name,
                                       base.absoluteFilePath(name)});
            }
        }

        // 检测同名文件 → 需要显示路径区分
        QHash<QString, int> freq;
        for (const auto& m : matches)
            freq[m.name]++;

        // 填入列表
        int idx = 0;
        for (const auto& m : matches) {
            QString label;
            if (freq[m.name] > 1) {
                // 有同名文件 → 显示 "filename (relative_dir)"
                QString dir = QFileInfo(m.rel).path();
                if (dir == ".")
                    label = QString("%1、%2").arg(++idx).arg(m.name);
                else
                    label = QString("%1、%2 (%3)")
                            .arg(++idx).arg(m.name).arg(dir);
            } else {
                label = QString("%1、%2").arg(++idx).arg(m.name);
            }
            auto* item = new QListWidgetItem(label);
            item->setData(Qt::UserRole, m.full);
            file_list_->addItem(item);
        }

        QString src = from_tree ? "tree.tree" : "目录扫描";
        if (matches.isEmpty())
            statusBar()->showMessage("未找到匹配文件 [" + src + "]");
        else
            statusBar()->showMessage(
                QString("找到 %1 个文件 [%2]").arg(matches.size()).arg(src));
    }

    void on_open()
    {
        auto items = file_list_->selectedItems();
        if (items.isEmpty()) {
            QMessageBox::warning(this, "错误", "请选择要打开的文件");
            return;
        }
        for (auto* item : items) {
            QString path = item->data(Qt::UserRole).toString();
            if (QFileInfo::exists(path))
                QDesktopServices::openUrl(QUrl::fromLocalFile(path));
            else
                QMessageBox::warning(this, "错误", "文件不存在: " + path);
        }
    }

    void on_delete()
    {
        auto items = file_list_->selectedItems();
        if (items.isEmpty()) {
            QMessageBox::warning(this, "错误", "请选择要删除的文件");
            return;
        }
        auto reply = QMessageBox::question(this, "确认删除",
            QString("确定要将选中的 %1 个文件移至回收站吗?").arg(items.size()),
            QMessageBox::Yes | QMessageBox::No);
        if (reply != QMessageBox::Yes)
            return;

        for (auto* item : items) {
            QString path = item->data(Qt::UserRole).toString();
            if (QFileInfo::exists(path)) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                if (!QFile::moveToTrash(path))
                    QMessageBox::warning(this, "错误", "删除失败: " + path);
#else
                if (!QFile::remove(path))
                    QMessageBox::warning(this, "错误", "删除失败: " + path);
#endif
                delete file_list_->takeItem(file_list_->row(item));
            }
        }
        statusBar()->showMessage("已移至回收站");
    }

    void on_build_tree()
    {
        QDir base(QDir::current());
        QDirIterator it(base.absolutePath(),
                        QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
                        QDirIterator::Subdirectories);

        QVector<FileEntry> entries;
        while (it.hasNext()) {
            it.next();
            QString filename = it.fileName();
            if (filename == "tree.tree") continue;
            entries.push_back({
                filename,
                base.relativeFilePath(it.fileInfo().absoluteFilePath())
            });
        }

        // 按文件名排序
        std::sort(entries.begin(), entries.end(),
                  [](const FileEntry& a, const FileEntry& b) {
                      return a.filename < b.filename;
                  });

        QString path = base.absoluteFilePath("tree.tree");
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(this, "错误", "无法创建 tree.tree");
            return;
        }

        QTextStream out(&file);
        out << "=== FILE TABLE ===\n";
        for (const auto& e : entries)
            out << e.filename << "|" << e.rel_path << "\n";
        out << "--- END OF FILE TABLE ---\n";
        file.close();

        statusBar()->showMessage(
            QString("tree.tree 已生成 (%1 个文件/文件夹)").arg(entries.size()));
    }

private:
    QLineEdit*   search_input_ = nullptr;
    QListWidget* file_list_    = nullptr;
};

// ── 命令行: 生成 tree.tree ─────────────────────────────────────
static int generate_tree_cmd()
{
    QDir base(QDir::current());
    QDirIterator it(base.absolutePath(),
                    QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);

    QVector<FileEntry> entries;
    while (it.hasNext()) {
        it.next();
        QString filename = it.fileName();
        if (filename == "tree.tree") continue;
        entries.push_back({
            filename,
            base.relativeFilePath(it.fileInfo().absoluteFilePath())
        });
    }

    std::sort(entries.begin(), entries.end(),
              [](const FileEntry& a, const FileEntry& b) {
                  return a.filename < b.filename;
              });

    QString path = base.absoluteFilePath("tree.tree");
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCritical("无法创建 tree.tree");
        return 1;
    }
    QTextStream out(&file);
    out << "=== FILE TABLE ===\n";
    for (const auto& e : entries)
        out << e.filename << "|" << e.rel_path << "\n";
    out << "--- END OF FILE TABLE ---\n";
    file.close();

    printf("tree.tree 已生成 (%d 个文件/文件夹)\n", entries.size());
    return 0;
}

int main(int argc, char* argv[])
{
    if (argc >= 2) {
        QString arg = QString::fromUtf8(argv[1]);
        if (arg == "--generate-tree" || arg == "-g")
            return generate_tree_cmd();
    }

    QApplication app(argc, argv);
    QApplication::setApplicationName("easysearch");
    QApplication::setApplicationVersion("1.0.0");

    EasySearchWindow w;
    w.show();
    return app.exec();
}

#include "main.moc"
