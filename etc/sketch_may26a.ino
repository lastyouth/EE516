#include <Wire.h>                                // I2C 사용 예정
#include <SoftwareSerial.h>
int temp_addr = 0x48;                           // JMOD-TEMP-1 어드레스 = 0b01001000

SoftwareSerial btWithRpi(3,2);

byte current_temp;

byte read_temp()
{
  Wire.beginTransmission(temp_addr);       // Start (버스 시작) 및 타겟 어드레스 지정
  Wire.write(byte(0x00));                         // Temperature 레지스터(00) 지정
  Wire.requestFrom(temp_addr, 2);                      // 2 bytes read
  while(Wire.available() < 2)  ;                 // Wating 2 byte available
  byte c = Wire.read();                           // 첫번째 데이터 저장
  byte d = Wire.read();                           // 두번째 데이터 저장
  Wire.endTransmission();                       // Stop (버스 종료)
  bool isNegative = false;
  
  word e = (c << 8) | d;                         // 읽어온 2바이트 계산
  if ((e & 0x8000) == 0x8000)               // MSB는 부호비트
  {
    isNegative = true;                           // - 부호이면, print '-' sign
    e = ~e + 1;                                 // 2’s complement값 추출
  }
  c = ((e & 0x7f00) >> 8);                    // 정수 부분 추출
  
  if(isNegative)
  {
    c*= -1;
  }
  return c;
}

void setup() {
  // put your setup code here, to run once:
  btWithRpi.begin(115200);
  Serial.begin(9600);                              // 시리얼모니터 가동
  Wire.begin();                                     // I2C 활성화
  Wire.beginTransmission(temp_addr);       // Start (버스 시작) 및 타겟 어드레스 지정
  Wire.write(byte(0x01));                         // Configuration 레지스터(01) 지정
  Wire.write(byte(0x00));                         // Configuration 레지스터에 값(00)을 Write : Normal 모드
  Wire.endTransmission();                       // Stop (버스 종료)
  
  read_temp();
  current_temp = read_temp();
}

void loop() {
  current_temp = read_temp();
  Serial.println(current_temp);
  if(btWithRpi.available())
  {
    char cd = btWithRpi.read();
    btWithRpi.println(current_temp,DEC);
   // Serial.println(cd);
  }
  else
  {
    //Serial.println("Sleep");
  }
  delay(400);
}
