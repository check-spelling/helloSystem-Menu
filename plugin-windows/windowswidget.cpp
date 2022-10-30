#include "windowswidget.h"
#include <QHBoxLayout>
#include <QLocale>
#include <QProcess>
#include <QDebug>
#include <KF5/KWindowSystem/KWindowSystem>

#include "../src/applicationwindow.h"


WindowsWidget::WindowsWidget(QWidget *parent)
    : QWidget(parent),
      m_menubar(new QMenuBar),
      m_menu(new QMenu),
      m_refreshTimer(new QTimer(this))
{

    m_menu->setTitle(tr("Windows"));

    QFont f = m_menu->menuAction()->font();
    f.setBold(true);
    m_menu->menuAction()->setFont(f);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);

    layout->addWidget(m_menubar);
    m_menubar->addMenu(m_menu);
    setLayout(layout);

    updateWindows();

    connect(KWindowSystem::self(), &KWindowSystem::activeWindowChanged, this, &WindowsWidget::updateWindows);
}

void WindowsWidget::updateWindows()

// FIXME: Instead of doing this whole thing each time the frontmost window changes
// we could probably keep some internal state and change just what needs to be changed;
// but writing the code for this is more complex and error-prone; so let's see whether
// we can get away with this

{
    m_menu->setTitle(applicationNiceNameForWId(KWindowSystem::activeWindow()));

    m_menu->clear();

    QList<unsigned int> distinctPids = {};
    QList<WId> distinctApps = {};

    // Find out one process ID per application that has at least one window
    // TODO: No Dock windows
    for (WId id : KWindowSystem::windows()){
        KWindowInfo info(id, NET::WMPid, NET::WM2TransientFor | NET::WM2WindowClass | NET::WM2WindowRole);
        if (! distinctPids.contains(info.pid())) {
            distinctPids.append(info.pid());
            distinctApps.append(id);
        }
    }

    // Hide frontmost app
    WId id = KWindowSystem::activeWindow();
    QAction *hideAction = m_menu->addAction(tr("Hide %1").arg(applicationNiceNameForWId(id)));
    hideAction->setShortcut(QKeySequence("Ctrl+H"));
    connect(hideAction, &QAction::triggered, this, [hideAction, id, this]() {
        KWindowSystem::minimizeWindow(id);
    });

    // Hide others
    QAction *hideOthersAction = m_menu->addAction(tr("Hide Others"));
    // hideOthersAction->setShortcut(QKeySequence("Shift+Alt+H"));
    connect(hideOthersAction, &QAction::triggered, this, [hideOthersAction, id, this]() {
        for (WId cand_id : KWindowSystem::windows()){
            if(cand_id != id)
                KWindowSystem::minimizeWindow(cand_id);
        }
    });

    // Show all
    QAction *showAllAction = m_menu->addAction(tr("Show All"));
    // showAllAction->setShortcut(QKeySequence("Shift+Ctrl+H"));
    connect(showAllAction, &QAction::triggered, this, [showAllAction, id, this]() {
        for (WId cand_id : KWindowSystem::windows()){
            KWindowSystem::unminimizeWindow(cand_id);
        }
    });

    m_menu->addSeparator();

    // Add one menu item for each appliction

    for (WId id : distinctApps){
        // TODO: If there are multiple windows for the same PID, then add multiple submenus
        // So don't add an action here, but a submenu which contains all windows that beloong to that PID
        QString niceName = applicationNiceNameForWId(id);
        if(niceName == "cyber-dock") // FIXME: Don't harcode, see "No Dock windows" comment above
            continue;
        QAction *appAction = m_menu->addAction(niceName);
        appAction->setCheckable(true);
        if(id == KWindowSystem::activeWindow())
                appAction->setChecked(true);
        // appAction->setIcon(QIcon(KWindowSystem::icon(id))); // Why does this not work? TODO: Get icon from bundle?
        connect(appAction, &QAction::triggered, this, [appAction, id, this]() {
            KWindowSystem::activateWindow(id);
        });
    }

    m_menu->addSeparator();

    // Show all
    QAction *fullscreenAction = m_menu->addAction(tr("Full Screen"));
    // TODO: Need a way to undo this...
    connect(fullscreenAction, &QAction::triggered, this, [fullscreenAction, id, this]() {
        KWindowSystem::setState(id, KWindowSystem::FullScreen);
    });

}
