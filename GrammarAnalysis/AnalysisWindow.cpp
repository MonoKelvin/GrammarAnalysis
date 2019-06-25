#include "AnalysisWindow.h"
#include "ui_analysiswindow.h"

#include <QDebug>
#include <QMessageBox>
#include <QStack>
#include <QStringList>

AnalysisWindow::AnalysisWindow(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::AnalysisWindow) {
    ui->setupUi(this);
    this->resize(900, 550);

    /* 初始化成员变量 */
    m_textInputLL1 = ui->le_inputLL1;
    m_twOutputLL1 = ui->tw_outputLL1;
    m_twAnalysisTable = ui->tw_analysisTable;

    /* 文法定义 */
    m_grammar << Production("E", QStringList() << "E" << "+" << "T");
    m_grammar << Production("E", QStringList() << "T");
    m_grammar << Production("T", QStringList() << "T" << "*" << "F");
    m_grammar << Production("T", QStringList() << "F");
    m_grammar << Production("F", QStringList() << "P" << "↑" << "F");
    m_grammar << Production("F", QStringList() << "P");
    m_grammar << Production("P", QStringList() << "(" << "E" << ")");
    m_grammar << Production("P", QStringList() << "Q");
    m_grammar << Production("Q", QStringList() << "i");

    /* 一定要先获得所有的符号类型才可以往下继续 */
    getSymbolType();

    /* 输出所给的原文法 */
    {
        QString contents;
        for (int i = 0; i < m_grammar.length(); i++) {
            contents += QString("(%1) ").arg(i + 1) + printProduction(m_grammar[i]) + "\n";
        }
        ui->te_showGrammar->setPlainText(contents);
    }

    /* 消除文法二义性 */
    m_grammar[4] = Production("F", QStringList() << "P" << "F'");
    m_grammar[5] = Production("F'", QStringList() << "ε");
    m_grammar.insert(5, Production("F'", QStringList() << "↑" << "F"));

    /* 消除文法的左递归 */
    eliminateLeftRecursion();
    // eliminateDirectLeftRecursion();

    /* 输出新的文法 */
    {
        QString contents;
        if (m_grammar.isEmpty()) {
            contents = "当前文法为空，点击上方创建文法或从下方预设中选择内置文法";
        }
        for (int i = 0; i < m_grammar.length(); i++) {
            contents += QString("(%1) ").arg(i + 1) + printProduction(m_grammar[i]) + "\n";
        }
        ui->te_newGrammar->append(contents);
    }

    /* 控件初始值 */
    m_twAnalysisTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_twAnalysisTable->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tw_operatorTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tw_operatorTable->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_twOutputLL1->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_textInputLL1->setText("i+i*i#");
    flashLL1Table();

    /* 信号槽连接：创建文法分析表 */
    connect(ui->btn_createGAT, &QPushButton::clicked, [ = ] {
        if (m_grammar.isEmpty()) {
            QMessageBox::warning(this, "警告", "文法内容不能为空，请先创建文法!");
        } else if (!m_isCreatedAT) {
            getSymbolType();
            FIRST();
            FOLLOW();
            FIRSTVT();
            LASTVT();
            createLL1AnalysisTable();
            createOperatorPrecedenceRelationTable();
            m_isCreatedAT = true;
            //m_twAnalysisTable->setItem(2, 5, new QTableWidgetItem("F->P"));
            //m_LLT[SymbolPair("F", "i")] = Production("F", QStringList("P"));
        }
    });

    /* 信号槽连接：点击显示FIRST集按钮 */
    connect(ui->btn_first, &QPushButton::clicked, [ = ] {
        if (grammarAlert()) {
            printSet(m_firstSet, "FIRST");
        }
    });

    /* 信号槽连接：点击显示FLLOW集按钮 */
    connect(ui->btn_follow, &QPushButton::clicked, [ = ] {
        if (grammarAlert()) {
            printSet(m_followSet, "FLLOW");
        }
    });

    /* 信号槽连接：点击显示FIRSTVT集按钮 */
    connect(ui->btn_firstvt, &QPushButton::clicked, [ = ] {
        if (grammarAlert()) {
            printSet(m_firstVT, "FIRSTVT");
        }
    });

    /* 信号槽连接：点击显示LASTVT集按钮 */
    connect(ui->btn_lastvt, &QPushButton::clicked, [ = ] {
        if (grammarAlert()) {
            printSet(m_lastVT, "LASTVT");
        }
    });
}

