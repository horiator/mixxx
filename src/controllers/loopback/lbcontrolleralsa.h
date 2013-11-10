
#ifndef LBCONTROLLERALSA_H
#define LBCONTROLLERALSA_H

#if defined(__APPLE__)

#elif defined(__WINDOWS__)

#else

#include "lbcontroller.h"



#include <alsa/asoundlib.h>

class LbControllerAlsa : public LbController {
  public:
    LbControllerAlsa();
    virtual ~LbControllerAlsa();

    virtual bool openClient(const QString& name);
    virtual bool registerInputDevice(const QString& name);
    virtual bool registerOutputDevice(const QString& name);
    virtual bool closeClient();

  private:

    snd_seq_t *m_pAlsaSeq;
    int  m_iAlsaClient;

    int m_portNrIn;
    int m_portNrOut;
};
#endif
#endif // LBCONTROLLERALSA_H
