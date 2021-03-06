#include "PassageMainWindow.h"

#include <QDesktopWidget>
#include <QApplication>
#include <QAction>
#include <QToolBar>
#include <QMenu>
#include <QWidget>
#include <QMenuBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QColorDialog>
#include <QFileDialog>
#include <QDebug>
#include <QPrintDialog>
#include <QAbstractPrintDialog>
#include <QPrinter>
#include <QPainter>

#include <QPalette>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QDir>

#include "Doc/DocPassage.h"
#include "ActionConnector/ActionConnector.h"
#include "Loaders/ZipTool.h"                // 压缩文件工具
#include "ofd_parser.h"
#include "DataTypes/document/ofd.h"
#include "Convert/OFD_DocConvertor.h"       // OFD 转 Doc 工具
#include "Doc/DocPage.h"
#include "Doc/DocTextBlock.h"
#include "Doc/DocImageBlock.h"
#include "Widget/FindAndReplaceDock.h"
#include "Convert/Doc_OFDConvertor.h"
#include "ofd_writer.h"
#include "Widget/SelectTemplateDialog.h"
#include "Tool/UnitTool.h"
#include "Core/GlobalSetting.h"
#include "DataTypes/document/CT_DocInfo.h"
#include "Doc/DocTable.h"

#include "Settings/RecentFileList.h"
#include "Settings/RecentFileItem.h"
#include "ui/RecentFiles.h"


PassageMainWindow* PassageMainWindow::m_instance = NULL;

PassageMainWindow *PassageMainWindow::getInstance()
{
    if(m_instance == NULL)
    {
        m_instance = new PassageMainWindow();
    }

    return m_instance;
}

PassageMainWindow::PassageMainWindow(QWidget *parent)
    :QMainWindow(parent)
{
    m_instance = this;
    init();
}

PassageMainWindow::~PassageMainWindow()
{

}

///
/// \brief PassageMainWindow::activePassage
///     获得当前被激活的文章
/// \return
///
DocPassage *PassageMainWindow::activedPassage()
{
    QWidget* widget = this->tabArea->currentWidget();
    if(widget == 0)
    {
        // 如果返回空指针
        return NULL;
    }

    return qobject_cast<DocPassage*>(widget);
}

///
/// \brief PassageMainWindow::activateFindAndReplaceDock
///     激活查找替换窗口
///
void PassageMainWindow::activateFindAndReplaceDock()
{
    if (connector->getActivePassage())
    {
        FindAndReplaceDock* find_and_replace_dock
                = FindAndReplaceDock::getInstance();    // 获得单例
        find_and_replace_dock->setCurrentPassage(
                    this->activedPassage());     // 设置当前操作的文章
        find_and_replace_dock->show();
        find_and_replace_dock->update();
        find_and_replace_dock->setBackgroundRole(QPalette::Window);
    }
}

/**
 * @Author Chaoqun
 * @brief  初始化窗口
 * @param  void
 * @return void
 * @date   2017/05/13
 */
void PassageMainWindow::init()
{

    this->isEditable = true;

    /// 标签页
    this->tabArea = new QTabWidget();
    this->setCentralWidget(this->tabArea);
    this->tabArea->setTabsClosable(true);               // 允许关闭标签页
    this->tabArea->setMovable(true);                    // 允许调整标签页的顺序

    // 最近打开文件
    RecentFiles* recentWidget = RecentFiles::getInstance();

    this->tabArea->addTab(recentWidget,tr("Welcome!"));

    this->connector = new ActionConnector(this);        // 新建连接器

    // 初始化变量
    this->imageBlock = NULL;
    this->textBlock = NULL;
    this->tableBlock = NULL;

    initAction();
    connectAction();

    QLabel * status = new QLabel();
    this->statusBar()->addWidget(status);

    this->setMinimumSize(960,720);
    this->setBackgroundRole(QPalette::Text);

    this->setWindowTitle(tr("OFD Editor"));
    this->setWindowIcon(QIcon(":/icons/source/icons/ofdEditor2.png"));

    // 配置查找替换窗口
    FindAndReplaceDock* find_and_replace_dock
            = FindAndReplaceDock::getInstance();    // 获得单例
    this->addDockWidget(
                Qt::BottomDockWidgetArea,
                find_and_replace_dock);     // 设置容器

    find_and_replace_dock->setWindowIcon(
                QIcon(":/icons/source/icons/Find.png"));    // 设置图标
    find_and_replace_dock->setMaximumHeight(60);
    find_and_replace_dock->setMinimumHeight(60);
    find_and_replace_dock->setVisible(false);

    // 选择模板页面
    this->select_template_dialog = new SelectTemplateDialog(NULL, this);
    this->select_template_dialog->setFixedSize(select_template_dialog->size());
    connect(this->select_template_dialog, SIGNAL(createTemplate(int)),
          this, SLOT(createTemplatePassage(int)));

    // 测试
    RecentFileList * recentfilelist = RecentFileList::getInstance();
    recentfilelist->qDebugFileList();
    recentfilelist->save();
}

/**
 * @Author Chaoqun
 * @brief  初始化动作
 * @param  参数
 * @return 返回值
 * @date   2017/05/13
 */