AnalysisWindow::~AnalysisWindow() {
    delete ui;
}

void AnalysisWindow::eliminateLeftRecursion() {
    /*
    * 一般情况下，假定关于A的产生式是：
    *     A -> Aα1| Aα2 |… |Aαm
    *     A -> β1|β2 |…|βn
    * 其中，αi(1≤i≤m)均不为空，βj(1≤j≤n)均不以A打头。
    * 则消除直接左递归后改写为：
    *     A -> β1A'| β2 A' |…| βnA'
    *     A'-> α1A' | α2A' |…| αmA' |ε
    */
    for (auto iter = m_nTerminal.begin(); iter != m_nTerminal.end(); iter++) {
        QList<Production*> _A;
        QList<Production*> _nA;
        auto it = m_grammar.begin();
        for (; it != m_grammar.end(); it++) {
            if ((*it).first == *iter) {
                // 找到所有A->Aa (A推出以A打头的产生式)
                if (*iter == (*it).second[0]) {
                    _A << &(*it);
                    // 找到所有A->b (A推出不以A打头的产生式)
                } else if ((*it).second != QStringList("ε")) {
                    _nA << &(*it);
                }
            }
        }

        if (!_A.isEmpty()) {
            QString _Ap = _A.first()->first + "'";
            for (auto ai = _A.begin(); ai != _A.end(); ai++) {
                (*ai)->first = _Ap;
                (*ai)->second.removeFirst();
                (*ai)->second.append(_Ap);
            }
            for (auto nai = _nA.begin(); nai != _nA.end(); nai++) {
                (*nai)->second.append(_Ap);
            }
            m_grammar.push_back(Production(_Ap, QStringList("ε")));
        }
    }
    m_Terminal << "#";
}

/*void AnalysisWindow::eliminateDirectLeftRecursion() {
    for (int i = m_grammar.length() - 1; i >= 0; --i) {
        //当遇到P->Pα
        QString nT = m_grammar[i].first;
        if (nT == QString(m_grammar[i].second[0])) {
            QString a;
            if (m_grammar[i].second.length() > 1)
                a = m_grammar[i].second.mid(1);
            //找到所有的P->β
            for (int j = m_grammar.length() - 1; j >= 0; --j) {
                if (j != i && nT == m_grammar[j].first && nT != QString(m_grammar[j].second[0])) {
                    //得到新的P->βP' P'->αP'|ε
                    QString newNT = nT.toLower();
                    addProduction(Production(nT, m_grammar[j].second + newNT));
                    addProduction(Production(newNT, a + newNT));
                    addProduction(Production(newNT, "ε"));
                    m_grammar.removeAt(j);
                }
            }
            m_grammar.removeAt(i);
        }
    }
}*/

