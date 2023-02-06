/*
* Mô tả và giải thích các biến cũng như hàm trong chương trình được viết trong file 'Bao Cao_ Nguyen Truong Danh.pdf'
*/

#include<C:\Users\Admin\Desktop\DS18B20\Lib\OWire.h>

#include <Wire.h>
#include "rgb_lcd.h"

rgb_lcd lcd;

#define FALSE 0
#define TRUE  1

unsigned char ROM_ID[4][8];
unsigned char ROM_NO[8];
int LastDiscrepancy;
int LastFamilyDiscrepancy;
int Check_Device;


// __________________________DS18B20 subfunctions______________________________________________


int FisrtSearch()
{

   LastDiscrepancy = 0;
   Check_Device = FALSE;
   LastFamilyDiscrepancy = 0;

   return SearchRom();
}


int NextSearch()
{

   return SearchRom();
}

// Search Rom function

int SearchRom()
{
   int bit_number;
   int last_zero, rom_byte_number, search_result;
   int bit_, inv_bit_;
   unsigned char rom_byte_mask, master_bit;


   bit_number = 1;
   last_zero = 0;
   rom_byte_number = 0;
   rom_byte_mask = 1;
   search_result = 0;

  
   //Serial.println(search_result,HEX);
  
   if (!Check_Device)
   {
      //Serial.println("Reset");
      if (DS18B20_Reset())
      {
        //Serial.println("Reset Fail");
         LastDiscrepancy = 0;
         Check_Device = FALSE;
         LastFamilyDiscrepancy = 0;
         return FALSE;
      }

    // Serial.println("Reset OK");
     DS18B20_Write_Byte(0xF0);  


      do
      {
        // Serial.println("do"); 
         bit_ = DS18B20_Read_Bit();
         inv_bit_ =DS18B20_Read_Bit();

   
         if ((bit_ == 1) && (inv_bit_ == 1))
            break;
         else
         {
  
            if (bit_ != inv_bit_)
               master_bit = bit_;  
            else
            {

               if (bit_number < LastDiscrepancy)
                  master_bit = ((ROM_NO[rom_byte_number] & rom_byte_mask) > 0);
               else
                 
                  master_bit = (bit_number == LastDiscrepancy);

          
               if (master_bit == 0)
               {
                  last_zero = bit_number;

               
                  if (last_zero < 9)
                     LastFamilyDiscrepancy = last_zero;
               }
            }

            if (master_bit == 1){
              ROM_NO[rom_byte_number] |= rom_byte_mask;
              
            }
            else{
              ROM_NO[rom_byte_number] &= ~rom_byte_mask;
              
            }


            DS18B20_Write_Bit(master_bit);


            bit_number++;
            rom_byte_mask <<= 1;

            if (rom_byte_mask == 0)
            {
                 
                rom_byte_number++;
                rom_byte_mask = 1;
            }
         }
      }
      while(rom_byte_number < 8);  

      
      if (!((bit_number < 65)))
      {
       
         LastDiscrepancy = last_zero;

      
         if (LastDiscrepancy == 0)
            Check_Device = TRUE;
         
         search_result = TRUE;
      }
   }

   
   if (!search_result || !ROM_NO[0])
   {
      LastDiscrepancy = 0;
      Check_Device = FALSE;
      LastFamilyDiscrepancy = 0;
      search_result = FALSE;
   }
   //Serial.println(search_result,HEX);
   return search_result;
}

//_________________________________________________________________________



// Match ROM and measure temperature




