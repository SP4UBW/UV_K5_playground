#pragma once
#include "system.hpp"
#include "uv_k5_display.hpp"
#include "radio.hpp"
#include "registers.hpp"
#include "menu.hpp"

namespace Rssi
{
   inline const unsigned char U8RssiMap[] =
       {
         //  141, 
         //  135,
           129,
           123,
           117,
           111,
           105,
           99,
           93,
           83,
           73,
           63,
           53,
   };

   struct TRssi
   {
      TRssi(){};
      TRssi(signed short s16Rssi)
          : s16Rssi(s16Rssi)
      {
         s16Rssi *= -1;
         unsigned char i;
         for (i = 0; i < sizeof(U8RssiMap); i++)
         {
            if (s16Rssi >= U8RssiMap[i])
            {
               u8SValue = i + 1;
               return;
            }
         }
         u8SValue = i + 1;
      }

      short s16Rssi;
      unsigned char u8SValue;
   };
}

template <
    TUV_K5Display &DisplayBuff,
    CDisplay<TUV_K5Display> &Display,
    CDisplay<TUV_K5Display> &DisplayStatusBar,
    const TUV_K5SmallNumbers &FontSmallNr,
    Radio::CBK4819 &RadioDriver>
class CRssiSbar : public IView, public IMenuElement
{
public:
   static constexpr auto ChartStartX = 5 * 7 + 3 + 3 * 7; // 32;
   static constexpr auto BlockSizeX = 4;
   static constexpr auto BlockSizeY = 7;
   static constexpr auto BlockSpace = 1;
   static constexpr auto BlocksCnt = (128 - ChartStartX) / (BlockSizeX + BlockSpace);
   static constexpr auto LinearBlocksCnt = 9;
   static constexpr auto VoltageOffset = 77;
   static constexpr auto MaxBarPoints = 13;
   static inline unsigned char *const pDData = gDisplayBuffer + 128 * 3;
   unsigned int u32DrawVoltagePsc = 0;
   Rssi::TRssi RssiData;
   unsigned char u8AfAmp = 0;
   bool bPtt = false;
   bool b59Mode = false;

   CRssiSbar()
   {
      Display.SetFont(&FontSmallNr);
      DisplayStatusBar.SetFont(&FontSmallNr);
   }

   const char *GetLabel() override
   {
      if (!b59Mode)
         return "S-metr  normal";
         return "S-metr    59";
   }

   void HandleUserAction(unsigned char u8Button) override
   {
      if (u8Button != Button::Ok)
      {
         return;
      }
       b59Mode = !b59Mode;
   }

   eScreenRefreshFlag HandleBackground(TViewContext &Context) override
   {
      static bool bIsCleared = true;
      static unsigned char u8SqlDelayCnt = 0xFF;
      
      if (Context.OriginalFwStatus.b1RadioSpiCommInUse || Context.OriginalFwStatus.b1LcdSpiCommInUse)
      {
         return eScreenRefreshFlag::NoRefresh;
      }

      if (Context.ViewStack.GetTop() || !(u32DrawVoltagePsc++ % 8))
      {
         PrintBatteryVoltage();
         return eScreenRefreshFlag::StatusBar;
      }

      bPtt = !(GPIOC->DATA & GPIO_PIN_5);
     
     if (!bPtt)  // wylaczenie fabrycznego smetra jak jest odbior lub wlaczone menu, pozostaje jako wskaznik nadawania
      { 
         if (!b59Mode)
         {
          //Sprawdzenie czy wylaczony skaner/czestosciomierz
//          if (!(gDisplayBuffer + 128 * 1 + 2) && !(gDisplayBuffer + 128 * 5 + 2))  
           {      
            memset(gDisplayBuffer + 128 * 2, 0, 22);
            memset(gDisplayBuffer + 128 * 6, 0, 22);
           }   
         }   
      }
  
      if (RadioDriver.IsSqlOpen() || bPtt)
      {
         u8SqlDelayCnt = 0;
      }

      if (u8SqlDelayCnt > 10 || Context.OriginalFwStatus.b1MenuDrawed)
      {
         if (!bIsCleared)
         {
            bIsCleared = true;
            if (!Context.OriginalFwStatus.b1MenuDrawed)
            {
               return eScreenRefreshFlag::MainScreen;
            }
         }

         return eScreenRefreshFlag::NoRefresh;
      }

      u8SqlDelayCnt++;
      bIsCleared = false;

      if(b59Mode)
      {
         RssiData = 0;
      }
      else if (bPtt)
      {
         RssiData.s16Rssi = RadioDriver.GetAFAmplitude();
         RssiData.s16Rssi = RssiData.s16Rssi < 60 ? 0 : RssiData.s16Rssi - 50; 
         RssiData.u8SValue = (MaxBarPoints * RssiData.s16Rssi) >> 6;
      }
      else
      {
         RssiData = RadioDriver.GetRssi();
         
      }

      ProcessDrawings();
      return eScreenRefreshFlag::MainScreen;
   }