void AnalysisWindow::createLL1AnalysisProcess() {
    QStack<QString> stack;                      //分析栈
    QString remaining = m_textInputLL1->text(); //剩余输入串
    int rowCount = m_twOutputLL1->rowCount();   //0

    /*------------------核心算法--------------------*/
    //表内初始化设置
    stack.push("#");
    stack.push(m_grammar[1].first);
    m_twOutputLL1->insertRow(rowCount); //增加一行,第0行
    m_twOutputLL1->setItem(rowCount, 0, new QTableWidgetItem(getStackData(stack)));
    m_twOutputLL1->setItem(rowCount, 1, new QTableWidgetItem(remaining));
    m_twOutputLL1->setItem(rowCount, 3, new QTableWidgetItem("初始化"));

    while (remaining != "#" || stack.count() != 1) {
        rowCount++;
        m_twOutputLL1->insertRow(rowCount);   // 增加一行
        QString pushSymbol = stack.pop();     // 栈顶弹出的一个字符
        QString pushContents;                 // 弹栈的内容（可以有多个字符）
        Production doProduction = m_LLT.value(SymbolPair(pushSymbol, QString(remaining[0]))); //表中查到的产生式
        if (doProduction.second.isEmpty()) { // 表中不存在递推文法
            if (pushSymbol != remaining[0]) {
                QMessageBox::warning(this, "分析错误", "无法找到产生式！");
                return;
            } else {
                remaining.remove(0, 1);
                pushContents = "GETNEXT(I)";
            }
        } else {
            if (doProduction.second.contains("ε")) {
                pushContents = "POP";
            } else {
                int i = doProduction.second.length();
                while (--i >= 0) {
                    pushContents += doProduction.second[i];
                    stack.push(doProduction.second[i]);
                }
                pushContents = QString("POP,PUSH(%1)").arg(pushContents);
            }
            m_twOutputLL1->setItem(rowCount, 2, new QTableWidgetItem(printProduction(doProduction)));
        }
        m_twOutputLL1->setItem(rowCount, 0, new QTableWidgetItem(getStackData(stack)));
        m_twOutputLL1->setItem(rowCount, 1, new QTableWidgetItem(remaining));
        m_twOutputLL1->setItem(rowCount, 3, new QTableWidgetItem(pushContents));
    }

    m_twOutputLL1->insertRow(++rowCount);
    m_twOutputLL1->setItem(rowCount, 3, new QTableWidgetItem("分析成功"));

    return;
}

void AnalysisWindow::createOperatorPrecedenceRelationTable() {
    ui->tw_operatorTable->clearContents();
    ui->tw_operatorTable->setRowCount(m_Terminal.length());
    ui->tw_operatorTable->setColumnCount(m_Terminal.length());
    for (int i = 0; i < m_Terminal.length(); i++) {
        ui->tw_operatorTable->setHorizontalHeaderItem(i, new QTableWidgetItem(m_Terminal[i]));
        ui->tw_operatorTable->setVerticalHeaderItem(i, new QTableWidgetItem(m_Terminal[i]));
        ui->tw_operatorTable->horizontalHeaderItem(i)->setTextAlignment(Qt::AlignCenter);
        ui->tw_operatorTable->verticalHeaderItem(i)->setTextAlignment(Qt::AlignCenter);
    }

    for (auto iter = m_grammar.begin(); iter != m_grammar.end(); iter++) {
        int _n = (*iter).second.length();
        for (int i = 0; i < _n - 1; i++) {
            QString _X = (*iter).second[i];
            QString _Xplus1 = (*iter).second[i + 1];

            if (isTerminal(_X) && isTerminal(_Xplus1)) {
                int _index = m_Terminal.indexOf(_X);
                int _index2 = m_Terminal.indexOf(_Xplus1);
                ui->tw_operatorTable->setItem(_index, _index2, new QTableWidgetItem("="));
            }
            if (isTerminal(_X) && isNonTerminal(_Xplus1)) {
                QStringList _T = m_firstVT[_Xplus1];
                int _index = m_Terminal.indexOf(_X);
                for (auto j = _T.begin(); j != _T.end(); j++) {
                    int _index2 = m_Terminal.indexOf(*j);
                    ui->tw_operatorTable->setItem(_index, _index2, new QTableWidgetItem("<"));
                }
                if (i < _n - 2 && isTerminal((*iter).second[i + 2])) {
                    int _index2 = m_Terminal.indexOf((*iter).second[i + 2]);
                    ui->tw_operatorTable->setItem(_index, _index2, new QTableWidgetItem("="));
                }
            }
            if (isNonTerminal(_X) && isTerminal(_Xplus1)) {
                QStringList _T = m_lastVT[_X];
                int _index2 = m_Terminal.indexOf(_Xplus1);
                for (auto k = _T.begin(); k != _T.end(); k++) {
                    int _index = m_Terminal.indexOf(*k);
                    ui->tw_operatorTable->setItem(_index, _index2, new QTableWidgetItem(">"));
                }
            }
        }
    }
}