void PassageMainWindow::initAction()
{
    this->newFileAction = new QAction(tr("New File"),NULL);      // 新建文件
    this->newFileAction->setStatusTip(tr("Create a new ofd file"));
    this->newFileAction->setShortcut(QKeySequence::New);
    this->newFileAction->setIcon(QIcon(":/icons/source/icons/newFile.png"));

    // 模板
    this->templateAction = new QAction(tr("Template"), NULL);   // 新建模板页
    this->templateAction->setStatusTip(tr("Use a template to create a new document"));
    this->templateAction->setIcon(QIcon(":/icons/source/icons/template.png"));

    this->openFileAtcion = new QAction(tr("Open File"),NULL);    // 打开文件
    this->openFileAtcion->setStatusTip(tr("Open an existing ofd file"));
    this->openFileAtcion->setShortcut(QKeySequence::Open);
    this->openFileAtcion->setIcon(QIcon(":/icons/source/icons/openFile.png"));

    this->saveAction = new QAction(tr("Save"),NULL);             // 保存
    this->saveAction->setStatusTip(tr("Save file"));
    this->saveAction->setShortcut(QKeySequence::Save);
    this->saveAction->setIcon(QIcon(":/icons/source/icons/save.png"));

    this->saveAsAction = new QAction(tr("Save as"),NULL);        // 另存为
    this->saveAsAction->setStatusTip(tr("Save as"));
    this->saveAsAction->setShortcut(QKeySequence::SaveAs);
    this->saveAsAction->setIcon(QIcon(":/icons/source/icons/saveAs.png"));

    this->printAction = new QAction(tr("Print"),NULL);       // 打印
    this->printAction->setStatusTip(tr("Print your document"));
    this->printAction->setIcon(QIcon(":/icons/source/icons/print.png"));

    this->attributeAction = new QAction(tr("Attribute"),NULL);      // 文档属性
    this->attributeAction->setStatusTip(tr("Show you the attribute of the actived passage"));

    // 程序首选项
    this->softwareSettingAction = new QAction(tr("Setting"),NULL);  // setting
    this->softwareSettingAction->setStatusTip(tr("Set the software's preference"));
    this->softwareSettingAction->setIcon(QIcon(":/icons/source/icons/Setting.png"));

    // 编辑模式
    this->editModeAction = new QAction(tr("EditMode"), NULL);
    this->editModeAction->setStatusTip(tr("change current state to edit mode"));
    this->editModeAction->setIcon(QIcon(":/icons/source/icons/EditMode.png"));
    this->editModeAction->setCheckable(true);
    this->editModeAction->setChecked(true);

    // 阅读模式
    this->viewModeAction = new QAction(tr("ViewMode"),NULL);
    this->viewModeAction->setStatusTip(tr("change current state to edit mode"));
    this->viewModeAction->setIcon(QIcon(":/icons/source/icons/ViewMode.png"));
    this->viewModeAction->setCheckable(true);

    // 放大
    this->zoomInAction = new QAction(tr("ZoomIn"),NULL);
    this->zoomInAction->setIcon(QIcon(":/icons/source/icons/ZoomIn.png"));

    // 缩小
    this->zoomOutAction = new QAction(tr("ZoomOut"), NULL);
    this->zoomOutAction->setIcon(QIcon(":/icons/source/icons/ZoomOut.png"));

    this->undoAction = new QAction(tr("Undo"),NULL);             // 撤销操作
    this->undoAction->setStatusTip(tr("Undo your last action"));
    this->undoAction->setShortcut(QKeySequence::Undo);
    this->undoAction->setIcon(QIcon(":/icons/source/icons/undo.png"));

    this->redoAction = new QAction(tr("Redo"),NULL);             // 重新操作
    this->redoAction->setStatusTip(tr("Redo the action you undo"));
    this->redoAction->setShortcut(QKeySequence::Redo);
    this->redoAction->setIcon(QIcon(":/icons/source/icons/redo.png"));

    this->copyAction = new QAction(tr("Copy"),NULL);             // 复制文本
    this->copyAction->setStatusTip(tr("Copy the content you selected"));
    this->copyAction->setShortcut(QKeySequence::Copy);
    this->copyAction->setIcon(QIcon(":/icons/source/icons/copy.png"));

    this->cutAction = new QAction(tr("Cut"),NULL);             // 剪切
    this->cutAction->setStatusTip(tr("Cut the content you selected"));
    this->cutAction->setShortcut(QKeySequence::Cut);
    this->cutAction->setIcon(QIcon(":/icons/source/icons/cut.png"));

    this->pasteAction = new QAction(tr("Paste"),NULL);           // 粘贴
    this->pasteAction->setStatusTip(tr("Paste your pasteboard content"));
    this->pasteAction->setShortcut(QKeySequence::Paste);
    this->pasteAction->setIcon(QIcon(":/icons/source/icons/paste.png"));

    // 查找
    this->find_and_replace = new QAction(tr("Find/Replace"), NULL);     //查找和替换
    this->find_and_replace->setStatusTip(tr("Find specific text or replace them"));
    this->find_and_replace->setShortcut(QKeySequence::Find);
    this->find_and_replace->setIcon(QIcon(":/icons/source/icons/Find.png"));
    //缺少Icon

    this->insertNewPageAction = new QAction(tr("Insert New Page"),NULL);     // 插入新页面
    this->insertNewPageAction->setStatusTip(tr("Insert a new Page into document"));
    this->insertNewPageAction->setIcon(QIcon(":/icons/source/icons/insertNewPage.png"));

    this->insertTextBlockAction = new QAction(tr("Insert TextBlock"),NULL);  // 插入文本框
    this->insertTextBlockAction->setStatusTip(tr("Insert a new TextBlock"));
    this->insertTextBlockAction->setIcon(QIcon(":/icons/source/icons/insertTextBlock.png"));

    this->insertImageAction = new QAction(tr("Insert Image"),NULL);           // 插入图片
    this->insertImageAction->setStatusTip(tr("Insert a image"));
    this->insertImageAction->setIcon(QIcon(":/icons/source/icons/insertImage.png"));

    this->insertTableAction = new QAction(tr("Insert Table"),NULL);          // 插入一个表格
    this->insertTableAction->setStatusTip(tr("Insert a table"));
    this->insertTableAction->setIcon(QIcon(":/icons/source/icons/insertTable.png"));

    this->pageFormat = new QAction(tr("Page Format"), NULL);     // 页面格式
    this->pageFormat->setStatusTip(tr("Set the page format"));
    this->pageFormat->setIcon(QIcon(":/icons/source/icons/pageFormat.png"));

    this->textFormat = new QAction(tr("Text Format"),NULL);      // 文字格式
    this->textFormat->setStatusTip(tr("Set the selected texts' format"));
    this->textFormat->setIcon(QIcon(":/icons/source/icons/TextFormat.png"));

    this->paragraphFormat = new QAction(tr("Paragraph Format"),NULL);    // 段落格式
    this->paragraphFormat->setStatusTip(tr("Set this paragarph format"));
    this->paragraphFormat->setIcon(QIcon(":/icons/source/icons/paragraphFormat.png"));

    this->imageFormat = new QAction(tr("Image Format"),NULL);        // 图片格式
    this->imageFormat->setStatusTip(tr("Set the Selected image's format"));
    this->imageFormat->setIcon(QIcon(":/icons/source/icons/ImageFormat.png"));

    this->tableFormat = new QAction(tr("Table Format"),NULL);    // 表格格式
    this->tableFormat->setStatusTip(tr("Set the selected table's format"));
    this->tableFormat->setIcon(QIcon(":/icons/source/icons/tableFormat.png"));

    this->aboutQtAction = new QAction(tr("about Qt"),NULL);      // 关于QT

    this->aboutAppAction = new QAction(tr("About App"),NULL);    // 关于本应用
    this->aboutAppAction->setStatusTip(tr("About this Application"));
    this->aboutAppAction->setIcon(QIcon(":/icons/source/icons/AboutQpp.png"));

    this->helpAciton = new QAction(tr("Help"),NULL);
    this->helpAciton->setStatusTip(tr("Show the help Window"));
    this->helpAciton->setIcon(QIcon(":/icons/source/icons/help.png"));

    this->filesMenu = this->menuBar()->addMenu(tr("Files"));
    this->editMenu = this->menuBar()->addMenu(tr("Edit"));
    this->formatMenu = this->menuBar()->addMenu(tr("Format"));
    this->insertMenu = this->menuBar()->addMenu(tr("Insert"));
    this->aboutMenu = this->menuBar()->addMenu(tr("About"));

    this->filesMenu->addAction(this->newFileAction);
    this->filesMenu->addAction(this->templateAction);       // 模板
    this->filesMenu->addAction(this->openFileAtcion);
    this->filesMenu->addAction(this->saveAction);
    this->filesMenu->addAction(this->saveAsAction);
    this->filesMenu->addAction(this->printAction);
    this->filesMenu->addAction(this->attributeAction);
    this->filesMenu->addSeparator();
    this->filesMenu->addAction(this->softwareSettingAction);    // 软件设置

    this->editMenu->addAction(this->editModeAction);        // 编辑状态
    this->editMenu->addAction(this->viewModeAction);        // 阅读状态
    this->editMenu->addSeparator();
    this->editMenu->addAction(this->zoomInAction);          // 放大
    this->editMenu->addAction(this->zoomOutAction);         // 缩小
    this->editMenu->addSeparator();
    this->editMenu->addAction(this->undoAction);
    this->editMenu->addAction(this->redoAction);
    this->editMenu->addSeparator();     // 分割线
    this->editMenu->addAction(this->copyAction);
    this->editMenu->addAction(this->cutAction);
    this->editMenu->addAction(this->pasteAction);
    this->editMenu->addAction(this->find_and_replace);
    this->editMenu->addSeparator();


    this->formatMenu->addAction(this->textFormat);
    this->formatMenu->addAction(this->paragraphFormat);
    this->formatMenu->addAction(this->imageFormat);
    this->formatMenu->addAction(this->tableFormat);
    this->formatMenu->addAction(this->pageFormat);

    this->insertMenu->addAction(this->insertNewPageAction);
    this->insertMenu->addAction(this->insertTextBlockAction);
    this->insertMenu->addAction(this->insertImageAction);
    this->insertMenu->addAction(this->insertTableAction);

    this->aboutMenu->addAction(this->aboutQtAction);
    this->aboutMenu->addAction(this->aboutAppAction);
    this->aboutMenu->addAction(this->helpAciton);

    // 文本操作工具栏部分/
    this->fontCombox = new QFontComboBox();
    this->fontCombox->setEditable(false);
    this->fontSizeCombox = new QComboBox();
    this->fontSizeCombox->insertItem(0,tr("72"));
    this->fontSizeCombox->insertItem(0,tr("48"));
    this->fontSizeCombox->insertItem(0,tr("36"));
    this->fontSizeCombox->insertItem(0,tr("28"));
    this->fontSizeCombox->insertItem(0,tr("26"));
    this->fontSizeCombox->insertItem(0,tr("24"));
    this->fontSizeCombox->insertItem(0,tr("22"));
    this->fontSizeCombox->insertItem(0,tr("20"));
    this->fontSizeCombox->insertItem(0,tr("18"));
    this->fontSizeCombox->insertItem(0,tr("16"));
    this->fontSizeCombox->insertItem(0,tr("14"));
    this->fontSizeCombox->insertItem(0,tr("12"));
    this->fontSizeCombox->insertItem(0,tr("11"));
    this->fontSizeCombox->insertItem(0,tr("10.5"));
    this->fontSizeCombox->insertItem(0,tr("10"));
    this->fontSizeCombox->insertItem(0,tr("9"));
    this->fontSizeCombox->insertItem(0,tr("8"));
    this->fontSizeCombox->insertItem(0,tr("7.5"));
    this->fontSizeCombox->insertItem(0,tr("6.5"));
    this->fontSizeCombox->insertItem(0,tr("5.5"));
    this->fontSizeCombox->insertItem(0,tr("5"));
    this->fontSizeCombox->insertItem(0,tr("Size 8"));
    this->fontSizeCombox->insertItem(0,tr("Size 7"));
    this->fontSizeCombox->insertItem(0,tr("Size 6 Minor"));
    this->fontSizeCombox->insertItem(0,tr("Size 6"));
    this->fontSizeCombox->insertItem(0,tr("Size 5 Minor"));
    this->fontSizeCombox->insertItem(0,tr("Size 5"));
    this->fontSizeCombox->insertItem(0,tr("Size 4 Minor"));
    this->fontSizeCombox->insertItem(0,tr("Size 4"));
    this->fontSizeCombox->insertItem(0,tr("Size 3 Minor"));
    this->fontSizeCombox->insertItem(0,tr("Size 3"));
    this->fontSizeCombox->insertItem(0,tr("Size 2 Minor"));
    this->fontSizeCombox->insertItem(0,tr("Size 2"));
    this->fontSizeCombox->insertItem(0,tr("Size 1 Minor"));
    this->fontSizeCombox->insertItem(0,tr("Size 1"));
    this->fontSizeCombox->insertItem(0,tr("Prime Minor"));
    this->fontSizeCombox->insertItem(0,tr("Prime"));



    // 加粗
    this->boldAction = new QAction(tr("Bold"), NULL);
    this->boldAction->setStatusTip(tr("Set selected text Bold or not bold"));
    this->boldAction->setIcon(QIcon(":/icons/source/icons/bold.png"));
    this->boldAction->setCheckable(true);

    // 倾斜
    this->italicAction = new QAction(tr("Italic"), NULL);
    this->italicAction->setStatusTip(tr("Set the selected text Italic"));
    this->italicAction->setIcon(QIcon(":/icons/source/icons/italic.png"));
    this->italicAction->setCheckable(true);

    // 下划线
    this->underlineAction = new QAction(tr("Underline" ),NULL);
    this->underlineAction->setStatusTip(
                tr("Set the selected text underline"));
    this->underlineAction->setIcon(QIcon(":/icons/source/icons/underline.png"));
    this->underlineAction->setCheckable(true);

    // 居中
    this->middleAction = new QAction(tr("middle"),NULL);
    this->middleAction->setStatusTip(
                tr("Set the selected paragraph align middle"));
    this->middleAction->setIcon(QIcon(":/icons/source/icons/middle.png"));
    this->middleAction->setCheckable(true);

    // 左对齐
    this->leftAction = new QAction(tr("Left"),NULL);
    this->leftAction->setStatusTip(
                tr("Set the selected paragraph align by left"));
    this->leftAction->setIcon(QIcon(":/icons/source/icons/left.png"));
    this->leftAction->setCheckable(true);


    // 右对齐
    this->rightAction = new QAction(tr("Right"),NULL);
    this->rightAction->setStatusTip(
                tr("Set the selected paragraph align by right"));
    this->rightAction->setIcon(QIcon(":/icons/source/icons/right.png"));
    this->rightAction->setCheckable(true);

    // 两端对齐
    this->justifyAction = new QAction(tr("jutify"),NULL);
    this->justifyAction->setStatusTip(
                tr("Set the selected paragraph align by left and right"));
    this->justifyAction->setIcon(QIcon(":/icons/source/icons/justify.png"));
    this->justifyAction->setCheckable(true);

    // 添加成组
    this->alignGroup = new QActionGroup(0);
    this->alignGroup->addAction(this->middleAction);
    this->alignGroup->addAction(this->leftAction);
    this->alignGroup->addAction(this->rightAction);
    this->alignGroup->addAction(this->justifyAction);

/*---------------------------------------------------------------------*/
    this->file_toolBar = this->addToolBar(tr("File"));
    this->file_toolBar->setMovable(false);
    this->edit_toolBar = this->addToolBar(tr("Edit"));
    this->edit_toolBar->setMovable(false);
    this->format_toolBar = this->addToolBar(tr("Format"));
    this->format_toolBar->setMovable(false);
    this->insert_toolBar = this->addToolBar(tr("Insert"));
    this->insert_toolBar->setMovable(false);

    this->file_toolBar->addAction(this->newFileAction);
    this->file_toolBar->addAction(this->openFileAtcion);
    this->file_toolBar->addAction(this->saveAction);

    this->edit_toolBar->addAction(this->editModeAction);
    this->edit_toolBar->addAction(this->viewModeAction);
    this->edit_toolBar->addSeparator();
    this->edit_toolBar->addAction(this->undoAction);
    this->edit_toolBar->addAction(this->redoAction);
    this->edit_toolBar->addSeparator();
    this->edit_toolBar->addAction(this->zoomInAction);
    this->edit_toolBar->addAction(this->zoomOutAction);
    this->edit_toolBar->addSeparator();
    this->edit_toolBar->addAction(this->find_and_replace);

    this->format_toolBar->addAction(this->textFormat);
    this->format_toolBar->addAction(this->paragraphFormat);
    this->format_toolBar->addAction(this->imageFormat);
    this->format_toolBar->addAction(this->tableFormat);

    this->insert_toolBar->addAction(this->insertNewPageAction);
    this->insert_toolBar->addAction(this->insertTextBlockAction);
    this->insert_toolBar->addAction(this->insertImageAction);
    this->insert_toolBar->addAction(this->insertTableAction);


    // 添加工具栏
    this->textBlock_toolBar = this->addToolBar(tr("TextBlock"));
    this->textBlock_toolBar->setMovable(false);
    this->textBlock_toolBar->addWidget(this->fontCombox);           // 字体选择
//    this->textBlock_toolBar->addWidget(this->fontSizeCombox);       // 字体大小设置
    this->textBlock_toolBar->addSeparator();                        // 分隔符
    this->textBlock_toolBar->addAction(this->boldAction);           // 字体加粗
    this->textBlock_toolBar->addAction(this->italicAction);         // 斜体
    this->textBlock_toolBar->addAction(this->underlineAction);      // 下划线
    this->textBlock_toolBar->addSeparator();                        // 分隔符
    this->textBlock_toolBar->addAction(this->leftAction);           // 左对齐
    this->textBlock_toolBar->addAction(this->middleAction);         // 居中
    this->textBlock_toolBar->addAction(this->rightAction);          // 居右
    this->textBlock_toolBar->addAction(this->justifyAction);        // 两端对齐

}

