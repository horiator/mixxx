#ifndef GLVSYNCTESTWIDGET_H
#define GLVSYNCTESTWIDGET_H

#include <QGLWidget>

#include "waveformwidgetabstract.h"

class GLVSyncTestWidget : public QGLWidget, public WaveformWidgetAbstract {
    Q_OBJECT
  public:
    GLVSyncTestWidget(const char* group, QWidget* parent);
    virtual ~GLVSyncTestWidget();

    virtual WaveformWidgetType::Type getType() const { return WaveformWidgetType::GLVSyncTest;}

    static inline QString getWaveformWidgetName() { return tr("VSyncTest");}
    static inline bool useOpenGl() { return true;}
    static inline bool useOpenGLShaders() { return false;}

  protected:
    virtual void castToQWidget();
    virtual void paintEvent(QPaintEvent* event);
    virtual void postRender();

  private:
    friend class WaveformWidgetFactory;
};
#endif // GLVSYNCTESTWIDGET_H