void AnalysisWindow::createLL1AnalysisTable() {
    if (!m_isCreatedAT) {
        m_twAnalysisTable->clear();
        m_twAnalysisTable->setRowCount(m_nTerminal.length());
        m_twAnalysisTable->setColumnCount(m_Terminal.length());
        for (int j = 0; j < m_Terminal.length(); ++j) {
            m_twAnalysisTable->setHorizontalHeaderItem(j, new QTableWidgetItem(m_Terminal[j]));
            m_twAnalysisTable->horizontalHeaderItem(j)->setTextAlignment(Qt::AlignCenter);
        }
        for (int i = 0; i < m_nTerminal.length(); ++i) {
            m_twAnalysisTable->setVerticalHeaderItem(i, new QTableWidgetItem(m_nTerminal[i]));
            m_twAnalysisTable->verticalHeaderItem(i)->setTextAlignment(Qt::AlignCenter);
            Production theProduction;
            for (int m = m_firstSet[m_nTerminal[i]].length() - 1; m >= 0; --m) {
                theProduction = isFirstEqualsProduction(m_nTerminal[i], m_firstSet[m_nTerminal[i]][m]);
                m_LLT.insert(SymbolPair(m_nTerminal[i], m_firstSet[m_nTerminal[i]][m]), theProduction);
                m_twAnalysisTable->setItem(i, getTableColumn(m_firstSet[m_nTerminal[i]][m]), new QTableWidgetItem(printProduction(theProduction)));
            }
            if (m_firstSet[m_nTerminal[i]].contains("ε")) {
                theProduction = Production(m_nTerminal[i], QStringList("ε"));
                for (int k = m_followSet[m_nTerminal[i]].length() - 1; k >= 0; --k) {
                    m_LLT.insert(SymbolPair(m_nTerminal[i], m_followSet[m_nTerminal[i]][k]), theProduction);
                    m_twAnalysisTable->setItem(i, getTableColumn(m_followSet[m_nTerminal[i]][k]), new QTableWidgetItem(printProduction(theProduction)));
                }
            }
        }
    }
}

void AnalysisWindow::FIRST() {
    if (!m_isCreatedAT) {
        for (int i = m_nTerminal.length() - 1; i >= 0; --i) {
            m_firstSet.insert(m_nTerminal[i], QList<QString>());
        }
        int len = m_grammar.length();
        while (--len >= 0) {
            QString theFirstSymbol(m_grammar[len].second[0]);
            if (m_Terminal.contains(theFirstSymbol)) {
                appendToSet(m_firstSet[m_grammar[len].first], theFirstSymbol);
            } else {
                for (int k = m_firstSet[theFirstSymbol].length() - 1; k >= 0; --k) {
                    if (!m_firstSet[m_grammar[len].first].contains(m_firstSet[theFirstSymbol][k])) {
                        m_firstSet[m_grammar[len].first] << m_firstSet[theFirstSymbol];
                    }
                }
            }
        }
    }
    return;
}