/**
 * @Author Chaoqun
 * @brief  为QActions链接响应事件
 * @param  参数
 * @return 返回值
 * @date   2017/05/13
 */
void PassageMainWindow::connectAction()
{
    connect(this->newFileAction, SIGNAL(triggered(bool)),
            this,SLOT(createEmptyPassage()));   // 新建窗口

    connect(this->openFileAtcion, SIGNAL(triggered(bool)),
            this, SLOT(openFile()));  //打开文件

    connect(this->saveAction,SIGNAL(triggered(bool)),
            this,SLOT(saveFile()));     // 保存文件

    connect(this->saveAsAction, SIGNAL(triggered(bool)),
            this,SLOT(saveFileAs()));

    // 打印文章
    connect(this->printAction, SIGNAL(triggered(bool)),
            this, SLOT(printPassage()));

    connect(this->attributeAction, SIGNAL(triggered(bool)),
            this->connector, SLOT(showAttribute()));        // 显示文档属性

    connect(this->insertNewPageAction, SIGNAL(triggered(bool)),
            this->connector, SLOT(addNewPage()));    // 在文章尾部加入新的一页

//    connect(this->insertTextBlockAction, &QAction::triggered,
//            this->connector, &ActionConnector::addNewBlock);    // 插入新块

    //undo operation
    connect(this->undoAction,SIGNAL(triggered(bool)),
            this,SLOT(undo()));

    //redo operation
    connect(this->redoAction,SIGNAL(triggered(bool)),
            this,SLOT(redo()));

    // 放大
    connect(this->zoomInAction, SIGNAL(triggered(bool)),
            this, SLOT(zoomIn()));

    // 缩小
    connect(this->zoomOutAction, SIGNAL(triggered(bool)),
            this, SLOT(zooomOut()));

    connect(this->find_and_replace, SIGNAL(triggered(bool)),
            this->connector, SLOT(startFindAndReplace()));//查找/替换

    connect(this->templateAction, SIGNAL(triggered(bool)),
            this, SLOT(templateDialog()));      //模板选择

    connect(this->insertTextBlockAction, SIGNAL(triggered()),
            this->connector, SLOT(addTextBlock()));   // 插入文本框

    connect(this->insertImageAction, SIGNAL(triggered()),
            this->connector, SLOT(addImageBlock()));  // 插入图片

    connect(this->insertTableAction, SIGNAL(triggered()),
            this->connector, SLOT(addTableBlock()));  // 插入表格

    connect(this->textFormat,SIGNAL(triggered(bool)),
            this,SLOT(fontDialog()));                 // 修改字体
    connect(this->pageFormat, SIGNAL(triggered(bool)),
            this, SLOT(pageDialog()));

    connect(this->paragraphFormat,SIGNAL(triggered(bool)),
            this,SLOT(paragraphDialog()));            // 修改段落

    connect(this->imageFormat, SIGNAL(triggered(bool)),
            this, SLOT(imageDialog()));                 //修改图片

    // 设置表格
    connect(this->tableFormat, SIGNAL(triggered(bool)),
            this, SLOT(tableSetting()));

    connect(this->boldAction, SIGNAL(triggered(bool)),
            this,SLOT(Bold()));         // 加粗

    connect(this->italicAction, SIGNAL(triggered(bool)),
            this,SLOT(Italic()));       // 斜体

    connect(this->underlineAction, SIGNAL(triggered(bool)),
            this,SLOT(underline()));    // 下划线

    // 切换文章
    connect(this->tabArea, SIGNAL(currentChanged(int)),
            this, SLOT(changeCurrentPassage(int)));

    connect(this, SIGNAL(updateActivedPassage(DocPassage*)),
            this->connector, SLOT(updateActivePassage(DocPassage*)));

    // 关闭文章
    connect(this->tabArea, SIGNAL(tabCloseRequested(int)),
            this, SLOT(closePassageRequest(int)));

    // 切换为编辑模式
    connect(this->editModeAction, SIGNAL(triggered(bool)),
            this, SLOT(switchToEditMode()));

    // 切换为阅读模式
    connect(this->viewModeAction, SIGNAL(triggered(bool)),
            this, SLOT(switchToViewMode()));

    // 字体选择
    connect(this->fontCombox, SIGNAL(currentFontChanged(QFont)),
            this, SLOT(slots_setFont(QFont)));
    // 对其样式
    connect(this->alignGroup, SIGNAL(triggered(QAction*)),
            this, SLOT(slots_selectAlign(QAction*)));
}

