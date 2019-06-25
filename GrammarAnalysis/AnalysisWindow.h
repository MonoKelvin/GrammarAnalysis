#ifndef ANALYSISWINDOW_H
#define ANALYSISWINDOW_H

#include <QMap>
#include <QWidget>
#include <QMessageBox>

class QPushButton;
class QLineEdit;
class QTextEdit;
class QTableWidget;

/* 定义类型：终结符非终结符组成的符号对 */
typedef QPair<QString, QString> SymbolPair;

/* 定义类型：产生式，左边推出右边一连串字符 */
typedef QPair<QString, QStringList> Production;

/* 定义类型：文法，产生式的集合 */
typedef QList<Production> Grammar;

/* 定义类型：文法分析表，终结符非终结符所确定的产生式 */
typedef QMap<SymbolPair, Production> GAT;

/* 定义类型：FIRST集，每个非终结符的First集组成的映射表 */
typedef QMap<QString, QStringList> FirstSet;

/* 定义类型：FLLOW集，每个非终结符的Follow集组成的映射表 */
typedef QMap<QString, QStringList> FollowSet;

/* 定义类型：FIRSTVT集，每个非终结符的FirstVT集组成的映射表 */
typedef QMap<QString, QStringList> FirstVTSet;

/* 定义类型：LASTVT集，每个非终结符的LastVT集组成的映射表 */
typedef QMap<QString, QStringList> LastVTSet;

namespace Ui {
class AnalysisWindow;
}

/* 类：主要的分析窗口
 * 继承：QWidget控件类
 */
class AnalysisWindow : public QWidget {
    Q_OBJECT

  public:
    explicit AnalysisWindow(QWidget* parent = nullptr);
    ~AnalysisWindow();

  private slots:
    /* 槽函数：输入串按下回车事件 */
    void on_le_inputLL1_returnPressed();

  private:
    Ui::AnalysisWindow* ui;

    /* LL1文法输入框控件 */
    QLineEdit* m_textInputLL1;

    /* LL1文法步骤输出表控件 */
    QTableWidget* m_twOutputLL1;

    /* 预测分析表 */
    QTableWidget* m_twAnalysisTable;

    /* 文法 */
    Grammar m_grammar;

    /* 所有的非终结符 */
    QStringList m_nTerminal;

    /* 所有的终结符 */
    QStringList m_Terminal;

    /* FIRST集 */
    FirstSet m_firstSet;

    /* FOLLOW集 */
    FollowSet m_followSet;

    /* FIRSTVT集 */
    FirstVTSet m_firstVT;

    /* LASTVT集 */
    LastVTSet m_lastVT;

    /* 文法分析表 */
    GAT m_LLT;

    /* 是否已经创建了文法分析表 */
    bool m_isCreatedAT;

  private:
    /* 刷新LL1规约表 */
    void flashLL1Table();

    /* 内联：打印产生式 */
    inline QString printProduction(const Production& production) {
        QString _str = production.first + "->";
        for (int i = 0; i < production.second.length(); i++) {
            _str.append(production.second[i]);
        }
        return _str;
    }

    /* 内联：添加一个产生式到文法中，若文法中存在则不作任何操作 */
    inline void addProduction(const Production& production) {
        if (!m_grammar.contains(production)) {
            m_grammar << production;
        }
    }

    /* 内联：判断给定字符是否是终结符 */
    inline bool isTerminal(const QString& s) {
        return !isNonTerminal(s);
    }

    /* 内联：判断给定字符是否是非终结符 */
    inline bool isNonTerminal(const QString& s) {
        return s.contains(QRegExp("[A-Z]"));
        //return "A" <= s && s <= "Z";
    }

    /* 内联：文法相关的警告：是否创建了文法或文法分析表 */
    inline bool grammarAlert() {
        if (m_grammar.isEmpty()) {
            QMessageBox::warning(this, "警告", "文法内容不能为空，请先创建文法!");
            return false;
        } else if (!m_isCreatedAT) {
            QMessageBox::warning(this, "警告", "请先生成分析表!");
            return false;
        }
        return true;
    }
    /* 分析文法中所有的终结符和非终结符类型 */
    void getSymbolType();

    /* 获得表中一列的元素的位置 */
    int getTableColumn(const QString& headerStr);

    /* 获得第一次匹配到的产生式 */
    Production isFirstEqualsProduction(const QString& nT, const QString& termainal);

    /* 模板函数：获得弹栈的内容 */
    template<class T>
    QString getStackData(QStack<T> stack, bool isReverse = true);

    /* 模板函数：根据类型打印集合 */
    template<class T>
    void printSet(const T& setType, const QString& type);

    /* 把symbol添加到集合theSet中，返回theSet集合中的元素是否有变化 */
    bool appendToSet(QStringList& theSet, const QString& symbol);

    /* 把withSet的元素添加到集合theSet中，返回theSet集合中的元素是否有变化 */
    bool appendToSet(QStringList& theSet,  const QStringList& withSet);

    /* 消除左递归 */
    void eliminateLeftRecursion();

    /* 消除直接左递归 */
    //void eliminateDirectLeftRecursion();

    /* 创建文法分析表 */
    void createLL1AnalysisTable();

    /* 创建LL1预测分析表 */
    void createLL1AnalysisProcess();

    /* 创建算符优先关系表 */
    void createOperatorPrecedenceRelationTable();

    /* 求FIRST集 */
    void FIRST();

    /* 求FLLOW集 */
    void FOLLOW();

    /* 求FIRSTVT集 */
    void FIRSTVT();

    /* 求LASTVT集 */
    void LASTVT();
};

#endif // ANALYSISWINDOW_H
