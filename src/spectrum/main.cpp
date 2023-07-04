#include "system.hpp"
#include "hardware/hardware.hpp"
#include "registers.hpp"
#include "uv_k5_display.hpp"
#include "spectrum.hpp"
#include <string.h>

Hardware::THardware Hw;
const System::TOrgFunctions& Fw = System::OrgFunc_01_26;
const System::TOrgData& FwData = System::OrgData_01_26;

CSpectrum<System::OrgFunc_01_26, System::OrgData_01_26> Spectrum;

int main()
{
   System::JumpToOrginalFw();
   return 0;
} 

void MultiIrq_Handler(unsigned int u32IrqSource)
{
   static bool bFirstInit = false;
   if(!bFirstInit)
   {
      System::CopyDataSection();
      __libc_init_array();
      bFirstInit = true;
   }

   static unsigned int u32StupidCounter = 1;
   if(u32StupidCounter++ > 200)
   {
      Spectrum.Handle();
   }
}