/**
 * @Author Chaoqun
 * @brief  解除QAction的所有事件响应
 * @param  void
 * @return 返回值
 * @date   2017/05/13
 */
void PassageMainWindow::disconnectAction()
{

}

void PassageMainWindow::slots_selectAlign(QAction *action)
{
    Qt::Alignment alignment;
    if(action == this->leftAction)
    {
        alignment = Qt::AlignLeft;
    }
    else if(action == this->rightAction)
    {
        alignment = Qt::AlignRight;
    }
    else if(action == this->middleAction)
    {
        alignment = Qt::AlignHCenter;
    }
    else if(action == this->justifyAction)
    {
        alignment = Qt::AlignJustify;
    }

    if(this->textBlock)
    {
        QTextBlockFormat blockFormat;
        blockFormat.setAlignment(alignment);
        this->textBlock->mergeBlockFormatOnBlock(blockFormat);
    }
    else if(this->tableBlock)
    {
        QTextBlockFormat blockFormat;
        blockFormat.setAlignment(alignment);
        this->tableBlock->mergeBlockFormatOnBlock(blockFormat);
    }
}

///
/// \brief PassageMainWindow::slots_setFont
///     快速设定字体
/// \param font
///
void PassageMainWindow::slots_setFont(const QFont &font)
{
    if(this->textBlock != NULL)
    {
        this->textBlock->setFont(font.family());
    }
    else if(this->tableBlock != NULL)
    {
        this->tableBlock->setFont(font.family());
    }
}

