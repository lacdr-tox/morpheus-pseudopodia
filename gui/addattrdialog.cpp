#include "addattrdialog.h"
#include "config.h"

addNodeDialog::addNodeDialog(nodeController *nodeContr)
{
    this->contr = nodeContr;

    QWidget *lwid = new QWidget(this);
    QWidget *rwid = new QWidget(this);

    trW = new QTreeWidget(lwid);
    trW->header()->setVisible(false);
    trW->setColumnCount(2);
    trW->setColumnWidth(0, 200);
    trW->setRootIsDecorated(false);
    QLabel *label = new QLabel("Select a plugin:", lwid);
    QLabel *lb_docu = new QLabel("Description:", rwid);

	docu_view = new WebViewer(rwid);
	QFrame* docu_frame = new QFrame(rwid);
	docu_frame->setLayout(new QVBoxLayout());
	docu_frame->layout()->addWidget(docu_view);
	docu_frame->setFrameStyle(QFrame::Plain);
	docu_frame->setFrameShape(QFrame::StyledPanel);
	docu_frame->setFrameShadow(QFrame::Shadow::Raised);

    QPushButton *bt_select = new QPushButton("Select", lwid);
    bt_select->setDefault(true);
    QPushButton *bt_cancel = new QPushButton("Cancel", lwid);

    QFont lFont;
    lFont.setFamily( lFont.defaultFamily() );
    lFont.setWeight( QFont::Light );
    lFont.setItalic( true );

	QStringList allChilds = contr->getAddableChilds(true);
	QStringList addableChilds = contr->getAddableChilds(false);
    for(int i = 0; i < allChilds.size(); i++)
    {
        QTreeWidgetItem* trWItem = new QTreeWidgetItem(trW, QStringList() << allChilds.at(i)  << contr->childInformation(allChilds.at(i)).type->pluginClass);
		if (! addableChilds.contains(allChilds.at(i))) {
			trWItem->setIcon(0,style()->standardIcon(QStyle::SP_MessageBoxWarning));
			trWItem->setToolTip(0,"This node will disable an existing node!");
		}
        trWItem->setFont(1, lFont);
    }

    trW->sortByColumn(1, Qt::AscendingOrder);

    QGridLayout *gl = new QGridLayout(lwid);
    gl->addWidget(label, 0, 0, 1, 3);
    gl->addWidget(trW, 1, 0, 1, 3);
    gl->addWidget(bt_select, 2, 0, 1, 1);
    gl->addWidget(bt_cancel, 2, 1, 1, 1);
    gl->setColumnStretch(0, 0);
    gl->setColumnStretch(1, 0);

    QVBoxLayout *vl = new QVBoxLayout(rwid);
	lb_docu->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    vl->addWidget(lb_docu);
    vl->addWidget(docu_frame);

    splitter = new QSplitter(this);
    splitter->addWidget(lwid);
    splitter->addWidget(rwid);
	splitter->setStretchFactor(0,1);
	splitter->setStretchFactor(1,4);

    QHBoxLayout *lay = new QHBoxLayout(this);
    lay->addWidget(splitter);

    QObject::connect(bt_select, SIGNAL(clicked()), this, SLOT(accept()));
    QObject::connect(bt_cancel, SIGNAL(clicked()), this, SLOT(reject()));
    QObject::connect(trW, SIGNAL(itemSelectionChanged()), this, SLOT(clickedTreeItem()));
    QObject::connect(trW, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(accept()));

    trW->topLevelItem(0)->setSelected(true);
    clickedTreeItem();

    setWindowTitle("Plugins");
    setMinimumSize( 750, 400 );

    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
}

//------------------------------------------------------------------------------

void addNodeDialog::clickedTreeItem()
{
    QTreeWidgetItem *item = trW->selectedItems()[0];
    nodeName = item->text(0);

	QMap <QString, QUrl > identifiers = config::getHelpEngine()->linksForIdentifier(nodeName);
	qDebug() << "Searching for help for " << nodeName;
	if (!identifiers.empty()) {
		docu_view->setUrl(identifiers.begin().value());
	}
}

//------------------------------------------------------------------------------

addNodeDialog::~addNodeDialog()
{delete splitter;}

//------------------------------------------------------------------------------

//void addNodeDialog::keyReleaseEvent(QKeyEvent *kre)
//{
//    if(kre->key() == Qt::Key_Enter || kre->key() == Qt::Key_Return)
//    {setSelectedName();}
//}
