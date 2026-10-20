// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef SEEN_OPERATION_BLOCKER_H
#define SEEN_OPERATION_BLOCKER_H

// cooperative counter-based pending operation blocking

class OperationBlocker {
public:
    OperationBlocker() = default;
    
    bool pending() const {
        return _counter > 0;
    }

    class scoped_block {
    public:
        scoped_block(unsigned int& counter): _c(counter) {
            ++_c;
        }

        ~scoped_block() {
            --_c;
        }

    private:
        unsigned int& _c;
    };

    [[nodiscard]] scoped_block block() {
        return scoped_block(_counter);
    }

private:
    unsigned int _counter = 0;
};

#endif