/**
 * @Author Chaoqun
 * @brief  找到当前活跃的textblock，加粗
 * @param  void
 * @return void
 * @date   2017/06/27
 */
void PassageMainWindow::Bold()
{
    if(this->textBlock !=NULL)
    {
        this->textBlock->textBold();        // 加粗
    }
}

void PassageMainWindow::Italic()
{
    if(this->textBlock != NULL)
    {
        this->textBlock->textItalic();      // 斜体
    }
}

void PassageMainWindow::underline()
{
    if(this->textBlock != NULL)
    {
        this->textBlock->textUnderline();   // 下划线
    }
}

///
/// \brief PassageMainWindow::printPassage
///     弹出打印机设置
///
void PassageMainWindow::printPassage()
{
    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog printDialog(&printer, this);

    printDialog.setOptions(QAbstractPrintDialog::PrintToFile
                          | QAbstractPrintDialog::PrintSelection
                          | QAbstractPrintDialog::PrintPageRange
                          | QAbstractPrintDialog::PrintShowPageSize
                          | QAbstractPrintDialog::PrintCollateCopies
                          | QAbstractPrintDialog::PrintCurrentPage);

    DocPassage* passage = this->activedPassage();
    printDialog.setMinMax(1,passage->pageCount());

    if(printDialog.exec() == QDialog::Accepted)
    {
        // 接受打印机设置 -- 全部打印
        printer.setPageSize(QPrinter::A4);
        QPainter painterPixmap;
        painterPixmap.begin(&printer);

        for(int i =0; i < passage->pageCount(); i++)
        {
            QPixmap pixmap = QPixmap::grabWidget(
                        passage->getPage(i));

            // 调整比例
            QRect rect = painterPixmap.viewport();
            QSize size = passage->getPage(i)->size();
            size.scale(rect.size(), Qt::KeepAspectRatio);     //此处保证图片显示完整
            painterPixmap.setViewport(rect.x(), rect.y(),
                                size.width(), size.height());
            painterPixmap.setWindow(passage->getPage(i)->rect());
            painterPixmap.drawPixmap(0,0,pixmap);

            // 防止多增加一页
            if(i != passage->pageCount() - 1)
                printer.newPage();

        }

        painterPixmap.end();

    }
}

void PassageMainWindow::undo()
{
    if(this->textBlock != NULL)
    {
        this->textBlock->undo();
    }
    else if(this->tableBlock != NULL)
    {
        this->tableBlock->undo();
    }
}

void PassageMainWindow::redo()
{

    if(this->textBlock != NULL)
    {
        this->textBlock->redo();
    }
    else if(this->tableBlock != NULL)
    {
        this->tableBlock->redo();
    }
}

/**
 * @Author Chaoqun
 * @brief  放大函数
 * @param  void
 * @return void
 * @date   2017/06/27
 */
void PassageMainWindow::zoomIn()
{
    DocPassage* passage = this->activedPassage();
    passage->zoomIn();
}

/**
 * @Author Chaoqun
 * @brief  缩小函数/
 * @param  void
 * @return void
 * @date   2017/06/27
 */
void PassageMainWindow::zooomOut()
{
//    DocPage* page = this->connector->getActivePage();
//    page->scale(0.5,0.5);
    DocPassage* passage = this->activedPassage();
    passage->zoomOut();
}

/**
 * @Author Chaoqun
 * @brief  打开 *.ofd 文件
 * @param  void
 * @return void
 * @date   2017/05/22
 */
void PassageMainWindow::openFile()
{
    QFileDialog * fileDialog = new QFileDialog(this);           // 新建一个QFileDialog
//    fileDialog->setWindowIcon(QIcon(":/icon/source/open.png")); // 设置打开文件图标
    fileDialog->setAcceptMode(QFileDialog::AcceptOpen);         // 设置对话框为打开文件类型
    fileDialog->setFileMode(QFileDialog::ExistingFile);         // 设置文件对话框能够存在的文件
    fileDialog->setViewMode(QFileDialog::Detail);               // 文件以细节形式显示出来
    fileDialog->setNameFilter(tr("JSON files(*.ofd)"));            // 设置文件过滤器
    fileDialog->setWindowTitle(tr("Choose an ofd document file!"));

    if(fileDialog->exec() == QDialog::Accepted)
    {
        QString path = fileDialog->selectedFiles()[0];      // 用户选择文件名
        qDebug() << path;

        this->openFile(path);       // 调用打开文件

    }
}

/**
 * @Author Chaoqun
 * @brief  文件保存测试
 * @param  void
 * @return void
 * @date   2017/06/25
 */
void PassageMainWindow::saveFile()
{

    DocPassage* passage = this->activedPassage();
    if(passage == NULL)
    {
        qDebug() << "Select NULL Passage";
        return;
    }
    QString filePath = passage->getFilePath();      // 获得文件路径
    if(filePath.size() == 0)
    {
        qDebug() << "passage filePath == 0";
        this->saveFileAs();     // 如果文件名不存在则使用另存为

        return;
    }

    passage->updateEditTime();                      // 更新文件修改日期
    this->addRecentFile(passage);                   // 添加到最近打开的文件列表

    Doc_OFDConvertor docToOfd;
    OFD* ofdFile = docToOfd.doc_to_ofd(passage);

    QString tempPath = passage->getTempStorePath();

    QDir dir;
    if(dir.exists(tempPath))
    {
        qDebug() << "the file is existing";
        // 如果文件夹已存在则要删除该文件夹
        ZipTool::deleteFolder(tempPath);                  // 删除文件夹
    }
    dir.mkdir(tempPath);                          // 生成文件夹
    OFDWriter writer(ofdFile, tempPath+"/");      // 写出文件
    writer.setTempPath(passage->getTempSavePath());         // 保存着图片的临时路径
    writer.writeOFD();

    ZipTool::compressDir(filePath,
                         tempPath);               // 将文件夹压缩为指定文件

}

///
/// \brief PassageMainWindow::saveFileAs
///     另存为文件
///     先判断文件是否可以
///     弹出文件对话框
///     保存文件
///
void PassageMainWindow::saveFileAs()
{
    // 检查文档
    DocPassage* passage = this->connector->getActivePassage();
    if(passage == NULL)
    {
        qDebug() << "Select NULL Passage";
        return;
    }
    passage->updateEditTime();

    // 弹出保存文件对话框
    QFileDialog * fileDialog = new QFileDialog(this);           // 新建一个QFileDialog

    fileDialog->setAcceptMode(QFileDialog::AcceptSave);         // 设置对话框为保存文件类型
    fileDialog->setFileMode(QFileDialog::AnyFile);              // 设置文件对话框能够显示任何文件
    fileDialog->setViewMode(QFileDialog::Detail);               // 文件以细节形式显示出来
    fileDialog->setWindowTitle(tr("Save the passage content as a ofd file"));

    fileDialog->setNameFilter(tr("OFD files(*.ofd)"));            // 设置文件过滤器
    if(fileDialog->exec() == QDialog::Accepted)
    {
        QString path = fileDialog->selectedFiles()[0];      // 用户选择文件名

        Doc_OFDConvertor docToOfd;
        OFD* ofdFile = docToOfd.doc_to_ofd(passage);

        QString tempPath = passage->getTempStorePath();
        QDir dir;
        if(dir.exists(tempPath))
        {
            qDebug() << "the file is existing";
            // 如果文件夹已存在则要删除该文件夹
            ZipTool::deleteFolder(tempPath);                  // 删除文件夹
        }

        dir.mkdir(tempPath);                                  // 生成文件夹
        OFDWriter writer(ofdFile, tempPath + "/");            // 写出文件
        writer.setTempPath(passage->getTempSavePath());
        writer.writeOFD();

        qDebug() << "temp Files Path:" << tempPath;
        qDebug() << "selected ofd"<< path;

        bool flag = ZipTool::compressDir(path,
                             tempPath, false);                 // 将文件夹压缩为指定文件

        if(flag == true)
        {
            passage->setFilePath(path);                         // 设置文件路径
            this->addRecentFile(passage);                       // 添加到最近文件
        }
        else
        {
            qDebug() << "Save File Failed";
        }
    }

}

