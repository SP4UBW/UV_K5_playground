#pragma once
#include "system.hpp"
#include "uv_k5_display.hpp"
#include "radio.hpp"
#include "registers.hpp"
#include "menu.hpp"

namespace Rssi
{
   inline const short U8RssiMap[] = { 135, 129, 123, 117, 111, 105, 99, 93, 83, 73, 63, 53, 43, 33, 23, 13, 3, -7, -17,};

   struct TRssi
   {
      TRssi(){};
      TRssi(signed short s16Rssi) : s16Rssi(s16Rssi)
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
   static constexpr auto ChartStartX = 3;
   static constexpr auto BlockSizeX = 5;
   static constexpr auto BlockSizeY = 9;
   static constexpr auto BlockSpace = 2;
   static constexpr auto LinearBlocksCnt = 17;  
   static constexpr auto VoltageOffset = 77;
   static constexpr auto MaxBarPoints = 17;
   static inline unsigned char *const pDData = gDisplayBuffer + 128 * 3;
   
   unsigned int u32DrawVoltagePsc = 0;
   Rssi::TRssi RssiData;
   bool procenty = true;
   short licznik = 0;
   unsigned char Light = 0;
   unsigned char RXAB = 0;
   CRssiSbar()
   {
      Display.SetFont(&FontSmallNr);
      DisplayStatusBar.SetFont(&FontSmallNr);
   }

   const char *GetLabel() override
   {
     return " ";
   }

   void HandleUserAction(unsigned char u8Button) override
   {
     return;
   }

