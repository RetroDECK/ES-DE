//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  CECInput.h
//
//  CEC (Consumer Electronics Control) input.
//

#ifndef ES_CORE_CECINPUT_H
#define ES_CORE_CECINPUT_H

#include <string>

namespace CEC
{
    class ICECAdapter;
}

class CECInput
{
public:
    CECInput();
    ~CECInput();

    static std::string getAlertTypeString(const unsigned int _type);
    static std::string getOpCodeString(const unsigned int _opCode);
    static std::string getKeyCodeString(const unsigned int _keyCode);

private:
    CEC::ICECAdapter* mlibCEC;
};

#endif // ES_CORE_CECINPUT_H
