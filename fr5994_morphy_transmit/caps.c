#include <stdint.h>
#include <caps.h>

void initCapBank(CapBank* bank){
    // Set control GPIOs for bank to output
    *(bank->enPort + OUT_DIR_OFFSET) |= bank->enMask;
    *(bank->modePort + OUT_DIR_OFFSET) |= bank->modeMask;

    // Init bank to series mode and disconnected
    *(bank->enPort) &= ~(bank->enMask);
    *(bank->modePort) &= ~(bank->modeMask);
    bank->state = DISCONNECTED_SER;
    return;
}

inline void setCapBankState(CapBank* bank, CapBankState state){
    switch(state){
        case DISCONNECTED_SER:
            *(bank->enPort) &= ~(bank->enMask);
            *(bank->modePort) &= ~(bank->modeMask);
            break;
        case DISCONNECTED_PAR:
            *(bank->enPort) &= ~(bank->enMask);
            *(bank->modePort) |= bank->modeMask;
            break;
        case CONNECTED_SER:
            *(bank->enPort) |= bank->enMask;
            *(bank->modePort) &= ~(bank->modeMask);
            break;
        case CONNECTED_PAR:
            *(bank->enPort) |= bank->enMask;
            *(bank->modePort) |= bank->modeMask;
            break;
        default:
            *(bank->enPort) &= ~(bank->enMask);
            *(bank->modePort) &= ~(bank->modeMask);
            bank->state = DISCONNECTED_SER;
            return;
    }
    bank->state = state;
    return;
}

// Increases bank capacitance:
// DISCONNECTED_SER -> CONNECTED_SER -> CONNECTED_PAR
// Returns 1 on successful bank expansion, 0 if bank is already max size
// Doesn't support DISCONNECTED_PAR. Will sit in while(1) if it encounters it
inline uint8_t expandCapBank3State(CapBank* bank){
    switch(bank->state){
        case DISCONNECTED_SER:
            setCapBankState(bank, CONNECTED_SER);
            return 1;
        case CONNECTED_SER:
            setCapBankState(bank, CONNECTED_PAR);
            return 1;
        case CONNECTED_PAR:
            return 0;
        default:
            while(1);
    }
}

// Decreases bank capacitance:
// CONNECTED_PAR -> CONNECTED_SER -> DISCONNECTED_SER
// Returns 1 on successful bank contraction, 0 if bank is already min size
// Doesn't support DISCONNECTED_PAR. Will sit in while(1) if it encounters it
inline uint8_t contractCapBank3State(CapBank* bank){
    switch(bank->state){
        case CONNECTED_PAR:
            setCapBankState(bank, CONNECTED_SER);
            return 1;
        case CONNECTED_SER:
            setCapBankState(bank, DISCONNECTED_SER);
            return 1;
        case DISCONNECTED_SER:
            return 0;
        default:
            while(1);
    }
}