   void ProcessDrawings()
   {
      ClearSbarLine();

if (bPtt)
     {
//        if ((gDisplayBuffer + 128 * 2 + 1) || (gDisplayBuffer + 128 * 6 + 1))  // wylaczenie MIC i sbar jak DISABLE TX
         {   
          PrintSValue(RssiData.u8SValue);
          PrintSbar(RssiData.u8SValue);
         }   
     }
else
     {
      //PrintNumber(RssiData.s16Rssi); wyłączone dB w RX
     if ((gDisplayBuffer + 128 * 0 + 16) || (gDisplayBuffer + 128 * 4 + 16))  // wylaczenie sbara jak nie ma napisow RX
      {    
        memcpy(pDData + 3 + 5*0 + 0, gSmallLeters + 128 * 1 + 206, 5); //Napis R
        memcpy(pDData + 3 + 5*1 + 1, gSmallLeters + 128 * 1 + 242, 5); //Napis X 
        if (gDisplayBuffer + 128 * 0 + 16)
         {
          memcpy(pDData + 3 + 5*2 + 4, gSmallLeters + 128 * 1 + 96, 5); //Napis A
         }
        if (gDisplayBuffer + 128 * 4 + 16)
        {
         Display.SetCoursor(3, 5*2 + 5);                                        //Cyfra 8 (szerokosc 6 pikseli)
         Display.PrintCharacter('8');
         memcpy(pDData + 3 + 5*2 + 3, gSmallLeters + 128 * 1 + 119, 2); //Kreska pionowa do litery B do zamalowania 8 
        }
       PrintSValue(RssiData.u8SValue);
       PrintSbar(RssiData.u8SValue);
      }
     }  
      
   }

   void ClearSbarLine()
   {
      //Sprawdzenie czy wylaczony skaner/czestosciomierz
//      if (!(gDisplayBuffer + 128 * 1 + 2) && !(gDisplayBuffer + 128 * 5 + 2))
     {  
      memset(pDData, 0, DisplayBuff.SizeX);
     }
   }

//   void PrintNumber(short s16Number)
//   {
//      Display.SetCoursor(3, 0);
//      if (s16Number > 0)
//      {
//         Display.PrintCharacter(' ');
//      }
//
//      Display.PrintFixedDigitsNumber2(s16Number, 0, 3);
//   }

   void PrintSValue(unsigned char u8SValue)
   {
   if (bPtt) // print MIC
   {
        memcpy(pDData + 39 + 5*0 + 0, gSmallLeters + 128 * 1 + 102, 5); //Napis M
        memcpy(pDData + 39 + 5*1 + 1, gSmallLeters + 128 * 1 + 102, 1); //Napis I
        memcpy(pDData + 39 + 5*2 - 2, gSmallLeters + 128 * 1 + 108, 5); //Napis C
        return;
   }

      char C8SignalString[] = "  ";
      if(b59Mode)
      {
         C8SignalString[0] = '5';
         C8SignalString[1] = '9';
      }
      else if (u8SValue > 9)
      {
         memcpy(pDData + 5 * 7, gSmallLeters + 109 - 3 * 8, 8); //Znak +
         C8SignalString[1] = '0';
         C8SignalString[0] = '0' + u8SValue - 9;
      }
      else
      {
         if (u8SValue > 1)
         { 
           memcpy(pDData + 5 * 7, gSmallLeters + 109, 8);  //Litera S
           C8SignalString[0] = '0' + u8SValue;
           C8SignalString[1] = ' ';
         } 
         else
         {
         char C8SignalString[] = "  ";   //Wylaczenie Wskazania S po puszczeniu PTT
         memset(pDData, 0, DisplayBuff.SizeX);   
         }
      }

      Display.SetCoursor(3, 5 * 7 + 8);
      Display.Print(C8SignalString);
   }

   void PrintSbar(unsigned char u8SValue)
   {
      u8SValue = u8SValue > MaxBarPoints ? MaxBarPoints : u8SValue;
      for (unsigned char i = 0; i < u8SValue; i++)
      {
         unsigned char u8BlockHeight = i + 1 > BlockSizeY ? BlockSizeY : i + 1;
         unsigned char u8X = i * (BlockSizeX + BlockSpace) + ChartStartX;
         Display.DrawRectangle(u8X, 24 + BlockSizeY - u8BlockHeight, BlockSizeX,
                               u8BlockHeight, i < LinearBlocksCnt);
      }
   }

   void PrintBatteryVoltage()
   {
     if (gStatusBarData[VoltageOffset + 4 * 6 + 1] || gStatusBarData[VoltageOffset + 4 * 6 - 6])
      {  // disable printing when function or charging icon are printed
         return;
      }

      unsigned short u16Voltage = gVoltage > 1000 ? 999 : gVoltage;

      memset(gStatusBarData + VoltageOffset, 0, 4 * 5);
      DisplayStatusBar.SetCoursor(0, VoltageOffset);
      DisplayStatusBar.PrintFixedDigitsNumber2(u16Voltage, 2, 1);
      memset(gStatusBarData + VoltageOffset + 7 + 1, 0b1100000, 2); // dot
      DisplayStatusBar.SetCoursor(0, VoltageOffset + 7 + 4);
      DisplayStatusBar.PrintFixedDigitsNumber2(u16Voltage, 0, 2);
      memcpy(gStatusBarData + VoltageOffset + 4 * 6 + 2, gSmallLeters + 128 * 2 + 102, 5); // V character
      BK4819Write(0x78, (40 << 8) | (40 & 0xFF));  //Przesuniecie o 20dB w dol SQL, nieanulowane po wybraniu wartosci z menu
   }
};
