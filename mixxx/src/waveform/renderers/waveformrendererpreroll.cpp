#include <QBrush>
#include <QPen>
#include <QPainter>
#include <QPolygonF>

#include "waveform/renderers/waveformrendererpreroll.h"

#include "waveform/renderers/waveformwidgetrenderer.h"
#include "waveform/waveform.h"
#include "widget/wskincolor.h"
#include "widget/wwidget.h"

WaveformRendererPreroll::WaveformRendererPreroll(WaveformWidgetRenderer* waveformWidgetRenderer)
  : WaveformRendererAbstract( waveformWidgetRenderer) {
}

WaveformRendererPreroll::~WaveformRendererPreroll() {
}

void WaveformRendererPreroll::setup(const QDomNode& node) {
    m_color.setNamedColor(
        WWidget::selectNodeQString(node, "SignalColor"));
    m_color = WSkinColor::getCorrectColor(m_color);
}

void WaveformRendererPreroll::draw(QPainter* painter, QPaintEvent* event) {
    Q_UNUSED(event);
    const TrackPointer track = m_waveformRenderer->getTrackInfo();
    if (!track) {
        return;
    }

    int samplesPerPixel = m_waveformRenderer->getVisualSamplePerPixel();
    int numberOfSamples = m_waveformRenderer->getWidth() * samplesPerPixel;

    // TODO (vRince) not really accurate since waveform size une visual reasampling and
    // have two mores samples to hold the complete visual data
    int currentPosition = m_waveformRenderer->getPlayPosVSample();
    // m_waveformRenderer->regulateVisualSample(currentPosition);

    // Some of the pre-roll is on screen. Draw little triangles to indicate
    // where the pre-roll is located.
    if (currentPosition < numberOfSamples) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        //painter->setRenderHint(QPainter::HighQualityAntialiasing);
        //painter->setBackgroundMode(Qt::TransparentMode);
        painter->setWorldMatrixEnabled(false);
        painter->setPen(QPen(QBrush(m_color), 1));
        double start_index = 0;
        int end_index = (numberOfSamples - currentPosition) / 2;
        QPolygonF polygon;
        const int polyWidth = 40.0 / samplesPerPixel;
        const float halfHeight = m_waveformRenderer->getHeight()/2.0;
        const float halfPolyHeight = m_waveformRenderer->getHeight()/5.0;
        polygon << QPointF(0, halfHeight)
                << QPointF(-polyWidth, halfHeight - halfPolyHeight)
                << QPointF(-polyWidth, halfHeight + halfPolyHeight);
        polygon.translate(((qreal)end_index)/samplesPerPixel, 0);

        int index = end_index;
        while (index > start_index) {
            painter->drawPolygon(polygon);
            polygon.translate(-(polyWidth+1), 0);
            index -= (polyWidth+1);
        }
        painter->restore();
    }
}