/**
 * @Author Chaoqun
 * @brief  打开字体框
 * @param  void
 * @return void
 * @date   2017/06/23
 */
void PassageMainWindow::fontDialog()
{
    if (textBlock)
        this->textBlock->customFontDialog();    // 用自定义窗口修改字体
    else if(this->tableBlock)
        this->tableBlock->customFontDialog();
}

/**
 * @Author Chaoqun
 * @brief  用自定义窗口修改段落格式
 * @param  void
 * @return void
 * @date   2017/06/23
 */
void PassageMainWindow::paragraphDialog()
{
    if (textBlock)
        this->textBlock->textParagraph();       // 用自定义段落窗口修改段落
    else if(this->tableBlock)
        this->tableBlock->customFontDialog();
}

void PassageMainWindow::imageDialog()
{
    if (imageBlock)
        this->imageBlock->setImageProperties();
}

void PassageMainWindow::pageDialog()
{
    DocPassage * passage = activedPassage();
    if (passage)
    {
        passage->activatePageDialog();
    }
    else
    {
        qDebug() << "No active DocPassage.";
    }
}

void PassageMainWindow::templateDialog()
{
    this->select_template_dialog->exec();
}

void PassageMainWindow::tableSetting()
{
    if(this->tableBlock != NULL)
    {
        this->tableBlock->tableSetting();
    }
}

/**
 * @Author Chaoqun
 * @brief  接受当前处理的文字块的更新
 * @param  DocTextBlock *textBlock
 * @return void
 * @date   2017/06/23
 */
void PassageMainWindow::acceptTextBlock(DocTextBlock *textBlock)
{
    this->textBlock = textBlock;        // 修改引用
    this->imageBlock = NULL;
    this->tableBlock = NULL;
}

/**
 * @Author Chaoqun
 * @brief  接受当前处理的块格式
 * @param  QTextBlockFormat &blockFormat
 * @return void
 * @date   2017/06/23
 */
void PassageMainWindow::acceptTextBlockFormat(QTextBlockFormat blockFormat)
{
    this->_currentBlockFormat = blockFormat;   // 留下引用
    qDebug() << "PassageMainWindow::acceptTextBlockFormat(QTextBlockFormat blockFormat)";

    // 更新界面显示
    Qt::Alignment align =  blockFormat.alignment() & Qt::AlignHorizontal_Mask;
    switch (align)
    {
    case Qt::AlignHCenter:
        this->middleAction->setChecked(true);
        this->leftAction->setChecked(false);
        this->rightAction->setChecked(false);
        this->justifyAction->setChecked(false);
        break;
    case Qt::AlignLeft:
        this->middleAction->setChecked(false);
        this->leftAction->setChecked(true);
        this->rightAction->setChecked(false);
        this->justifyAction->setChecked(false);
        break;
    case Qt::AlignRight:
        this->middleAction->setChecked(false);
        this->leftAction->setChecked(false);
        this->rightAction->setChecked(true);
        this->justifyAction->setChecked(false);
        break;
    case Qt::AlignJustify:
        this->middleAction->setChecked(false);
        this->leftAction->setChecked(false);
        this->rightAction->setChecked(false);
        this->justifyAction->setChecked(true);
        break;
    default:
        break;
    }


}

/**
 * @Author Chaoqun
 * @brief  接受字符格式
 * @param  QTextCharFormat &charFormat
 * @return void
 * @date   2017/06/23
 */
void PassageMainWindow::acceptTextCharFormat(QTextCharFormat charFormat)
{
    this->_currentCharFormat  = charFormat;    // 留下引用
    // 更新界面显示

    // 字体
    this->fontCombox->setCurrentFont(charFormat.font());

    // 粗体
    if(charFormat.fontWeight() >= 75)
    {
        this->boldAction->setChecked(true);
    }
    else
    {
        this->boldAction->setChecked(false);
    }

    // 斜体
    if(charFormat.fontItalic())
    {
        this->italicAction->setChecked(true);
    }
    else
    {
        this->italicAction->setChecked(false);
    }

    // 下划线
    if(charFormat.fontUnderline())
    {
        this->underlineAction->setChecked(true);
    }
    else
    {
        this->underlineAction->setChecked(false);
    }

    // 字号
}

void PassageMainWindow::acceptImageBlock(DocImageBlock *imageBlock)
{
    this->imageBlock = imageBlock;
    this->textBlock = NULL;
    this->tableBlock = NULL;
}

void PassageMainWindow::acceptTableBlock(DocTable *table)
{
    this->tableBlock = table;
    this->textBlock = NULL;
    this->imageBlock = NULL;
}

void PassageMainWindow::switchToEditMode()
{
    this->viewModeAction->setChecked(false);
    this->editModeAction->setChecked(true);

    this->isEditable = true;
    emit this->setEditable(this->isEditable);
}

void PassageMainWindow::switchToViewMode()
{
    this->editModeAction->setChecked(false);
    this->viewModeAction->setChecked(true);

    this->isEditable = false;
    emit this->setEditable(this->isEditable);
}

///
/// \brief PassageMainWindow::changeCurrentPassage
///     切换当前操作文档
/// \param index
///
void PassageMainWindow::changeCurrentPassage(int index)
{
    qDebug() << "index " << index;
    if(index == -1)
    {
        // 如果没有则返回 NULL
        emit updateActivedPassage(NULL);
        return;
    }

    emit updateActivedPassage(
                qobject_cast<DocPassage *>(
                    this->tabArea->widget(index)));

    qobject_cast<DocPassage *>(
        this->tabArea->widget(index))->testMessage();
}

///
/// \brief PassageMainWindow::closePassageRequest
///     关闭文件时需要处理的事情：
///     1. 是否需要提示（保存文件）
///     2. 是否需要更新最近打开文档
///     3. 只有全部处理好的，才可以关闭该选项
/// \param index
///
void PassageMainWindow::closePassageRequest(int index)
{


    this->tabArea->removeTab(index);

    if(this->tabArea->count() == 0)
    {
        RecentFiles* recentWidget = RecentFiles::getInstance();
        recentWidget->init();
        this->tabArea->addTab(recentWidget,tr("Welcome!"));
    }
}

///
/// \brief PassageMainWindow::setStatusMessage
/// \param msg
///     在状态栏中显示信息
///
void PassageMainWindow::setStatusMessage(QString msg)
{

}

