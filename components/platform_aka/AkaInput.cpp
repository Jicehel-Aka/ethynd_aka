#include "AkaInput.h"
#include "PlatformTime.h"
#include "esp_timer.h"

void AkaInput::poll()
{
    previous = current;
    Keys k;
    input_poll( k );
    current.up = k.up; current.down = k.down; current.left = k.left; current.right = k.right;
    current.actionA = k.A; current.actionB = k.B; current.actionC = k.C; current.actionD = k.D;
    current.l1 = k.L1; current.r1 = k.R1;
}

bool AkaInput::justPressed( bool InputState::*b ) const { return ( current.*b ) && !( previous.*b ); }

uint32_t platformMillis() { return (uint32_t)( esp_timer_get_time() / 1000 ); }