void AnalysisWindow::FOLLOW() {
    if (!m_isCreatedAT) {
        for (int i = m_nTerminal.length() - 1; i >= 0; --i) {
            if (m_nTerminal[i] == m_grammar[0].first) {
                m_followSet.insert(m_nTerminal[i], QList<QString>() << "#");
            } else {
                m_followSet.insert(m_nTerminal[i], QList<QString>());
            }
        }
        for (int i = 0; i < m_nTerminal.length(); ++i) {
            //遍历所有产生式
            for (int j = 0; j < m_grammar.length(); ++j) {
                QStringList _visited = m_grammar[j].second;     //要访问的产生式右部
                int pos = _visited.indexOf(m_nTerminal[i]); //产生式右边包含要访问的非终结符的位置
                //如果产生式右边包含要访问的非终结符
                if (pos != -1) {
                    /* 使用判定FOLLOW集的定义 */
                    //1.如果字符出现在最后一个,则其FOLLOW集+=产生式左边(不等于自己)的FOLLOW集
                    if (pos == _visited.length() - 1) {
                        if (m_nTerminal[i] != m_grammar[j].first) {
                            appendToSet(m_followSet[m_nTerminal[i]], m_followSet[m_grammar[j].first]);
                        }
                        //2.当字符后有别的非终结符时,其FOLLOW集+=后边字符的FIRST集(去除"ε")
                    } else if (m_nTerminal.contains(_visited[pos + 1])) {
                        appendToSet(m_followSet[m_nTerminal[i]], m_firstSet[_visited[pos + 1]]);
                        //3.当后面的字符还可以推出ε时,其FOLLOW集+=产生式左边(不等于自己)的FOLLOW集
                        if (m_followSet[m_nTerminal[i]].removeOne("ε") && m_nTerminal[i] != m_grammar[j].first) {
                            appendToSet(m_followSet[m_nTerminal[i]], m_followSet[m_grammar[j].first]);
                        }
                        //4.当字符后有终结符时,其FOLLOW集+=后边的字符
                    } else if (m_Terminal.contains(_visited[pos + 1])) {
                        if (!m_followSet[m_nTerminal[i]].contains(_visited[pos + 1])) {
                            m_followSet[m_nTerminal[i]] << _visited[pos + 1];
                        }
                    }
                }
            }
        }
    }
    return;
}

void AnalysisWindow::FIRSTVT() {
    bool _isChanged = true;
    while (_isChanged) {
        _isChanged = false;
        for (auto iter = m_grammar.end() - 1; iter != m_grammar.begin() - 1; --iter) {
            QString _P = (*iter).first;
            QString _Q = (*iter).second.first();

            /* 1.如果存在形如：P->Q...的产生式，则 FIRST(Q)∈FIRST(P) */
            if (isNonTerminal(_Q)) {
                _isChanged = appendToSet(m_firstVT[_P], m_firstVT[_Q]);

                /* 2.如果存在形如：P->Qa...的产生式，则 a∈FIRST(P) */
                if ((*iter).second.length() > 1) {
                    QString _a = (*iter).second[1];
                    if (isTerminal(_a)) {
                        _isChanged = appendToSet(m_firstVT[_P], _a);
                    }
                }
            }
            /* 3.如果存在形如：P->q...的产生式，则 q∈FIRST(P) */
            else {
                _isChanged = appendToSet(m_firstVT[_P], _Q);
            }
        }
    }
}

void AnalysisWindow::LASTVT() {
    bool _isChanged = true;
    while (_isChanged) {
        _isChanged = false;
        for (auto iter = m_grammar.begin(); iter != m_grammar.end(); ++iter) {
            QString _P = (*iter).first;
            QString _Q = (*iter).second.last();

            /* 1.如果存在形如：P->...Q的产生式，则 FIRST(Q)∈FIRST(P) */
            if (isNonTerminal(_Q)) {
                _isChanged = appendToSet(m_lastVT[_P], m_lastVT[_Q]);

                /* 2.如果存在形如：P->...aQ的产生式，则 a∈FIRST(P) */
                if ((*iter).second.length() > 1) {
                    QString _a = *((*iter).second.end() - 2);
                    if (isTerminal(_a)) {
                        _isChanged = appendToSet(m_lastVT[_P], _a);
                    }
                }
            }
            /* 3.如果存在形如：P->...a的产生式，则 a∈FIRST(P) */
            else {
                _isChanged = appendToSet(m_lastVT[_P], _Q);
            }
        }
    }
}

void AnalysisWindow::getSymbolType() {
    for (auto it = m_grammar.begin(); it != m_grammar.end(); it++) {
        if (!m_nTerminal.contains(it->first)) {
            m_nTerminal << it->first;
        }
        for (auto iter = (*it).second.begin(); iter != (*it).second.end(); iter++) {
            if (isTerminal(*iter) && !m_Terminal.contains(*iter)) {
                m_Terminal << *iter;
            }
        }
    }
}

