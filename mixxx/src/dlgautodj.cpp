#include <QSqlTableModel>

#include "dlgautodj.h"

#include "controlobject.h"
#include "controlobjectthreadmain.h"
#include "library/playlisttablemodel.h"
#include "library/trackcollection.h"
#include "playerinfo.h"
#include "widget/wskincolor.h"
#include "widget/wtracktableview.h"
#include "widget/wwidget.h"

#define CONFIG_KEY "[Auto DJ]"
const char* kTransitionPreferenceName = "Transition";
const int kTransitionPreferenceDefault = 10;

DlgAutoDJ::DlgAutoDJ(QWidget* parent, ConfigObject<ConfigValue>* pConfig,
                     TrackCollection* pTrackCollection,
                     MixxxKeyboard* pKeyboard, AutoDJ* pAutoDJ)
        : QWidget(parent),
          Ui::DlgAutoDJ(),
          m_pConfig(pConfig),
          m_pTrackCollection(pTrackCollection),
          m_pTrackTableView(
              new WTrackTableView(this, pConfig, m_pTrackCollection)),
          m_playlistDao(pTrackCollection->getPlaylistDAO()),
          m_pAutoDJ(pAutoDJ) {
    setupUi(this);

    m_pTrackTableView->installEventFilter(pKeyboard);
    connect(m_pTrackTableView, SIGNAL(loadTrack(TrackPointer)),
            m_pAutoDJ, SIGNAL(loadTrack(TrackPointer)));
    connect(m_pTrackTableView, SIGNAL(loadTrackToPlayer(TrackPointer, QString)),
            m_pAutoDJ, SIGNAL(loadTrackToPlayer(TrackPointer, QString)));

    QBoxLayout* box = dynamic_cast<QBoxLayout*>(layout());
    Q_ASSERT(box); //Assumes the form layout is a QVBox/QHBoxLayout!
    box->removeWidget(m_pTrackTablePlaceholder);
    m_pTrackTablePlaceholder->hide();
    box->insertWidget(1, m_pTrackTableView);

    m_pAutoDJTableModel = m_pAutoDJ->getTableModel();
    m_pTrackTableView->loadTrackModel(m_pAutoDJTableModel);

    // Override some playlist-view properties:

    // Do not set this because it disables auto-scrolling
    //m_pTrackTableView->setDragDropMode(QAbstractItemView::InternalMove);

    // Disallow sorting.
    m_pTrackTableView->disableSorting();

    pushButtonSkipNext->setEnabled(false);

    connect(spinBoxTransition, SIGNAL(valueChanged(int)),
            m_pAutoDJ, SLOT(transitionValueChanged(int)));
    connect(comboBoxTransition, SIGNAL(currentIndexChanged(int)),
    		m_pAutoDJ, SLOT(transitionSelect(int)));
	switch(comboBoxTransition->currentIndex()) {
		case 0:
			spinBoxTransition->setEnabled(true);
			break;
		case 1:
			spinBoxTransition->setEnabled(true);
			break;
		case 2:
			spinBoxTransition->setEnabled(false);
			break;
	}

    // TODO(smstewart): These COs should be instantiated in AutoDJ

    m_pCOShufflePlaylist = new ControlObjectThreadMain(
    		ControlObject::getControl(ConfigKey("[AutoDJ]", "shuffle_playlist")));
    connect(pushButtonShuffle, SIGNAL(clicked(bool)),
            this, SLOT(shufflePlaylist(bool)));

    m_pCOSkipNext = new ControlObjectThreadMain(
    		ControlObject::getControl(ConfigKey("[AutoDJ]", "skip_next")));
    connect(pushButtonSkipNext, SIGNAL(clicked(bool)),
            this, SLOT(skipNext(bool)));

    m_pCOToggleAutoDJ = new ControlObjectThreadMain(
    		ControlObject::getControl(ConfigKey("[AutoDJ]", "toggle_autodj")));
    connect(pushButtonAutoDJ, SIGNAL(toggled(bool)),
            this, SLOT(toggleAutoDJ(bool))); _blah;
}