   eScreenRefreshFlag HandleBackground(TViewContext &Context) override
   {
      static bool bIsCleared = true;
      static unsigned char u8SqlDelayCnt = 0xFF;
     
     if (Context.OriginalFwStatus.b1RadioSpiCommInUse || Context.OriginalFwStatus.b1LcdSpiCommInUse)
      {
        Light++;               //GPIOB->DATA |= GPIO_PIN_6; wlaczenie podswietlenia
        if (Light > 9) {Light=0; GPIOB->DATA &= ~GPIO_PIN_6;} //Wylacz LCD po 10s przy skanowaniu pamieci
        if (gDisplayBuffer[128 * 3 + 49]) Light=0;
        if (!gDisplayBuffer[128 * 1 + 3] && gDisplayBuffer[128 * 0 + 3]) Light=0;
        if (!gDisplayBuffer[128 * 5 + 3] && gDisplayBuffer[128 * 4 + 3]) Light=0;
 
        return eScreenRefreshFlag::NoRefresh;
      }

      if (Context.ViewStack.GetTop() || !(u32DrawVoltagePsc++ % 8))
       {
      
       //Zerowanie licznika wylaczenia podswietlenia jak wcisniety klawisz UP/DOWN
      if (!gStatusBarData[VoltageOffset + 23]) Light=0; //Srodek litery V lub kropka jak VOX
       
       PrintBatteryVoltage();
       //Przesuniecie SQL o 20dB   BK4819Write(0x78, (40 << 8) | (40 & 0xFF));  
       BK4819Write(0x78, 0x2828);  //Wyliczenie dla 20dB - dla skrócenia kodu 
       return eScreenRefreshFlag::StatusBar;
       }

      if (RadioDriver.IsSqlOpen()) u8SqlDelayCnt = 0;
    
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

//Obsluga wielokrotnego nacisniecia klawisza F
       if (gStatusBarData[VoltageOffset + 22])
       {
        if (licznik < 1000) licznik = licznik + 1000;  
       }
       else
       {
        if (licznik > 0 ) licznik--;
       }

       if (licznik > 10) 
       {   
        licznik = 0;
        procenty = !procenty;
       }   

      








      
   RssiData = RadioDriver.GetRssi();
      
   if (!gDisplayBuffer[128 * 1 + 2])
     ProcessDrawings();
     return eScreenRefreshFlag::MainScreen;
   }

void ProcessDrawings()
   {
     if ( (RadioDriver.IsSqlOpen()) && (gDisplayBuffer[128 * 0 + 16] || gDisplayBuffer[128 * 4 + 16])  ) // wlaczenie sbara jak jest RX
      {    
        if (!gDisplayBuffer[128 * 0 + 14] && gDisplayBuffer[128 * 0 + 16])
         {
          memset(gDisplayBuffer + 128 * 2, 0, 22);
          memset(pDData, 0, 512);
          RXAB = 4;  //Linia w ktorej ma byc wyswietlane
          memcpy(pDData - 256 + RXAB * 128 + 120, gSmallLeters + 128 * 1 + 96, 5);  //Litera A
         }
        if (gDisplayBuffer[128 * 4 + 16])
         {
          memset(gDisplayBuffer + 128 * 6, 0, 22);
          memset(pDData - 384, 0, 512);
          RXAB = 1;  //Linia w ktorej ma byc wyswietlane
          memset(pDData - 256 + RXAB * 128 + 120, 0b1111111, 1);                    //Litera B 
          memset(pDData - 256 + RXAB * 128 + 121, 0b1001001, 3); 
          memset(pDData - 256 + RXAB * 128 + 124, 0b0110110, 1);  
         }
       PrintSValue(RssiData.u8SValue, RssiData.s16Rssi);  
       PrintSbar(RssiData.u8SValue);
           memcpy(pDData - 256 + RXAB * 128 + 105, gSmallLeters + 128 * 1 + 206, 5);  //Napis R
           memcpy(pDData - 256 + RXAB * 128 + 111, gSmallLeters + 128 * 1 + 242, 5);  //Napis X   
      }
    }
/*
   void PrintNumber(short s16Number)
   {
   if (s16Number > -129)
    { 
      if (s16Number >= 0)    
         Display.SetCoursor(RXAB+1, 88);  //2 lub 5
      else
         Display.SetCoursor(RXAB+1, 81);
              
      
         Display.PrintFixedDigitsNumber2(s16Number, 0, 3); 
         //Wyswietlanie napisu dBm
         memset(pDData - 256 + RXAB * 128 + 111, 0b0110000, 1); // znak d
         memset(pDData - 256 + RXAB * 128 + 112, 0b1001000, 2);
         memset(pDData - 256 + RXAB * 128 + 114, 0b1111111, 1);
         
         memset(pDData - 256 + RXAB * 128 + 116, 0b1111111, 1); // znak B 
         memset(pDData - 256 + RXAB * 128 + 117, 0b1001001, 2); 
         memset(pDData - 256 + RXAB * 128 + 119, 0b0110110, 1);
         
         memset(pDData - 256 + RXAB * 128 + 121, 0b1110000, 5); // znak m
         memset(pDData - 256 + RXAB * 128 + 122, 0b0001000, 1);
         memset(pDData - 256 + RXAB * 128 + 124, 0b0001000, 1);
    }    
   }
*/
   void PrintSValue(unsigned char u8SValue, short s16Number)
   {
      Display.SetCoursor(RXAB+1, 22+3);
      char C8SignalString[] = " ";
        if (s16Number > -92)
      {
         memset(pDData - 256 + RXAB * 128 + 17+2, 0b0001000, 2); // -
         memset(pDData - 256 + RXAB * 128 + 19+2, 0b0111110, 1); // |
         memset(pDData - 256 + RXAB * 128 + 20+2, 0b0001000, 2); // -
         if (s16Number > 6)  Display.PrintFixedDigitsNumber2(99, 0, 2);
           else  Display.PrintFixedDigitsNumber2(s16Number + 92, 0, 2);

 //        memset(pDData - 256 + RXAB * 128 + 38+3, 0b0110000, 1); // znak d
 //        memset(pDData - 256 + RXAB * 128 + 39+3, 0b1001000, 2);
 //        memset(pDData - 256 + RXAB * 128 + 41+3, 0b1111111, 1);
         
 //        memset(pDData - 256 + RXAB * 128 + 43+3, 0b1111111, 1); // znak B 
 //        memset(pDData - 256 + RXAB * 128 + 44+3, 0b1001001, 2); 
 //        memset(pDData - 256 + RXAB * 128 + 46+3, 0b0110110, 1);
      }
         if (u8SValue > 1)
         { 
           memcpy(pDData - 256 + RXAB * 128 + 3, gSmallLeters + 128 * 1 + 194, 5);  //Litera S wąska  
           C8SignalString[0] = (u8SValue > 9) ? '9' : '0' + u8SValue;
           Display.SetCoursor(RXAB+1, 8);
           Display.Print(C8SignalString);  
         } 
   }

void PrintSbar(unsigned char u8SValue)
   {
     Display.DrawRectangle(0, RXAB*8-6, 128, 24, false);
      u8SValue = u8SValue > MaxBarPoints ? MaxBarPoints : u8SValue;
    if (u8SValue>1) 
     { 
      for (unsigned char i = 0; i < u8SValue; i++)
      {
         unsigned char u8BlockHeight = i + 1 > BlockSizeY ? BlockSizeY : i + 1;
         unsigned char u8X = i * (BlockSizeX + BlockSpace) + ChartStartX;
         Display.DrawRectangle(u8X, RXAB*8-3 + BlockSizeY - u8BlockHeight, BlockSizeX, u8BlockHeight, i < LinearBlocksCnt);
      }
    }  
   }

