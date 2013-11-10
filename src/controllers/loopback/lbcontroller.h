

#ifndef LBCONTROLLER_H
#define LBCONTROLLER_H

#include <QString>

class LbController {
public:
    virtual ~LbController() {};

    virtual bool registerInputDevice(const QString& name) = 0;
    virtual bool registerOutputDevice(const QString& name) = 0;
};

#endif // LBCONTROLLER_H