DlgAutoDJ::~DlgAutoDJ() {
    qDebug() << "~DlgAutoDJ()";
    delete m_pCOSkipNext;
    delete m_pCOShufflePlaylist;
    delete m_pCOToggleAutoDJ;
    // Delete m_pTrackTableView before the table model. This is because the
    // table view saves the header state using the model.
    delete m_pTrackTableView;
    delete m_pAutoDJTableModel;
}

void DlgAutoDJ::onShow() {
    m_pAutoDJTableModel->select();
}

void DlgAutoDJ::setup(QDomNode node) {
    QPalette pal = palette();

    // Row colors
    if (!WWidget::selectNode(node, "BgColorRowEven").isNull() &&
        !WWidget::selectNode(node, "BgColorRowUneven").isNull()) {
        QColor r1;
        r1.setNamedColor(WWidget::selectNodeQString(node, "BgColorRowEven"));
        r1 = WSkinColor::getCorrectColor(r1);
        QColor r2;
        r2.setNamedColor(WWidget::selectNodeQString(node, "BgColorRowUneven"));
        r2 = WSkinColor::getCorrectColor(r2);

        // For now make text the inverse of the background so it's readable In
        // the future this should be configurable from the skin with this as the
        // fallback option
        QColor text(255 - r1.red(), 255 - r1.green(), 255 - r1.blue());
        QColor fgColor;
        fgColor.setNamedColor(WWidget::selectNodeQString(node, "FgColor"));
        fgColor = WSkinColor::getCorrectColor(fgColor);

        pal.setColor(QPalette::Base, r1);
        pal.setColor(QPalette::AlternateBase, r2);
        pal.setColor(QPalette::Text, text);
        pal.setColor(QPalette::WindowText, fgColor);
    }

    setPalette(pal);

    pushButtonAutoDJ->setPalette(pal);

    // Since we're getting this passed into us already created, shouldn't need
    // to set the palette.
    //m_pTrackTableView->setPalette(pal);
}

void DlgAutoDJ::onSearchStarting() {
}

void DlgAutoDJ::onSearchCleared() {
}

void DlgAutoDJ::onSearch(const QString& text) {
    Q_UNUSED(text);
    // Do not allow filtering the Auto DJ playlist, because
    // Auto DJ will work from the filtered table
}

void DlgAutoDJ::loadSelectedTrack() {
    m_pTrackTableView->loadSelectedTrack();
}

void DlgAutoDJ::loadSelectedTrackToGroup(QString group) {
    m_pTrackTableView->loadSelectedTrackToGroup(group);
}

void DlgAutoDJ::moveSelection(int delta) {
    m_pTrackTableView->moveSelection(delta);
}

void DlgAutoDJ::shufflePlaylist(bool buttonChecked) {
	m_pCOShufflePlaylist->slotSet(0.0);
}

void DlgAutoDJ::skipNext(bool buttonChecked) {
	m_pCOSkipNext->slotSet(0.0);
}

void DlgAutoDJ::fadeNowRight(bool buttonChecked) {

}

void DlgAutoDJ::fadeNowLeft(bool buttonChecked) {

}

void DlgAutoDJ::toggleAutoDJ(bool) {
	qDebug() << "toggle pushed";
	m_pCOToggleAutoDJ->slotSet(!m_pCOToggleAutoDJ->get());
}

void DlgAutoDJ::setAutoDJEnabled() {
	pushButtonAutoDJ->setToolTip(tr("Disable Auto DJ"));
	pushButtonAutoDJ->setText(tr("Disable Auto DJ"));
	pushButtonSkipNext->setEnabled(true);
}

void DlgAutoDJ::setAutoDJDisabled() {
	pushButtonAutoDJ->setToolTip(tr("Enable Auto DJ"));
	pushButtonAutoDJ->setText(tr("Enable Auto DJ"));
	pushButtonSkipNext->setEnabled(false);
}

bool DlgAutoDJ::appendTrack(int trackId) {
    return m_pAutoDJTableModel->appendTrack(trackId);
}
