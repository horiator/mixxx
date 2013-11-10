// wlibrarytextbrowser.h
// Created 10/23/2009 by RJ Ryan (rryan@mit.edu)

#ifndef WLIBRARYTEXTBROWSER_H
#define WLIBRARYTEXTBROWSER_H

#include <QTextBrowser>

#include "library/abstractlibraryview.h"

class WLibraryTextBrowser : public QTextBrowser, public virtual AbstractLibraryView {
    Q_OBJECT
  public:
    WLibraryTextBrowser(QWidget* parent = NULL);
    virtual ~WLibraryTextBrowser();

    virtual void onShow() {};
};

#endif /* WLIBRARYTEXTBROWSER_H */
