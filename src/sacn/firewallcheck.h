#ifndef FIREWALLCHECK_H
#define FIREWALLCHECK_H

namespace FwCheck {
    struct FwCheck_t {
        bool allowed = true;
        bool restricted = false;
    };
    FwCheck_t isFWBlocked(QHostAddress ip);
}

#endif // FIREWALLCHECK_H