void Control_ROM(unsigned char *ROM)
{
  DS18B20_Reset();
  DS18B20_Write_Byte(0x55);             //match ROM
  for (int i = 0; i < 8; i++) {
    DS18B20_Write_Byte(ROM[i]);
  }

  
    DS18B20_Write_Byte(0x44);        	//convert_T
    delay(750);

    DS18B20_Reset();
    DS18B20_Write_Byte(0x55);             //match ROM
    for (int i = 0; i < 8; i++) {
      DS18B20_Write_Byte(ROM[i]);
    }
   
    DS18B20_Write_Byte(0xBE);      	//read Scratchpad
    delayMicroseconds(10);
    uint8_t lsB = DS18B20_Read_Byte();              // đọc LSByte cua nhiet đô
    uint8_t msB = DS18B20_Read_Byte();              // đọc MSByte của nhiệt độ
    float t = ((float)(lsB & 0x0F)) / 16;          //byte thứ nhất lấy 4 bit đầu (phần thập phân)
    t = ((lsB & 0xF0) >> 4 | (msB & 0x0F) << 4) + t;  //(Lấy 4 bit cao cua msByte) + (Lấy 4 bit thấp của lsByte) + phần thập phân ở trên
   //  char checkSign = (msB & 0xF0) >> 4;             //Lấy 4 bit cao của msByte
   //  if (checkSign == 0x0F) {
   //    t = t-256;                                    // Do nhiệt độ âm được biểu diễn dưới dạng bù 2 nên cần phải trừ đi 256  để tìm nhiệt độ chính xác
   //  }                                               // t = t-2^8  do có 8 bit nên số bù được tính theo công thức trên              

    Serial.print("Nhiet do: ");
    Serial.print(t,4);
    Serial.println( " C");
    lcd.print(t,3);
    lcd.print(" ");
    lcd.print("C");
}

// match rom if matched return 1 else return 0

int DS18B20_Match_ROM(unsigned char *ROM){
   int rslt = FALSE;
   int i;

   if (FisrtSearch())
   {
      do
      {
         rslt = TRUE;
         for (i = 0; i < 8; i++)
         {
            if (ROM[i] != ROM_NO[i])
            {  
               Serial.println("ROM no match");
               lcd.clear();
               lcd.setCursor(0,0);
               lcd.print("ROM no match");
               rslt = FALSE;
               break;
            }
         }
         if (rslt){
            break;
         }
          
      }
      while (NextSearch());
   }

   return rslt;

}

// Search Rom and save to ROM_ID 

void DS18B20_Search_ROM(){
   int count=0;
  int result = FisrtSearch();
  while(result){
    for(int j=0;j<8;j++){
      ROM_ID[count][j] = ROM_NO[j];
    }
    count++;
    result = NextSearch();
  }
  Serial.println("done search rom");
  //print sum  devices
  Serial.print("Tim thay so cam bien la: ");
  Serial.println(count);

  if(count == 0){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Sensor not found");
      Serial.println("Sensor not found");
  }

  for(int i=0;i<count;i++){
    Serial.println();
    Serial.print("Cam bien ");
    Serial.println(i+1);
    Serial.print("Rom code: ");
    for(int j=0;j<8;j++){
      Serial.print(ROM_ID[i][j],HEX);
      Serial.print(" ");
    }
    Serial.println();

    if (count <= 2){
      lcd.setCursor(0,i);
      lcd.print("Temp ");
      lcd.print(i+1);
      lcd.print(": ");
      Control_ROM(ROM_ID[i]);
    }
    else{
      lcd.setCursor(0,i);
      lcd.print("Temp: ");
      lcd.print(i+1);
      lcd.print(": ");
      Control_ROM(ROM_ID[i]);
      if(i%2!=0){
        lcd.clear();
      }
      delay(1000);
    }
  }
}


void setup(){
  Serial.begin(9600);
  Serial.println("Start");

  lcd.begin(16, 2);
}

void loop(){
  //search all devices
  Serial.println("______________________");
  Serial.println("start search rom");
  DS18B20_Search_ROM();
  //unsigned char Rom_addr[8] = {0x28,0x53,0x91,0x14,0x00,0x00,0x00,0x25};
  //Serial.println(DS18B20_Match_ROM(Rom_addr));
  //lcd.print("hello, world!");
  delay(1000);

  
}