   void PrintBatteryVoltage()
   {
      // wylaczenie gdy ikona ladowania lub funkcji
      if (gStatusBarData[VoltageOffset + 25] || gStatusBarData[VoltageOffset + 18])
   {  
    return;
   }

if (gStatusBarData[VoltageOffset - 3]) memset(gStatusBarData + VoltageOffset + 23, 0b1000000, 1); else 
   {
   unsigned short u16Voltage = gVoltage - 0; //dodana kalibracja -0.00V   

if (!procenty)          
      {
      //Wartosc w woltach
      DisplayStatusBar.SetCoursor(0, VoltageOffset + 2);
      DisplayStatusBar.PrintFixedDigitsNumber2(u16Voltage, 2, 1);
      memset(gStatusBarData + VoltageOffset + 8 +2, 0b1100000, 2); // kropka
      DisplayStatusBar.SetCoursor(0, VoltageOffset + 11 + 2);
      DisplayStatusBar.PrintFixedDigitsNumber2(u16Voltage, 1, 1);
      memcpy(gStatusBarData + VoltageOffset + 23, gSmallLeters + 358, 5); // Litera V 
      }
else
      {   
      //Wartosc w procentach
      DisplayStatusBar.SetCoursor(0, VoltageOffset); 
      unsigned char percentage;         

      if (u16Voltage >= 810) percentage = 89 + ((u16Voltage - 810) >> 1);
      else if (u16Voltage >= 680) percentage = (u16Voltage - 680) * 88 >> 7;
      else percentage = 0;
         
      if (percentage >= 100) DisplayStatusBar.PrintFixedDigitsNumber2(100, 0, 3); 
      else if (percentage >= 10) 
           {
           DisplayStatusBar.SetCoursor(0, VoltageOffset + 7);
           DisplayStatusBar.PrintFixedDigitsNumber2(percentage, 0, 2);
           }
      else if (percentage >= 0)
           {
           DisplayStatusBar.SetCoursor(0, VoltageOffset + 14);
           DisplayStatusBar.PrintFixedDigitsNumber2(percentage, 0, 1);
           }
           memset(gStatusBarData + VoltageOffset + 23 + 0, 0b0100011, 1); // %
           memset(gStatusBarData + VoltageOffset + 23 + 1, 0b0010011, 1);  
           memset(gStatusBarData + VoltageOffset + 23 + 2, 0b0001000, 1);  
           memset(gStatusBarData + VoltageOffset + 23 + 3, 0b1100100, 1);  
           memset(gStatusBarData + VoltageOffset + 23 + 4, 0b1100010, 1);  
     }
    }     
   }


};




//UV K5 - normalny firmware do normalnego użytkowania















































/*
#pragma once
#include "system.hpp"
#include "uv_k5_display.hpp"
#include "radio.hpp"
#include "registers.hpp"
#include "menu.hpp"

namespace Rssi
{
   inline const short U8RssiMap[] = { 129, 123, 117, 111, 105, 99, 93, 83, 73, 63, 53, 43, 33, 23, 13, 3, -7, -17,};

   struct TRssi
   {
      TRssi(){};
      TRssi(signed short s16Rssi) : s16Rssi(s16Rssi)
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
   static constexpr auto ChartStartX = 34;
   static constexpr auto BlockSizeX = 3;
   static constexpr auto BlockSizeY = 7;
   static constexpr auto BlockSpace = 1;
   static constexpr auto LinearBlocksCnt = 9;  
   static constexpr auto VoltageOffset = 77;
   static constexpr auto MaxBarPoints = 13;
   static inline unsigned char *const pDData = gDisplayBuffer + 128 * 3;
   unsigned int u32DrawVoltagePsc = 0;
   Rssi::TRssi RssiData;
   unsigned char u8AfAmp = 0;
   bool bPtt = false;
   bool b59Mode = false;
   unsigned char Light = 0;

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
        Light++;               //GPIOB->DATA |= GPIO_PIN_6; wlaczenie podswietlenia
        if (Light > 5) {Light=0; GPIOB->DATA &= ~GPIO_PIN_6;} //Wylacz LCD po 6s przy skanowaniu
        if (gDisplayBuffer[128 * 3 + 49]) Light=0;
        if (!gDisplayBuffer[128 * 1 + 3] && gDisplayBuffer[128 * 0 + 3]) Light=0;
        if (!gDisplayBuffer[128 * 5 + 3] && gDisplayBuffer[128 * 4 + 3]) Light=0;
 
        return eScreenRefreshFlag::NoRefresh;
      }

      if (Context.ViewStack.GetTop() || !(u32DrawVoltagePsc++ % 8))
       {
      
     //Zerowanie licznika wylaczenia podswietlenia jak wcisniety klawisz UP/DOWN
     if (!gStatusBarData[VoltageOffset + 23]) Light=0; //Srodek litery V lub kropka jak VOX
       
       PrintBatteryVoltage();
       return eScreenRefreshFlag::StatusBar;
       }

      bPtt = !(GPIOC->DATA & GPIO_PIN_5);

      //if (RadioDriver.IsSqlOpen() || bPtt) u8SqlDelayCnt = 0;   
      
      if (RadioDriver.IsSqlOpen()) u8SqlDelayCnt = 0;
    
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

      if (b59Mode)
      {
         RssiData = 0;
      }
    //  else if (bPtt)
    //  {
    //     RssiData.s16Rssi = RadioDriver.GetAFAmplitude();
    //     RssiData.s16Rssi = RssiData.s16Rssi < 62 ? 0 : RssiData.s16Rssi - 45; 
    //     RssiData.u8SValue = (MaxBarPoints * RssiData.s16Rssi) >> 6;
    //  }
      else
      {
         RssiData = RadioDriver.GetRssi();
      }
         
   if (!(gDisplayBuffer[128 * 1 + 2])) //&& ((gDisplayBuffer[128 * 0 + 3]) || (gDisplayBuffer[128 * 4 + 3])))
    {
     ProcessDrawings();
    }
   return eScreenRefreshFlag::MainScreen;
   }

void ProcessDrawings()
   {
      memset(pDData, 0, DisplayBuff.SizeX);
//if (bPtt)
//     {
//        if ((gDisplayBuffer[128 * 2 + 1]) || (gDisplayBuffer[128 * 6 + 1]))  // wylaczenie MIC i sbar jak DISABLE TX
//         {   
//          PrintSValue(RssiData.u8SValue);
//          PrintSbar(RssiData.u8SValue);
//         }   
//     }
//else
//     {

     if ( (gDisplayBuffer[128 * 0 + 16]) || (gDisplayBuffer[128 * 4 + 16])  ) // wylaczenie sbara jak nie ma napisow RX
      {    
        if (gDisplayBuffer[128 * 0 + 16])
         {
          memcpy(pDData + 3 + 5*0 + 0, gSmallLeters + 128 * 1 + 96, 5);  //Litera A
         }
        if (gDisplayBuffer[128 * 4 + 16])
         {
          memset(pDData + 3, 0b1111111, 1);                               //Litera B 
          memset(pDData + 4, 0b1001001, 3); 
          memset(pDData + 7, 0b0110110, 1);
         }
       memset(gDisplayBuffer + 128 * 2, 0, 22);
       memset(gDisplayBuffer + 128 * 6, 0, 22);
       PrintSValue(RssiData.u8SValue);
       PrintNumber(RssiData.s16Rssi);
       PrintSbar(RssiData.u8SValue);
      }
//   }  
    }

   void PrintNumber(short s16Number)
   {
   if (s16Number > -129)
    { 
      if (s16Number >= 0)
      {
         Display.SetCoursor(3, 91);
         Display.PrintFixedDigitsNumber2(s16Number, 0, 3);
      }
      else
      {   
         Display.SetCoursor(3, 84);
         Display.PrintFixedDigitsNumber2(s16Number, 0, 3);
         memset(pDData + 85, 0, 2);          //Skrocenie znaku minus
      }
        // Display.PrintFixedDigitsNumber2(s16Number, 0, 3); 
         //Wyswietlanie napisu dBm
         memset(pDData + 113, 0b0110000, 1); // znak d 
         memset(pDData + 114, 0b1001000, 2);
         memset(pDData + 116, 0b1111111, 1);
         
         memset(pDData + 118, 0b1111111, 1); // znak B 
         memset(pDData + 119, 0b1001001, 2); 
         memset(pDData + 121, 0b0110110, 1);
         
         memset(pDData + 123, 0b1110000, 5); // znak m
         memset(pDData + 124, 0b0001000, 1);
         memset(pDData + 126, 0b0001000, 1);
    }    
   }

   void PrintSValue(unsigned char u8SValue)
   {
 //  if (bPtt) // print MIC
 //  {
 //       memcpy(pDData + 3 + 5*0 + 0, gSmallLeters + 128 * 1 + 102, 5);   //Napis M
 //       memset(pDData + 3 + 5*1 + 1, 0b1111111, 1);                      //Napis I
 //       memcpy(pDData + 3 + 5*2 - 2, gSmallLeters + 128 * 1 + 108, 5);   //Napis C
 //       return;
 //  }

      char C8SignalString[] = "  ";
      if(b59Mode)
      {
         C8SignalString[0] = '5';
         C8SignalString[1] = '9';
      }
      else if (u8SValue > 9)
      {
         if (u8SValue < 19)  //Ograniczenie do +90dBm
          {  
           memset(pDData + 14, 0b0001000, 2); // -
           memset(pDData + 16, 0b0111110, 1); // |
           memset(pDData + 17, 0b0001000, 2); // -
           C8SignalString[1] = '0';
           C8SignalString[0] = '0' + u8SValue - 9;
          }   
      }
      else
      {
         if (u8SValue > 1)
         { 
           memcpy(pDData + 14, gSmallLeters + 128 * 1 + 194, 5);  //Litera S wąska  
           C8SignalString[0] = '0' + u8SValue;
           C8SignalString[1] = ' ';
         } 
         else
         {
           char C8SignalString[] = "  ";       //Wylaczenie Wskazania S po puszczeniu PTT
           memset(pDData + 3, 0, 5);           //Wylaczenie litery A lub B
         }  
      }

      Display.SetCoursor(3, 19);
      Display.Print(C8SignalString);
      Light=5; GPIOB->DATA |= GPIO_PIN_6;  //Podtrzymanie podswietlenia jak jest odbior
   }

void PrintSbar(unsigned char u8SValue)
   {
      u8SValue = u8SValue > MaxBarPoints ? MaxBarPoints : u8SValue;
    if (u8SValue>1) 
     { 
      for (unsigned char i = 0; i < u8SValue; i++)
      {
         unsigned char u8BlockHeight = i + 1 > BlockSizeY ? BlockSizeY : i + 1;
         unsigned char u8X = i * (BlockSizeX + BlockSpace) + ChartStartX;
         Display.DrawRectangle(u8X, 24 + BlockSizeY - u8BlockHeight, BlockSizeX, u8BlockHeight, i < LinearBlocksCnt);
      }
    }  
   }

   void PrintBatteryVoltage()
   {
      if (gStatusBarData[VoltageOffset + 4 * 6 + 1] || gStatusBarData[VoltageOffset + 4 * 6 - 6])
      {  // wylaczenie gdy ikona ladowania lub funkcji
         return;
      }
      if (gStatusBarData[VoltageOffset - 3]) memset(gStatusBarData + VoltageOffset + 23, 0b1000000, 1); else 
      {
      unsigned short u16Voltage = gVoltage - 30; //dodana kalibracja -0.30V   
 //Wartosc w woltach
      DisplayStatusBar.SetCoursor(0, VoltageOffset);
      DisplayStatusBar.PrintFixedDigitsNumber2(u16Voltage, 2, 1);
      memset(gStatusBarData + VoltageOffset + 7 + 1 - 0, 0b1100000, 2); // dot
      DisplayStatusBar.SetCoursor(0, VoltageOffset + 7 + 4 - 0);
      DisplayStatusBar.PrintFixedDigitsNumber2(u16Voltage, 1, 1);
      memcpy(gStatusBarData + VoltageOffset + 3 * 6 + 2 - 0, gSmallLeters + 128 * 2 + 102, 5); // V character 
         
//Wartosc w procentach
//DisplayStatusBar.SetCoursor(0, VoltageOffset);
//unsigned char percentage;         

//if (u16Voltage >= 810) percentage = 89 + ((u16Voltage - 810) >> 1);
//else if (u16Voltage >= 680) percentage = (u16Voltage - 680) * 88 >> 7;
//else percentage = 0;
         
//if (percentage >= 100) DisplayStatusBar.PrintFixedDigitsNumber2(100, 0, 3); 
//     else if (percentage >= 10) 
//           {
//           DisplayStatusBar.SetCoursor(0, VoltageOffset + 7);
//           DisplayStatusBar.PrintFixedDigitsNumber2(percentage, 0, 2);
//           }
//      else if (percentage >= 0)
//           {
//           DisplayStatusBar.SetCoursor(0, VoltageOffset + 14);
//           DisplayStatusBar.PrintFixedDigitsNumber2(percentage, 0, 1);
//           }
         
//           memset(gStatusBarData + VoltageOffset + 3 * 6 + 5 + 0, 0b0100011, 1); // %
//           memset(gStatusBarData + VoltageOffset + 3 * 6 + 5 + 1, 0b0010011, 1);  
//           memset(gStatusBarData + VoltageOffset + 3 * 6 + 5 + 2, 0b0001000, 1);  
//           memset(gStatusBarData + VoltageOffset + 3 * 6 + 5 + 3, 0b1100100, 1);  
//           memset(gStatusBarData + VoltageOffset + 3 * 6 + 5 + 4, 0b1100010, 1);  
         
 
      }
      //Przesuniecie SQL o 20dB   BK4819Write(0x78, (40 << 8) | (40 & 0xFF));  
      BK4819Write(0x78, 10280);  //Wyliczenie dla 20dB - dla skrócenia kodu
      
   }
};
*/