Production AnalysisWindow::isFirstEqualsProduction(const QString& nT, const QString& termainal) {
    for (int m = 0; m < m_grammar.length(); ++m) {
        if (nT == m_grammar[m].first) {
            if (termainal == m_grammar[m].second.first()) {
                return m_grammar[m];
            } else if (m_firstSet[m_grammar[m].second.first()].contains(termainal)) {
                return m_grammar[m];
            }
        }
    }
    return Production("", QStringList(""));
}


int AnalysisWindow::getTableColumn(const QString& headerStr) {
    for (int i = m_twAnalysisTable->columnCount() - 1; i >= 0; --i) {
        if (m_twAnalysisTable->horizontalHeaderItem(i)->text() == headerStr) {
            return i;
        }
    }
    return 0;
}

template<class T>
void AnalysisWindow::printSet(const T& setType, const QString& type) {
    QString contents;
    for (int i = 0; i < m_nTerminal.length(); ++i) {
        contents += type + "(" + m_nTerminal[i] + ")={";
        QStringList elements = setType[m_nTerminal[i]];
        for (int j = elements.length() - 1; j >= 0; --j) {
            contents += elements[j];
            if (j > 0) {
                contents += ",  ";
            }
        }
        contents += "}\n";
    }

    QDialog d(this);
    QLabel lb_showContents;
    QGridLayout* gl = new QGridLayout(&d);

    lb_showContents.adjustSize();
    lb_showContents.setWordWrap(true);
    lb_showContents.setText(contents);
    lb_showContents.setAlignment(Qt::AlignTop);

    gl->addWidget(&lb_showContents);

    d.setGeometry(100, 100, 400, 300);
    d.setLayout(gl);
    d.setWindowTitle(type);
    d.exec();

    delete gl;
}

template<class T>
QString AnalysisWindow::getStackData(QStack<T> stack, bool isReverse) {
    QString data = "";
    if (typeid(int) == typeid(T)) {
        while (!stack.isEmpty()) {
            if (isReverse) {
                data.insert(0, QString("%1").arg(stack.pop()));
            } else {
                data += stack.pop();
            }
        }
    } else {
        while (!stack.isEmpty()) {
            if (isReverse) {
                data.insert(0, stack.pop());
            } else {
                data += stack.pop();
            }
        }
    }
    return data;
}

void AnalysisWindow::on_le_inputLL1_returnPressed() {
    if (!m_isCreatedAT) {
        QMessageBox::warning(this, "分析错误", "请先生成文法分析表");
        return;
    }
    if (!m_textInputLL1->text().endsWith("#")) {
        m_textInputLL1->setText(m_textInputLL1->text() + "#");
    }
    flashLL1Table();
    if (m_isCreatedAT && !m_grammar.isEmpty()) {
        createLL1AnalysisProcess();
    }
}

void AnalysisWindow::flashLL1Table() {
    m_twOutputLL1->clearContents();
    m_twOutputLL1->setRowCount(0);
    m_twOutputLL1->setColumnCount(4);
    m_twOutputLL1->setHorizontalHeaderItem(0, new QTableWidgetItem("分析栈"));
    m_twOutputLL1->setHorizontalHeaderItem(1, new QTableWidgetItem("剩余输入串"));
    m_twOutputLL1->setHorizontalHeaderItem(2, new QTableWidgetItem("所用产生式"));
    m_twOutputLL1->setHorizontalHeaderItem(3, new QTableWidgetItem("动作"));
}

bool AnalysisWindow::appendToSet(QStringList& set, const QString& symbol) {
    bool _isChanged = false;
    if (!set.contains(symbol)) {
        set << symbol;
        _isChanged = true;
    }
    return _isChanged;
}

bool AnalysisWindow::appendToSet(QStringList& theSet, const QStringList& withSet) {
    bool _isChanged = false;
    for (auto iter = withSet.begin(); iter != withSet.end(); iter++) {
        if (!theSet.contains(*iter)) {
            theSet << *iter;
            _isChanged = true;
        }
    }
    return _isChanged;
}
