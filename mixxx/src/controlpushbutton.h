/***************************************************************************
                          controlpushbutton.h  -  description
                             -------------------
    begin                : Wed Feb 20 2002
    copyright            : (C) 2002 by Tue and Ken Haste Andersen
    email                :
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef CONTROLPUSHBUTTON_H
#define CONTROLPUSHBUTTON_H

#include "controlobject.h"
#include "defs.h"
#include <QTimer>

/**
  *@author Tue and Ken Haste Andersen
  */

class ControlPushButton : public ControlObject
{
    Q_OBJECT
public:
    enum MidiButtonMode {
        PUSH,
        TOGGLE,
        POWERWINDOW
    };

    ControlPushButton(ConfigKey key);
    ~ControlPushButton();
    void setMidiButtonMode(enum MidiButtonMode mode);
    void setStates(int num_states);

protected:
    void setValueFromMidi(MidiCategory c, double v);

private:
    enum MidiButtonMode m_midiButtonMode;
    int m_iNoStates;
    QTimer m_pushTimer;
};

#endif