void PassageMainWindow::createTemplatePassage(int index)
{
    qDebug() << "???";
    DocPassage * new_passage = new DocPassage();
    //**********************************************
    DocPage * new_page_1 = new DocPage();

    new_passage->addPage(new_page_1);
    //***********************************************
    QFont font1("SimHei", 15);
    font1.setBold(false);
    DocTextBlock * headtext_1 = new DocTextBlock();
    DocBlock * head_1 = new DocBlock();
    head_1->setWidget(headtext_1);
    headtext_1->setContent("000001");
    head_1->setPos(UnitTool::mmToPixel(28), UnitTool::mmToPixel(37));
    head_1->resize(80, 30);
    headtext_1->setFont(font1);
    new_page_1->addBlock(head_1, DocPage::Body);

    DocTextBlock * headtext_2 = new DocTextBlock();
    DocBlock * head_2 = new DocBlock();
    head_2->setWidget(headtext_2);
    headtext_2->setContent(tr("JiMi 1Nian"));
    head_2->setPos(UnitTool::mmToPixel(28), UnitTool::mmToPixel(47));
    head_2->resize(120, 30);
    headtext_2->setFont(font1);
    new_page_1->addBlock(head_2, DocPage::Body);

    DocTextBlock * headtext_3 = new DocTextBlock();
    DocBlock * head_3 = new DocBlock();
    head_3->setWidget(headtext_3);
    headtext_3->setContent(tr("Te Ji"));
    head_3->setPos(UnitTool::mmToPixel(28), UnitTool::mmToPixel(57));
    head_3->resize(50, 30);
    headtext_3->setFont(font1);
    new_page_1->addBlock(head_3, DocPage::Body);

    QFont font2("SimSun", 43);
    font2.setBold(true);
    DocTextBlock * titletext_1 = new DocTextBlock();
    DocBlock * title_1 = new DocBlock();
    title_1->setWidget(titletext_1);

    titletext_1->setAlignment(Qt::AlignCenter);
    title_1->setPos(UnitTool::mmToPixel(41), UnitTool::mmToPixel(73));
    title_1->resize(500, 80);
    titletext_1->setFont(font2);
    QTextCharFormat fmt;
    fmt.setForeground(Qt::red);
    titletext_1->mergeCurrentCharFormat(fmt);
    titletext_1->setContent(tr("XXXXXWenJian"));
    new_page_1->addBlock(title_1, DocPage::Body);

    QFont font3("FangSong", 15);
    font3.setBold(false);
    DocTextBlock * titletext_2 = new DocTextBlock();
    DocBlock * title_2 = new DocBlock();
    title_2->setWidget(titletext_2);

    titletext_2->setAlignment(Qt::AlignCenter);
    title_2->setPos(UnitTool::mmToPixel(75), UnitTool::mmToPixel(104));
    title_2->resize(220, 35);
    titletext_2->setFont(font3);

    titletext_2->setContent(tr("XXX[2012]10Hao"));
    new_page_1->addBlock(title_2, DocPage::Body);

    QFont font4("SimSun", 27);
    font4.setBold(false);
    DocTextBlock * bodytext_1 = new DocTextBlock();
    DocBlock * body_1 = new DocBlock();
    body_1->setWidget(bodytext_1);

    bodytext_1->setAlignment(Qt::AlignCenter);
    body_1->setPos(UnitTool::mmToPixel(40), UnitTool::mmToPixel(128));
    body_1->resize(500, 50);
    bodytext_1->setFont(font4);

    bodytext_1->setContent(tr("XXXXXGuanYuXXXXXXDeTongZhi"));
    new_page_1->addBlock(body_1, DocPage::Body);

    QFont font5("FangSong", 22);
    font5.setBold(false);
    DocTextBlock * bodytext_2 = new DocTextBlock();
    DocBlock * body_2 = new DocBlock();
    body_2->setWidget(bodytext_2);

    bodytext_2->setAlignment(Qt::AlignLeft);
    body_2->setPos(UnitTool::mmToPixel(28), UnitTool::mmToPixel(150));
    body_2->resize(250, 35);
    bodytext_2->setFont(font5);

    bodytext_2->setContent(tr("XXXXXXXX:"));
    new_page_1->addBlock(body_2, DocPage::Body);

    DocTextBlock * bodytext_3 = new DocTextBlock();
    DocBlock * body_3 = new DocBlock();
    body_3->setWidget(bodytext_3);

    bodytext_3->setAlignment(Qt::AlignLeft);
    body_3->setPos(UnitTool::mmToPixel(28), UnitTool::mmToPixel(160));
    body_3->resize(600, 400);
    bodytext_3->setFont(font5);

    bodytext_3->setContent(tr("OOOOXXXXXXXXXXXXXXXXXXXXXX"
                              "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
                              "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX."
                              "\nOOOOXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
                              "XXXXXXXXXXXXXXXXXXX.\nOOOOXXXXXXXXX"
                              "XXXXXXXXXXXXX.\nOOOOXXXXXXXXXXXXXXX"
                              "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
                              "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
    new_page_1->addBlock(body_3, DocPage::Body);
    //***********************************************
    DocPage * new_page_2 = new DocPage();

    new_passage->addPage(new_page_2);
    //***********************************************
    DocTextBlock * bodytext_4 = new DocTextBlock();
    DocBlock * body_4 = new DocBlock();
    body_4->setWidget(bodytext_4);

    bodytext_4->setAlignment(Qt::AlignLeft);
    body_4->setPos(UnitTool::mmToPixel(28), UnitTool::mmToPixel(37));
    body_4->resize(600, 200);
    bodytext_4->setFont(font5);

    bodytext_4->setContent(tr("XXXXXXXXXXXXXXXXXXXXXX"
                              "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX."
                              "\nOOOOXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
                              "XXXXXXXXXXXXXXXXXXX."));
    new_page_2->addBlock(body_4, DocPage::Body);

    DocTextBlock * bodytext_5 = new DocTextBlock();
    DocBlock * body_5 = new DocBlock();
    body_5->setWidget(bodytext_5);

    bodytext_5->setAlignment(Qt::AlignCenter);
    body_5->setPos(UnitTool::mmToPixel(120), UnitTool::mmToPixel(85));
    body_5->resize(200, 35);
    bodytext_5->setFont(font5);

    bodytext_5->setContent(tr("XXXXXXXXXXXX"));
    new_page_2->addBlock(body_5, DocPage::Body);

    QFont font6("FangSong", 16);
    DocTextBlock * bodytext_6 = new DocTextBlock();
    DocBlock * body_6 = new DocBlock();
    body_6->setWidget(bodytext_6);

    bodytext_6->setAlignment(Qt::AlignCenter);
    body_6->setPos(UnitTool::mmToPixel(120), UnitTool::mmToPixel(108));
    body_6->resize(200, 35);
    bodytext_6->setFont(font6);

    bodytext_6->setContent(tr("2012Nian7Yue1Ri"));
    new_page_2->addBlock(body_6, DocPage::Body);

    DocTextBlock * bodytext_7 = new DocTextBlock();
    DocBlock * body_7 = new DocBlock();
    body_7->setWidget(bodytext_7);

    bodytext_7->setAlignment(Qt::AlignCenter);
    body_7->setPos(UnitTool::mmToPixel(34), UnitTool::mmToPixel(108));
    body_7->resize(250, 35);
    bodytext_7->setFont(font5);

    bodytext_7->setContent(tr("(XXXXXX)"));
    new_page_2->addBlock(body_7, DocPage::Body);

    DocTextBlock * endtext_1 = new DocTextBlock();
    DocBlock * end_1 = new DocBlock();
    end_1->setWidget(endtext_1);

    endtext_1->setAlignment(Qt::AlignLeft);
    end_1->setPos(UnitTool::mmToPixel(30), UnitTool::mmToPixel(230));
    end_1->resize(600, 35);
    endtext_1->setFont(font6);

    endtext_1->setContent(tr("ChaoSong:XXXXXXX,XXXXX,XXXXXX"));
    new_page_2->addBlock(end_1, DocPage::Body);

    DocTextBlock * endtext_2 = new DocTextBlock();
    DocBlock * end_2 = new DocBlock();
    end_2->setWidget(endtext_2);

    endtext_2->setAlignment(Qt::AlignLeft);
    end_2->setPos(UnitTool::mmToPixel(30), UnitTool::mmToPixel(240));
    end_2->resize(250, 35);
    endtext_2->setFont(font6);

    endtext_2->setContent(tr("XXXXXXXX"));
    new_page_2->addBlock(end_2, DocPage::Body);

    DocTextBlock * endtext_3 = new DocTextBlock();
    DocBlock * end_3 = new DocBlock();
    end_3->setWidget(endtext_3);

    endtext_3->setAlignment(Qt::AlignCenter);
    end_3->setPos(UnitTool::mmToPixel(110), UnitTool::mmToPixel(240));
    end_3->resize(300, 35);
    endtext_3->setFont(font6);

    endtext_3->setContent(tr("2012Nian7Yue1RiYinFa"));
    new_page_2->addBlock(end_3, DocPage::Body);

    DocImageBlock * line = new DocImageBlock();
    DocBlock * line_block = new DocBlock();
    line_block->setWidget(line);
    line->setPixmap(QPixmap(":/icons/source/red_line.png"));
    line_block->setPos(UnitTool::mmToPixel(28), UnitTool::mmToPixel(114));
    line_block->resize(580, 3);
    new_page_1->addBlock(line_block, DocPage::Body);

    addDocPassage(new_passage);

}

///
/// \brief PassageMainWindow::openFile
///         打开文件
/// \param filePath
///
void PassageMainWindow::openFile(QString filePath)
{

    QFile qfile;
    qfile.setFileName(filePath);
    if(!qfile.exists())
    {
        // 如果文件不存在，则中断程序
        return;
    }

    ZipTool zipTool;
    QString tempPath = zipTool.FilePathToFloderPath(filePath);
    qDebug() << "Temp path is :" << tempPath;

    ZipTool::extractDir(filePath,tempPath);     // 解压到临时文件夹

    // 解读文件
//        OFDParser ofdParser("C:\\Users\\User\\AppData\\Local\\Temp\\%表格.ofd%\\OFD.xml");
    OFDParser ofdParser(tempPath + "/OFD.xml");      // 新建临时路径
//        OFDParser ofdParser("C:/Users/User/Desktop/表格/OFD.xml");
    OFD* data = ofdParser.getData();    // 读取出OFD文件
    qDebug()<< "ofd file open" << tempPath + "/OFD.xml";
    OFD_DocConvertor convert;
    DocPassage* passage = convert.ofd_to_doc(data);
    passage->setFilePath(filePath);         // 设置文件路径

    this->addRecentFile(passage);       // 添加到最近文件
    this->addDocPassage(passage);       // 添加文章
}

///
/// \brief PassageMainWindow::closeEvent
///     关闭时要检查是否有文档需要保存，有需要保存的时候，要终止关闭窗口
///     1.关闭可以关闭的文档
///     2.一个一个提醒还没有保存的文档
///     3.全部完成后才可以关闭窗口
/// \param event
///
void PassageMainWindow::closeEvent(QCloseEvent *event)
{

}

///
/// \brief PassageMainWindow::createEmptyPassage
///     创建一个空的文章
/// \return
///
DocPassage *PassageMainWindow::createEmptyPassage()
{
    DocPassage* child = new DocPassage();           // 新建文章
    child->addPage(new DocPage());                  // 增加空白页面

    this->addDocPassage(child);                     // 添加页面
    return child;
}

/**
 * @Author Chaoqun
 * @brief  添加一个文章
 * @param  DocPassage *passage
 * @return DocPassage *
 * @date   2017/05/23
 */
DocPassage *PassageMainWindow::addDocPassage(DocPassage *passage)
{
    if(passage == NULL)     // 如果参数为NULL 不执行并返回NULL
    {
        qDebug() << "DocPassage Pointer is NULL.";
        return NULL;
    }

    this->tabArea->addTab(
                passage,
                passage->getFilePath().section('/',-1));
    this->tabArea->setCurrentIndex(
                this->tabArea->count() - 1);
    this->connector->setDocPassage(passage);    // 设置引用

    passage->setVisible(true);            // 设置可见
    passage->showMaximized();

    // 处理变更的blockFormat
    this->connect(passage,SIGNAL(signals_currentBlockFormatChanged(QTextBlockFormat)),
                  this,SLOT(acceptTextBlockFormat(QTextBlockFormat)));
    // 处理变更的charFormat
    this->connect(passage,SIGNAL(signals_currentCharFormatChanged(QTextCharFormat)),
                  this,SLOT(acceptTextCharFormat(QTextCharFormat)));
    // 处理变更的textBlock
    this->connect(passage,SIGNAL(signals_currentTextBlock(DocTextBlock*)),
                  this,SLOT(acceptTextBlock(DocTextBlock*)));
    //处理变更的imageBlock
    this->connect(passage, SIGNAL(signals_currentImageBlock(DocImageBlock*)),
                  this, SLOT(acceptImageBlock(DocImageBlock*)));

    // 转发tableBlock
    this->connect(passage, SIGNAL(signals_currentTableBlock(DocTable*)),
                  this, SLOT(acceptTableBlock(DocTable*)));

    this->connect(this, SIGNAL(setEditable(bool)),
                  passage, SIGNAL(signals_setEditable(bool)));  // 转发设置可以编辑

    emit this->setEditable(this->isEditable);

    return passage;
}

///
/// \brief PassageMainWindow::disableButtons
///     禁用掉无关按钮，适用于尚未打开文件的状态
///
void PassageMainWindow::disableButtons()
{

}

///
/// \brief PassageMainWindow::enableButtons
///     开启所有按钮，适用于已经打开文件的状态
void PassageMainWindow::enableButtons()
{

}

///
/// \brief PassageMainWindow::addRecentFile
///     增加最近文件记录
///     打开文件、保存文件时调用
/// \param passages
///
void PassageMainWindow::addRecentFile(DocPassage *passages)
{

    CT_DocInfo * info = passages->getDocInfo();
    QDateTime date = QDateTime::currentDateTime();

    RecentFileList* list = RecentFileList::getInstance();
    RecentFileItem* item = new RecentFileItem(
                passages->getFilePath().section('/',-1),
                info->getAuthor(),
                info->getModDate(),
                date.toString("yyyy-MM-dd HH:mm:ss"),
                passages->getFilePath()
                );

    list->addItem(item);
    list->save();           // 保存到文件
}

